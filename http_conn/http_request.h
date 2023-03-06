#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H


#include <regex>
#include <string>
#include <unordered_map>
#include <regex>
#include <iostream>

#include "./http_request.h"
#include "http_enum.h"    
#include "../buffer/buffer.h"


// http解析结果类
class http_request
{
public:

    http_request();
    ~http_request();
    void init();
    HTTP_CODE parser(buffer& buf);

    const std::string get_method();
    const std::string get_url();
    const std::string get_version();
    const std::string get_head(const std::string);
    const std::string get_body();

private:
    
    bool parseRequestLine(const std::string&);
    bool parseRequestHead(const std::string& );
    bool parseContent(const std::string&);

    CHECK_STATE check_state;
    HTTP_CODE http_code;

    std::string method; // 请求方式
    std::string url; // 请求地址
    std::string version; // http请求版本
    std::string body; // 请求内容
    std::unordered_map<std::string, std::string> headers;
};

#endif