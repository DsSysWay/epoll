/*
 * =====================================================================================
 *
 *       Filename:  server.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  11/03/2016 10:10:50 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  chriswei
 *   Organization:  洪兴锐创网络科技
 *
 * =====================================================================================
 */

//#define uint32_t  (unsigned) int
/*  struct epoll_event{
    uint32_t events;
    epoll_data_t data;
};*/

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
#include    "ae.h"

#define     IPADDRESS "127.0.0.1"
#define     PORT 8888
#define     MAXSIZE 1024
#define     LISTENQ 5
#define     FDSIZE  1000
#define     EPOLLEVENTS 100
#define     AE_READABLE EPOLLIN
#define     AE_WRITABLE  EPOLLOUT



static int socket_bind(const char*ip, int port);
static int  do_epoll(int listenfd);
static int  handle_events(int epollfd, struct epoll_event *events, int num, int listenfd, char*buf  );

static int handle_accept(int epollfd, int listenfd);

//修改fd的注册事件
static int do_read(int epollfd, int fd, char*buf);
static int do_write(int epollfd, int fd, char*buf);

static int add_event(int epollfd, int fd, int mask);
static int del_event(int epollfd, int fd, int mask);
static int modify_event(int epollfd, int fd, int mask);




static int  do_epoll(int listenfd)
{
    char buf[MAXSIZE];
    struct epoll_event events[EPOLLEVENTS];
    memset(buf, 0, MAXSIZE);
    int epollfd = epoll_create(FDSIZE); 
    if(-1 == epollfd)
    {
        perror("epoll_create");
        return -1;
    }
    add_event(epollfd, listenfd, EPOLLIN);

    for( ; ; )
    {
        int num = epoll_wait(epollfd, events,  EPOLLEVENTS, -1);
        handle_events(epollfd, events,  num,  listenfd, buf);
    }
    close(epollfd);

}



static int  handle_events(int epollfd, struct epoll_event *events, int num, int listenfd, char*buf  )
{
    int i = 0;
    for(; i < num; i++ )
    {
        struct epoll_event *ee = &events[i]; 
        int fd  = ee->data.fd;
        int events = ee->events;
        //了解注册到epoll红黑树的单位，是以fd加事件为维度
        //还是fd为维度
        if(fd == listenfd && (events & AE_READABLE))
        {
            //监听套接字有事件，有新请求到达
            handle_accept(epollfd, fd); 

        }
        else
        {
            if(events & AE_READABLE)
            {
                do_read(epollfd, fd, buf);
            }
            else if(events & AE_WRITABLE)
            {
                do_write(epollfd, fd, buf);
            }

        }
    }
    return 0;
}

static int do_read(int epollfd, int fd, char*buf)
{
    int nread = 0;
    nread = read(fd, buf, MAXSIZE);
    if( -1 == nread)
    {
        perror("read error");
        close(fd);
        del_event(epollfd, fd, EPOLLIN);
    }
    else if(nread == 0)
    {
        perror("client close.  \n");
        close(fd);
        del_event(epollfd, fd, EPOLLIN);
    }
    else
    {
        printf("%s\n", buf); 
        //注册为写事件
        modify_event(epollfd, fd, EPOLLOUT);
    }
    return 0;

    
}



static int do_write(int epollfd, int fd, char* buf)
{
   int nwrite = 0;
   nwrite = write(fd, buf, MAXSIZE); 
   if(-1 == nwrite)
   {
       perror("write error");
       close(fd);
       del_event(epollfd, fd, EPOLLOUT);
   }
   else
   {
       //注册感兴趣的读事件
       modify_event(epollfd, fd, EPOLLIN);
   }
   return 0;
}



static int handle_accept(int epollfd, int listenfd)
{
     struct sockaddr_in cliaddr;
     socklen_t cliaddrlen;
     int fd = accept(listenfd, (struct sockaddr*)&cliaddr, &cliaddrlen ); 
     if(-1 == fd)
     {
         perror("accept error");
         return -1;
     }
     printf("accept a new client:%s:%d\n", inet_ntoa(cliaddr.sin_addr), cliaddr.sin_port);
     add_event(epollfd, fd, AE_READABLE);
}


static int add_event(int epollfd, int fd, int mask)
{
    struct epoll_event ee;
    ee.data.fd = fd;
    ee.events =  mask;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ee);
    return 0;
}

static int del_event(int epollfd, int fd, int mask)
{
    struct epoll_event ee;
    ee.data.fd = fd;
    ee.events |=  mask;
    //考虑兼容其他条件的情况，例如一个套接字有多个监听事件
    //这一步做什么操作，把fd对应的注册事件删掉，还是把fd从红黑树下掉
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, &ee);
    return 0;
}

static int modify_event(int epollfd, int fd, int mask)
{
    struct epoll_event ee;
    ee.data.fd = fd;
    ee.events =  mask;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &ee);
    return 0;
}



static int socket_bind(const char*ip, int port)
{
   int sockfd = socket(AF_INET, SOCK_STREAM, 0);
   if(-1 == sockfd)
   {
       perror("socket fail");
       return -1;
   }
   struct sockaddr_in addr;
   bzero(&addr, sizeof(addr));
   addr.sin_port = htons(port);
   addr.sin_family = AF_INET;
   inet_pton(AF_INET, ip, &addr.sin_addr);
   int ret = bind(sockfd, (struct sockaddr*)&addr, sizeof(addr));
   if(-1 == ret)
   {
       perror("bind fail ");
       return -1;
   }
   return sockfd;
}


int main(int argc, char *argv[])
{
    int listenfd;
    listenfd = socket_bind(IPADDRESS, PORT);
    listen(listenfd, LISTENQ);
    do_epoll(listenfd);
    return 0;
}
