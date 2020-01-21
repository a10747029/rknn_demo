#ifndef _CAMERA_RECORD_H_
#define _CAMERA_RECORD_H_
#include <string.h>
#include <rk_mpi.h>
#include <mpp_env.h>
#include <mpp_mem.h>
#include <mpp_log.h>
#include <mpp_time.h>
#include <mpp_common.h>
#include <time.h>
#include <stdio.h>
#include <fcntl.h>
//#include <rk_type.h>

#include <utils.h>

#ifdef __cplusplus
extern "C"{
#endif

#define RECORD_WIDTH          640
#define RECORD_HEIGHT         480
#define RECORD_STREAM_FORMAT  MPP_VIDEO_CodingAVC /*h.264/AVC Coding*/
#define RECORD_FRAMES         (30*60*1)                /*300s*/
#define RECORD_OUTFILE_PREFIX "/userdata/camera_record"
#define RECORD_FRAME_FORMAT   MPP_FMT_YUV420P /*NV12*/    
#define RECORD_ENCODE_MODE    MPP_ENC_RC_MODE_CBR /* MPP_ENC_RC_MODE_VBR*/

//extern size_t fifo_write_len;
typedef struct {
    // global flow control flag
    RK_U32 frm_eos;
    RK_U32 have_encoding_frame_count;
    FILE *fp_output;

    // base flow context
    MppCtx ctx;
    MppApi *mpi;
    MppEncPrepCfg prep_cfg;
    MppEncRcCfg rc_cfg;
    MppEncCodecCfg codec_cfg;

    // input / output
    MppBuffer frm_buf;
    MppEncSeiMode sei_mode;

    // paramter for resource malloc
    RK_U32 width;
    RK_U32 height;
    RK_U32 hor_stride;
    RK_U32 ver_stride;
    MppFrameFormat fmt;
    MppCodingType type;
    RK_U32 num_frames;

    // resources
    size_t frame_size;
    /* NOTE: packet buffer may overflow */
    size_t packet_size;

    // rate control runtime parameter
    RK_S32 gop;
    RK_S32 fps;
    RK_S32 bps;
} MpiEncTestData;

void record_init();
void record_put_frame(char* buf);
#ifdef __cplusplus
}
#endif

#endif

