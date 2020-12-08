#ifndef _UTIL_WAV_TO_CSV_H_
#define _UTIL_WAV_TO_CSV_H_

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

int util_wav_to_csv(int argc, char *argv[]);

#endif /* _UTIL_WAV_TO_CSV_H_ */