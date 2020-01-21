#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>
#include "others.h"
#include "tjstal.h"
#include "conf.h"


/****  终端服务器地址信息  ****/
#define SERVER_IP  "192.168.100.100"
#define DEFAULT_PORT 8888
#define MAXLINE 4096  

#define SecondsToMilliSeconds(sec,millisec) sec*1000+millisec/1000

static void SendTask(void *lpParam);


//char recvDataFlag = 0;

//char recvBuf[4096];
//int recvLen = 0;
pthread_t thread_id;
pthread_t thread_camera;

void main()
{
    struct sockaddr_in     servaddr;
    int i;
    char    buff[4096];
    int len;
    int ret;

    commuParam.commuState = COMMU_IDLE;

    
    config_init();
    ret = pthread_create(&thread_camera, NULL, (void *)camera_thread, NULL);
    if(ret){
        printf("Create pthread error!\r\n");
    }

    MultiMediaFifo_Init(&multiMediaFifo);

    if(pthread_mutex_init(&sendLock, NULL) == 0){
        printf("pthread_mutex_init success!\r\n");
    }
    if(pthread_mutex_init(&connectFlagLock, NULL) == 0){
        printf("pthread_mutex_init success!\r\n");
    }

    printf("init the socket!!!!!!!!!!!!!!!!!!!!!!\n");
    if((commuParam.socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1 ){
        printf("create socket error: %s(errno: %d)\n",strerror(errno),errno);
        return ;
    }
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(SERVER_IP);
    servaddr.sin_port = htons(DEFAULT_PORT);

    printf("conncet to %s:%d\r\n",SERVER_IP, DEFAULT_PORT);

    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    //setsockopt(commuParam.socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    //setsockopt(commuParam.socket_fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

    //连接服务器，成功返回0，错误返回-1
    if(connect(commuParam.socket_fd,(struct sockaddr *)&servaddr, sizeof(servaddr)) < 0){
        printf("connect to server failed!!\r\n");
        connectServerSuccessFlag = 0;
    }
    else{
        connectServerSuccessFlag = 1;
        printf("server connect success\r\n");
    }


    //pthread_create(&thread_id, NULL, (void *)(SendData), (void *)&commuParam);

    while(1){

        //UpldAlertData(1,1,"test");

        pthread_mutex_lock(&connectFlagLock);
        if(connectServerSuccessFlag == 1){
            pthread_mutex_unlock(&connectFlagLock);

            timeout.tv_sec = 0;
            timeout.tv_usec = 0;
            setsockopt(commuParam.socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        
            //printf("start receive !!!\r\n");

            while(len = recv(commuParam.socket_fd, buff, MAXLINE, 0))
            {
                pthread_mutex_lock(&sendLock);
                for(i=0; i<len; i++){
                    printf("[%d]:0x%x", i, buff[i]);
                    printf("\r\n");
                }
    
                commuParam.recvDataLen = len;
                memcpy(commuParam.recvBuf,buff, len);
            
                if(do_deal_request(commuParam.recvBuf, commuParam.recvDataLen) == 1){
                    commuParam.isSendAckFlag = 1;
                    SendData(&commuParam);
                }
                pthread_mutex_unlock(&sendLock);
            
            }
        }
        else{
            
            pthread_mutex_unlock(&connectFlagLock);
            //连接服务器，成功返回0，错误返回-1
            if(connect(commuParam.socket_fd,(struct sockaddr *)&servaddr, sizeof(servaddr)) < 0){
                printf("connect to server failed!!\r\n");
                pthread_mutex_lock(&connectFlagLock);
                connectServerSuccessFlag = 0;
                pthread_mutex_unlock(&connectFlagLock);
                sleep(5);
            }
            else{
                pthread_mutex_lock(&connectFlagLock);
                connectServerSuccessFlag = 1;
                pthread_mutex_unlock(&connectFlagLock);
                printf("server connect success\r\n");
            }
        }
    }

    close(commuParam.socket_fd);
}



/**
 * 外设发送数据线程处理函数
 */
/*static void SendTask(void *lpParam)
{
    
    struct timeval sendTime;    //发送数据的时间
    struct timeval waitTime;    //发送后的等待时间
    struct timeval currTime;    //当前时间
    struct timeval oldTime;

    int socket_fd = (int)lpParam;
    int retransmitCounter = 0;
    char    sendWorkStatusFlag = 0;

    //gettimeofday(&sendTime, NULL);
    gettimeofday(&currTime,NULL);
    gettimeofday(&oldTime,NULL);

    while(1){
        gettimeofday(&currTime,NULL);
        if(currTime.tv_sec - oldTime.tv_sec > 10){
            sendWorkStatusFlag = 1;
        }

        switch(commuState){
            case COMMU_IDLE:

                retransmitCounter = 0;
                
                pthread_mutex_lock(&flagLock);
                if(recvDataFlag == 1){
                    commuState = COMMU_RECVED_ORDER;
                }
                pthread_mutex_unlock(&flagLock);


                if(commuState == COMMU_IDLE && sendWorkStatusFlag == 1){
                    sendWorkStatusFlag = 0;
                    commuState = COMMU_SEND_ORDER;
                    allFlowId ++;
                }
                break;
            case COMMU_SEND_ORDER:
                GetPeripheralStatusToSend(allFlowId, TJSATL_FUNCTION_CODE_PERIPHERAL_STATUS_REPORT);
                gettimeofday(&sendTime,NULL);
                commuState = COMMU_WAIT_ACK;

                break;
            case COMMU_RECVED_ORDER:

                do_deal_request(recvBuf, recvLen);

                pthread_mutex_lock(&flagLock);
                recvDataFlag = 0;
                pthread_mutex_unlock(&flagLock);

                break;
            case COMMU_WAIT_ACK:
                
                if(recvDataFlag == 1){
                    PeripheralStatusAck_Decode(recvBuf, recvLen);
                    recvDataFlag = 0;
                    commuState = COMMU_IDLE;
                    gettimeofday(&oldTime,NULL);
                }
                else{
                    gettimeofday(&waitTime,NULL);
                    if(SecondsToMilliSeconds(waitTime.tv_sec,waitTime.tv_usec) - SecondsToMilliSeconds(sendTime.tv_sec,sendTime.tv_usec) > 1000){
                        commuState = COMMU_WAIT_TIMEOUT;
                    }
                }

                break;
            case COMMU_WAIT_TIMEOUT:

                retransmitCounter ++;
                if(retransmitCounter <= 3){
                    commuState = COMMU_SEND_ORDER;
                }
                else
                {
                    commuState = COMMU_RETRANSMIT;
                }
                break;
            case COMMU_RETRANSMIT:
                printf("send peripheral status ack retransmit failed!!!\r\n");
                commuState = COMMU_IDLE;
                gettimeofday(&oldTime,NULL);
                break;
            case COMMU_COMPLETE:
                break;
            default:
                break;
        }

	  usleep(50); 
    }
}
*/



