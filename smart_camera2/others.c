#include"tjstal.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <math.h>
#include "others.h"
#include <stdio.h>
#include "algorithm.h"

int g_rga_algorithm_buf_fd;
bo_t g_rga_algorithm_buf_bo;
int g_rga_record_buf_fd;
bo_t g_rga_record_buf_bo;
char *g_mem_buf;
char *g_algorithm_mem_buf;

long getCurrentTime(void)
{
    struct timeval tv;
    gettimeofday(&tv, 0);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}



WORD flowId;

/**
 * 发送应答指令函数
 * @param flowId        应答相应的流水号
 * @param functionCode  应答相应的功能码
 * @param dataArea      功能码相应的数据区的指针
 * @param dataLen       数据区长度
 * @return NULL
 */
/*void SendRequestAck(WORD flowId, const BYTE functionCode, void *dataArea,int dataLen)
{
    BYTE rawBinarySeq[1024];
    BYTE binarySeq[1024];
    int rawBinarySeqLen = 0;
    int binarySeqLen = 0;

    memset(binarySeq, 0, 1024);

    rawBinarySeqLen = EncoderRequestAck(rawBinarySeq, flowId, functionCode, dataArea, dataLen);
    DoEscapeForSend(rawBinarySeq, binarySeq, rawBinarySeqLen, &binarySeqLen); //转义

    //发送ACK
    send(socket_fd, binarySeq, binarySeqLen, 0);
}*/

/**
 *
 *
 *
 */
int do_deal_request(char command_buf[],int length)
{
	BYTE checksum;
	BYTE valid_command[1024];
    WORD flowId = 0;
	int i;

    printf("enter do_deal_request()!!!\r\n");

	DoEscapeForReceive(command_buf, valid_command, length);
	CheckSum(&valid_command[3],length - 5,&checksum);
		
    printf("valid command is:\r\n");
    for(i=0;i<length-2;i++)
		printf("[%d]:0x%x\n", i, valid_command[i]);

	if(valid_command[0] != checksum)
	{
		printf("checksum error , 0x%x,0x%x\n",valid_command[0],checksum);
		return 0;
	}

    flowId = ((WORD)valid_command[1] << 8) |  valid_command[2];
    allFlowId = flowId;
    printf("flowId : 0x%x\r\n",flowId);
    
	printf("sequence id 0x%x%x,factory id 0x%x%x,periphral id 0x%x,function code %x\n",valid_command[1],valid_command[2],valid_command[3],valid_command[4],valid_command[5],valid_command[6]);
	switch(valid_command[6]){
		case TJSATL_FUNCTION_CODE_QUERY:                    //0x2F

            SendRequestAck(flowId, TJSATL_FUNCTION_CODE_QUERY, NULL, 0);
			printf("TJSATL_FUNCTION_CODE_QUERY\n");
			break;
		case TJSATL_FUNCTION_CODE_RECOVERY_DEFUALT_PARAM:   //0x30

            SendRequestAck(flowId, TJSATL_FUNCTION_CODE_RECOVERY_DEFUALT_PARAM, NULL, 0);
			printf("TJSATL_FUNCTION_CODE_RECOVERY_DEFUALT_PARAM\n");

			break;
		case TJSATL_FUNCTION_CODE_REALTIME_DATA:            //0x31

            RealTimeData_Decode(&valid_command[7]);

			printf("TJSATL_FUNCTION_CODE_REALTIME_DATA\n");
			break;
		case TJSATL_FUNCTION_CODE_QUERY_BASE_INFO:          //0x32

            GetBaseInfoToAck(flowId, TJSATL_FUNCTION_CODE_QUERY_BASE_INFO);
			printf("TJSATL_FUNCTION_CODE_QUERY_BASE_INFO\n");
			break;
		case TJSATL_FUNCTION_CODE_UPDATE:                   //0x33
			printf("TJSATL_FUNCTION_CODE_UPDATE\n");

			break;
		case TJSATL_FUNCTION_CODE_QUERY_PARAM:              //0x34

            GetBSDParamToAck(flowId, TJSATL_FUNCTION_CODE_QUERY_PARAM);
			printf("TJSATL_FUNCTION_CODE_QUERY_PARAM\n");

			break;
		case TJSATL_FUNCTION_CODE_SETTING_PARAM:            //0x35

            SetBSDParamToAck(flowId, TJSATL_FUNCTION_CODE_SETTING_PARAM);

			printf("TJSATL_FUNCTION_CODE_SETTING_PARAM\n");

			break;
		case TJSATL_FUNCTION_CODE_ALERT_UPDATE:             //0x36  无接收，只上传
			printf("TJSATL_FUNCTION_CODE_ALERT_UPDATE\n");

			break;
		case TJSATL_FUNCTION_CODE_PERIPHERAL_STATUS_QUERY:  //0x37  

            GetPeripheralStatusToAck(flowId, TJSATL_FUNCTION_CODE_PERIPHERAL_STATUS_QUERY);
			printf("TJSATL_FUNCTION_CODE_PERIPHERAL_STATUS_QUERY\n");

			break;
		case TJSATL_FUNCTION_CODE_PERIPHERAL_STATUS_REPORT:     //0x38 先上传，再接收应答
			printf("TJSATL_FUNCTION_CODE_PERIPHERAL_STATUS_REPORT\n");

			break;
		case TJSATL_FUNCTION_CODE_REQUEST_MULTIMEDIA_DATA:
			printf("TJSATL_FUNCTION_CODE_REQUEST_MULTIMEDIA_DATA\n");

			break;
		case TJSATL_FUNCTION_CODE_UPLOAD_MULTIMEDIA_DATA:
			printf("TJSATL_FUNCTION_CODE_UPLOAD_MULTIMEDIA_DATA\n");

			break;
		case TJSATL_FUNCTION_CODE_TAKE_PHOTO_NOW:
			printf("TJSATL_FUNCTION_CODE_TAKE_PHOTO_NOW\n");

			break;
        default:
            printf("TJSATL_FUNCTION_CODE_ERROR\r\n");
            return 0;
            break;
	};
    //commuState = COMMU_IDLE;
    return 1;

}

/**
 *  
 *
 */
void ssd_camera_callback(void *p, int w, int h, int *flag)
{
    unsigned char *srcbuf = (unsigned char *)p;
    rgaTRANSFER(g_mem_buf, SRC_RKRGA_FMT, srcbuf, w, h,
                    DST_RKRGA_RECORD_FMT, g_rga_record_buf_fd, 640, 480);
    record_put_frame(g_mem_buf);
    static int i = 0;
    if(!(i%30)){
        printf("get frame %d\r\n", i++);
    }

    if(1){
        long runTime1 = getCurrentTime();
        rgaTRANSFER(g_algorithm_mem_buf, SRC_RKRGA_FMT, srcbuf, w, h,
                    DST_RKRGA_FMT, g_rga_algorithm_buf_fd, 300, 300);
        run_ssd(g_algorithm_mem_buf);

        //YUV420toRGB24_RGA(SRC_RKRGA_FMT, srcbuf, w, h,
        //                  DST_RKRGA_FMT, g_rga_buf_fd, DST_W, DST_H);
        //YUV420toRGB24_RGA_update(g_mem_buf, SRC_RKRGA_FMT, srcbuf, w, h,
        //                  DST_RKRGA_FMT, g_rga_buf_fd, DST_W, DST_H);
        //memcpy(g_mem_buf, g_rga_buf_bo.ptr, DST_W * DST_H * DST_BPP / 8);
        long runTime2 = getCurrentTime();
        printf("kelland copy rknn run time:%0.2ldms\r\n", runTime2 - runTime1);
    }
}

void buffer_init()
{
    int ret = 0;
    ret = c_RkRgaInit();
    if(ret){
        printf("c_RkRgaInit error : %s\r\n", strerror(errno));
        return ret;
    }

    ret = c_RkRgaGetAllocBuffer(&g_rga_algorithm_buf_bo, 300, 300, 24);
    if(ret){
        printf("c_RkRgaGetAllocBuffer error : %s\r\n", strerror(errno));
        return ret;
    }

    ret = c_RkRgaGetMmap(&g_rga_algorithm_buf_bo);
    if(ret){
        printf("c_RkRgaGetMmap error : %s\r\n", strerror(errno));
        return ret;
    }

    ret = c_RkRgaGetBufferFd(&g_rga_algorithm_buf_bo, &g_rga_algorithm_buf_fd);
    if(ret){
        printf("c_rkRgaGetBufferFd error : %s\r\n", strerror(errno));
    }

    ret = c_RkRgaGetAllocBuffer(&g_rga_record_buf_bo, 640, 480, 24);
    if(ret){
        printf("c_RkRgaGetAllocBuffer error : %s\r\n", strerror(errno));
        return ret;
    }

    ret = c_RkRgaGetMmap(&g_rga_record_buf_bo);
    if(ret){
        printf("c_RkRgaGetMmap error : %s\r\n", strerror(errno));
        return ret;
    }

    ret = c_RkRgaGetBufferFd(&g_rga_record_buf_bo, &g_rga_record_buf_fd);
    if(ret){
        printf("c_RkRgaGetBufferFd error : %s\r\n", strerror(errno));
    }
}

void buffer_deinit()
{
    int ret = -1;
    close(g_rga_algorithm_buf_fd);
    ret = c_RkRgaUnmap(&g_rga_algorithm_buf_bo);
    if(ret){
        printf("c_RkRgaUnmap error : %s\r\n", strerror(errno));
    }
    ret = c_RkRgaFree(&g_rga_algorithm_buf_bo);

    close(g_rga_record_buf_fd);
    ret = c_RkRgaUnmap(&g_rga_record_buf_bo);
    if(ret){
        printf("c_RkRgaUnmap error : %s\r\n", strerror(errno));
    }

    ret = c_RkRgaFree(&g_rga_record_buf_bo);
}

void *camera_thread(void)
{
    record_init();
    buffer_init();
    if(!g_mem_buf){
        g_mem_buf = (char *)malloc(640 * 480 * DST_BPP / 8);
    }

    if(!g_algorithm_mem_buf){
        g_algorithm_mem_buf = (char *)malloc(300 * 300 * 24 / 8);
    }

    ssd_init();
    cameraRun("/dev/video0", SRC_W, SRC_H, SRC_FPS, SRC_V4L2_FMT, ssd_camera_callback, 1);
    buffer_deinit();
}





