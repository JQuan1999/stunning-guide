#ifndef M_EPOLL
#define M_EPOLL

#include <sys/epoll.h>
#include <vector>
#include <assert.h>
#include <unistd.h>

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
    void closeFd();
    event_type getEvents(size_t index); // 获取触发事件的第i个文件描述符的事件
private:
    int m_epoll_fd;
    int max_event_num;
    int ready_event_num;
    std::vector<epoll_event> m_events;
};

#endif