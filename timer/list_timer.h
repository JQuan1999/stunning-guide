#ifndef LIST_TIMER
#define LIST_TIMER

#include<time.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<iostream>
#include<cstring>

#define BUFFER_SIZE 64

class util_timer;

// 用户数据结构
class client_data{
public:
    // client_data(int fd = 0, char* s = nullptr) :sockfd(fd){
    //     memset(buf, '\0', sizeof(buf));
    //     strcpy(buf, s);
    // }
    struct sockaddr_in address;
    int sockfd;
    char buf[BUFFER_SIZE];
    util_timer* timer;
};

class util_timer{
public:
    util_timer(time_t ex, util_timer* p = nullptr, util_timer* nxt = nullptr): expire(ex), prev(nullptr), next(nullptr) {}
public:
    time_t expire; // 绝对超时时间
    void (*cb_func)(client_data* ); // 回调函数
    client_data* user_data;
    util_timer* prev;
    util_timer* next;
};

class sort_list_timer{
public:
    sort_list_timer();
    ~sort_list_timer();
    void add_timer(util_timer* timer); // 将目标定时器加入到链表中
    void adjust_timer(util_timer* timer); // 目标定时器超时时间延长
    void del_timer(util_timer* timer); // 删除目标定时器
    void tick(); // SIGALARM触发tick函数，处理链表上的到期任务
    util_timer* begin();
    util_timer* end();
private:
    void insert(util_timer* tmp, util_timer* timer); // 将timer插入begin及以后的位置
    util_timer* head; // 头指针
    util_timer* tail; // 尾指针
};
#endif