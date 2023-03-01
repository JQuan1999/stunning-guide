#include"http_conn.h"

httpConn::httpConn(sockaddr_in address, int fd)
{
    m_address = address;
    m_connfd = fd;
}

httpConn::~httpConn()
{
    
}