/*
http请求相关的枚举类
*/
#ifndef HTTP_ENUM
#define HTTP_ENUM

// 次级状态解析请求行，解析请求头，解析内容,解析完成
enum CHECK_STATE{
    REQUEST_LINE = 0, 
    HEADER, 
    CONTENT,
    FINISH
};

// http状态
enum HTTP_CODE{
    NO_REQUEST = 0, 
    GET_REQUEST, 
    BAD_REQUEST, 
    FORBIDDEN_REQUEST, 
    INTERNAL_ERROR, 
    CLOSED_CONNCTION
};


#endif