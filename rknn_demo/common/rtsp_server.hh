#ifndef _RTSP_SERVER_H_
#define _RTSP_SERVER_H_
#include <string.h>
#include <utils.h>

#ifdef __cplusplus
extern "C"{
#endif
typedef struct Tframe{
        int size;
        void* p;
};

void rtsp_init();
int getRtspPlay_flag();
void setRtspPlay_flag();
void push_vc(int size,void*p);
#ifdef __cplusplus
}
#endif

#endif
