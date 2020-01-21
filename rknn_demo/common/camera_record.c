#include<time.h>
#include<stdio.h>
#include"camera_record.h"
#include <fcntl.h>
#include"rtsp_server.hh"
#include"yuv.h"
#include"../rknn/ssd/ssd_1808/ssd.h"

//global defines for camera record begin
#define RECORD_WIDTH          640
#define RECORD_HEIGHT         480
#define RECORD_STREAM_FORMAT  MPP_VIDEO_CodingAVC /*h.264/AVC Coding*/
#define RECORD_FRAMES         (30*60*10)                /*300s*/
#define RECORD_OUTFILE_PREFIX "/userdata/camera_record"
#define RECORD_FRAME_FORMAT   MPP_FMT_YUV420P /*NV12*/    
#define RECORD_ENCODE_MODE    MPP_ENC_RC_MODE_CBR /* MPP_ENC_RC_MODE_VBR*/

//global defines for camera record end
//*******************************************************
//global datastrcut begin
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
MpiEncTestData* p = NULL;
static char g_filename[128];
//global datastruct end
//*******************************************************
//global functions begin
static void getFileNameUpdate()
{
    struct timespec time;
    clock_gettime(CLOCK_REALTIME, &time);
    struct tm nowtime;
    localtime_r(&time.tv_sec, &nowtime);
    sprintf(g_filename, "%s_%04d-%02d-%02d_%02d:%02d:%02d.264", RECORD_OUTFILE_PREFIX,nowtime.tm_year + 1900, nowtime.tm_mon+1, nowtime.tm_mday,
    		nowtime.tm_hour, nowtime.tm_min, nowtime.tm_sec);
    printf("file name update is %s\n",g_filename);
}

//global functions end
void record_init()
{
    MPP_RET ret = MPP_OK;
    p = mpp_calloc(MpiEncTestData, 1);
    if (!p) {
        mpp_err_f("create MpiEncTestData failed\n");
        ret = MPP_ERR_MALLOC;
	return;
    }
    p->have_encoding_frame_count = 0;

    p->width        = RECORD_WIDTH;
    p->height       = RECORD_HEIGHT;
    p->hor_stride   = MPP_ALIGN(RECORD_WIDTH, 16);
    p->ver_stride   = MPP_ALIGN(RECORD_HEIGHT, 16);
    p->fmt          = RECORD_FRAME_FORMAT;
    p->type         = RECORD_STREAM_FORMAT;
    p->num_frames   = RECORD_FRAMES;

    if (p->fmt <= MPP_FMT_YUV420SP_VU)
        p->frame_size = p->hor_stride * p->ver_stride * 3 / 2;
    else if (p->fmt <= MPP_FMT_YUV422_UYVY) {
        p->hor_stride *= 2;
        p->frame_size = p->hor_stride * p->ver_stride;
    } else
        p->frame_size = p->hor_stride * p->ver_stride * 4;
    
    p->packet_size  = p->width * p->height;
 
    ret = mpp_buffer_get(NULL, &p->frm_buf, p->frame_size);
    if (ret) {
        mpp_err_f("failed to get buffer for input frame ret %d\n", ret);
	return;
    }

    mpp_log("record_init start w %d h %d type %d\n",p->width, p->height, p->type);
 
    ret = mpp_create(&p->ctx, &p->mpi);
    if (ret) {
        mpp_err("mpp_create failed ret %d\n", ret);
	return;
    }
    
    ret = mpp_init(p->ctx, MPP_CTX_ENC, p->type);
    if (ret) {
        mpp_err("mpp_init failed ret %d\n", ret);
	return;
    }

//prep config
    p->fps = 18;
    p->gop = 36;
    p->bps = p->width * p->height / 8 * p->fps; 
    p->prep_cfg.change        = MPP_ENC_PREP_CFG_CHANGE_INPUT |
                              MPP_ENC_PREP_CFG_CHANGE_ROTATION |
                              MPP_ENC_PREP_CFG_CHANGE_FORMAT;
    p->prep_cfg.width         = p->width;
    p->prep_cfg.height        = p->height;
    p->prep_cfg.hor_stride    = p->hor_stride;
    p->prep_cfg.ver_stride    = p->ver_stride;
    p->prep_cfg.format        = p->fmt;
    p->prep_cfg.rotation      = MPP_ENC_ROT_0;
    ret = p->mpi->control(p->ctx, MPP_ENC_SET_PREP_CFG, &p->prep_cfg);
    if (ret) {
        mpp_err("mpi control enc set prep cfg failed ret %d\n", ret);
	return;
    }
//rc config
    p->rc_cfg.change  = MPP_ENC_RC_CFG_CHANGE_ALL;
    p->rc_cfg.rc_mode = RECORD_ENCODE_MODE;
    p->rc_cfg.quality = MPP_ENC_RC_QUALITY_MEDIUM;
    p->rc_cfg.bps_target   = p->bps;
    p->rc_cfg.bps_max      = p->bps * 17 / 16;
    p->rc_cfg.bps_min      = p->bps * 15 / 16;
    
    /* fix input / output frame rate */
    p->rc_cfg.fps_in_flex      = 0;
    p->rc_cfg.fps_in_num       = p->fps;
    p->rc_cfg.fps_in_denorm    = 1;
    p->rc_cfg.fps_out_flex     = 0;
    p->rc_cfg.fps_out_num      = p->fps;
    p->rc_cfg.fps_out_denorm   = 1;
    p->rc_cfg.gop              = p->gop;
    p->rc_cfg.skip_cnt         = 0;
    mpp_log("mpi_enc_test bps %d fps %d gop %d\n",
            p->rc_cfg.bps_target, p->rc_cfg.fps_out_num, p->rc_cfg.gop);
    ret = p->mpi->control(p->ctx, MPP_ENC_SET_RC_CFG, &p->rc_cfg);
    if (ret) {
        mpp_err("mpi control enc set rc cfg failed ret %d\n", ret);
	return;
    }
//codec config
    p->codec_cfg.coding = p->type;
    p->codec_cfg.h264.change = MPP_ENC_H264_CFG_CHANGE_PROFILE |
                                 MPP_ENC_H264_CFG_CHANGE_ENTROPY |
                                 MPP_ENC_H264_CFG_CHANGE_TRANS_8x8;
        /*
         * H.264 profile_idc parameter
         * 66  - Baseline profile
         * 77  - Main profile
         * 100 - High profile
         */
    p->codec_cfg.h264.profile  = 100;
        /*
         * H.264 level_idc parameter
         * 10 / 11 / 12 / 13    - qcif@15fps / cif@7.5fps / cif@15fps / cif@30fps
         * 20 / 21 / 22         - cif@30fps / half-D1@@25fps / D1@12.5fps
         * 30 / 31 / 32         - D1@25fps / 720p@30fps / 720p@60fps
         * 40 / 41 / 42         - 1080p@30fps / 1080p@30fps / 1080p@60fps
         * 50 / 51 / 52         - 4K@30fps
         */
    p->codec_cfg.h264.level    = 40;
    p->codec_cfg.h264.entropy_coding_mode  = 1;
    p->codec_cfg.h264.cabac_init_idc  = 0;
    p->codec_cfg.h264.transform8x8_mode = 1;
    ret = p->mpi->control(p->ctx, MPP_ENC_SET_CODEC_CFG, &p->codec_cfg);
    if (ret) {
        mpp_err("mpi control enc set codec cfg failed ret %d\n", ret);
        return;
    }
//optional config
    p->sei_mode = MPP_ENC_SEI_MODE_ONE_FRAME;
    ret = p->mpi->control(p->ctx, MPP_ENC_SET_SEI_CFG, &p->sei_mode);
    if (ret) {
        mpp_err("mpi control enc set sei cfg failed ret %d\n", ret);
        return;
    }
}
void * global_sps_pps_ptr=NULL;
int global_sps_pps_len=0;
int global_rtsp_fd = -1;
#define FIFO_NAME     "/tmp/H264_fifo"

size_t fifo_write_len = 0;
int sps_pps_flag = 0;
void frame_post_process(char* fbuf);
void record_put_frame(char* fbuf)
{
    MPP_RET ret;
    char * temp;
    MppFrame frame = NULL;
    MppPacket packet = NULL;
    frame_post_process(fbuf);
    if(0 == p->have_encoding_frame_count){
	getFileNameUpdate();
	p->fp_output = fopen(g_filename, "w+b");
        ret = p->mpi->control(p->ctx, MPP_ENC_GET_EXTRA_INFO, &packet);
        if (ret) {
            mpp_err("mpi control enc get extra info failed\n");
	    return;
        }
        /* get and write sps/pps for H.264 */
        if (packet) {
            void *ptr   = mpp_packet_get_pos(packet);
            size_t len  = mpp_packet_get_length(packet);
	    if(global_sps_pps_ptr!=NULL)
		    free(global_sps_pps_ptr);
	    global_sps_pps_ptr = malloc(len);
	    memcpy(global_sps_pps_ptr,ptr,len);
	    global_sps_pps_len = len;
            fwrite(ptr, 1, len, p->fp_output);
            packet = NULL;
        }
    }

    void *buf = mpp_buffer_get_ptr(p->frm_buf);
//file yuv image
    memcpy(buf,fbuf,p->frame_size);
    ret = mpp_frame_init(&frame);
    if (ret) {
	mpp_err_f("mpp_frame_init failed\n");
        return;
    }

    mpp_frame_set_width(frame, p->width);
    mpp_frame_set_height(frame, p->height);
    mpp_frame_set_hor_stride(frame, p->hor_stride);
    mpp_frame_set_ver_stride(frame, p->ver_stride);
    mpp_frame_set_fmt(frame, p->fmt);
    mpp_frame_set_eos(frame, p->frm_eos);
    mpp_frame_set_buffer(frame, p->frm_buf);
    
    ret = p->mpi->encode_put_frame(p->ctx, frame);
    if (ret) {
	mpp_err("mpp encode put frame failed\n");
	return;
    }
    ret = p->mpi->encode_get_packet(p->ctx, &packet);
    if (ret) {
	mpp_err("mpp encode get packet failed\n");
        return;
    }

    if (packet) {
	void *ptr   = mpp_packet_get_pos(packet);
        size_t len  = mpp_packet_get_length(packet);
	fwrite(ptr, 1, len, p->fp_output);
	if(!access(FIFO_NAME,F_OK)){
		if(global_rtsp_fd == -1){
			global_rtsp_fd = open(FIFO_NAME, O_WRONLY );
			write(global_rtsp_fd, global_sps_pps_ptr, global_sps_pps_len);
            printf("WANG: sps_pps is: ");
            temp = (char *)global_sps_pps_ptr;
            for(int ii=0;ii<global_sps_pps_len; ii++)
            {
                printf(" %x",*(temp+ii));
            }
            printf("\r\n");
			printf("write sps,pps!!!!!!!!!!!\n");
		}
        if(sps_pps_flag == 0){
            if(getRtspPlay_flag() == 1){
                sps_pps_flag = 1;
                setRtspPlay_flag();
			    write(global_rtsp_fd, global_sps_pps_ptr, global_sps_pps_len);
		    	printf("write sps,pps!!!!!!!!!!!\n");
            }
        }
        if(getRtspPlay_flag() == 0){
            sps_pps_flag = 0;
        }

		write(global_rtsp_fd,ptr,len);
		printf("write frame : len = [%d]!!!!\n",len);
        fifo_write_len = len;
		//void push_vc(int size,void*p)
	  
    }
        mpp_packet_deinit(&packet);
    }

    p->have_encoding_frame_count++;
    if(p->have_encoding_frame_count%30 == 0)
	printf("have_encoding_frame_count++  %d\n",p->have_encoding_frame_count);
    if(RECORD_FRAMES == p->have_encoding_frame_count){
	fclose(p->fp_output);
	p->have_encoding_frame_count = 0;
    }
}
extern struct ssd_group g_ssd_group[2];
extern int cur_group;
void frame_post_process(char* fbuf)
{
	struct ssd_group *group = &g_ssd_group[cur_group];
	stDrawLineInfo drawLineInfo;
	drawLineInfo.lineWidth = 1;

	stYuvBuffInfo yuvBuffInfo;
	yuvBuffInfo.pYuvBuff = fbuf;
	yuvBuffInfo.width = RECORD_WIDTH;
	yuvBuffInfo.height = RECORD_HEIGHT;
	yuvBuffInfo.yuvType = TYPE_YUV420P_YV12;

	drawLineInfo.clrIdx = LINE_1_COLOR;
	drawLineInfo.startPoint.x = LINE_1_B_X;
	drawLineInfo.startPoint.y = LINE_1_B_Y;
	drawLineInfo.endPoint.x = LINE_1_E_X;
	drawLineInfo.endPoint.y = LINE_1_E_Y;
	yuv_drawline(&yuvBuffInfo, &drawLineInfo);

	drawLineInfo.clrIdx = LINE_2_COLOR;
	drawLineInfo.startPoint.x = LINE_2_B_X;
	drawLineInfo.startPoint.y = LINE_2_B_Y;
	drawLineInfo.endPoint.x = LINE_2_E_X;
	drawLineInfo.endPoint.y = LINE_2_E_Y;
	yuv_drawline(&yuvBuffInfo, &drawLineInfo);

	drawLineInfo.clrIdx = LINE_3_COLOR;
	drawLineInfo.startPoint.x = LINE_3_B_X;
	drawLineInfo.startPoint.y = LINE_3_B_Y;
	drawLineInfo.endPoint.x = LINE_3_E_X;
	drawLineInfo.endPoint.y = LINE_3_E_Y;
	yuv_drawline(&yuvBuffInfo, &drawLineInfo);

	stDrawRectInfo pDrawRectInfo;
        pDrawRectInfo.lineWidth = 4;
//	pDrawRectInfo.clrIdx = DEFAULT_RECT_COLOR;
//        pDrawRectInfo.startPoint.x = 100;
//        pDrawRectInfo.startPoint.y = 100;
//        pDrawRectInfo.endPoint.x = 400;
//        pDrawRectInfo.endPoint.y = 400;
//	draw_rect(&yuvBuffInfo,&pDrawRectInfo);
	for (int i = 0; i < group->count; ++i) {
        	pDrawRectInfo.startPoint.x = group->objects[i].select.left * 640/300;
        	pDrawRectInfo.startPoint.y = group->objects[i].select.top*480/300;
        	pDrawRectInfo.endPoint.x = group->objects[i].select.right* 640/300;
        	pDrawRectInfo.endPoint.y = group->objects[i].select.bottom*480/300;
		if(LINE3(pDrawRectInfo.startPoint.x) > pDrawRectInfo.endPoint.y){
			pDrawRectInfo.clrIdx = DEFAULT_RECT_COLOR;
			printf("DEFAULT_RECT_COLOR,%d,%d,%d\n",pDrawRectInfo.startPoint.x,LINE3(pDrawRectInfo.startPoint.x),pDrawRectInfo.endPoint.y);
		}else if (LINE2(pDrawRectInfo.startPoint.x) > pDrawRectInfo.endPoint.y){
			pDrawRectInfo.clrIdx = LINE_3_COLOR;
			printf("3_COLOR,%d,%d,%d\n",pDrawRectInfo.startPoint.x,LINE2(pDrawRectInfo.startPoint.x),pDrawRectInfo.endPoint.y);
		}else if(LINE1(pDrawRectInfo.startPoint.x) > pDrawRectInfo.endPoint.y){
			pDrawRectInfo.clrIdx = LINE_2_COLOR;
			printf("2_COLOR,%d,%d,%d\n",pDrawRectInfo.startPoint.x,LINE1(pDrawRectInfo.startPoint.x),pDrawRectInfo.endPoint.y);
		}else if(LINE1(pDrawRectInfo.startPoint.x) < pDrawRectInfo.endPoint.y){
			pDrawRectInfo.clrIdx = LINE_1_COLOR;
			printf("1_COLOR,%d,%d,%d\n",pDrawRectInfo.startPoint.x,LINE3(pDrawRectInfo.startPoint.x),pDrawRectInfo.endPoint.y);
		}
		draw_rect(&yuvBuffInfo,&pDrawRectInfo);
	}
}
