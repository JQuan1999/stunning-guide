#include "http_response.h"

std::string http_response::file_dir = "files";
std::string http_response::list_html = "filelist.html";
std::string http_response::index_html = "index.html";

// http状态码 转化为数字
std::unordered_map<HTTP_CODE, int> http_response::httpCode2number = {
    {NO_REQUEST, -1},
    {GET_REQUEST, 200},
    {BAD_REQUEST, 400},
    {FORBIDDEN_REQUEST, 403},
};

// 状态码
std::unordered_map<int, std::string> http_response::code_status = {
    {200, "OK"},
    {400, "Bad Request"},
    {403, "Forbidden"},
    {404, "Not Found"},
};

// 错误状态码对应的html文件
std::unordered_map<int, std::string> http_response::error_code_path = {
    {400, "html/400.html"},
    {403, "html/403.html"},
    {404, "html/404.html"},
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
void http_response::init(HTTP_CODE http_code, bool keep_alive, std::string url, std::string root)
{
    assert(root.size() > 0);
    assert(httpCode2number.count(http_code));
    if(url.size() == 0 || (url.size() == 1 && url[0] == '/'))
    {
        url = "index.html";
    }

    if(mm_file_address)
    {
        unmapFile();
    }
    file_stat = {0};

    code = httpCode2number[http_code]; // 状态码转换
    is_keep_alive = keep_alive;
    request_file = url;
    root_path = root;

    _updateIndexHtml();

    if(stat((root_path + request_file).c_str(), &file_stat) < 0 || S_ISDIR(file_stat.st_mode))
    {
        code = 404; // 找不到
    }

    if(error_code_path.count(code))
    {
        request_file = error_code_path[code];
    }

    abs_file_path = root_path + request_file;
    std::cout<<"request url: "<<url<<", root path: "<<root<< ", code:" <<code<< ", after modify the fact file path: "<<abs_file_path<<"\n";

    assert(stat(abs_file_path.c_str(), &file_stat) == 0 ); // 初始话后的文件一定是可以找到且能访问的

}

const char* http_response::getFileAddress()
{
    return mm_file_address;
}

size_t http_response::getFileBytes()
{
    return file_stat.st_size;
}


void http_response::_updateIndexHtml()
{
    
    std::string index_html, temp_line;
    // 用个配置文件保存信息
    std::string filelist_path = root_path + list_html;
    assert(access(filelist_path.c_str(), F_OK) == 0); // 文件路径存在

    std::ifstream filelist(filelist_path.c_str());
    // 找到插入位置
    while(1)
    {
        std::getline(filelist, temp_line, '\n');
        if(temp_line == "<!--插入位置-->")
        {
            break;
        }
        index_html += temp_line + '\n';
    }

    // 获取file_dir文件下的所有文件名
    std::string dir_name = root_path + file_dir;
    DIR* dir = opendir(dir_name.c_str());
    assert(dir != nullptr);

    struct dirent* stdinfo;
    while(1)
    {
        // 获取文件夹下的所有文件
        stdinfo = readdir(dir);
        if(stdinfo == nullptr) break;
        std::string name = stdinfo->d_name;
        if(name == "." || name == "..") continue;

        // 加入表格内容
        index_html += "            <tr><td class=\"col1\">" + name +
                    "</td> <td class=\"col2\"><a href=\"download/" + name +
                    "\">下载</a></td> <td class=\"col3\"><a href=\"delete/" + name +
                    "\" onclick=\"return confirmDelete();\">删除</a></td></tr>" + "\n";
    }

    // 加上插入位置之后的内容
    while(std::getline(filelist, temp_line, '\n')){
        index_html += temp_line + "\n";
    }

    std::string save_path = root_path + "index.html";
    std::ofstream save_file(save_path, std::ios_base::out);
    save_file<<index_html;

    filelist.close();
    save_file.close();
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
    if(is_keep_alive)
    {
        write_buf += "Connection: keep-alive\r\n";
    }else{
        write_buf += "Connection: close\r\n";
    }
    
    // 文件类型(根据request_file文件类型回复content-type)
    write_buf += "Content-type: text/html; chartset=UTF-8\r\n";
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