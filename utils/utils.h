#include <sys/epoll.h>
#include <signal.h>
#include <fcntl.h>
#include <iostream>

// 公用的一些函数
class utils
{ 
public:
    ~utils(){}
    utils(){}
    // 将fd注册到epollfd上
    static int addFd(int epollfd, int fd, bool epollet = false);
    // 修改fd及fd的事件
    static int modifyFd(int epollfd, int fd, bool epoolout = false, bool epollshot = false, bool epollet = false);
    static int setNoBlocking(int fd);
};


int addFd(int epollfd, int fd, bool epollet)
{
    epoll_event ev;
    ev.data.fd = fd;
    ev.events |= EPOLLIN;
    if(epollet)
    {
        ev.events |= EPOLLET;
    }
    int ret = epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);
    if(ret != 0)
    {
        std::cout<<"注册文件描述符失败"<<std::endl;
        return -1;
    }
    return 0;
}

// epoll默认为水平出发
// 是否注册epollout、epollshot、epollet
int utils::modifyFd(int epollfd, int fd, bool epollout, bool epollshot, bool epollet)
{
    epoll_event ev;
    ev.data.fd = fd;
    ev.events |= EPOLLIN;
    if(epollout)
    {
        ev.events |= EPOLLOUT;
    }
    if(epollshot)
    {
        ev.events |= EPOLLONESHOT;
    }
    if(epollet)
    {
        ev.events |= EPOLLET;
    }
    int ret = epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &ev);
    if(ret != 0)
    {
        std::cout<<"修改文件描述符失败"<<std::endl;
        return -1;
    }
    return 0;
}


int utils::setNoBlocking(int fd){
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}