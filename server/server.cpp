#include"server.h"

// 初始化信息
server::server(event_type listen_mode , event_type conn_mode, std::string root_path)
{
    m_epoll_ptr = std::make_unique<m_epoll>();
    t_pool = std::make_unique<thread_pool>(2);
    this->root_path = root_path;
    assert(m_epoll_ptr != nullptr);
    initSocket();
}

server::~server()
{
    close(listen_fd);
    m_epoll_ptr->closeFd();
    for(auto& p: users)
    {
        p.second.closeData();
    }
}

void server::initSocket()
{
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    assert(listen_fd != -1);
    // 设置端口复用，测试时使用
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, (const void*)&opt, sizeof(opt));

    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    inet_pton(AF_INET, IP, &address.sin_addr);

    int ret = bind(listen_fd, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);

    ret = listen(listen_fd, 5);
    assert(ret != -1);

    _setNoBlocking(listen_fd);

    // 往m_epollPtr注册fd的读事件
    // assert
    m_epoll_ptr->addFd(listen_fd, EPOLLIN);

    // 监听信号
    // ...

    // 初始化日志...
}


// 接受连接请求
void server::_acceptClientResquest()
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
    _addClient(connfd, client_address);
}


void server::_addClient(int fd, sockaddr_in address)
{
    // 待实现：添加定时器
    users[fd].init(fd, address, root_path);
    // 待实现设置listenfd的event和connfd的event
    m_epoll_ptr->addFd(fd, EPOLLIN | EPOLLET);
    _setNoBlocking(fd);
}


void server::_removeClient(int fd)
{
    // 待实现：移除定时器
    assert(m_epoll_ptr->removeFd(fd) != -1);
    users[fd].closeData();
}


void server::_onRead(int fd)
{
    int ret = 0, save_errno;
    ret = users[fd].httpRead(save_errno);
    if(ret == -1)
    {
        if(save_errno == EAGAIN)
        {
            m_epoll_ptr->modifyFd(fd, EPOLLIN);
            return;
        }
        _removeClient(fd);
    }
    _onProcess(fd);
}


// 将该函数加入线程池、线程池读数据、调用connfd的函数解析http请求，发送回复报文、根据返回值修改注册事件
void server::_dealRead(int fd)
{
    // todo 延长定时器
    task = [this, fd]()
    {
        _onRead(fd);
    };
    t_pool->append(task);
}


void server::_onProcess(int fd)
{
    if(users[fd].process())
    {
        m_epoll_ptr->modifyFd(fd, EPOLLOUT); // 加上conn_ev_type
    }else
    {
        m_epoll_ptr->modifyFd(fd, EPOLLIN);
    }
}


void server::_dealWrite(int fd)
{
    // 延长定时器
    task = [this, fd]()
    {
        _onWrite(fd);
    };
    t_pool->append(task);
}


void server::_onWrite(int fd)
{
    int ret = 0, save_errno;
    ret = users[fd].httpWrite(save_errno);
    if(ret <= 0)
    {
        if(save_errno == EAGAIN)
        {
            // 继续传输
            m_epoll_ptr->modifyFd(fd, EPOLLOUT);
            return;
        }
        _removeClient(fd);
    }
    // 传输完成 重新初始化相关信息
    _onProcess(fd);
}

// server的主循环
void server::run()
{    
    while(true)
    {
        int number = m_epoll_ptr->waitFd();
        for(size_t i = 0; i < number; i++)
        {
            int sockfd = m_epoll_ptr->getFd(i);
            event_type events = m_epoll_ptr->getEvents(i);
            if(sockfd == listen_fd && (events & EPOLLIN))
            {
                _acceptClientResquest();
            }
            // 监听到connfd上的读事件
            else if(events & EPOLLIN)
            {
                _dealRead(sockfd);
            }
            else if(events & EPOLLOUT)
            {
                // 线程池写
                _dealWrite(sockfd);
            }
        }
    }
}

httpConn& server::getHttpConn(int fd)
{
    return users[fd];
}

int server::_setNoBlocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, old_option | O_NONBLOCK);
    return old_option;
}