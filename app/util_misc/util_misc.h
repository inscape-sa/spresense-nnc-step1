#ifndef _UTIL_MISC_H_
#define _UTIL_MISC_H_

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/time.h>
#include <nuttx/config.h>
#include <sdk/config.h>

#include <asmp/asmp.h>
#include <asmp/mptask.h>
#include <asmp/mpshm.h>
#include <asmp/mpmq.h>
#include <asmp/mpmutex.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define CXD5602_SINGLE_TILE_SIZE  (1024 * 128)

/****************************************************************************
 * Public Functions
 ****************************************************************************/
void dump_char(uint8_t *buf, int num);
void dump_int(uint32_t *buf, int num);
void dump_float(float *buf, int num);

void *memtile_alloc(mpshm_t *pshm, int size);
void memtile_free(mpshm_t *pshm);

#endif /* _UTIL_MISC_H_ */