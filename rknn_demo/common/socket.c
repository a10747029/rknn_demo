#include<stdio.h>  
#include<stdlib.h>  
#include<string.h>  
#include<errno.h>  
#include<sys/types.h>  
#include<sys/socket.h>  
#include<netinet/in.h>  
#define DEFAULT_PORT 8889
#define MAXLINE 4096  
int global_socket_write = 0;
int connect_fd = -2;
void *socket_init(void *arg) 
{  
    int    socket_fd;  
    struct sockaddr_in     servaddr;  
    int     n;  
    printf("init the socket!!!!!!!!!!!!!!!!!!!!!!\n");
    if((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1 ){  
    	printf("create socket error: %s(errno: %d)\n",strerror(errno),errno);  
    	return ;
    }  
    memset(&servaddr, 0, sizeof(servaddr));  
    servaddr.sin_family = AF_INET;  
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(DEFAULT_PORT);
  
    if( bind(socket_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1){  
    	printf("bind socket error: %s(errno: %d)\n",strerror(errno),errno);  
    	return ;  
    }  

    if( listen(socket_fd, 10) == -1){  
    	printf("listen socket error: %s(errno: %d)\n",strerror(errno),errno);  
    	return ;  
    }  
    printf("===================waiting for client's request=====================\n");  
    while(1){  
        if( (connect_fd = accept(socket_fd, (struct sockaddr*)NULL, NULL)) == -1){  
        	printf("accept socket error: %s(errno: %d)",strerror(errno),errno);  
        	continue;  
    	}  
	printf("socket process get connect fd = %d\n",connect_fd);
	global_socket_write = 1;
	while(global_socket_write){
    		printf("======global_socket_wait======\n");  
		sleep(2);
	}
    }  
    close(socket_fd);  
    return ;
}  

