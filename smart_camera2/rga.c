#include "rga.h"
int rgaTRANSFER(unsigned char* dest_buf,unsigned int src_fmt, unsigned char* src_buf,
                      int src_w, int src_h,
                      unsigned int dst_fmt, int dst_fd,
                      int dst_w, int dst_h)
{
    int ret;
    rga_info_t src;
    rga_info_t dst;
    int Format;

    memset(&src, 0, sizeof(rga_info_t));
    src.fd = -1; //rga_src_fd;
    src.virAddr = src_buf;
    src.mmuFlag = 1;
    memset(&dst, 0, sizeof(rga_info_t));
    dst.fd = -1;
    dst.virAddr = dest_buf;
    dst.mmuFlag = 1;


    rga_set_rect(&src.rect, 0, 0, src_w, src_h, src_w, src_h, src_fmt);
    rga_set_rect(&dst.rect, 0, 0, dst_w, dst_h, dst_w, dst_h, dst_fmt);
    ret = c_RkRgaBlit(&src, &dst, NULL);
    if (ret)
        printf("c_RkRgaBlit0 error : %s\n", strerror(errno));
    return ret;
}
