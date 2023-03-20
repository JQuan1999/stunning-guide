#include"http_conn.h"

httpConn::httpConn()
{

}
void httpConn::init(int fd, sockaddr_in addr, const std::string& r_dir, const std::string& h_dir, uint32_t conn_ev)
{
    sockfd = fd;
    address = addr;
    conn_mode = conn_ev;
    bzero(ip, sizeof(ip));
    inet_ntop(AF_INET, &address.sin_addr, ip, sizeof(ip));
    port = ntohs(address.sin_port);
    read_buffer = std::make_unique<buffer>();
    read_buffer->delAll();
    write_buffer = std::make_unique<buffer>();
    write_buffer->delAll();
    m_request = std::make_unique<http_request>(r_dir, fd);
    m_response = std::make_unique<http_response>(r_dir, h_dir, fd);
    m_iv_cnt = 0;
}

httpConn::~httpConn()
{
    closeData();
}

void httpConn::closeData()
{
    m_response->unmapFile(); // 删除映射的文件
    close(sockfd); // 关闭文件描述符
}

bool httpConn::isKeepAlive()
{
    return m_request->isKeepAlive();
}

const char* httpConn::getIp()
{
    return ip;
}

const int httpConn::getPort()
{
    return port;
}

const buffer& httpConn::getReadBuf()
{
    return *read_buffer;
}

const buffer& httpConn::getWriteBuf()
{
    return *write_buffer;
}


bool httpConn::httpRead()
{   
    int ret, save_errno;
    // 重新初始化
    if(m_request->getState() == FINISH){
        m_request->init();
    }
    do{
        ret = read_buffer->readFd(sockfd, save_errno);
        // 读完数据
        if(ret == -1){
            if(save_errno == EAGAIN){
                return true;
            }else{
                return false;
            }
        }
        // 客户端已断开连接
        else if(ret == 0) 
        {
            return false;
        }
        parse_code = m_request->parser(*read_buffer);
        read_buffer->delAll();
        if(parse_code == NO_REQUEST){
            continue;
        }else if(parse_code == BAD_REQUEST){
            // std::cout<<"get bad request"<<std::endl;
            return false;
        }else if(parse_code == GET_REQUEST){
            // std::cout<<"get request"<<std::endl;
            return true;
        }
    } while (conn_mode & EPOLLET);
    return m_request->getState() == FINISH;
}


bool httpConn::httpWrite()
{
    post_code = m_request->getPCode();
    // 根据解析的结果 初始化request 传入状态码、请求方法、GET请求的mode(delete || download)、 是否长连接、请求文件
    m_response->init(parse_code, post_code, m_request->isKeepAlive(), m_request->getMethod(), m_request->getMode(),  m_request->getUrl());
    m_response->response(*write_buffer);
    // 响应头
    m_iv[0].iov_base = (char*)write_buffer->peek();
    m_iv[0].iov_len = write_buffer->readAbleBytes();
    m_iv_cnt = 1;

    if(m_response->getFileAddress() && m_response->getFileBytes() > 0)
    {
        m_iv[1].iov_base = (char*)m_response->getFileAddress();
        m_iv[1].iov_len = m_response->getFileBytes();
        m_iv_cnt += 1;
    }
    int len = -1;
    do{
        len = writev(sockfd, m_iv, m_iv_cnt);
        if(len < 0){
            if(errno != EAGAIN){
                return false;
            }else{
                return true;
            }
        }

        // 调整iv 的base位置, 修改write_buf的发送信息
        if(m_iv[0].iov_len + m_iv[1].iov_len == 0) { break; }
        else if(len > m_iv[0].iov_len)
        {
            m_iv[1].iov_base = (char*)m_iv[1].iov_base + (len - m_iv[0].iov_len);
            m_iv[1].iov_len -= (len - m_iv[0].iov_len);
            if(m_iv[0].iov_len)
            {   
                write_buffer->delAll();
                m_iv[0].iov_len = 0;
            }
        }
        else{
            m_iv[0].iov_base = (char*)m_iv[0].iov_base + len;
            m_iv[0].iov_len -= len;
            write_buffer->deln(len);
        }
    }while(conn_mode & EPOLLET);
    LOG_DEBUG("the file size is %d bytes, send to client sucessfully", m_response->getFileBytes());
    return true;
}