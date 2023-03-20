/*
http请求相关的枚举类
*/
#ifndef HTTP_ENUM
#define HTTP_ENUM

// 主解析流程
enum CHECK_STATE
{
    REQUEST_LINE = 0, 
    HEADER, 
    CONTENT,
    FINISH
};

// post内容解析过程
enum CHECK_POST_CONTENT
{
    POST_SEPE = 0,
    POST_FORMER,
    POST_CONTENT,
    POST_FINISH
};

// http解析状态
enum HTTP_CODE
{
    NO_REQUEST = 0, 
    GET_REQUEST, 
    BAD_REQUEST, 
    FORBIDDEN_REQUEST, 
    INTERNAL_ERROR, 
    CLOSED_CONNCTION
};

// post状态
enum POST_CODE
{
    NO_POST = 0,
    POST_CONTENT_CONTINUE,
    POST_FAILED,
    POST_FILE_EXISTED,
    POST_GET_CONTENT
};

#endif