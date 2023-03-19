#include "http_request.h"

http_request::http_request(const std::string& r_dir, const int& connfd)
{
    res_dir = r_dir;
    fd = connfd;
    init();
}

http_request::~http_request()
{

}


void http_request::init()
{
    check_state = REQUEST_LINE;
    check_post_state = POST_SEPE;
    req_mode = method = url = version = sep = upload_file ="";
    first_write = true;
    headers.clear();
}

CHECK_STATE http_request::getState()
{
    return check_state;
}

bool http_request::isKeepAlive()
{
    if(headers.count("Connection"))
    {
        return headers["Connection"] == "keep-alive";
    }
    return false;
}

HTTP_CODE http_request::parser(buffer& buf)
{
    std::string partition = "\r\n", line;
    int pos;
    // std::cout<<"buf = "<<buf<<std::endl;
    while(!buf.empty())
    {
        
        switch (check_state)
        {

        case REQUEST_LINE:
            if(!parseRequestLine(buf))
            {
                check_state = FINISH;
                return BAD_REQUEST;
            }else{
                check_state = HEADER;
            }
            break;

        case HEADER:
            if(!parseRequestHead(buf))
            {
                check_state = FINISH;
                return BAD_REQUEST;
            }else{
                check_state = CONTENT;
                if(buf.empty()){
                    check_state = FINISH;
                    return GET_REQUEST;
                }
            }
            break;
        
        case CONTENT:
            if(!parseContent(buf))
            {
                if(method == "POST" && check_post_state == POST_CONTENT)
                {
                    break;
                }else{
                    check_state = FINISH;
                    return BAD_REQUEST;
                }
            }
            check_state = FINISH;
            return GET_REQUEST;

        default:
            break;
        }
    }
    return NO_REQUEST;
}

bool http_request::parseRequestLine(buffer& buf)
{   
    std::string request = buf.subsepstr("\r\n");
    request.erase(request.end() - 2, request.end()); // 删除\r\n
    if(request.size() == 0){
        return false;
    }
    // 正则表达式匹配每个()匹配一个字段, ^表示开始，$表示结束
    std::regex pattern("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    std::smatch sub_match;
    bool ret;
    if(ret = std::regex_match(request, sub_match, pattern))
    {
        method = sub_match[1];
        url = sub_match[2];
        version = sub_match[3];
        if(method != "GET" && method != "POST")
        {
            ret = false;
        }
    }
    if(ret && method == "GET")
    {
        // url是否为 /delete/file 或者 /download/file
        int pos, begin = 0;
        if(url.size() != 0){
            if(url[0] == '/'){
                begin = 1;
            }
            pos = url.find('/', begin);
            if(pos != -1){
                req_mode = url.substr(begin, pos - begin);
            }
        }

        // 是delete或者download  需要截取真正的url
        if(req_mode.size() != 0 && (req_mode == "delete" || req_mode == "download")){
            int len = req_mode.size() + begin;
            url.erase(0, len); // 删除mode
        }
        LOG_DEBUG("fd: %d, method: %s, url: %s, version: %s", fd, method.c_str(), url.c_str(), version.c_str());
        if(req_mode.size() != 0){
            LOG_DEBUG("fd: %d, request mode: %s", fd, req_mode.c_str());
        }
    }
    else if(!ret)
    {
        // 解析出错
        LOG_ERROR("fd: %d, parser error the request line is: %s", fd, request.c_str());
    }
    return ret;
}


bool http_request::parseRequestHead(buffer& buf)
{
    std::string line;
    int pos;
    // 正则表达式匹配每个()匹配一个字段, 第一个()匹配head字段键、第二个匹配head字段值
    std::regex pattern("^([^:]*): ?(.*)$");
    std::smatch sub_match;
    while(1)
    {
        line = buf.subsepstr("\r\n");
        line.erase(line.end() - 2, line.end()); // 删除\r\n
        if(line.size() == 0){
            break;
        }
        if(std::regex_match(line, sub_match, pattern))
        {
            // Content-Type: multipart/form-data; boundary=----分隔符 需要特殊处理
            if(sub_match[1] == "Content-Type"){
                std::string value = sub_match[2]; // content_type类型
                if( (pos = value.find(';')) != -1 ){
                    int begin = 0;
                    while(value[begin] == ' ') begin++; // 去除空格
                    std::string content_type = value.substr(begin, pos - begin);
                    value.erase(0, pos + 1);
                    headers[sub_match[1]] = content_type;
                    // 保存分隔符
                    if((pos = value.find('=')) != -1 ){
                        headers[value.substr(0, pos)] = value.erase(0, pos + 1);
                    }
                }
            }else{
                headers[sub_match[1]] = sub_match[2];
            }
        }else{
            return false;
        }
    }
    return true;
}


bool http_request::parseContent(buffer& buf)
{
    if(method == "GET")
    {
        return true;
    }
    // post需要解析表单字段, 将post的内容写入文件中
    std::string line;
    int pos = -1;
    // 解析流程：1 解析分隔符 2 解析表单字段 3 解析内容
    // 只支持解析上传文件的post请求multipart/form-data
    while(check_post_state != POST_CHECK_FINISH && !buf.empty())
    {
        if(check_post_state == POST_SEPE)
        {
            line = buf.subsepstr("\r\n");
            if(line.size() == 0){
                LOG_DEBUG("fd: %d, while checking post content, doesn't find seperator", fd);
                break;
            }
            line.erase(line.end() - 2, line.end());
            if(line.substr(0, 2) == "--"){
                sep = line;
                check_post_state = POST_FORMER;
            }else{
                LOG_DEBUG("fd: %d, while checking post content, the first line is not the seperator, the first line of content is: %s", fd, line.c_str());
                return false;
            }
        }
        if(check_post_state == POST_FORMER)
        {
            line = buf.subsepstr("\r\n");
            if(line.size() == 0)
            {
                LOG_DEBUG("fd: %d, while checking post content, doesn't find seperator", fd);
                break;
            }
            line.erase(line.end() - 2, line.end());
            if(line.size() == 0){
                check_post_state = POST_CONTENT;
                continue;
            }
            // 找到文件名 Content-Disposition: form-data; name="upload"; filename="xxx"
            if((pos = line.find("filename")) == -1){
                continue;
            }
            // 找到"xxx"的起始位置
            int begin = line.find('"', pos);
            if(begin == -1){
                continue;
            }
            int end = line.find('"', begin + 1);
            if(end == -1)
            {
                continue;
            }
            upload_file = line.substr(begin + 1, end - begin - 1);
            LOG_DEBUG("fd: %d, get the post filename: %s", fd, upload_file.c_str());
        }
        if(check_post_state == POST_CONTENT)
        {
            if(upload_file.size() == 0 || sep.size() == 0){
                return false;
            }
            // 判断最后的字符是不是 sep--\r\n
            size_t readable = buf.readAbleBytes();
            if(readable == 0)
            {
                break;
            }
            size_t sep_len = sep.size();
            size_t last_len = sep_len + 4; // 加上--\r\n

            std::string file_path = res_dir + upload_file;
            std::ofstream f;
            if(first_write){
                f.open(file_path, std::ios::out);
                first_write = false;
            }else{
                f.open(file_path, std::ios::app);
            }

            if(readable >= last_len)
            {
                line = std::string(buf.peek() + readable - last_len, sep_len);
            }else{
                line = "";
            }
            if(line == sep)
            {
                f.write(buf.peek(), readable - last_len); // 减去\r\n + sep--\r\n
                buf.delAll();
                LOG_DEBUG("fd: %d, filename: %s is upload sucessfully!", fd, upload_file.c_str());
                check_post_state = POST_CHECK_FINISH;
            }else{
                f.write(buf.peek(), readable);
                buf.delAll();
            }
            f.close();
            break;
        }
    }
    return check_post_state == POST_CHECK_FINISH;
}


const std::string http_request::getMethod()
{
    return method;
}

const std::string http_request::getUrl()
{
    return url;
}

const std::string http_request::getVersion()
{
    return version;
}


const std::string http_request::getHead(const std::string head)
{
    if(headers.count(head))
    {
        return headers[head];
    }
    return "";
}

const std::string http_request::getMode()
{
    return req_mode;
}