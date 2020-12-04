
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
#include "../util_misc/util_misc.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define NUM_ENTRY_BUF 512

/****************************************************************************
 * Private Type Declarations
 ****************************************************************************/
struct util_wav_header_s
{
  /*! character "RIFF" */
  uint32_t riff;
  /*! Whole size exclude "RIFF" */
  uint32_t total_size;
  /*! character "WAVE" */
  uint32_t wave;
  /*! character "fmt " */
  uint32_t fmt;
  /*! fmt chunk size */
  uint32_t fmt_size;
  /*! format type */
  uint16_t format;
  /*! channel number */
  uint16_t channel;
  /*! sampling rate */
  uint32_t rate;
  /*! rate * block */
  uint32_t avgbyte;
  /*! channels * bit / 8 */
  uint16_t block;
  /*! bit length */
  uint16_t bit;
  /*! character "data" */
  uint32_t data;
  /*! payload size */
  uint32_t data_size;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/
typedef struct util_wav_header_s UTIL_WAVH;

/****************************************************************************
 * Private Functions
 ****************************************************************************/
static int check_wav_header_and_get_entries(FILE *srcfp)
{
  int ret;
  int mainret = 0;
  int src_record_num = 0;
  UTIL_WAVH s_wav_header;
  
  ret = fread(&s_wav_header, 1, sizeof(UTIL_WAVH), srcfp);
  if (ret != sizeof(UTIL_WAVH)) {
    printf("ERROR: fread faild on src header (REQ=%d, RES=%d)\n", sizeof(UTIL_WAVH), ret);
    mainret = -EINVAL;
    goto err_fread_wav_header;
  }
  // dump_char((uint8_t *)&s_wav_header, sizeof(UTIL_WAVH));
  printf("INFO:fmtcode=%d, ch=%d, birrate=%d, sampling-rate=%fkHz payload-size=%dbyte(%08x)\n",
    s_wav_header.format,
    s_wav_header.channel,
    s_wav_header.bit,
    ((float)s_wav_header.rate) / 1000.0,
    s_wav_header.data_size,
    s_wav_header.data_size);

  if (s_wav_header.channel != 1) {
    printf("ERROR: this code supports only MONO (file has %dCH)\n", s_wav_header.channel);
    mainret = -EINVAL;
    goto err_src_fmt;
  }
  if (s_wav_header.bit != 16) {
    printf("ERROR: this code supports only 16bit/ch (file format is %dbit/ch)\n", s_wav_header.bit);
    mainret = -EINVAL;
    goto err_src_fmt;
  }
  src_record_num = s_wav_header.data_size / ((s_wav_header.bit >> 3) * s_wav_header.channel);
  mainret = src_record_num;
err_src_fmt:
err_fread_wav_header:
  return mainret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
int util_wav_to_csv_main(int argc, char *argv[])
{
  int mainret = -EINVAL;

  int ret;
  char *srcpath;
  char *dstpath;
  FILE *srcfp;
  FILE *dstfp;
  int src_record_num;
  signed short buf[NUM_ENTRY_BUF];
  signed short wavmin = INT16_MAX;
  signed short wavmax = INT16_MIN;
  int outidx;
  int outnum;
  int remain_num;
  int conv_num = -1;
  int skip_num = 0;

  if (argc < 3) {
    printf("ERROR : USAGE : util_wav_to_csv_main [srcfile.wav] [dstfile.csv]");
    mainret = -EINVAL;
    goto bad_arg;
  }

  srcpath = argv[1];
  dstpath = argv[2];
  if (argc >= 5) {
    skip_num = atoi(argv[3]);
    conv_num = atoi(argv[4]);
  }
  printf("Convert %s -> %s (skip=%d, conv=%d)\n", srcpath, dstpath, skip_num, conv_num);
  
  srcfp = fopen(srcpath, "r");
  if (srcfp == NULL) {
    mainret = -errno;
    printf("ERROR: fopen faild on src (%s)\n", srcpath);
    goto err_fopen_src;
  }

  dstfp = fopen(dstpath, "w");
  if (dstfp == NULL) {
    mainret = -errno;
    printf("ERROR: fopen faild on dst (%s)\n", dstpath);
    goto err_fopen_dst;
  }

  src_record_num = check_wav_header_and_get_entries(srcfp);
  printf("INFO: src has %d records\n", src_record_num);
  if (src_record_num <= 0) {
    printf("ERROR: check_wav_header_and_get_entries(%d)\n", src_record_num);
    mainret = -EINVAL;
    goto err_src_fmt;
  }
  if (conv_num <= 0) {
    conv_num = src_record_num;
  }

  if ((conv_num + skip_num) > (src_record_num)) {
    printf("ERROR: Export Target is in Out-of-Range"
      "(conv=%d + skip=%d > record=%d).\n", conv_num, skip_num, src_record_num);
    mainret = -EINVAL;
    goto err_src_fmt;
  }

  outnum = 0;
  remain_num = conv_num;
  int req_num;
  if (skip_num > 0) {
    fseek(srcfp, skip_num * sizeof(buf[0]), SEEK_CUR);
  }
  do {
    req_num = (remain_num > NUM_ENTRY_BUF)? NUM_ENTRY_BUF : remain_num;
    ret = fread(buf, sizeof(signed short), req_num, srcfp);
    for (outidx = 0; outidx < ret; outidx++) {
      register signed short power = buf[outidx];
      if (wavmin > power) {
        wavmin = power;
      }
      if (wavmax < power) {
        wavmax = power;
      }
      fprintf(dstfp, "%d,\n", power);
    }
    outnum += ret;
    remain_num -= ret;
  } while ((ret >= req_num) && (remain_num > 0));
  printf("INFO: convert %d entries. (skip=%d, max=%d, min=%d)\n", outnum, skip_num, wavmax, wavmin);

  ret = 0;
err_src_fmt:
  fclose(dstfp);
err_fopen_dst:
  fclose(srcfp);
err_fopen_src:
bad_arg:
  return mainret;
}

