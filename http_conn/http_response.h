#ifndef HTTP_RESPONSE
#define HTTP_RESPONSE

#include <string>
#include <fcntl.h>
#include <sys/stat.h>
#include <assert.h>
#include <unistd.h>
#include <sys/mman.h>
#include <unordered_map>
#include <fstream>
#include <vector>
#include <dirent.h>

#include "http_enum.h"
#include "../log/log.h"
#include "../buffer/buffer.h"


class http_response
{
public:
    http_response(const std::string&, const std::string&, const int&);
    ~http_response();
    void unmapFile();
    void init(HTTP_CODE, bool, const std::string& , const std::string&, const std::string&);
    void response(buffer&);

    size_t getFileBytes();
    const char* getFileAddress();

private:
    void _remove(const std::string&);
    void _initDownLoad(const std::string&);
    void _initOthers(const std::string&);
    void _updateIndexHtml();
    void _writeStateLine(buffer&);
    void _writeHeader(buffer&);
    void _writeContent(buffer&);

private:
    int fd;
    bool is_keep_alive;
    std::string res_dir; 
    std::string htmls_dir; 
    std::string request_file; // 请求文件
    std::string method;
    std::string mode;
    int code; 
    
    static std::string index_html;
    static std::string list_html;
    static std::unordered_map<int, std::string> code_status;
    static std::unordered_map<int, std::string> error_code_path;
    static std::unordered_map<HTTP_CODE, int> httpCode2number;
    static std::unordered_map<std::string, std::string> suffix_type;

    struct stat file_stat; // 文件状态
    char* mm_file_address; // 请求文件映射到内存中的地址
};

#endif