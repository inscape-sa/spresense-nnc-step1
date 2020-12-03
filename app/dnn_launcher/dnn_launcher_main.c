#include <sdk/config.h>
#include <stdio.h>

int audio_recorder_main(int argc, char *argv[]);
int dnnrt_autoencoder_main(int argc, char *argv[]);
int util_wav_to_csv_main(int argc, char *argv[]);

#define SET_ARGC(argv)  (sizeof(argv)/sizeof(char*))

int dnn_launcher_main(int argc, char *argv[])
{
  char filepath_rec[] = "/mnt/sd0/REC/fixname.wav";
  char filepath_dnnsrc[] = "/mnt/sd0/REC/fixname.csv";

  char* audio_recoder_argv[] = { 
    "audio_recorder", 
    filepath_rec, 
    "2"
    };

  char* util_wav_to_csv_argv[] = { "util_wav_to_csv", 
    filepath_rec, 
    filepath_dnnsrc, 
    "16",
    "49152" /* 48kHz(48*1024=) */ 
    };

  audio_recorder_main(SET_ARGC(audio_recoder_argv), audio_recoder_argv);
  util_wav_to_csv_main(SET_ARGC(util_wav_to_csv_argv), util_wav_to_csv_argv);
  dnnrt_autoencoder_main(argc, argv);
  return 0;
}
