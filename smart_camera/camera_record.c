#include"camera_record.h"

MpiEncTestData* p = NULL;
void * global_sps_pps_ptr=NULL;
int global_sps_pps_len=0;

static void getFileNameUpdate(char* filename)
{
    struct timespec time;
    clock_gettime(CLOCK_REALTIME, &time);
    struct tm nowtime;
    localtime_r(&time.tv_sec, &nowtime);
    sprintf(filename, "%s_%04d-%02d-%02d_%02d:%02d:%02d.264", RECORD_OUTFILE_PREFIX,nowtime.tm_year + 1900, nowtime.tm_mon+1, nowtime.tm_mday,
                nowtime.tm_hour, nowtime.tm_min, nowtime.tm_sec);
    printf("file name update is %s\n",filename);
}

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
void record_put_frame(char* fbuf)
{
    MPP_RET ret;
    char * temp;
    MppFrame frame = NULL;
    MppPacket packet = NULL;
    if(0 == p->have_encoding_frame_count){
	char filename[128];
        getFileNameUpdate(filename);
        p->fp_output = fopen(filename, "w+b");
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
        mpp_packet_deinit(&packet);
    }

    p->have_encoding_frame_count++;
    if(RECORD_FRAMES == p->have_encoding_frame_count){
        fclose(p->fp_output);
        p->have_encoding_frame_count = 0;
        printf("save .h264 file success !!!\r\n");
    }
}

