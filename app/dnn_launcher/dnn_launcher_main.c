#include <sdk/config.h>
#include <stdio.h>

int audio_recorder_main(int argc, char *argv[]);
int dnnrt_autoencoder_main(int argc, char *argv[]);



int dnn_launcher_main(int argc, char *argv[])
{
  char appname[] = "audio_recorder";
  char filepath[] = "/mnt/sd0/REC/fixname.wav";
  char* audio_recoder_argv[2] = { appname, filepath };
  audio_recorder_main(2, audio_recoder_argv);
  dnnrt_autoencoder_main(argc, argv);
  return 0;
}
