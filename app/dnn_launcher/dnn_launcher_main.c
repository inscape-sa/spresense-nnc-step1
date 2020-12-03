
/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <sdk/config.h>
#include <nuttx/config.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define MAX_PATH_LENGTH       (128)
#define SET_ARGC(argv)        (sizeof(argv)/sizeof(char*))
#define BASE_DIR              "/mnt/sd0/REC"
#define BASE_DNN_INPUT_EXT    "csv"
#define ONESEC_CSV_CHUNK_NUM  (48 * 1024) /* 48kHz */
#define GEN_WAV_CSV_NUM       (50)

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/
int audio_recorder_main(int argc, char *argv[]);
int dnnrt_autoencoder_main(int argc, char *argv[]);
int util_wav_to_csv_main(int argc, char *argv[]);

/****************************************************************************
 * Private Type Declarations
 ****************************************************************************/
typedef enum exec_mode_type_e
{
  EXEC_MODE_DATA_GEN = 0,
  EXEC_MODE_RECONGNITION,
  EXEC_MODE_INVAL
} EXEC_MODE;

/****************************************************************************
 * Private Data
 ****************************************************************************/
static char filepath_rec[] = "/mnt/sd0/REC/tmp.wav"; 
static char filename_recognition[] = "recognition";
static char filepath_dnnsrc[MAX_PATH_LENGTH];
static char str_shift[16];
static char str_len[16];
static char str_recsec[16];

static char* audio_recoder_argv[] = { 
  "audio_recorder", 
  filepath_rec, 
  str_recsec
  };

static char* util_wav_to_csv_argv[] = {
  "util_wav_to_csv", 
  filepath_rec, 
  filepath_dnnsrc, 
  str_shift,
  str_len
  };

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/
int dnn_launcher_main(int argc, char *argv[])
{
  EXEC_MODE exec_mode = EXEC_MODE_INVAL;
  
  /* learnig mode */
  if (argc >= 2) {
    if (strncmp(argv[1], "gen", 4) == 0) {
      exec_mode = EXEC_MODE_DATA_GEN;
    } else if (strncmp(argv[1], "run", 4) == 0) {
      exec_mode = EXEC_MODE_RECONGNITION;
    } else {
      /* NOP */
    }
  }

  if (exec_mode == EXEC_MODE_DATA_GEN) {
    /*****************************************************/
    /** Gen DATA */
    /*****************************************************/
    /* Gen Mode - Phase.1 [Recording] */
    snprintf(str_recsec, 15, "%d", (GEN_WAV_CSV_NUM / 2) + 2);
    audio_recorder_main(SET_ARGC(audio_recoder_argv), audio_recoder_argv);

    /* Gen Mode - Phase.2 [Gen Training Data] */
    int gen_idx;
    for (gen_idx = 0; gen_idx < GEN_WAV_CSV_NUM; gen_idx++) {
      filepath_dnnsrc[MAX_PATH_LENGTH - 1] = '\0';
      snprintf(filepath_dnnsrc, MAX_PATH_LENGTH - 1, "%s/%d.%s", BASE_DIR, gen_idx, BASE_DNN_INPUT_EXT);
      snprintf(str_shift, 15, "%d", gen_idx * (ONESEC_CSV_CHUNK_NUM / 2));
      snprintf(str_len, 15, "%d", ONESEC_CSV_CHUNK_NUM);
      util_wav_to_csv_main(SET_ARGC(util_wav_to_csv_argv), util_wav_to_csv_argv);
    }

  } else if (exec_mode == EXEC_MODE_RECONGNITION) {
    /*****************************************************/
    /** RECONGINITION */
    /*****************************************************/
    /* Recog.Mode - Phase.1 [Recording] */
    snprintf(str_recsec, 15, "%d", 2);
    audio_recorder_main(SET_ARGC(audio_recoder_argv), audio_recoder_argv);

    /* Recog.Mode - Phase.2 [convert to CSV from WAV] */
    filepath_dnnsrc[MAX_PATH_LENGTH - 1] = '\0';
    snprintf(filepath_dnnsrc, MAX_PATH_LENGTH - 1, "%s/%s.%s", BASE_DIR, filename_recognition, BASE_DNN_INPUT_EXT);
    snprintf(str_shift, 15, "%d", 0);
    snprintf(str_len, 15, "%d", ONESEC_CSV_CHUNK_NUM);
    util_wav_to_csv_main(SET_ARGC(util_wav_to_csv_argv), util_wav_to_csv_argv);

    /* Recog.Mode - Phase.3 [Recognition] */
    dnnrt_autoencoder_main(argc, argv);

  } else {
    printf("INFO: usage: dnn_launcher [option, \"gen\"=gen_learnig_data mode, \"run\"=recognition mode]\n");
    return -EINVAL;
  }
  return 0;
}
