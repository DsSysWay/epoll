/*
 * =====================================================================================
 *
 *       Filename:  client.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  11/05/2016 03:43:10 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  chriswei
 *   Organization:  洪兴锐创网络科技
 *
 * =====================================================================================
 */



#include   <netinet/in.h>
#include   <sys/socket.h>
#include   <string.h>
#include   <stdlib.h>
#include   <sys/epoll.h>
#include   <time.h>
#include   <unistd.h>
#include   <sys/types.h>
#include   <arpa/inet.h>


#define     MAXSIZE 1024
#define     IPADDRESS  "127.0.0.1"
#define     SERVER_PORT 8888
#define     FDSIZE 1024
#define     EPOLLEVENTS 20



static void handle_events(int epollfd, struct epoll_event *events, int num, int sockfd, char*buf );
static void handle_connection(int sockfd);
static void do_read(int epollfd, int fd, int sockfd, char*buf);
static void do_write(int epollfd, int fd, int sockfd, char*buf);
static void  add_event(int epollfd, int fd, int mask );
static void  modify_event(int epollfd, int fd, int mask );
static void  del_event(int epollfd, int fd, int mask );

int main(int argc, char *argv[])
{

    int sockfd;
    struct sockaddr_in serveraddr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family  = AF_INET;
    serveraddr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, IPADDRESS, &serveraddr.sin_addr); 
    connect(sockfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
    handle_connection(sockfd);
    close(sockfd);
    return 0; 
}



static void handle_connection(int sockfd)
{
    int epollfd;
    struct epoll_event events[EPOLLEVENTS]; 
    char buf[MAXSIZE];
    int ret;
    epollfd  = epoll_create(FDSIZE);
    add_event(epollfd, STDIN_FILENO, EPOLLIN);
    for(;;)
    {
        ret = epoll_wait(epollfd, events, EPOLLEVENTS, -1);
        handle_events(epollfd, events, ret, sockfd, buf);
    }
}


static void handle_events(int epollfd, struct epoll_event *events, int num, int sockfd,  char*buf )
{
    int fd;
    int i;
    for(i = 0; i < num; i++)
    {
        struct  epoll_event *ee = &events[i];
        fd = ee->data.fd;
        if(ee->events & EPOLLIN)
        {
            do_read(epollfd, fd, sockfd, buf);
        }
        else if(ee->events & EPOLLOUT)
        {
            do_write(epollfd, fd, sockfd, buf);
        }
    }
}


static void do_read(int epollfd, int fd, int sockfd, char*buf)
{
       int nread;
       nread = read(fd, buf, MAXSIZE);
       if(-1 == nread)
       {
           perror("read error");
           close(fd);
       }
       else if(nread == 0)
       {  
           perror("server close.\n");
           close(fd);
       }
       else
       {
           if(fd == STDIN_FILENO)
           {
               add_event(epollfd, sockfd, EPOLLOUT);
           }
           else
           {
               //epoll响应事件后，该注册事件还是不会被删除的，会继续监听，所以读
               //完后如果需要切换状态需要主调自己控制，先删除读事件，再注册写事
               //件
               del_event(epollfd, sockfd, EPOLLIN);
               add_event(epollfd, STDOUT_FILENO, EPOLLOUT);
           }
       }
}

static void do_write(int epollfd, int fd, int sockfd, char*buf)
{
    int nwrite;
    nwrite = write(fd, buf, strlen(buf));
    if(nwrite == -1)
    {
        perror("write error");
        close(fd);
    }
    else
    {
        if(fd == STDOUT_FILENO)
        {
            del_event(epollfd, fd, EPOLLOUT);
        }
        else
        {
            modify_event(epollfd, fd, EPOLLIN);
        }
    }
    memset(buf, 0 , MAXSIZE);
}

static void  add_event(int epollfd, int fd, int mask )
{
    struct epoll_event ee;
    ee.events = mask;
    ee.data.fd = fd;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ee);
}
static void  modify_event(int epollfd, int fd, int mask )
{  
    struct epoll_event ee;
    ee.events = mask;
    ee.data.fd = fd;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd,  &ee);
}
static void  del_event(int epollfd, int fd, int mask )
{  
    struct epoll_event ee;
    ee.events = mask;
    ee.data.fd = fd;
    epoll_ctl(epollfd,  EPOLL_CTL_DEL, fd,  &ee);
}
