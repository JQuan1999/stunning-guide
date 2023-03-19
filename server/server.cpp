#include"server.h"

int server::pipefd[2] = {0};

server::server(int port, event_type listen_ev, event_type conn_ev, int threads, 
                std::string r_dir, std::string res_dir, std::string lg_dir, std::string h_dir, 
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

    resource_dir = root_path + res_dir;
    assert(stat(resource_dir.c_str(), &st) == 0 && S_ISDIR(st.st_mode));

    log_dir = root_path + lg_dir;
    assert(stat(log_dir.c_str(), &st) == 0 && S_ISDIR(st.st_mode));

    htmls_dir = root_path + h_dir;
    assert(stat(htmls_dir.c_str(), &st) == 0 && S_ISDIR(st.st_mode));

    m_epoll_ptr = std::make_unique<m_epoll>();
    t_pool = std::make_unique<thread_pool>(threads);
    if(open_log)
    {
        Log::getInstance()->init(log_level, log_dir.c_str(), ".log", split_lines, log_cap);
        LOG_INFO("==================server start==================");
        LOG_INFO("listen port = %d", port);
        LOG_INFO("work thread number = %d", threads);
        const char* in = (conn_mode & EPOLLIN) == 0 ? "No EPOLLIN": "EPOLLIN";
        const char* et = (conn_mode & EPOLLET) == 0 ? "No EPOLLET": "EPOLLET";
        const char* oneshot = (conn_mode & EPOLLONESHOT) == 0 ? "No EPOLLONESHOT": "EPOLLONESHOT";
        LOG_INFO("server connention mode = %s %s %s", in, et, oneshot);
        LOG_INFO("root dir = %s, resource dir = %s, log dir = %s", root_path.c_str(), resource_dir.c_str(), log_dir.c_str());
        LOG_INFO("log level = %d, split line = %d, capacity = %d", log_level, split_lines, log_cap);
    }
    _initSocket();
}


server::~server()
{
    _stop();
}

void server::_stop()
{
    stop = true;
    close(listen_fd);
    m_epoll_ptr->closeFd();
    for(auto& p: users)
    {
        p.second.closeData();
        users.erase(p.first);
    }
}

void server::sigHandler(int sig)
{
    int save_errno = errno;
    int msg = sig;
    send(pipefd[1], (char*)&msg, 1, 0);
    errno = save_errno;
}

void server::_addSig(int sig)
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = server::sigHandler;
    sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, nullptr) != -1);
}

void server::_initSocket()
{
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    assert(listen_fd != -1);
    // int opt = 1;
    // setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, (const void*)&opt, sizeof(opt));

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
    m_epoll_ptr->addFd(listen_fd, listen_mode);

    // 监听信号
    ret = socketpair(AF_UNIX, SOCK_STREAM, 0, pipefd);
    assert(ret != -1);
    _setNoBlocking(pipefd[1]);
    m_epoll_ptr->addFd(pipefd[0], EPOLLIN);
    _addSig(SIGINT);
    _addSig(SIGTERM);
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
        LOG_INFO("too many client connection, server closed the connfd: %d", connfd);
        close(connfd);
        return;
    }
    _addClient(connfd, client_address);
}


void server::_addClient(int fd, sockaddr_in address)
{
    // 待实现：添加定时器
    users[fd].init(fd, address, resource_dir, htmls_dir, conn_mode);
    LOG_INFO("accept new connection from client, connfd = %d, ip = %s, port = %d", fd, users[fd].getIp(), users[fd].getPort());
    m_epoll_ptr->addFd(fd, EPOLLIN | conn_mode);
    _setNoBlocking(fd);
}


void server::_removeClient(int fd)
{
    // 待实现：移除定时器
    assert(m_epoll_ptr->removeFd(fd) != -1);
    LOG_INFO("close client connection, connfd = %d, ip = %s, port = %d", fd, users[fd].getIp(), users[fd].getPort());
    users[fd].closeData();
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


// 循环读边读边解析
void server::_onRead(int fd)
{
    if(users[fd].httpRead())
    {
        LOG_DEBUG("fd: %d, data has read over, add epollout event", fd);
        m_epoll_ptr->modifyFd(fd, EPOLLOUT | conn_mode); // 加上conn_ev_type
    }else
    {
        LOG_DEBUG("fd: %d, error has happened while read data", fd);
        _removeClient(fd);
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
    if(users[fd].httpWrite())
    {
        if(users[fd].isKeepAlive()){
            LOG_DEBUG("fd: %d, write sucessfuly, modify fd EPOLLIN event", fd);
            m_epoll_ptr->modifyFd(fd, EPOLLIN | conn_mode);
            return;
        }else{
            LOG_DEBUG("fd: %d, write sucessfuly close connection", fd);
            _removeClient(fd);
        }
    }else{
        LOG_ERROR("fd: %d, something error has happen close connection", fd);
        _removeClient(fd);
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
            else if(sockfd == pipefd[0] && (events & EPOLLIN))
            {
                // 接收到中止信号
                LOG_INFO("==============receive stop singal server stop=================");
                _stop();
            }
            // 监听到connfd上的读事件
            else if(events & EPOLLIN)
            {   
                _dealRead(sockfd);
            }
            // 监听到connfd上的写事件
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