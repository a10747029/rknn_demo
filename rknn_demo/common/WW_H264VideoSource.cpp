#include "WW_H264VideoSource.hh"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <vector>
#include "camera_record.h"

#define FIFO_NAME     "/tmp/H264_fifo"
#define BUFFER_SIZE   fMaxSize//PIPE_BUF
#define REV_BUF_SIZE  (1024*256)
#define mSleep(ms)    usleep(ms*1000)
WW_H264VideoSource::WW_H264VideoSource(UsageEnvironment & env) :
FramedSource(env),
m_pToken(0),
m_pFrameBuffer(0),
m_hFifo(0)
{
	printf("WW_H264VideoSource ::WW_H264VideoSource\n");
	mkfifo(FIFO_NAME, S_IFIFO|0666);
        m_hFifo = open(FIFO_NAME,O_RDONLY);
        if(m_hFifo == -1)
        {
                return;
        }
        printf("[MEDIA SERVER] open fifo result = [%d]\n",m_hFifo);

        m_pFrameBuffer = new char[REV_BUF_SIZE];
        if(m_pFrameBuffer == NULL)
        {
                printf("[MEDIA SERVER] error malloc data buffer failed\n");
                return;
        }
        memset(m_pFrameBuffer,0,REV_BUF_SIZE);
}

WW_H264VideoSource::~WW_H264VideoSource(void)
{
//	printf("WW_H264VideoSource ::~WW_H264VideoSource\n");
        if(m_hFifo)
        {
                ::close(m_hFifo);
        }

        envir().taskScheduler().unscheduleDelayedTask(m_pToken);

        if(m_pFrameBuffer)
        {
        delete[] m_pFrameBuffer;
                m_pFrameBuffer = NULL;
        }

        printf("[MEDIA SERVER] rtsp connection closed\n");
}

void WW_H264VideoSource::doGetNextFrame()
{
//	printf("WW_H264VideoSource ::doGetNextFrame\n");
        double delay = 1000.0 / (FRAME_PER_SEC * 2);  // ms
        int to_delay = delay * 1000;  // us

        m_pToken = envir().taskScheduler().scheduleDelayedTask(to_delay, getNextFrame, this);
}

unsigned int WW_H264VideoSource::maxFrameSize() const
{
//	printf("WW_H264VideoSource ::maxFrameSize\n");
        return 1024*100;
}

void WW_H264VideoSource::getNextFrame(void * ptr)
{
        ((WW_H264VideoSource *)ptr)->GetFrameData();
}

void WW_H264VideoSource::GetFrameData()
{
        gettimeofday(&fPresentationTime, 0);
//        printf("[MEDIA SERVER] %s,%d\n",__func__,__LINE__);
        fFrameSize = 0;

        int len = 0;
        unsigned char buffer[BUFFER_SIZE] = {0};
        while((len = read(m_hFifo,buffer,BUFFER_SIZE))>0)
        {
                memcpy(m_pFrameBuffer+fFrameSize,buffer,len);
//		printf("copy frames\n");
                fFrameSize+=len;
		    break;
        }
        printf("[MEDIA SERVER] GetFrameData len = [%d],fMaxSize = [%d]\n",fFrameSize,fMaxSize);


        if (fFrameSize > fMaxSize)
        {
                fNumTruncatedBytes = fFrameSize - fMaxSize;
                fFrameSize = fMaxSize;
        }
        else
        {
                fNumTruncatedBytes = 0;
        }
        
        // fill frame data
        memcpy(fTo,m_pFrameBuffer,fFrameSize);

        afterGetting(this);
}

