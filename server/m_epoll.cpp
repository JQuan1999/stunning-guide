#include "m_epoll.h"

m_epoll::m_epoll(int max_ev)
{
    m_epoll_fd = epoll_create(5);
    assert(m_epoll_fd != -1);
    // max_ev的resize
    epoll_event ev;
    m_events.resize(max_ev, ev);
    max_event_num = max_ev;
    ready_event_num = -1;
}

m_epoll::~m_epoll()
{
    close(m_epoll_fd);
}

int m_epoll::addFd(int fd, event_type events)
{   
    if(fd < 0) return -1;
    epoll_event ev;
    ev.data.fd = fd;
    ev.events = events;
    int ret = epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, fd, &ev);
    return ret;
}

int m_epoll::removeFd(int fd)
{
    if(fd < 0) return -1;
    int ret = epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
    close(fd);
    return ret;
}

int m_epoll::modifyFd(int fd, event_type events)
{   
    if(fd < 0) return -1;
    epoll_event ev;
    ev.data.fd = fd;
    ev.events = events;
    int ret = epoll_ctl(m_epoll_fd, EPOLL_CTL_MOD, fd, &ev);
    return ret;
}

int m_epoll::waitFd(int ms)
{
    ready_event_num = epoll_wait(m_epoll_fd, &m_events[0], max_event_num, ms);
    return ready_event_num;
}

int m_epoll::getFd(size_t index)
{
    assert(ready_event_num != -1);
    return m_events[index].data.fd;
}

event_type m_epoll::getEvents(size_t index)
{
    assert(ready_event_num != -1);
    return m_events[index].events;
}

void m_epoll::closeFd()
{
    close(m_epoll_fd);
}