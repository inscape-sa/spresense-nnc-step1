#ifndef _DNNRT_AUDIOCNN_H_
#define _DNNRT_AUDIOCNN_H_

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

#include "./util_misc.h"
#include "./csv_util.h"
#include "./loader_large_nnb.h"

int dnnrt_audiocnn(int argc, char *argv[]);

#endif /* _DNNRT_AUDIOCNN_H_ */