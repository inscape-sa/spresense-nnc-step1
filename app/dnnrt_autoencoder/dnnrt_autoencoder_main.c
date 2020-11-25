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
#define WAVE_SIZE_PT    (128*1)
#define WAVE_SIZE_BUF   (WAVE_SIZE_PT * (sizeof(float)))

/****************************************************************************
 * Private Data
 ****************************************************************************/
static float *s_wave_buffer;
static void *s_model_buffer;

/****************************************************************************
 * Private Functionsdn
 ****************************************************************************/
static void parse_args(int argc, char *argv[], autoencoder_setting_t * setting)
{
  setting->skip_norm = false;

  /* set autoencoder_setting_t::{nnb_path,pgm_path} to argv[] if necessary */
  setting->nnb_path = (optind < argc) ? argv[optind++] : DNN_NNB_PATH;
  setting->csv_path = (optind < argc) ? argv[optind] : DNN_WAV_PATH;

  /* print autoencoder_setting_t */
  printf("Load nnb file: %s\n", setting->nnb_path);
  printf("Load wave file: %s\n", setting->csv_path);
  printf("Wave Normalization (1.0/255.0): skipped\n");
}

static void dump_int(uint32_t *buf, int num)
{
  int idx;
  printf("\n");
  for(idx = 0; idx < num; idx++) {
    printf("buf[%d] %08x |", idx, buf[idx]);
  }
  printf("\n");
}

static void dump_float(float *buf, int num)
{
  int idx;
  printf("\n");
  for(idx = 0; idx < num; idx++) {
    printf("buf[%d] %f |", idx, buf[idx]);
  }
  printf("\n");
}

int dnnrt_autoencoder_main(int argc, char *argv[])
{
  int ret = 0;
  mpshm_t shm;
  void* shm_buf;
  void* shm_used;
  dnn_runtime_t rt;
  dnn_config_t config = {.cpu_num = 1 };
  nn_network_t *network;
  autoencoder_setting_t setting = { 0 };
  void *inputs[1];
  struct timeval begin, end;
  int *output_buffer;

  ret = mpshm_init(&shm, 0, 1024 * 512);
  shm_buf = mpshm_attach(&shm, 0);
  shm_used = mpshm_virt2phys(&shm, shm_buf);
  printf("ALLOC:%08x: as shm\n", shm_used);

  s_wave_buffer = (float *)shm_used;
  printf("attach buf (0x%08x) for s_wave_buffer\n");
  inputs[0] = s_wave_buffer;
  printf("ADDR:%08x: for s_wave_buffer\n", s_wave_buffer);
  shm_used += WAVE_SIZE_BUF;

  s_model_buffer = (void *)shm_used;
  printf("ADDR:%08x: for s_model_buffer\n", s_model_buffer);

  parse_args(argc, argv, &setting);

  ret = csv_load(setting.csv_path, 1.0f, s_wave_buffer, WAVE_SIZE_PT);
  if (ret != 0) {
    printf("ERROR occur in csv_load(ret=%d)\n", ret);
    goto load_error;
  }

  int idx;
  for (idx = 0; idx < WAVE_SIZE_PT; idx++) {
    printf("[%d] %f |", idx, s_wave_buffer[idx]);
  }

  printf("----load_nnb_network----\n");
  fflush(stdout);
  network = load_nnb_network(setting.nnb_path, s_model_buffer);
  if (network == NULL)
    {
      printf("load nnb file failed\n");
      goto load_error;
    }
  else
    {
      printf("exit load nnb file\n");
      //goto load_error;
    }
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
  output_buffer = dnn_runtime_output_buffer(&rt, 0u);
  printf("<0> ready output (0x%08x)\n", output_buffer);
  dump_int((uint32_t *)output_buffer, 128);
  dump_float((float *)output_buffer, 128);

  output_buffer = dnn_runtime_output_buffer(&rt, 1u);
  printf("<1> ready output (0x%08x)\n", output_buffer);
  dump_int((uint32_t *)output_buffer, 128);
  dump_float((float *)output_buffer, 128);

fin:
  /* Step-F: free memories allocated to dnn_runtime_t */
  dnn_runtime_finalize(&rt);
rt_error:
  /* Step-G: finalize the whole dnnrt subsystem */
  dnn_finalize();
dnn_error:
load_error:
  mpshm_detach(&shm);
  mpshm_destroy(&shm);
  return 0;
}
