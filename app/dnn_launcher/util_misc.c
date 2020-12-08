
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

#include "util_misc.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/
static void insert_chlr(int idx, int chlr_len)
{
  if (idx % chlr_len == (chlr_len - 1)) {
    printf("\n");
  }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
void *memtile_alloc(mpshm_t *pshm, int size)
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

void memtile_free(mpshm_t *pshm)
{
  mpshm_detach(pshm);
  mpshm_destroy(pshm);
}

void dump_char(uint8_t *buf, int num)
{
  int idx;
  int chlr_len = 16;
  printf("\n");
  for(idx = 0; idx < num; idx++) {
    printf("%02x ", buf[idx]);
    insert_chlr(idx, chlr_len);
    fflush(stdout);
  }
  printf("\n\n");
  fflush(stdout);
}

void dump_int(uint32_t *buf, int num)
{
  int idx;
  int chlr_len = 4;
  printf("\n");
  for(idx = 0; idx < num; idx++) {
    printf("buf[%d] %08x,", idx, buf[idx]);
    insert_chlr(idx, chlr_len);
    fflush(stdout);
  }
  printf("\n\n");
  fflush(stdout);
}

void dump_float(float *buf, int num)
{
  int idx;
  int chlr_len = 4;
  printf("\n");
  for(idx = 0; idx < num; idx++) {
    printf("buf[%d] %f,", idx, buf[idx]);
    insert_chlr(idx, chlr_len);
    fflush(stdout);
  }
  printf("\n\n");
  fflush(stdout);
}
