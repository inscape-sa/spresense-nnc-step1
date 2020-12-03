#ifndef _UTIL_DUMP_H_
#define _UTIL_DUMP_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

void dump_char(uint8_t *buf, int num);
void dump_int(uint32_t *buf, int num);
void dump_float(float *buf, int num);

#endif /* _UTIL_DUMP_H_ */