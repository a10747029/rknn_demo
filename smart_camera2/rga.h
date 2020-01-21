#ifndef RGA__H_
#define RGA__H_
#include <rga/RgaApi.h>
#include <stdlib.h>

int rgaTRANSFER(unsigned char* dest_buf,unsigned int src_fmt, unsigned char* src_buf,
                      int src_w, int src_h,
                      unsigned int dst_fmt, int dst_fd,
                      int dst_w, int dst_h);






#endif
