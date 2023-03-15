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
#include <memory>
#include <unordered_map>
#include <functional>

#include "../http_conn/http_conn.h"
#include "../pool/thread_pool.h"
#include "../http_conn/http_enum.h"
#include "../log/log.h"
#include "m_epoll.h"

class server
{

public:
    server(int port, event_type listen_event , event_type connfd_event, int threads = 8, 
    std::string root_path = "", std::string resource_dir = "", std::string log_dir = "", 
    bool open_log = true, int log_level = 0, int split_lines = 50000, int log_cap = 1024);
    ~server();

    void initSocket();
    void run();
    void _acceptClientResquest(); // 接受客户连接

    httpConn& getHttpConn(int); // 取出users信息

private:
    void _initEventMode(); // 初始化listen_fd和connfd事件触发模式
    int _setNoBlocking(int fd); // 设置非阻塞
    void _addClient(int fd, sockaddr_in client_address); // 添加文件描述符
    void _removeClient(int fd); // 移除文件描述符
    void _dealRead(int fd); // 处理fd的可读事件
    void _onRead(int fd);
    void _dealWrite(int fd); // 处理fd的可写事件
    void _onWrite(int fd);
    void _onProcess(int fd); 

private:
    bool stop;
    int m_port; // 监听的端口
    int listen_fd; // 监听的文件描述符
    int epoll_fd; // epoll监听文件描述符
    int pipefd[2]; // 接受信号处理的管道
    std::string root_path;
    std::string resource_dir;
    std::string log_dir;
    event_type listen_mode;
    event_type conn_mode;
    std::unordered_map<int, httpConn> users; // 保存文件描述符到客户连接请求的哈希表users
    std::unique_ptr<m_epoll> m_epoll_ptr;
    std::unique_ptr<thread_pool> t_pool;
    std::function<void(void)> task; // 由线程池进行处理的任务
};

#endif