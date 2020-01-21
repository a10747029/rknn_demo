#ifndef OTHERS__H_
#define OTHERS__H_

#include <linux/videodev2.h>
#include "camera_record.h"
#include <rga/RgaApi.h>
#include "rga.h"

#define SRC_W   1920
#define SRC_H   1080
#define SRC_FPS 25
#define SRC_BPP 24
#define DST_W   300
#define DST_H   300
#define DST_BPP 24

#define SRC_V4L2_FMT    V4L2_PIX_FMT_NV16
//RK_FORMAT_YCbCr_420_P == NV12
//RK_FORMAT_YCbCr_422_SP == NV16
#define SRC_RKRGA_FMT           RK_FORMAT_YCrCb_422_SP
#define DST_RKRGA_RECORD_FMT    RK_FORMAT_YCbCr_420_P
#define DST_RKRGA_FMT           RK_FORMAT_RGB_888


int do_deal_request(char command_buf[],int length);
void *camera_thread(void);


#endif
