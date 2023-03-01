#ifndef SERVER
#define SERVER

#define MAX_FD 65535
#define IP "127.0.0.1"
#define PORT 12345

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdlib.h>
#include <fcntl.h>
#include <cstring>
#include <unistd.h>
#include <iostream>
#include <unordered_map>

#include "../http_conn/http_conn.h"
#include "m_epoll.h"

class server
{
public:
    server(int trigMode = 0);
    ~server();

    void initSocket();
    void run();
    void acceptClientResquest(); // 接受客户连接
private:
    void initEventMode(); // 初始化listen_fd和connfd事件触发模式
    int setNoBlocking(int fd); // 设置非阻塞
private:
    int listen_fd; // 监听的文件描述符
    int epoll_fd; // epoll监听文件描述符
    int pipefd[2]; // 接受信号处理的管道
    event_type listenfd_ev_mode;
    event_type connfd_ev_mode;
    std::unordered_map<int, httpConn> users; // 保存文件描述符到客户连接请求的哈希表users
    // 智能指针实现
    m_epoll* m_epollPtr;
};

#endif