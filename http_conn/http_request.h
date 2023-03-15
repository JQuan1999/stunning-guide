#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H


#include <regex>
#include <string>
#include <unordered_map>
#include <regex>
#include <iostream>
#include <fstream>
#include <vector>
#include <sys/stat.h>

#include "../log/log.h"
#include "../buffer/buffer.h"
#include "http_enum.h"


// http解析结果类
class http_request
{
public:

    http_request(const std::string&);
    ~http_request();
    void init();
    HTTP_CODE parser(buffer& buf);
    CHECK_STATE getState();
    bool isKeepAlive();

    const std::string getMethod();
    const std::string getUrl();
    const std::string getVersion();
    const std::string getHead(const std::string);
    const std::string getMode();

private:
    
    bool parseRequestLine(const std::string&);
    bool parseRequestHead(const std::string& );
    bool parseContent(buffer&);
    void _remove(const std::string&);

    CHECK_STATE check_state;
    HTTP_CODE http_code;

    std::string res_dir; 
    std::string req_mode; // get请求是上传还是下载
    std::string method; // 请求方式
    std::string url; // 请求地址
    std::string version; // http请求版本
    std::unordered_map<std::string, std::string> headers;
    std::unordered_map<std::string, std::string> post_former; // Post表单信息
};

#endif