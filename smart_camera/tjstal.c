#include "tjstal.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <math.h>
#include <string.h>
#include <pthread.h>


int connectServerSuccessFlag = 0;


//int socket_fd;
WORD allFlowId = 0;

//COMMU_STATE_E commuState = COMMU_IDLE;
REALTIME_DATA_T realTime_Data;
BASE_INFO_T base_Info;
BSD_PARAMETER_T BSD_Param = {2,2};
PERIPHERAL_STATUS_T peripheral_Status;

COMMU_PARAM_T commuParam;

pthread_mutex_t sendLock;
pthread_mutex_t connectFlagLock;

struct timeval timeout;

MULTIMEDIA_FIFO_T multiMediaFifo;
volatile DWORD multiMediaId = 0;

//char sendBuf[4096] = {0};       //待发送缓冲区
//int  sendDataLen = 0;
//char sendData_flag = 0;         //是否发送数据标志
//char isSendAck_flag = 0;        //发送的数据是否是应答


/**
 * 检查是否只包含两个标识位，如果包含的标识位不是两个（0，1或者不止两个），则这条信息是非法的。
 * @param rawBinarySeq  接收的二进制序列
 * @param rawbinarySeqLen  rawBinarySeq序列数组长度
 * @return JTT_ERROR_E           没有错误将返回  ERR_NONE, 非法序列则返回  ERR_INVALIDATE_MSG
 */
static JTT_ERROR_E validatedIdentifierBits(const BYTE rawBinarySeq[], const int rawbinarySeqLen) {
    int total = 0;
    int i = 0;

    for (i = 0; i < rawbinarySeqLen; i ++) {
        if (0x7e == rawBinarySeq[i]) {
            total += 1;
        }
    }
    return 2 == total ? ERR_NONE : ERR_INVALIDATE_MSG;
}

/**
 * 判断当前数据包是否有效，即验证校验码
 *
 *
 */
JTT_ERROR_E CheckSum(BYTE binarySeq[], const int len,BYTE* checksum) {
	int i = 0;
	*checksum = 0;
	for(i;i<len;i++)
		*checksum += binarySeq[i];
}

// 转义相关函数
/**
 * 接收一个从服务端发来的原始二进制序列，
 * 去除标识位并转义，返回只包含消息头、消息体以及校验码的二进制序列
 * @param rawBinarySeq  转义前的原始二进制序列
 * @param binarySeq     转义后的二进制序列
 * @param rawbinarySeqLen rawBinarySeq数组长度
 * @return JTT_ERROR_E 错误类型。
 */
//  * @param binarySeqLen binarySeq数组长度
JTT_ERROR_E DoEscapeForReceive(const BYTE rawBinarySeq[], BYTE binarySeq[], const int rawbinarySeqLen/*, const int binarySeqLen*/) {
    int i = 0, j = 0;
    JTT_ERROR_E err = ERR_NONE;


    err = validatedIdentifierBits(rawBinarySeq, rawbinarySeqLen);
    if (ERR_NONE != err) {
        return err;
    }

    if (MSG_MIN_LEN > rawbinarySeqLen) {
        return ERR_LENGTH_TOO_SHORT;
    }

    // if (MSG_MAX_LEN < rawbinarySeqLen) {
    //     return ERR_LENGTH_TOO_LONG;
    // }

    for (i = 0; i < rawbinarySeqLen; i++) {

        if (0x7e == rawBinarySeq[i]) {
            continue;
        }

        if (0x7d == rawBinarySeq[i]) {
            if (0x02 == rawBinarySeq[i + 1]) {
                binarySeq[j++] = 0x7e;
                i++;
                continue;
            }
            if (0x01 ==  rawBinarySeq[i + 1]) {
                binarySeq[j++] = 0x7d;
                i++;
                continue;
            }
            return ERR_INVALIDATE_MSG;
        }
        binarySeq[j++] = rawBinarySeq[i];
    }
    return ERR_NONE;
}


/**
 * 接收一个消息体以及校验码的二进制序列，将其转义并加上标识符，等待发送
 * @param   rawBinarySeq    转义前的原始二级制序列
 * @param   binarySeq       转义后的二进制序列，包含标识符
 * @param   rawBinarySeqLen rawBinarySeq数组长度
 * @param   *binarySeqLen   binarySeq数组长度
 * @return  JTT_ERROR_E       错误类型
 */
JTT_ERROR_E DoEscapeForSend(const BYTE rawBinarySeq[], BYTE binarySeq[], const int rawBinarySeqLen, int *binarySeqLen)
{
    int i = 0, j = 0;
    binarySeq[j++] = 0x7e;
    for(i=0; i<rawBinarySeqLen; i++){
        if(0x7e == rawBinarySeq[i]){
            binarySeq[j++] = 0x7d;
            binarySeq[j++] = 0x02;
            continue;
        }

        if(0x7d == rawBinarySeq[i]){
            binarySeq[j++] = 0x7d;
            binarySeq[j++] = 0x01;
            continue;
        }
        binarySeq[j++] = rawBinarySeq[i];
    }
    binarySeq[j++] = 0x7e;
    *binarySeqLen = j;

    return ERR_NONE;
}


/**
 * 发送应答指令函数
 * @param flowId        应答相应的流水号
 * @param functionCode  应答相应的功能码
 * @param dataArea      功能码相应的数据区的指针
 * @param dataLen       数据区长度
 * @return NULL     
 */
void SendRequestAck(WORD flowId, const BYTE functionCode, void *dataArea,int dataLen)
{
    BYTE rawBinarySeq[1024];
    BYTE binarySeq[1024];
    int rawBinarySeqLen = 0;
    int binarySeqLen = 0;

    memset(binarySeq, 0, 1024);

    rawBinarySeqLen = EncoderRequestAck(rawBinarySeq, flowId, functionCode, dataArea, dataLen);
    DoEscapeForSend(rawBinarySeq, binarySeq, rawBinarySeqLen, &binarySeqLen);
    
    memcpy(commuParam.sendBuf, binarySeq, binarySeqLen);
    commuParam.sendDataLen = binarySeqLen;
    commuParam.sendDataFlag = 1;

    //send(socket_fd, binarySeq, binarySeqLen, 0);
}


/**
 *
 *
 *
 */
void QueryToAck(WORD flowId, const BYTE functionCode)
{
    commuParam.isSendAckFlag = 1;
    SendRequestAck(flowId, functionCode, NULL, 0);
}

/**
 *
 *
 *
 */
void RecoveryDefaultParamToAck(WORD flowId, const BYTE functionCode)
{
    commuParam.isSendAckFlag = 1;
    SendRequestAck(flowId, functionCode, NULL, 0);
}


/**
 * 编码应答指令帧，用以获取完整的应答,不包含标识符
 * @param rawBinarySeq  未转义的原始数组
 * @param flowId    流水号
 * @param vendorId  厂商编号
 * @param peirphId  外设编号
 * @param functionCode 功能码
 * @param *dataArea    功能码对应的数据区
 * @param dataLen      数据区的长度
 * @return  编码好的应答指令帧的长度
 */
int EncoderRequestAck(BYTE rawBinarySeq[], WORD flowId,BYTE functionCode,void *dataArea,int dataLen)
{
    BYTE checksum = 0;
    BYTE *datap = dataArea;

    memset(rawBinarySeq, 0, 1024);

    rawBinarySeq[1] = (flowId >> 8) & 0xff;
    rawBinarySeq[2] = flowId & 0xff;

    rawBinarySeq[3] = (VENDOR_ID >> 8) & 0xff;
    rawBinarySeq[4] = VENDOR_ID &0xff;

    rawBinarySeq[5] = PEIRPH_ID;
    rawBinarySeq[6] = functionCode;

    if(dataLen != 0){
        memcpy(&rawBinarySeq[7], datap, dataLen);
    }

    CheckSum(&rawBinarySeq[3], dataLen+4, &checksum);

    rawBinarySeq[0] = checksum;

    return (dataLen + 7);
}

/**
 * 终端发给外设的实时数据分析函数 功能码：0x31
 * @param   void *realTimeData  实时数据的指针
 * @return NULL
 */
void RealTimeData_Decode(BYTE *data)
{
    realTime_Data.speed = data[0];
    realTime_Data.hold0 = data[1];
    realTime_Data.distance = ((DWORD)data[2]<<24 & 0xff000000) | ((DWORD)data[3]<<16 & 0xff0000) | ((DWORD)data[4]<<8 & 0xff00) | data[5];
    realTime_Data.hold1[0] = data[6];
    realTime_Data.hold1[1] = data[7];
    realTime_Data.altitude = ((WORD)data[8]<<8 & 0xff00) | data[9];
    realTime_Data.latitude = ((DWORD)data[10]<<24 & 0xff000000) | ((DWORD)data[11]<<16 & 0xff0000) | ((DWORD)data[12]<<8 & 0xff00) | data[13];
    realTime_Data.longitude = ((DWORD)data[14]<<24 & 0xff000000) | ((DWORD)data[15]<<16 & 0xff0000) | ((DWORD)data[16]<<8 & 0xff00) | data[17]; 
    realTime_Data.time[0] = data[18];
    realTime_Data.time[1] = data[19];
    realTime_Data.time[2] = data[20];
    realTime_Data.time[3] = data[21];
    realTime_Data.time[4] = data[22];
    realTime_Data.time[5] = data[23];
    
    
}

/**
 * 获取外设基本信息，保存并等待反馈         0x32
 * @param   *dataArea   保存的数据区的指针
 * @return dataLen      数据区的长度
 */
int GetBaseInfoToAck(WORD flowId, BYTE functionCode)
{
    BYTE dataArea[4096] = {0};
    int dataLen = 0;
    
    dataArea[0] = base_Info.company_name_length;
    dataLen ++;
    strncpy((char *)&dataArea[dataLen], base_Info.company_name, base_Info.company_name_length);
    dataLen += base_Info.company_name_length;
    
    dataArea[dataLen] = base_Info.product_code_length;
    dataLen ++;
    strncpy((char *)&dataArea[dataLen], base_Info.product_code, base_Info.product_code_length);
    dataLen += base_Info.product_code_length;

    dataArea[dataLen] = base_Info.hardware_version_length;
    dataLen ++;
    strncpy((char *)&dataArea[dataLen], base_Info.hardware_version, base_Info.hardware_version_length);
    dataLen += base_Info.hardware_version_length;

    dataArea[dataLen] = base_Info.software_version_length;
    dataLen ++;
    strncpy((char *)&dataArea[dataLen], base_Info.software_version, base_Info.software_version_length);
    dataLen += base_Info.software_version_length;

    dataArea[dataLen] = base_Info.machine_id_length;
    dataLen ++;
    strncpy((char *)&dataArea[dataLen], base_Info.machine_id, base_Info.machine_id_length);
    dataLen += base_Info.machine_id_length;

    dataArea[dataLen] = base_Info.customer_code_length;
    dataLen ++;
    strncpy((char *)&dataArea[dataLen], base_Info.customer_code, base_Info.customer_code_length);
    dataLen += base_Info.customer_code_length;


    commuParam.isSendAckFlag = 1;

    SendRequestAck(flowId, functionCode, dataArea, dataLen);
    
}

/**
 * 查询忙去监测系统参数         0x34
 * @param flowId        流水号
 * @param functionCode  功能码
 * @return NULL
 */
void GetBSDParamToAck(WORD flowId, BYTE functionCode)
{
    BYTE dataArea[20] = {0};

    dataArea[0] = BSD_Param.rear_threshold;
    dataArea[1] = BSD_Param.lateral_rear_threshold;

    commuParam.isSendAckFlag = 1;

    SendRequestAck(flowId, functionCode, &dataArea, 2);
}

/**
 * 设置盲区尖刺系统参数             0x35
 * @param flowId        流水号
 * @param functionCode  功能码
 * @param *dataArea     数据区
 * @param dataLen       数据区长度
 * @return NULL
 */
void SetBSDParamToAck(WORD flowId, BYTE functionCode, BYTE *dataArea)
{
    BYTE dataAck = 0;
    if(dataArea[0] < 0 || dataArea[0] > 10 || dataArea[1] < 0 || dataArea[1] > 10){
        dataAck = 1;
    }
    else {
        BSD_Param.rear_threshold = dataArea[0];
        BSD_Param.lateral_rear_threshold = dataArea[1];

        dataAck = 0;
    }

    commuParam.isSendAckFlag = 1;

    SendRequestAck(flowId, functionCode, &dataAck, 1);

}

/**
 * 查询外设工作状态         0x37
 * @param flowId        流水号
 * @param functionCode  功能码
 * @return NULL
 */
void GetPeripheralStatusToAck(WORD flowId, BYTE functionCode)
{
    BYTE dataArea[5] = {0};

    peripheral_Status.workStatus = periphStatus_NORMAL;
    peripheral_Status.alert.alertStatus = 0;

    dataArea[0] = peripheral_Status.workStatus;
    dataArea[1] = (peripheral_Status.alert.alertStatus >> 24) & 0xff;
    dataArea[2] = (peripheral_Status.alert.alertStatus >> 16) & 0xff;
    dataArea[3] = (peripheral_Status.alert.alertStatus >> 8) & 0xff;
    dataArea[4] = (peripheral_Status.alert.alertStatus) & 0xff;

    commuParam.isSendAckFlag = 1;

    SendRequestAck(flowId, functionCode, dataArea, 5);
    
}


/**
 * 外设上传工作状态指令
 * @param flowId        流水号
 * @param functionCode  功能码
 * @return NULL
 */
void GetPeripheralStatusToSend(WORD flowId, BYTE functionCode)
{

    BYTE dataArea[5] = {0};

    peripheral_Status.workStatus = periphStatus_NORMAL;
    peripheral_Status.alert.alertStatus = 0;

    dataArea[0] = peripheral_Status.workStatus;
    dataArea[1] = (peripheral_Status.alert.alertStatus >> 24) & 0xff;
    dataArea[2] = (peripheral_Status.alert.alertStatus >> 16) & 0xff;
    dataArea[3] = (peripheral_Status.alert.alertStatus >> 8) & 0xff;
    dataArea[4] = (peripheral_Status.alert.alertStatus) & 0xff;

    commuParam.isSendAckFlag = 1;

    SendRequestAck(flowId, functionCode, dataArea, 5);
}

/**
 * 外设上传工作状态回复指令分析
 * @param command_buf   接收到的数据
 * @param length        接收到的数据长度
 * return NULL
 */
/*void PeripheralStatusAck_Decode(char command_buf[], int length)
  {
  BYTE checksum;
  BYTE valid_command[1024];
  WORD flowId = 0gtrftr
    int i;
    DoEscapeForReceive(command_buf, valid_command, length);
    CheckSum(&valid_command[1],length - 3,&checksum);

    if(valid_command[0] != checksum){
        printf("checksum error , 0x%x,0x%x\r\n", valid_command[0], checksum);
        return;
    }

    flowId = (WORD)valid_command[2] << 8 + valid_command[3];

    printf("sequence id 0x%x%x,factory id 0x%x%x,periphral id 0x%x%x,function code %x\r\n",valid_command[1],valid_command[2],valid_command[3],valid_command[4],valid_command[5],valid_command[6]);

    if(valid_command[6] == TJSATL_FUNCTION_CODE_PERIPHERAL_STATUS_REPORT && flowId == allFlowId){
        commuState = COMMU_IDLE;
    }
}*/


/**
 *  多媒体数据待上传处理函数
 *
 */
void UpldMultiMediaData(char *buf, int len)
{
    
}

/**************** 多媒体相关处理函数 ****************/
/**
 * 多媒体数据结构初始化函数
 *
 */
void MultiMediaFifo_Init(MULTIMEDIA_FIFO_T *mp)
{
    memset(mp, 0, sizeof(MULTIMEDIA_FIFO_T));
    mp->rd = 0;
    mp->wt = 0;
}

/**
 *  判断 multiMediaFifo 是否为空
 *
 */
int MultiMediaFifo_isEmpty(MULTIMEDIA_FIFO_T *mp)
{
    if(mp->rd == mp->wt){
        return 1;
    }
    return 0;
}


/**
 *  上传报警类型函数 客户显示 内测使用
 *  @param  alertType   报警类型
 *  @param  multiMediaType  多媒体类型  0x00:图片   0x02:视频
 *  @param  multiMediaName  多媒体文件名称
 *  @param  x0,y0,x1,y1     识别物体的框坐标值
 *  @return NULL
 */
void UpldAlertDataForCustomer(BYTE alertType, BYTE multiMediaType, char *multiMediaName,
                                DWORD x0, DWORD y0, DWORD x1, DWORD y1)
{
    BYTE dataArea[50] = {0};
    int dataLen = 0;
    int slen = 0;


    multiMediaFifo.multiMediaParam[multiMediaFifo.wt].alertType = alertType;
    multiMediaFifo.multiMediaParam[multiMediaFifo.wt].multiMediaType = multiMediaType;
    slen = strlen(multiMediaName);
    multiMediaFifo.multiMediaParam[multiMediaFifo.wt].multiMediaNameLen = slen;
    strncpy(multiMediaFifo.multiMediaParam[multiMediaFifo.wt].multiMediaName, multiMediaName, slen);
    multiMediaFifo.multiMediaParam[multiMediaFifo.wt].multiMediaId = multiMediaId ++;



    dataArea[0] = 0x00;
    dataArea[1] = 0x00;
    dataArea[2] = 0x00;
    dataArea[3] = 0x00;

    dataArea[4] = 0x00;
    dataArea[5] = alertType;
    dataArea[6] = realTime_Data.speed;
    dataArea[7] = (realTime_Data.altitude >> 8) & 0xff;
    dataArea[8] = realTime_Data.altitude & 0xff;

    dataArea[9] = (realTime_Data.latitude >> 24) & 0xff;
    dataArea[10] = (realTime_Data.latitude >> 16) & 0xff;
    dataArea[11] = (realTime_Data.latitude >> 8) & 0xff;
    dataArea[12] = (realTime_Data.latitude) & 0xff;

    dataArea[13] = (realTime_Data.longitude >> 24) & 0xff;
    dataArea[14] = (realTime_Data.longitude >> 16) & 0xff;
    dataArea[15] = (realTime_Data.longitude >> 8) & 0xff;
    dataArea[16] = (realTime_Data.longitude) & 0xff;

    dataArea[17] = realTime_Data.time[0];
    dataArea[18] = realTime_Data.time[1];
    dataArea[19] = realTime_Data.time[2];
    dataArea[20] = realTime_Data.time[3];
    dataArea[21] = realTime_Data.time[4];
    dataArea[22] = realTime_Data.time[5];

    dataArea[23] = (realTime_Data.carStatus >> 8) & 0xff;
    dataArea[24] = (realTime_Data.carStatus) & 0xff;

    dataArea[25] = (x0 >> 24) & 0xff;
    dataArea[26] = (x0 >> 16) & 0xff;
    dataArea[27] = (x0 >> 8) & 0xff;
    dataArea[28] = (x0) & 0xff; 

    dataArea[29] = (y0 >> 24) & 0xff;
    dataArea[30] = (y0 >> 16) & 0xff;
    dataArea[31] = (y0 >> 8) & 0xff;
    dataArea[32] = (y0) & 0xff; 

    dataArea[33] = (x1 >> 24) & 0xff;
    dataArea[34] = (x1 >> 16) & 0xff;
    dataArea[35] = (x1 >> 8) & 0xff;
    dataArea[36] = (x1) & 0xff; 

    dataArea[37] = (y1 >> 24) & 0xff;
    dataArea[38] = (y1 >> 16) & 0xff;
    dataArea[39] = (y1 >> 8) & 0xff;
    dataArea[40] = (y1) & 0xff; 

    dataLen = 41;

    allFlowId ++;
    pthread_mutex_lock(&sendLock);
    SendRequestAck(allFlowId, TJSATL_FUNCTION_CODE_ALERT_UPDATE_FRAME, dataArea, dataLen);
    //SendData(&commuParam);
    send(commuParam.socket_fd, commuParam.sendBuf, commuParam.sendDataLen, 0);       //通过 socket 发送数据
    pthread_mutex_unlock(&sendLock);
}

/**
 *  上传报警类型函数
 *  @param  alertType   报警类型
 *  @param  multiMediaType  多媒体类型  0x00:图片   0x02:视频
 *  @param  multiMediaName  多媒体文件名称
 *  @return NULL
 */
void UpldAlertData(BYTE alertType, BYTE multiMediaType, char *multiMediaName)
{
    BYTE dataArea[50] = {0};
    int dataLen = 0;
    int slen = 0;


    multiMediaFifo.multiMediaParam[multiMediaFifo.wt].alertType = alertType;
    multiMediaFifo.multiMediaParam[multiMediaFifo.wt].multiMediaType = multiMediaType;
    slen = strlen(multiMediaName);
    multiMediaFifo.multiMediaParam[multiMediaFifo.wt].multiMediaNameLen = slen;
    strncpy(multiMediaFifo.multiMediaParam[multiMediaFifo.wt].multiMediaName, multiMediaName, slen);
    multiMediaFifo.multiMediaParam[multiMediaFifo.wt].multiMediaId = multiMediaId ++;



    dataArea[0] = 0x00;
    dataArea[1] = 0x00;
    dataArea[2] = 0x00;
    dataArea[3] = 0x00;

    dataArea[4] = 0x00;
    dataArea[5] = alertType;
    dataArea[6] = realTime_Data.speed;
    dataArea[7] = (realTime_Data.altitude >> 8) & 0xff;
    dataArea[8] = realTime_Data.altitude & 0xff;

    dataArea[9] = (realTime_Data.latitude >> 24) & 0xff;
    dataArea[10] = (realTime_Data.latitude >> 16) & 0xff;
    dataArea[11] = (realTime_Data.latitude >> 8) & 0xff;
    dataArea[12] = (realTime_Data.latitude) & 0xff;

    dataArea[13] = (realTime_Data.longitude >> 24) & 0xff;
    dataArea[14] = (realTime_Data.longitude >> 16) & 0xff;
    dataArea[15] = (realTime_Data.longitude >> 8) & 0xff;
    dataArea[16] = (realTime_Data.longitude) & 0xff;

    dataArea[17] = realTime_Data.time[0];
    dataArea[18] = realTime_Data.time[1];
    dataArea[19] = realTime_Data.time[2];
    dataArea[20] = realTime_Data.time[3];
    dataArea[21] = realTime_Data.time[4];
    dataArea[22] = realTime_Data.time[5];

    dataArea[23] = (realTime_Data.carStatus >> 8) & 0xff;
    dataArea[24] = (realTime_Data.carStatus) & 0xff;

    dataLen = 25;

    allFlowId ++;
    pthread_mutex_lock(&sendLock);
    SendRequestAck(allFlowId, TJSATL_FUNCTION_CODE_ALERT_UPDATE, dataArea, dataLen);
    SendData(&commuParam);
    pthread_mutex_unlock(&sendLock);
}


/**
 * 终端应答帧解析
 *
 */
int TerminalAckDecode(BYTE command_buf[], int length)
{
    BYTE checksum;
    BYTE valid_command[1024];
    WORD flowId = 0;

    int i;

    printf("enter TerminalAckDecode()!!!\r\n");

    DoEscapeForReceive(command_buf, valid_command, length);
    CheckSum(&valid_command[3], length-5, &checksum);

    if(valid_command[0] != checksum){
        printf("Terminal ack checksum err, 0x%x,0x%x\n", valid_command[0], checksum);
        return 0;
    }

    allFlowId  = (WORD)valid_command[1] << 8 + valid_command[2];

    printf("sequence id 0x%x%x,factory id 0x%x%x,periphral id 0x%x,function code %x\n",\ 
        valid_command[1],valid_command[2],valid_command[3],valid_command[4],valid_command[5],\
        valid_command[6]);
    
    
    return 1;
    

}



/**
 *  外设发送数据函数
 *  发送的可能是应答数据，主动发送数据
 */
void SendData(void *lpParam)
{
    COMMU_PARAM_T *cp = (COMMU_PARAM_T *)lpParam;

    struct timeval sendTime;    //发送数据的时间
    struct timeval waitTime;    //发送后的等待时间
    struct timeval currTime;    //当前时间
    struct timeval oldTime;

    int res;
    int acksucc;

    int circualStat = 1;

    int retransmitCounter = 0;
    char sendWorkStatusFlag = 0;

    //gettimeofday(&sendTime, NULL);
    //gettimeofday(&currTime,NULL);
    //gettimeofday(&oldTime,NULL);

    while(circualStat){

        switch(cp->commuState){
            case COMMU_IDLE:

                retransmitCounter = 0;
                if(cp->sendDataFlag == 1){
                    cp->commuState = COMMU_SEND_ORDER;
                    printf("COMMU_IDLE!!!\r\n");
                }

                break;
            case COMMU_SEND_ORDER:
                
                send(cp->socket_fd, cp->sendBuf, cp->sendDataLen, 0);       //通过 socket 发送数据
                
                if(cp->isSendAckFlag == 0){
                    cp->isSendAckFlag = 0;
                    //gettimeofday(&sendTime,NULL);
                    cp->commuState = COMMU_WAIT_ACK;

                    printf("COMMU_SEND_ORDER : ORDER !!!\r\n");
                    
                    timeout.tv_sec = 1;
                    timeout.tv_usec = 0;
                    setsockopt(commuParam.socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
                }
                else{
                    printf("COMMU_SEND_ORDER : ACK !!!\r\n");
                    cp->isSendAckFlag = 0;
                    cp->commuState = COMMU_COMPLETE;
                }

                break;
            case COMMU_WAIT_ACK:
                
                res = recv(cp->socket_fd, cp->recvBuf, MAXLINE, 0);
                if(res > 0){
                    printf("COMMU_WAIT_ACK : %d\r\n", res);
                    cp->recvDataLen = res;
                    acksucc = TerminalAckDecode(cp->recvBuf, cp->recvDataLen);
                    if(acksucc)
                        cp->commuState = COMMU_COMPLETE;
                    else
                        cp->commuState = COMMU_WAIT_TIMEOUT;
                }
                else{
                    if(res == -1){
                        printf("recv ack time out!\r\n");
                        cp->commuState = COMMU_WAIT_TIMEOUT;
                    }
                }
                /*
                if(cp->recvDataFlag == 1){
                    cp->recvDataFlag = 0; 
                    TerminalAckDecode(cp->recvBuf, cp->recvDataLen);            //终端应答帧分析 待实现
                    cp->commuState = COMMU_COMPLETE;
                }
                
                else{
                    gettimeofday(&waitTime,NULL);
                    if(SecondsToMilliSeconds(waitTime.tv_sec,waitTime.tv_usec) - SecondsToMilliSeconds(sendTime.tv_sec,sendTime.tv_usec) > 1000){
                        cp->commuState = COMMU_WAIT_TIMEOUT;
                    }
                }*/

                break;
            case COMMU_WAIT_TIMEOUT:

                printf("COMMU_WAIT_TIMEOUT !!!\r\n");

                retransmitCounter ++;
                if(retransmitCounter <= 3){
                    cp->commuState = COMMU_SEND_ORDER;
                }
                else
                {
                    cp->commuState = COMMU_ERROR;
                }
                break;
            case COMMU_ERROR:
                printf("COMMU_ERROR!!!\r\n");
                cp->commuState = COMMU_IDLE;
                cp->sendDataFlag = 0;
                circualStat = 0;
                pthread_mutex_lock(&connectFlagLock);
                connectServerSuccessFlag = 0;
                pthread_mutex_unlock(&connectFlagLock);
                //gettimeofday(&oldTime,NULL);
                break;
            case COMMU_COMPLETE:

                printf("COMMU_COMPLETE!!!\r\n");
                cp->commuState = COMMU_IDLE;
                cp->sendDataFlag = 0;
                circualStat = 0;
                break;
            default:
                break;
        }

	  //usleep(50); 
    }
}



