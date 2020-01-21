#include"rtsp_server.hh"
#include <unistd.h>
#include<pthread.h>
#include <list>
#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "WW_H264VideoServerMediaSubsession.hh"
#include "WW_H264VideoSource.hh"
#include "RTSPServer.hh"

using std::list;
using namespace std;
list<Tframe> vc;
pthread_t tid;
void* thread_run(void* _val)
{
	TaskScheduler* scheduler = BasicTaskScheduler::createNew();
	UsageEnvironment* env = BasicUsageEnvironment::createNew(*scheduler);

	UserAuthenticationDatabase* authDB = NULL;
	RTSPServer* rtspServer = RTSPServer::createNew(*env, 8554, authDB);
  	if (rtspServer == NULL) {
    		*env << "Failed to create RTSP server: " << env->getResultMsg() << "\n";
    		exit(1);
  	}
	
	{
		WW_H264VideoSource * videoSource = 0;
        	ServerMediaSession * sms = ServerMediaSession::createNew(*env, "live", 0, "ww live test");
        	sms->addSubsession(WW_H264VideoServerMediaSubsession::createNew(*env, videoSource));
        	rtspServer->addServerMediaSession(sms);

		printf("!!!!@@@@ : %s\n",rtspServer->rtspURL(sms));
  	}
	env->taskScheduler().doEventLoop();
	return NULL;
}

void rtsp_init(){
	int tret = pthread_create(&tid, NULL, thread_run,NULL);
	if (tret == 0)
	{
		printf("create pthread success\n");
	}else
		printf("create pthread failed info: %s", strerror(tret));
}

int getRtspPlay_flag()
{
    if(getPlay_flag == 1){
        printf("\r\n\r\n*** getPlay_flag = 1 ***\r\n\r\n");
        return 1;
    }
    else
        return 0;
}

void setRtspPlay_flag()
{
    if(getPlay_flag == 1)
        getPlay_flag = 0;
}

void push_vc(int size,void*p)
{
        Tframe tc;
        tc.p = malloc(size);
        memcpy(tc.p,p,size);
        vc.push_back(tc);
}


