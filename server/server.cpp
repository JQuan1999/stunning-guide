#include"server.h"

// 初始化信息
server::server(int trigMode)
{
    m_epollPtr = new m_epoll();
    assert(m_epollPtr != nullptr);
    initSocket();
}

void server::initSocket()
{
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    assert(listen_fd != -1);

    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    inet_pton(AF_INET, IP, &address.sin_addr);

    int ret = bind(listen_fd, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);

    ret = listen(listen_fd, 5);
    assert(ret != -1);

    setNoBlocking(listen_fd);

    // 往m_epollPtr注册fd的读事件
    m_epollPtr->addFd(listen_fd, EPOLLIN);

    // 监听信号
    // ...

    // 初始化线程池...

    // 初始化日志...
}


// 接受连接请求
void server::acceptClientResquest()
{
    struct sockaddr_in client_address;
    socklen_t len = sizeof(client_address);
    int connfd = accept(listen_fd, (struct sockaddr*)&client_address, &len);
    assert(connfd != -1);

    if(users.size() >= MAX_FD)
    {
        std::cout<<"客户连接数目过多"<<std::endl;
        close(connfd);
        return;
    }
    // 待实现：添加定时器、设置非阻塞
    users[connfd] = httpConn(client_address, connfd);
    setNoBlocking(connfd);
}

// server的主循环
// 监听listenfd上的客户链接请求、监听信号
void server::run()
{   
    char buf[1024];
    int ret;
    while(true)
    {
        int number = m_epollPtr->waitFd();
        for(size_t i = 0; i < number; i++)
        {
            int sockfd = m_epollPtr->getFd(i);
            event_type events = m_epollPtr->getEvents(i);
            if(sockfd == listen_fd && (events & EPOLLIN))
            {
                acceptClientResquest();
            }
            // 监听到connfd上的读事件
            else if(events & EPOLLIN)
            {
                // 读数据 
                // 待实现将读任务和写任务添加到线程池，由子线程进行读和写
                memset(buf, 0, sizeof(buf));
                ret = recv(sockfd, buf, 1024 - 1, 0);
                if(ret < 0 && errno != EAGAIN){
                    std::cout<<"客户端已断开连接"<<std::endl;
                    close(sockfd);
                    users.erase(sockfd);
                    m_epollPtr->removeFd(sockfd);
                }
                if(ret == 0){
                    continue;
                }
                std::cout<<"recv data from client buf: "<<buf<<std::endl;
                m_epollPtr->modifyFd(sockfd, EPOLLOUT);
                for(int i = 0; i < ret; i++)
                {
                    buf[i] = toupper(buf[i]);
                }

            }
            else if(events & EPOLLOUT)
            {
                send(sockfd, buf, ret, 0);
            }
        }
    }
}

int server::setNoBlocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, old_option | O_NONBLOCK);
    return old_option;
}