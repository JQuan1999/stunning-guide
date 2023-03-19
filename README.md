# c++实现的支持文件上传和下载的Web服务器

## 运行
```
make
./main
```

## 主要技术
* 采用EPOLL+ET模式接受客户端连接请求，线程池处理分别处理客户端的读和写请求
* 状态机解析http请求，支持GET和POST请求，其中post请求只支持解析Content-Type为multipart/form-data的请求
* vector封装char实现自动增长的缓冲区
* 同步/异步日志系统记录服务器允许状态


## 待完成的工作
* 上传大文件时的bug
* 中文路径报错的bug
* 实现注册登录每个用户操作自己的文件夹


## 参考
* 《游双高性能服务器编程》
* [tinyWebserver](https://github.com/qinguoyi/TinyWebServer)
* [Webserver](https://github.com/qinguoyi/TinyWebServer)
* [WebFileServer](https://github.com/shangguanyongshi/WebFileServer)