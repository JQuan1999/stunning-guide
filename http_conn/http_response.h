#ifndef HTTP_RESPONSE
#define HTTP_RESPONSE

#include <string>
#include <fcntl.h>
#include <sys/stat.h>
#include <assert.h>
#include <unistd.h>
#include <sys/mman.h>
#include <unordered_map>

#include "http_enum.h"
#include "../buffer/buffer.h"


class http_response
{
public:
    http_response();
    ~http_response();
    void unmapFile();
    void init(HTTP_CODE, std::string, std::string);
    void response(buffer&);

    size_t getFileBytes();
    const char* getFileAddress();

private:
    void _writeStateLine(buffer&);
    void _writeHeader(buffer&);
    void _writeContent(buffer&);

private:
    std::string root_path; // 根目录文件路径
    std::string request_file; // 请求文件
    std::string abs_file_path; // 文件的绝对路径

    static std::unordered_map<int, std::string> code_status;
    static std::unordered_map<int, std::string> error_code_path;

    struct stat file_stat; // 文件状态
    int code; 
    char* mm_file_address; // 请求文件映射到内存中的地址
};

#endif