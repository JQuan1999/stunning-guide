#include"http_conn.h"

httpConn::httpConn()
{
    once_read_bytes = 1024;
    event_mode = EPOLLET | EPOLLONESHOT; // 待做：server传递mode作为构造函数参数
}

void httpConn::init(int fd, sockaddr_in address, std::string root_path)
{
    address = address;
    sockfd = fd;
    root_path = root_path;
    read_buffer.delAll();
    write_buffer.delAll();
}

// 重新初始化信息
void httpConn::_reset()
{
    m_iv_cnt = 0;
}

httpConn::~httpConn()
{
    closeData();
}

void httpConn::closeData()
{
    m_response.unmapFile(); // 删除映射的文件
    close(sockfd); // 关闭文件描述符
}

size_t httpConn::httpRead(int& save_errno)
{   
    size_t ret;
    do{
        ret = read_buffer.readFd(sockfd, save_errno);
        if(ret == -1){
            save_errno = errno; 
            break;
        }
        else if(ret == 0)
        {
            // 客户端已断开连接
            break;
        }
    } while (event_mode & EPOLLET);
    
    return ret;
}

// 写数据
size_t httpConn::httpWrite(int& save_errno)
{
    size_t len = -1;
    do{
        len = writev(sockfd, m_iv, m_iv_cnt);
        if(len <= 0)
        {
            save_errno = errno;
            break;
        }

        // 调整iv 的base位置, 修改write_buf的发送信息
        if(m_iv[0].iov_len + m_iv[1].iov_len == 0) { break; }
        else if(len > m_iv[0].iov_len)
        {
            m_iv[1].iov_base = (char*)m_iv[1].iov_base + (len - m_iv[0].iov_len);
            m_iv[1].iov_len -= (len - m_iv[0].iov_len);
            if(m_iv[0].iov_len)
            {   
                write_buffer.delAll();
                m_iv[0].iov_len = 0;
            }
        }
        else{
            m_iv[0].iov_base = (char*)m_iv[0].iov_base + len;
            m_iv[0].iov_len -= len;
            write_buffer.deln(len);
        }
    }while(event_mode & EPOLLET);

    return len;
}

/*
客户连接请求处理的主流程: 2. 解析http请求 3.发送回复
*/
bool httpConn::process()
{   
    // 重新初始化
    m_request.init();
    if(read_buffer.readAbleBytes() <= 0)
    {
        return false;
    }
    HTTP_CODE code = m_request.parser(read_buffer);
    if(code == NO_REQUEST)
    {
        return false;
    }
    // 根据解析的结果 初始化request
    m_response.init(code, m_request.get_url(), root_path);
    m_response.response(write_buffer);
    
    // 响应头
    m_iv[0].iov_base = (char*)write_buffer.peek();
    m_iv[0].iov_len = write_buffer.writeAbleBytes();
    m_iv_cnt = 1;

    if(!m_response.getFileAddress() && m_response.getFileBytes() != 0)
    {
        m_iv[1].iov_base = (char*)m_response.getFileAddress();
        m_iv[1].iov_len = m_response.getFileBytes();
        m_iv_cnt += 1;
    }

    // 输出日志 cout<<...

    return true;
}

