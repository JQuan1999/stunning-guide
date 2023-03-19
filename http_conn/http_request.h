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

    http_request(const std::string&, const int&);
    ~http_request();
    void init();
    HTTP_CODE parser(buffer& buf);
    CHECK_STATE getState();
    POST_CODE getPCode();
    bool isKeepAlive();

    const std::string getMethod();
    const std::string getUrl();
    const std::string getVersion();
    const std::string getHead(const std::string);
    const std::string getMode();

private:
    
    bool parseRequestLine(buffer&);
    bool parseRequestHead(buffer&);
    POST_CODE parseContent(buffer&);

    CHECK_STATE check_state;
    CHECK_POST_CONTENT check_post_state;
    HTTP_CODE http_code;
    POST_CODE post_code;
    int fd;
    bool first_write;

    std::string res_dir; 
    std::string req_mode; // get请求是上传还是下载
    std::string method; // 请求方式
    std::string url; // 请求地址
    std::string version; // http请求版本
    std::string sep; // 分隔符
    std::string upload_file; // 上传文件名
    std::unordered_map<std::string, std::string> headers;
};

#endif