#ifndef M_EPOLL
#define M_EPOLL

#include <sys/epoll.h>
#include <vector>
#include <assert.h>

using event_type = uint32_t;

class m_epoll{
public:
    m_epoll(int max_ev = 65535);
    ~m_epoll();
    int addFd(int fd, event_type ev); // 注册fd上的事件
    int modifyFd(int fd, event_type ev); // 修改fd上的事件
    int waitFd(int ms = -1); // 获取触发事件的文件描述符个数
    int getFd(size_t index); // 获取触发事件的第i个文件描述符
    int removeFd(int fd);
    event_type getEvents(size_t index); // 获取触发事件的第i个文件描述符的事件
private:
    int m_epoll_fd;
    int max_event_num;
    int ready_event_num;
    std::vector<epoll_event> m_events;
};

m_epoll::m_epoll(int max_ev)
{
    m_epoll_fd = epoll_create(5);
    assert(m_epoll_fd != -1);
    m_events.resize(max_ev);
    max_event_num = max_ev;
    ready_event_num = -1;
}

m_epoll::~m_epoll()
{

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

#endif