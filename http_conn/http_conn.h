#ifndef HTTP_CONN
#define HTTP_CONN

#include <netinet/in.h>
// 解析客户连接请求的类
class httpConn
{
public:
    httpConn(sockaddr_in address, int connfd);
    ~httpConn();

private:
    struct sockaddr_in m_address;
    int m_connfd;
};

#endif