#include"server.h"


server::server(int port, event_type listen_ev, event_type conn_ev, int threads, 
                std::string r_dir, std::string res_dir, std::string lg_dir,
                bool open_log, int log_level, int split_lines, int log_cap)
{
    m_port = port;
    listen_mode = listen_ev;
    conn_mode = conn_ev;
    stop = false;

    char buf[256] = {0};
    if(r_dir == ""){
        getcwd(buf, 256);
        root_path = std::string(buf);
    }else{
        root_path = r_dir;
    }
    struct stat st;
    assert(stat(root_path.c_str(), &st) == 0 && S_ISDIR(st.st_mode));

    if(res_dir == ""){
        resource_dir = root_path + "/resource/";
    }else{
        resource_dir = root_path + res_dir;
    }

    if(lg_dir == ""){
        log_dir = root_path + "/Logs/";
    }else{
        log_dir = root_path + lg_dir;
    }

    m_epoll_ptr = std::make_unique<m_epoll>();
    t_pool = std::make_unique<thread_pool>(threads);
    if(open_log)
    {
        Log::getInstance()->init(log_level, log_dir.c_str(), ".log", split_lines, log_cap);
        LOG_INFO("server start, port = %d", port);
        LOG_INFO("work thread number = %d", threads);
        const char* in = conn_mode & EPOLLIN == 0 ? "No EPOLLIN": "EPOLLIN";
        const char* et = conn_mode & EPOLLET == 0 ? "No EPOLLET": "EPOLLET";
        const char* oneshot = conn_mode & EPOLLONESHOT ? "No EPOLLONESHOT": "EPOLLONESHOT";
        LOG_INFO("server connention mode = %s %s %s", in, et, oneshot);
        LOG_INFO("root dir = %s, resource dir = %s, log dir = %s", root_path.c_str(), resource_dir.c_str(), log_dir.c_str());
        LOG_INFO("log level = %d, split line = %d, capacity = %d", log_level, split_lines, log_cap);
    }
    initSocket();
}


server::~server()
{
    stop = true;
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
    address.sin_port = htons(m_port);
    inet_pton(AF_INET, IP, &address.sin_addr);

    int ret = bind(listen_fd, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);

    ret = listen(listen_fd, 5);
    assert(ret != -1);

    _setNoBlocking(listen_fd);

    // 往m_epollPtr注册fd的读事件
    m_epoll_ptr->addFd(listen_fd, EPOLLIN);

    // 监听信号...
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
        LOG_INFO("客户连接数目过多, 断开客户连接connfd = %d", connfd);
        close(connfd);
        return;
    }
    _addClient(connfd, client_address);
}


void server::_addClient(int fd, sockaddr_in address)
{
    // 待实现：添加定时器
    users[fd].init(fd, address, resource_dir, conn_mode);
    LOG_INFO("接受到新连接, connfd = %d, ip = %s, port = %d", fd, users[fd].getIp(), users[fd].getPort());
    m_epoll_ptr->addFd(fd, EPOLLIN | conn_mode);
    _setNoBlocking(fd);
}


void server::_removeClient(int fd)
{
    // 待实现：移除定时器
    assert(m_epoll_ptr->removeFd(fd) != -1);
    LOG_INFO("断开客户端连接, connfd = %d, ip = %s, port = %d", fd, users[fd].getIp(), users[fd].getPort());
    users[fd].closeData();
}


void server::_onRead(int fd)
{
    int ret = 0, save_errno;
    ret = users[fd].httpRead(save_errno);
    // 小于0表示客户端已断开连接 正常返回值 = -1 且error = EAGAIN表示数据已读完
    // std::cout<<"fd: "<<fd <<"read data:\n "<<users[fd].getReadBuf();
    if(ret <= 0 && save_errno != EAGAIN)
    {
        _removeClient(fd);
        return;
    }

    if(users[fd].process())
    {
        LOG_DEBUG("fd: %d, 数据读取完毕, 注册写事件", fd);
        m_epoll_ptr->modifyFd(fd, EPOLLOUT | conn_mode); // 加上conn_ev_type
    }else
    {
        LOG_DEBUG("fd: %d, 数据未读完, 继续进行读取", fd);
        m_epoll_ptr->modifyFd(fd, EPOLLIN | conn_mode);
    }
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
    if(ret == 0)
    {
        // 传输完成
        if(users[fd].isKeepAlive())
        {
            LOG_DEBUG("fd: %d, 数据发送完毕, 注册读事件", fd);
            m_epoll_ptr->modifyFd(fd, EPOLLIN | conn_mode);
            return;
        }
        else{
            LOG_DEBUG("fd: %d, 数据发送完毕, 关闭连接", fd);
            _removeClient(fd);
        }
        
    }
    else if(ret < 0)
    {
        if(save_errno == EAGAIN)
        {
            // 继续传输
            LOG_DEBUG("fd: %d, 数据未发送完, 继续进行写", fd);
            m_epoll_ptr->modifyFd(fd, EPOLLOUT | conn_mode);
            return;
        }
        else
        {
            LOG_DEBUG("fd: %d, 故障发生关闭连接", fd);
            _removeClient(fd);
        }
    }
}

// server的主循环
void server::run()
{    
    while(!stop)
    {
        int number = m_epoll_ptr->waitFd();
        for(int i = 0; i < number; i++)
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