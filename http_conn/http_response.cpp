#include "http_response.h"

// 状态码
std::unordered_map<int, std::string> http_response::code_status = {
    {200, "OK"},
    {400, "Bad Request"},
    {403, "Forbidden"},
    {404, "Not Found"},
};

// 错误状态码对应的html文件
std::unordered_map<int, std::string> http_response::error_code_path = {
    {400, "/400.html"},
    {403, "/403.html"},
    {404, "/404.html"},
};

http_response::http_response()
{
    mm_file_address = nullptr;

}

http_response::~http_response()
{
    unmapFile();
}

void http_response::unmapFile()
{
    if(mm_file_address)
    {
        munmap(mm_file_address, file_stat.st_size);
        mm_file_address = nullptr;
    }
}

// 待完成：解析相对路径和绝对路径、并判断请求文件是否存在
void http_response::init(HTTP_CODE http_code, std::string url, std::string root)
{
    assert(url.size() > 0 && root.size() > 0);
    if(url.size() == 1 && url[0] == '/')
    {
        request_file = "/hello.html";
    }
    request_file = url;
    root_path = root;
    

    if(http_code == GET_REQUEST)
    {
        code = 200;
    }
    else if(http_code == BAD_REQUEST)
    {
        code = 400;
    }

    if(stat((root_path + request_file).c_str(), &file_stat) < 0 || S_ISDIR(file_stat.st_mode))
    {
        code = 404; // 找不到
    }
    if(file_stat.st_mode & S_IROTH)
    {
        code = 403; // 不可访问
    }
    
    if(error_code_path.count(code))
    {
        request_file = error_code_path[code];
    }

    abs_file_path = root_path + request_file;
    assert(stat(abs_file_path.c_str(), &file_stat) > 0);

    if(mm_file_address)
    {
        unmapFile();
    }
    file_stat = {0};
}

const char* http_response::getFileAddress()
{
    return mm_file_address;
}

size_t http_response::getFileBytes()
{
    return file_stat.st_size;
}


void http_response::response(buffer& write_buf)
{
    _writeStateLine(write_buf); 
    _writeHeader(write_buf); 
    _writeContent(write_buf);
}


// 写入状态行
void http_response::_writeStateLine(buffer& write_buf)
{
    // 检测文件类型：是否能打开、是否是目录
    std::string status;
    if(code_status.count(code))
    {
        status = code_status[code];
    }
    else{
        code = 400;
        status = code_status[code];
    }
    write_buf += "HTTP/1.1 " + std::to_string(code) + " " + status + "\r\n";
}

// 写入头部字段
void http_response::_writeHeader(buffer& write_buf)
{
    // connection
    write_buf += "Connection: keep-alive\r\n";
    // 文件类型(根据request_file文件类型回复content-type)
    write_buf += "Content-type: html\r\n";
    write_buf += "Content-Length: " + std::to_string(file_stat.st_size) + "\r\n\r\n";
}

// 写入传输内容
void http_response::_writeContent(buffer& write_buf)
{
    int src_fd = open(abs_file_path.c_str(), O_RDONLY);
    assert(src_fd > 0);
    // 将文件映射到内存
    // 输出日志
    // std::cout<<...
    int* ret = (int*)mmap(nullptr, file_stat.st_size, PROT_READ, MAP_PRIVATE, src_fd, 0);
    assert(*ret != -1);

    mm_file_address = (char*)ret;
    close(src_fd);
}