#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include <netinet/in.h>
#include <cstring>
#include <string>
#include <sys/epoll.h>
#include <memory>
#include <arpa/inet.h>
#include <sys/uio.h>

#include "http_request.h"
#include "http_response.h"
#include "http_enum.h"
#include "../buffer/buffer.h"


// 客户连接请求的类
class httpConn
{
public:
    httpConn();
    void init(int connfd, sockaddr_in address, std::string root_path);
    ~httpConn();
    int httpRead(int&);
    bool parseHttpRequest();
    bool process();
    bool isKeepAlive();
    int httpWrite(int&);
    void closeData();
    const char* getIp();
    const int getPort();

private:
    bool _readProcess();
    void _reset();

private:

    struct sockaddr_in address;
    struct iovec m_iv[2];
    int m_iv_cnt;
    int sockfd;
    int port;
    char ip[20];
    uint32_t event_mode; // 事件类型

    size_t once_read_bytes; // 一次读取的最大字节

    buffer read_buffer;
    buffer write_buffer;

    std::string root_path; // 服务器根目录

    http_request m_request; // 请求解析
    http_response m_response; // 请求回复

};

#endif