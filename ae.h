#ifndef _AE_H_
#define _AE_H_
/*
 * =====================================================================================
 *
 *       Filename:  ae.cpp
 *
 *    Description:  
 *
 *
 *        Version:  1.0
 *        Created:  11/09/2016 08:17:15 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  chriswei
 *   Organization:  洪兴锐创网络科技
 *
 * =====================================================================================
 */

#include   <stdio.h>   
#include   <stdlib.h>
#include   <string.h>
#include   <errno.h>
#include   <netinet/in.h>
#include   <sys/socket.h>
#include   <arpa/inet.h>
#include   <sys/epoll.h>
#include   <unistd.h>
#include   <sys/types.h>
#include   "comm.h"


using namespace std;

#define MAXFDSIZE   10240
typedef class epollData{
    public:
    int epollfd;
    struct epoll_event events[EPOLLEVENTS];
}epollData;

class eventLoop;


typedef void aeFdReadProc(eventLoop *eventLoop, int fd, char*buf, int maxSize); 
typedef int aeFdWriteProc(eventLoop *eventLoop, int fd, char*buf, int maxSize); 

typedef class aeFileEvent{
 public:
     int fd;
     aeFdReadProc *rproc;
     aeFdWriteProc *wproc;
     int mask;
     //int ip; //对端ip
     //int port;//对端port
}aeFileEvent;

typedef class eventLoop{


    public:
         eventLoop(int setsize);
         ~eventLoop();
        //处理poll ，调用读写函数
        int aePoll();
        //主动连接
        int aeAddNewFd(int fd,  int mask,(aeFdReadProc *) rproc, (aeFdWriteProc *)wproc);


    private:

        int handle_events(int num, int listenfd, char *buf);
        int do_read(int fd, char* buf);
        int do_write(int fd, char* buf);
        //被动连接
        int handle_accept(int listenfd);
        int add_event(int fd, int mask);
        int del_event(int fd, int mask);
        int mod_event(int fd, int mask);

    public:
        bool bIsStop;
    private:
        map<int,aeFileEvent>  fdEventMap;
        map< pair<int, int >, int>  ipFdMap;
        void* pollData;
        int m_setsize;

}eventLoop;








#endif
