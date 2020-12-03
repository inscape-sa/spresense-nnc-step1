#include <sdk/config.h>
#include <stdio.h>
#include "util_dump.h"

static void insert_chlr(int idx, int chlr_len)
{
  if (idx % chlr_len == (chlr_len - 1)) {
    printf("\n");
  }
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

int util_dump_main(int argc, char *argv[])
{
  return 0;
}
