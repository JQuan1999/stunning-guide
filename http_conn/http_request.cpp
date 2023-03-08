#include "http_request.h"

http_request::http_request()
{
    init();
}

http_request::~http_request()
{

}


void http_request::init()
{
    check_state = REQUEST_LINE;
    mode = method = url = version = body = "";
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
    std::string partition = "\r\n";
    while(!buf.empty())
    {
        int pos = buf.find(partition);
        // 请求不完整
        if(pos == -1)
        {
            break;
        }
        std::string line = buf.dealToString(pos - buf.getReadPos() + 2);
        line.erase(line.end() - 2, line.end()); // 删除\r\n
        // 空行 头部已解析完毕
        if(line.size() == 0 && check_state == HEADER)
        {
            check_state = CONTENT;
            continue;
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
                std::cout<<"no content \n";
                buf.delAll(); // 删除\r\n
                check_state = FINISH;
                return GET_REQUEST;
            }
            break;
        
        case CONTENT:
            if(!parseContent(line))
            {
                // 调试信息
                std::cout <<"parseRequestLine error, buf data: " << buf<<std::endl;
                check_state = FINISH;
                return BAD_REQUEST;
            }
            return GET_REQUEST;

        default:
            break;
        }
    }
    return NO_REQUEST;
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
    if(ret)
    {
        std::cout<<"method: "<<method <<", url: "<<url <<", version: "<<version<<"\n";
        // 判断是否进行了删除或下载 找到第一个/作为分隔符
        std::string tmp = url;
        
    }
    else
    {
        // 解析出错
        std::cout<<"parser error, request line: "<<request<<std::endl;
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
        headers[sub_match[1]] = sub_match[2];
        // 调试信息
        // std::cout<<"key: "<<sub_match[1]<<" value: "<<sub_match[2]<<std::endl;
        return true;
    }
    //调试信息
    // std::cout<<"header parser error, line :"<<line<<std::endl;
    return false;
}


bool http_request::parseContent(const std::string& line)
{
    body = line;
    // 待做解析post请求
    // 调试信息
    // std::cout<<"request body: "<<body<<std::endl;
    return true;
}


const std::string http_request::get_method()
{
    return method;
}

const std::string http_request::get_url()
{
    return url;
}

const std::string http_request::get_version()
{
    return version;
}


const std::string http_request::get_head(const std::string head)
{
    if(headers.count(head))
    {
        return headers[head];
    }
    return "";
}

const std::string http_request::get_body()
{
    return body;
}