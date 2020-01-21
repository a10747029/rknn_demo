#ifndef TJSTAL_H_
#define TJSTAL_H_

#include <pthread.h>
#include <sys/socket.h>
#define MAXLINE 4096

#define VENDOR_ID 0x8888
#define PEIRPH_ID 0x67      //BSD


//command list
#define TJSATL_FUNCTION_CODE_QUERY                      0x2F
#define TJSATL_FUNCTION_CODE_RECOVERY_DEFUALT_PARAM 	0x30
#define TJSATL_FUNCTION_CODE_REALTIME_DATA		        0x31
#define TJSATL_FUNCTION_CODE_QUERY_BASE_INFO		    0x32
#define TJSATL_FUNCTION_CODE_UPDATE			            0x33
#define TJSATL_FUNCTION_CODE_QUERY_PARAM		        0x34
#define TJSATL_FUNCTION_CODE_SETTING_PARAM		        0x35
#define TJSATL_FUNCTION_CODE_ALERT_UPDATE		        0x36
#define TJSATL_FUNCTION_CODE_PERIPHERAL_STATUS_QUERY	0x37
#define TJSATL_FUNCTION_CODE_PERIPHERAL_STATUS_REPORT	0x38
#define TJSATL_FUNCTION_CODE_REQUEST_MULTIMEDIA_DATA	0x50
#define TJSATL_FUNCTION_CODE_UPLOAD_MULTIMEDIA_DATA	    0x51
#define TJSATL_FUNCTION_CODE_TAKE_PHOTO_NOW		        0x52

#define MSG_MIN_LEN 9
#define MAX_NAME_LEN 255


typedef unsigned char  BYTE;
typedef unsigned short WORD;
typedef unsigned int   DWORD;
// typedef unsigned int   DWORD;
typedef unsigned char  BCD;

extern struct timeval timeout;
extern pthread_mutex_t sendLock;
extern WORD allFlowId;
/*extern int socket_fd;
extern char sendBuf[4096];
extern int  sendDataLen = 0;
extern char sendData_flag = 0;
extern char isSendAck_flag = 0;
*/

typedef enum _COMMU_STATE{
    COMMU_IDLE          = 0,
    COMMU_SEND_ORDER,
    COMMU_RECVED_ORDER,
    COMMU_WAIT_ACK,
    COMMU_WAIT_TIMEOUT,
    COMMU_ERROR,
    COMMU_COMPLETE,

}COMMU_STATE_E;

extern COMMU_STATE_E commuState;

typedef enum _JTT_ERROR {
    ERR_NONE             = 0,
    ERR_INVALIDATE_MSG   = 1,
    ERR_INVALIDATE_PHONE = 2,
    ERR_LENGTH_TOO_SHORT = 3,
    ERR_LENGTH_TOO_LONG  = 4
}JTT_ERROR_E;

/**
 *  发送数据相关参数
 */
typedef struct _commuParam{
    int socket_fd;
    char sendBuf[4096];
    char recvBuf[4096];
    int sendDataLen;
    int recvDataLen;
    char recvDataFlag;
    char sendDataFlag;
    char isSendAckFlag;
    COMMU_STATE_E commuState;
}COMMU_PARAM_T;

extern COMMU_PARAM_T commuParam;

/**
 *  TJSATL_FUNCTION_CODE_REALTIME_DATA 0x31
 */
typedef struct _realtime_data {
	BYTE speed;
	BYTE hold0;
	DWORD distance;
	BYTE hold1[2];
	WORD altitude;
	DWORD latitude;
	DWORD longitude;
	BCD  time[6]; 
	WORD carStatus;
}REALTIME_DATA_T;

extern REALTIME_DATA_T realTime_Data;

/**
 * TJSATL_FUNCITON_CODE_QUERY_BASE_INFO 0x32
 */
typedef struct _base_info {
	BYTE company_name_length;
	BYTE company_name[MAX_NAME_LEN];
	BYTE product_code_length;
	BYTE product_code[MAX_NAME_LEN];	
	BYTE hardware_version_length;
	BYTE hardware_version[MAX_NAME_LEN];	
	BYTE software_version_length;
	BYTE software_version_code[MAX_NAME_LEN];	
	BYTE machine_id_length;
	BYTE machine_id_code[MAX_NAME_LEN];	
	BYTE customer_code_length;
	BYTE customer_code[MAX_NAME_LEN];	
}BASE_INFO_T;

extern BASE_INFO_T base_Info;

/**
 * TJSATL_FUNCTION_CODE_QUERY_PARAM 0x34
 */
typedef struct _bsd_parameter {
	BYTE rear_threshold;
	BYTE lateral_rear_threshold;
}BSD_PARAMETER_T;

extern BSD_PARAMETER_T BSD_Param;

/**
 * TJSATL_FUNCTION_CODE_ALERT_UPDATE
 *
typedef struct _alert_update_info{
    BYTE  reserved;
    BYTE  alertType;
    BYTE  speed;
    BYTE  altitude;
    DWORD latitude;
    DWORD longitude;
    BCD   time[6];
    WORD  carStatus;
}ALERT_INFO_T;
*/

/**
 * TJSATL_FUNCTION_CODE_PERIPHERAL_STATUS_QUERY
 */
typedef struct _peripheral_status{
        
    BYTE workStatus;
    
    union{
        struct{
            DWORD cameraErr:        1;
            DWORD mainMemErr:       1;
            DWORD additionMemErr:   1;
            DWORD infraredErr:      1;
            DWORD speakerErr:       1;
            DWORD batteryErr:       1;
            DWORD reserved1:         4;
            DWORD communiModErr:    1;
            DWORD defineModErr:     1;
            DWORD reserved2:         20;
        }alertStatus_Bit;
        DWORD alertStatus;
    }alert;
}PERIPHERAL_STATUS_T;

#define periphStatus_NORMAL 0x01
#define periphStatus_AWAIT  0x02
#define periphStauts_UPDATA 0x03
#define periphStatus_ABNORMAL 0x04

extern PERIPHERAL_STATUS_T peripheral_Status;

/**
 * TJSATL_FUNCTION_CODE_ALERT_UPDATA
 */
typedef struct _alert_updata_info{
    BYTE reserved1[4];
    BYTE flagStatus;
    BYTE alertType;
    BYTE speed;
    WORD altitude;
    DWORD latitude;
    DWORD longitude;
    BCD   dataTime[6];
    WORD carStatus;
}ALERT_INFO_T;

#define flagStatus_UNUSABLE 0x00    //不可用
#define flagStatus_START    0x01       //开始标志
#define flagStatus_END      0x02         //结束标志

#define alertType_REAR      0x01    //后方接近报警
#define alertType_LEFTREAR  0x02    //左侧后方接近报警
#define alertType_RIGHTREAR 0x03    //右侧后方接近报警

ALERT_INFO_T alert_Info;



/******** 多媒体报警相关类型 ********/
typedef struct MULTIMEDIA_PARAM{
    BYTE alertType;
    BYTE multiMediaType;
    char multiMediaName[50];
    BYTE multiMediaNameLen;
    DWORD multiMediaId;
}MULTIMEDIA_PARAM_T;

typedef struct MULTIMEDIA_FIFO{
    MULTIMEDIA_PARAM_T multiMediaParam[10];
    BYTE rd;
    BYTE wt;
}MULTIMEDIA_FIFO_T;

extern MULTIMEDIA_FIFO_T multiMediaFifo;
extern volatile DWORD multiMediaId;




JTT_ERROR_E DoEscapeForReceive(const BYTE rawBinarySeq[], BYTE binarySeq[], const int rawbinarySeqLen);
JTT_ERROR_E CheckSum(BYTE binarySeq[], const int len,BYTE* checksum);
JTT_ERROR_E DoEscapeForSend(const BYTE rawBinarySeq[],BYTE binarySeq[],const int rawBinarySeqLen,int *binarySeqLen);
int EncoderRequestAck(BYTE rawBinarySeq[],WORD flowId,BYTE functionCode,void *dataArea,int dataLen);
void RealTimeData_Decode(BYTE *data);
int GetBaseInfoToAck(WORD flowId,BYTE functionCode);
void GetBSDParamToAck(WORD flowId, BYTE functionCode);
void SetBSDParamToACK(WORD flowId,BYTE functionCode, BYTE *dataArea);
void GetPeripheralStatusToAck(WORD flowId,BYTE functionCode);
void GetPeripheralStatusToSend(WORD flowId, BYTE functionCode);
void PeripheralStatusAck_Decode(char command_buf[], int length);
int TerminalAckDecode(BYTE *buf, int len);
void SendData(void *lpParam);
void UpldAlertData(BYTE alertType, BYTE multiMediaType, char *multiMediaName);
void MultiMediaFifo_Init(MULTIMEDIA_FIFO_T *mp);
int MultiMediaFifo_isEmpty(MULTIMEDIA_FIFO_T *mp);



#endif
