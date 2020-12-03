/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/time.h>
#include <nuttx/config.h>
#include <dnnrt/runtime.h>

#include <asmp/asmp.h>
#include <asmp/mptask.h>
#include <asmp/mpshm.h>
#include <asmp/mpmq.h>
#include <asmp/mpmutex.h>

#include "loader_large_nnb.h"
#include "csv_util.h"

#include "../util_dump/util_dump.h"

/****************************************************************************
 * Type Definition
 ****************************************************************************/
typedef struct
{
  char *nnb_path;
  char *csv_path;
  bool skip_norm;
} autoencoder_setting_t;

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define DNN_WAV_PATH    "/mnt/sd0/autoencoder/0.csv"
#define DNN_NNB_PATH    "/mnt/sd0/autoencoder/model.nnb"
#define INPUT_WAVE_LEN  (128)
#define INPUT_WAVE_SIZE (INPUT_WAVE_LEN * (sizeof(float)))
#define OUTPUT_WAVE_LEN  (128)
#define OUTPUT_WAVE_SIZE (OUTPUT_WAVE_LEN * (sizeof(float)))
#define CXD5602_SINGLE_TILE_SIZE  (1024 * 128)
#define ALLOC_UNIT      (CXD5602_SINGLE_TILE_SIZE)

/****************************************************************************
 * Private Functionsdn
 ****************************************************************************/
static void parse_args(int argc, char *argv[], autoencoder_setting_t * setting)
{
  int idx;
  setting->skip_norm = false;

  for (idx = 0; idx < argc; idx++) 
    {
      printf("argv[%d]='%s'\n", idx, argv[idx]);
    }

  setting->nnb_path = DNN_NNB_PATH;
  setting->csv_path = DNN_WAV_PATH;
  if (argc == 3) {
    setting->nnb_path = argv[1];
    setting->csv_path = argv[2];
  } else {
    /* nop */
    printf("usage: dnnrt_autoencoder [path_model] [path_wav]");
  }

  /* print autoencoder_setting_t */
  printf("Load nnb file: %s\n", setting->nnb_path);
  printf("Load wave file: %s\n", setting->csv_path);
  printf("Wave Normalization (1.0/255.0): skipped\n");
  fflush(stdout);
}

static void *memtile_alloc(mpshm_t *pshm, int size)
{
  int ret;
  int alloc_size = CXD5602_SINGLE_TILE_SIZE;
  void *shm_vbuf;
  void *shm_phead;
  void *shm_ptail;

  if (size > alloc_size)
  {
    printf("ERROR: Too Large alloc unit for memtile_alloc (%dbyte > MEMTILE(%dbyte))\n", size, alloc_size);
    return NULL;
  }

  ret = mpshm_init(pshm, 0, size);
  if (ret != 0) {
    printf("ERROR: mpshm_init/memtile_alloc(%d)\n", ret);
    return NULL;
  }
  shm_vbuf = mpshm_attach(pshm, 0);
  shm_phead = (void *)mpshm_virt2phys(pshm, shm_vbuf);
  shm_ptail = (void *)mpshm_virt2phys(pshm, (void *)((uint32_t)shm_vbuf + (alloc_size - 1)));
  printf("ALLOC: [phys:0x%08x-0x%08x][virt:0x%08x, size %dbyte] for model\n", shm_phead, shm_ptail, shm_vbuf, alloc_size);
  fflush(stdout);
  return shm_phead;
}

static void memtile_free(mpshm_t *pshm)
{
  mpshm_detach(pshm);
  mpshm_destroy(pshm);
}

int dnnrt_autoencoder_main(int argc, char *argv[])
{
  int ret = 0;
  mpshm_t shm_model;
  mpshm_t shm_input;
  dnn_runtime_t rt;
  dnn_config_t config = {.cpu_num = 1 };
  nn_network_t *network;
  autoencoder_setting_t setting = { 0 };
  void *inputs[1];
  float *s_wave_buffer;
  void *s_model_buffer;
  struct timeval begin, end;
  int *output_buffer;

  parse_args(argc, argv, &setting);

  s_model_buffer = memtile_alloc(&shm_model, ALLOC_UNIT);
  if (s_model_buffer == NULL) {
    ret = -errno;
    goto err_model_alloc;
  }
  printf("ADDR:%08x: for s_model_buffer\n", s_model_buffer);
  s_wave_buffer = memtile_alloc(&shm_input, ALLOC_UNIT);
  if (s_wave_buffer == NULL) {
    ret = -errno;
    goto err_input_alloc;
  }
  s_wave_buffer = (void *)(((uint32_t)s_model_buffer) + 96 * 1024);
  printf("ADDR:%08x: for s_wave_buffer\n", s_wave_buffer);
  ret = csv_load(setting.csv_path, 1.0f, s_wave_buffer, INPUT_WAVE_LEN);
  if (ret != 0) {
    printf("ERROR occur in csv_load(ret=%d)\n", ret);
    goto load_error;
  }
  dump_float((float *)s_wave_buffer, INPUT_WAVE_LEN);
  inputs[0] = (void *)s_wave_buffer;

  printf("----load_nnb_network----\n");
  fflush(stdout);
  network = load_nnb_network(setting.nnb_path, s_model_buffer);
  if (network == NULL)
    {
      printf("load nnb file failed\n");
      goto load_error;
    }
  printf("exit load nnb file\n");
  fflush(stdout);

  /* Step-A: initialize the whole dnnrt subsystem */
  printf("----STEP:A----\n");
  fflush(stdout);
  ret = dnn_initialize(&config);
  if (ret)
    {
      printf("dnn_initialize() failed due to %d", ret);
      goto dnn_error;
    }

  /* Step-B: instantiate a neural network defined
   * by nn_network_t as a dnn_runtime_t object */
  printf("----STEP:B----\n");
  fflush(stdout);
  ret = dnn_runtime_initialize(&rt, network);
  if (ret)
    {
      printf("dnn_runtime_initialize() failed due to %d\n", ret);
      goto rt_error;
    }

  /* Step-C: perform inference after feeding inputs */
  printf("----STEP:C----\n");
  fflush(stdout);
  printf("start dnn_runtime_forward()\n");
  printf(" -->> input[0]=0x%08x\n", inputs[0]);
  fflush(stdout);
  
  gettimeofday(&begin, 0);
  ret = dnn_runtime_forward(&rt, (const void**)inputs, 1);
  gettimeofday(&end, 0);
  if (ret)
    {
      printf("dnn_runtime_forward() failed due to %d\n", ret);
      goto fin;
    }

  /* Step-D: obtain the output from this dnn_runtime_t */
  printf("----STEP:D----\n");
  fflush(stdout);

  int output_num;
  int output_idx;
  int buf_cnt;
  output_num = dnn_runtime_output_num(&rt);
  printf("output_num = %d\n", output_num);

  for (output_idx = 0; output_idx < output_num; output_idx++) 
    {
      output_buffer = dnn_runtime_output_buffer(&rt, output_idx);
      buf_cnt = dnn_runtime_output_size(&rt, output_idx);
      printf("<%d> ready output (0x%08x, num=%d)\n", output_idx, output_buffer, buf_cnt);
      dump_float((float *)output_buffer, buf_cnt);
    }

fin:
  /* Step-F: free memories allocated to dnn_runtime_t */
  dnn_runtime_finalize(&rt);
rt_error:
  /* Step-G: finalize the whole dnnrt subsystem */
  dnn_finalize();
dnn_error:
load_error:
  memtile_free(&shm_input);
err_input_alloc:
  memtile_free(&shm_model);
err_model_alloc:
  return 0;
}
