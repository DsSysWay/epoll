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

#include "ae.h"
#define     EPOLLEVENTS 100



eventLoop::eventLoop(int setsize)
{
    epollData pPollData = new EpollData();
    if(!pPollData)
    {
        return -1;
    }
    pPollData->epollfd = epoll_create(setsize);
    m_setsize  = setsize;
    pollData = (void*) pPollData;
    bIsStop = false;
}



eventLoop::~eventLoop()
{
    if(!bIsStop)
    { 
        bIsStop = true;
    }
    if(pollData)
    { 
        (epollData*)  pPollData = (epollData*)pollData;
        close(pPollData->epollfd);
        //怎么关闭数组哦
    }
    //怎么关闭map哦
}




int eventLoop::aePoll()
{

    char buf[MAXSIZE];
    (epollData*) epollData = (epollData*)pollData; 
    if(!epollData)
    {
        return -1;
    }
    for(;;)
    {
        int num = epoll_wait(epollData->epollfd, epollData->events, EPOLLEVENTS, -1);
        handle_events(num, listenfd, buf);

    }

    close(epollData->epollfd);
    return 0;
}



int  eventLoop::handle_events(int num, int listenfd, char *buf)    
{
    epollData *poll = (epollData*)polldata;
    if(!buf)     
    {
        return -1;
    }
    for(int i = 0; i < num; i++)
    {
        struct epoll_event *ee = &poll->events[i];
        int fd = ee->data.fd;
        int events = ee->events;
        if(fd == listenfd && (events & EPOLLIN))
        {
            handle_accept(fd);
        }
        else
        {
            if(events & EPOLLIN)
            {
                do_read(fd, buf);
            }
            else if(events & EPOLLOUT )
            {
                do_write(fd, buf);
            }
        }

    }

}



int eventLoop::add_event(int fd, int mask)
{
    (epollData*) poll = (epollData*)pollData;
    if(!poll)
    {
        return -1;
    }
    struct epoll_event ee;
    ee.data.fd = fd;
    ee.events = mask;
    epoll_ctl(poll->epollfd, EPOLL_CTL_ADD, fd, &ee);
    return 0;
}



int eventLoop::del_event(int fd, int mask)
{
    (epollData*) poll = (epollData*)pollData;
    if(!poll)
    {
        return -1;
    }
    struct epoll_event ee;
    ee.data.fd = fd;
    ee.events = mask;
    epoll_ctl(poll->epollfd, EPOLL_CTL_MOD, fd, &ee);
    return 0; 

}

int eventLoop::mod_event(int fd, int mask)
{
    (epollData*) poll = (epollData*)pollData;
    if(!poll)
    {
        return -1;
    }
    struct epoll_event ee;
    ee.data.fd = fd;
    ee.events = mask;
    epoll_ctl(poll->epollfd, EPOLL_CTL_DEL, fd, &ee);
    return 0; 


}

//客户端专用
int eventLoop::aeAddNewFd(int fd,  int mask,(aeFdReadProc *) rproc, (aeFdWriteProc *)wproc)
{
    aeFileEvent fileEvent = new aeFileEvent();
    fileEvent.fd = fd;
    fileEvent.mask = mask;
    fileEvent.wproc = wproc;
    fileEvent.rpoc = rproc;
    fdEventMap.insert(make_pair(ifd, fileEvent));
    epollData* poll = (epollData*)pollData;
    if(!poll)
    {
        return -1; 
    }
    add_event(fd, mask);
    return 0;
}



int eventLoop::handle_accept(int listenfd)
{
    struct sockaddr_in cliaddr;
    socklen_t cliaddrlen;
    int fd = accept(listenfd, (struct sockaddr*)&cliaddr, *cliaddrlen);
    if(-1 == fd)
    {
        perror("accept");
        return -1;
    }
     printf("accept a new client:%s:%d\n", inet_ntoa(cliaddr.sin_addr), cliaddr.sin_port);

     //ip，port列表添加映射
     ipFdMap.insert(make_pair(pair(cliaddr.sin_addr, cliaddr.sin_port), fd));

     add_event(fd, EPOLLIN);
    return 0;
}




int eventLoop::do_read(int fd, char* buf)
{

    (epollData*) pollData = (epollData*)pollData;
    if(!polldata)
    {
        return -1;
    }
    int nread = 0;
    nread = read(fd, buf, MAXSIZE);
    if( -1 == nread)
    {
        perror("read error");
        close(fd);
        del_event(pollData->epollfd, fd, EPOLLIN);
        //将fd从ipmap中删除
    }
    else if(nread == 0)
    {
        perror("client close.  \n");
        close(fd);
        del_event(pollData->epollfd, fd, EPOLLIN);
    }
    else
    {
        printf("%s\n", buf); 
        //注册为写事件
        modify_event(pollData->epollfd, fd, EPOLLOUT);
    }
    return 0;

}


int eventLoop::do_write(int fd, char* buf)
{
    (epollData*) pollData = (epollData*)pollData;
    if(!polldata)
    {
        return -1;
    }
    int nwrite = 0;
    nwrite = write(fd, buf, MAXSIZE); 
    if(-1 == nwrite)
    {
        perror("write error");
        close(fd);
        del_event(pollData->epollfd, fd, EPOLLOUT);
    }
    else
    {
        //注册感兴趣的读事件
        modify_event(pollData->epollfd, fd, EPOLLIN);
    }
    return 0;

}

