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
    req_mode = method = url = version = "";
    headers.clear();
    post_former.clear();
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
        // 请求不完整
        if((pos = buf.find(partition)) == -1)
        {
            LOG_DEBUG("fd: %d, 请求不完整找不到回车符, buf的内容为: %s", fd, buf.peek());
            break;
        }

        // 检查状态行和头部按行解析
        if(check_state == REQUEST_LINE || check_state == HEADER){
            line = buf.toString(pos - buf.getReadPos() + 2, true);
            line.erase(line.end() - 2, line.end()); // 删除\r\n
            // 连续两次上传POST没有请求头第一行就是分隔符
            if(check_state == REQUEST_LINE && line.substr(0, 2) == "--"){
                check_state = CONTENT;
                method = "POST";
                continue;
            }
            // 遇到空行 头部已解析完毕
            if(line.size() == 0 && check_state == HEADER)
            {
                check_state = CONTENT;
                continue;
            }
        }
        
        switch (check_state)
        {

        case REQUEST_LINE:
            if(!parseRequestLine(line))
            {
                // 调试信息
                // std::cout <<"parseRequestLine error, buf data: " << buf<<std::endl;
                check_state = FINISH;
                return BAD_REQUEST;
            }else{
                check_state = HEADER;
            }
            break;

        case HEADER:
            if(!parseRequestHead(line))
            {
                // 调试信息
                // std::cout <<"parseRequestLine error, buf data: " << buf<<std::endl;
                check_state = FINISH;
                return BAD_REQUEST;
            }
            // 判断是否有content
            if(buf.readAbleBytes() <= 2)
            {
                LOG_DEBUG("fd : %d, HTTP请求没有请求内容", fd);
                buf.delAll(); // 删除\r\n
                check_state = FINISH;
                return GET_REQUEST;
            }
            break;
        
        case CONTENT:
            if(!parseContent(buf))
            {
                check_state = FINISH;
                return BAD_REQUEST;
            }
            check_state = FINISH;
            return GET_REQUEST;

        default:
            break;
        }
    }
    return NO_REQUEST;
}

void http_request::_remove(const std::string& url)
{
    struct stat st;
    std::string file_path = res_dir + url;
    // 文件路径存在且不是目录
    if(stat(file_path.c_str(), &st) == 0 && !S_ISDIR(st.st_mode)){
        int ret = remove(file_path.c_str());
        if(ret == 0){
            LOG_INFO("fd: %d, 客户端请求删除文件: %s, 文件删除成功!", fd, url.c_str());
        }else{
            LOG_INFO("fd: %d, 客户端请求删除文件: %s, 文件删除失败!", fd, url.c_str());
        }
    }else{
        LOG_INFO("fd: %d, 文件路径 : %s存在问题删除失败",fd, url.c_str());
    }
}

bool http_request::parseRequestLine(const std::string& request)
{   
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
        std::string tmp = url;
        if(tmp.size() != 0){
            if(tmp[0] == '/'){
                begin = 1;
            }
            pos = tmp.find('/', begin);
            if(pos != -1){
                req_mode = tmp.substr(begin, pos - begin);
            }
        }

        // 是delete或者download  需要截取真正的url
        if(req_mode.size() != 0 && (req_mode == "delete" || req_mode == "download")){
            int len = req_mode.size() + begin;
            url.erase(0, len); // 删除mode
            if(req_mode == "delete"){
                _remove(url);
            }
        }
        LOG_DEBUG("fd: %d, method: %s, url: %s, version: %s", fd, method.c_str(), url.c_str(), version.c_str());
        if(req_mode.size() != 0){
            LOG_DEBUG("fd: %d, request mode: %s", fd, req_mode.c_str());
        }
    }
    else if(!ret)
    {
        // 解析出错
        LOG_ERROR("fd: %d, 解析出错请求行为: %s", fd, request.c_str());
    }
    return ret;
}


bool http_request::parseRequestHead(const std::string& line)
{
    // 正则表达式匹配每个()匹配一个字段, 第一个()匹配head字段键、第二个匹配head字段值
    std::regex pattern("^([^:]*): ?(.*)$");
    std::smatch sub_match;
    if(std::regex_match(line, sub_match, pattern))
    {
        // Content-Type: multipart/form-data; boundary=----分隔符 需要特殊处理
        if(sub_match[1] == "Content-Type"){
            std::string value = sub_match[2];
            int pos;
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
        // 调试信息
        // std::cout<<"key: "<<sub_match[1]<<" value: "<<sub_match[2]<<std::endl;
        return true;
    }
    //调试信息
    // std::cout<<"header parser error, line :"<<line<<std::endl;
    return false;
}


bool http_request::parseContent(buffer& buf)
{
    // 解析是post还是get
    // post需要解析表单字段, 将post的内容写入文件中
    std::string partition = "\r\n", line;
    int pos = -1;
    size_t sep_len = 0; // 分割字符串长度
    size_t file_len = 0; // 发送的文件长度
    if(method == "GET")
    {
        return true;
    }
    // 只支持解析上传文件的post请求multipart/form-data
    if(headers.count("Content-Type") && headers["Content-Type"] == "multipart/form-data"){
        LOG_DEBUG("fd: %d, 解析到POST请求, 进行请求内容的解析", fd);
        while(!buf.empty())
        {
            if((pos = buf.find(partition)) == -1){
                LOG_DEBUG("fd: %d, 未找到分隔符退出循环", fd);
                break;
            }
            line = buf.toString(pos - buf.getReadPos() + 2, true);
            line.erase(line.end() - 2, line.end());
            // 读到空行 进行文件内容的读取
            if(line.size() == 0){
                break;
            }
            // 读取到分隔符
            else if(line.substr(0, 2) == "--"){
                sep_len = line.size() + 2;
                continue;
            }
            // 解析表单字段
            else
            {
                // 找到文件名
                // Content-Disposition: form-data; name="upload"; filename="xxx"
                if((pos = line.find("filename")) == -1){
                    continue;
                }
                // 找到"xxx"的起始位置
                int begin = line.find('"', pos);
                if(begin == -1){
                    break;
                }
                int end = line.find('"', begin + 1);
                if(end == -1)
                {
                    break;
                }
                std::string filename = line.substr(begin + 1, end - begin - 1);
                LOG_DEBUG("fd: %d, 解析到 POST请求上传的文件名: %s", fd, filename.c_str());
                post_former["filename"] = filename;
            }
        }

        // 写入文件
        if(post_former.count("filename") && post_former["filename"].size() != 0){
            file_len = buf.readAbleBytes() - sep_len - 2; // 实际要写的长度 = 待写的字符长度 - (分隔符长度 + 2)
            // 上传的文件路径
            std::string file_path = res_dir + post_former["filename"];
            
            if(file_len == 0){
                LOG_DEBUG("fd: %d, 文件长度 = 0不进行保存, 文件名: %s", post_former["filename"].c_str());
                return true;
            }

            std::ofstream f(file_path, std::ios::out);
            // std::string to_write(buf.peek(), buf.peek() + file_len);
            // std::cout<<"待写入的字符 = "<<to_write<<std::endl;

            f.write(buf.peek(), file_len);
            buf.delAll(); // buf清空
            f.close(); // 文件写入
            LOG_DEBUG("fd: %d, 文件：%s上传成功! 文件长度为: %d", fd, file_path.c_str(), file_len);
            struct stat st;
            assert(stat(file_path.c_str(), &st) == 0);
        }
    }
    return true;
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