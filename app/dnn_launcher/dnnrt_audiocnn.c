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
#include "./dnnrt_audiocnn.h"

#include "./util_misc.h"
#include "./csv_util.h"
#include "./loader_large_nnb.h"

/****************************************************************************
 * Type Definition
 ****************************************************************************/
typedef struct
{
  char *nnb_path;
  char *csv_path;
  bool skip_norm;
} audiocnn_setting_t;

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define DNN_WAV_PATH    "/mnt/sd0/audiocnn/0.csv"
#define DNN_NNB_PATH    "/mnt/sd0/audiocnn/model.nnb"
//#define INPUT_WAVE_LEN  ((48 * 1024) / 2) /* 48kHz, 0,5sec */
#define INPUT_WAVE_LEN  ((4 * 1024) / 2) /* 4kHz, 0,5sec */
#define INPUT_WAVE_SIZE (INPUT_WAVE_LEN * (sizeof(float)))
#define ALLOC_UNIT      (CXD5602_SINGLE_TILE_SIZE)

// #define USE_STANDALONE_MODEL_MEM

/****************************************************************************
 * Private Functions
 ****************************************************************************/
static void parse_args(int argc, char *argv[], audiocnn_setting_t * setting)
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
    printf("usage: dnnrt_audiocnn [path_model] [path_wav]");
  }

  /* print audiocnn_setting_t */
  printf("Load nnb file: %s\n", setting->nnb_path);
  printf("Load wave file: %s\n", setting->csv_path);
  fflush(stdout);
}

/* get Input Types */
static void print_input_format(dnn_runtime_t *prt)
{ 
  int num;
  int idx;
  int cnt;
  int ndim;
  num = dnn_runtime_input_num(prt);
  for (idx = 0; idx < num; idx++) 
    {
      cnt = dnn_runtime_input_size(prt, idx);
      ndim = dnn_runtime_input_ndim(prt, idx);
      printf("input[%d] reqiore cnt=%d dmis=%d.\n", idx, cnt, ndim);
    }
}

/* get Output Types */
static void print_output_format(dnn_runtime_t *prt)
{ 
  
  int num;
  int idx;
  int cnt;
  int ndim;
  num = dnn_runtime_output_num(prt);
  for (idx = 0; idx < num; idx++) 
    {
      cnt = dnn_runtime_output_size(prt, idx);
      ndim = dnn_runtime_output_ndim(prt, idx);
      printf("output[%d] is constructed from cnt=%d dmis=%d\n", idx, cnt, ndim);
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
int dnnrt_audiocnn(int argc, char *argv[])
{
  int ret = 0;
  mpshm_t shm_model;
  mpshm_t shm_input;
  dnn_runtime_t rt;
  dnn_config_t config = {.cpu_num = 1 };
  nn_network_t *network;
  audiocnn_setting_t setting = { 0 };
  void *inputs[1];
  float *s_wave_buffer;
  void *s_model_buffer;
  struct timeval begin, end;
  int *output_buffer;

  parse_args(argc, argv, &setting);

  s_wave_buffer = memtile_alloc(&shm_input, ALLOC_UNIT);
  if (s_wave_buffer == NULL) {
    ret = -errno;
    goto err_input_alloc;
  }
  printf("ADDR:%08x: for s_wave_buffer (%dbyte)\n", s_wave_buffer, INPUT_WAVE_LEN * sizeof(float));
  s_model_buffer = (void *)(((uint32_t)(s_wave_buffer)) + INPUT_WAVE_LEN * sizeof(float));
#ifdef USE_STANDALONE_MODEL_MEM
  s_model_buffer = memtile_alloc(&shm_model, ALLOC_UNIT);
  if (s_model_buffer == NULL) {
    ret = -errno;
    goto err_model_alloc;
  }
#endif
  printf("ADDR:%08x: for s_model_buffer (%dbyte)\n", s_model_buffer, CXD5602_SINGLE_TILE_SIZE - (INPUT_WAVE_LEN * sizeof(float)));
  printf("----load WAVE DATA as input data ----\n");
  ret = csv_load(setting.csv_path, 1.0f, s_wave_buffer, INPUT_WAVE_LEN);
  if (ret != 0) {
    printf("ERROR occur in csv_load(ret=%d)\n", ret);
    goto load_error;
  }
  inputs[0] = (void *)s_wave_buffer;

  printf("----load NNB network ----\n");
  fflush(stdout);
  network = load_nnb_network(setting.nnb_path, s_model_buffer);
  if (network == NULL)
    {
      printf("load nnb file failed\n");
      goto load_error;
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

  /* Start-B+: get Input Types */
  print_input_format(&rt);

  /* Step-C: perform inference after feeding inputs */
  printf("----STEP:C----\n");
  fflush(stdout);
  printf("-->> Start dnn_runtime_forward with... \n");
  printf("---->> Input Buffer is on 0x%08x\n", inputs[0]);
  printf("---->> NNB is on 0x%08x\n", s_model_buffer);
  fflush(stdout);
  
  gettimeofday(&begin, 0);
  ret = dnn_runtime_forward(&rt, (const void**)inputs, 1);
  gettimeofday(&end, 0);
  if (ret)
    {
      printf("dnn_runtime_forward() failed due to %d\n", ret);
      goto fin;
    }

  /* Start-D-: get Output Types */
  print_output_format(&rt);

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
      printf("<<-- Output[%d] on 0x%08x, num=%d\n", output_idx, output_buffer, buf_cnt);
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
#ifdef USE_STANDALONE_MODEL_MEM
err_model_alloc:
  memtile_free(&shm_model);
#endif /* USE_STANDALONE_MODEL_MEM */
err_input_alloc:
  return 0;
}
