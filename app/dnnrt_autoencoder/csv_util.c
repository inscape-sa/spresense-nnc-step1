/****************************************************************************
 * dnnrt_lenet/csv_util.c
 ****************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#define LINE_BUF_SIZE (128)
static int
load_csv_internal(const char *csv_path, float norm_factor, float *output_buffer, int output_num)
{
  int err = 0;
  char char_buf[LINE_BUF_SIZE];
  char *retbuf;
  char *brpos;
  FILE *file;
  int linenum = 0;

  file = fopen(csv_path, "r");
  if (file != NULL)
    {
      do { 
        retbuf = fgets(char_buf, LINE_BUF_SIZE, file);
        if (retbuf == NULL) {
          break;
        }
        brpos = strchr(char_buf, '\n');
        if (brpos != NULL) {
          *brpos = '\0'; 
        }
        if (strlen(char_buf) == 0) {
          continue;
        }
        //printf("%s(%f)-", char_buf, atof(char_buf));
        output_buffer[linenum] = atof(char_buf);
        linenum++;
      } while(linenum <= output_num);
      
      if (linenum < output_num) {
          err = -EINVAL;
          goto invalid_file;
      }
      fclose(file);
    }
  else
    {
      err = -errno;
      goto file_open_err;
    }

  return 0;

invalid_file:
  fclose(file);
file_open_err:
  return err;
}

int csv_load(const char *csv_path, float norm_factor, float *output_buffer,
             int output_num)
{
  int err;
  int valid_args = 1;
  valid_args &= (csv_path != NULL && output_buffer != NULL);
  if (valid_args)
    {
      if ((err = load_csv_internal(csv_path, norm_factor, output_buffer, output_num)) == 0)
        {
          /* normal case */
        }
      else
        {
          goto load_error;
        }
    }
  else
    {
      err = -EINVAL;
      goto invalid_args;
    }
  return 0;

load_error:
invalid_args:
  return err;
}
