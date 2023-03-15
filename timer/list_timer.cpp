#include"list_timer.h"

sort_list_timer::sort_list_timer(){
    head = new util_timer(0);
    tail = new util_timer(0);
    head->next = tail;
    tail->prev = head;
}

sort_list_timer::~sort_list_timer(){
    util_timer* tmp = head;
    while(tmp){
        head = head->next;
        // delete tmp->user_data;
        delete tmp;
        tmp = head;
    }
    std::cout<<"the list is destructed"<<std::endl;
}

// 加入定时器timer
void sort_list_timer::add_timer(util_timer* timer){
    if(timer == nullptr) return;
    util_timer* tmp = head->next;
    if(tmp == tail){
        head->next = timer;
        timer->prev = head;
        timer->next = tail;
        tail->prev = timer;
    }else{
        insert(tmp, timer);
    }
}

// 目标定时器超时时间延长
void sort_list_timer::adjust_timer(util_timer* timer){
    if(timer == nullptr) return;
    util_timer* tmp = timer->next;
    // 只有一个节点或者后继节点的超时时间大于timer，不用改变位置
    if(tmp == tail || timer->expire < tmp->expire){
        return;
    }
    // 取出该节点插入tmp以后
    timer->prev->next = tmp;
    tmp->prev = timer->prev;
    insert(tmp, timer);
}

// 删除timer
void sort_list_timer::del_timer(util_timer* timer){
    if(timer == head || timer == tail || !timer){
        return;
    }
    timer->prev->next = timer->next;
    timer->next->prev = timer->prev;
    delete timer;
}

void sort_list_timer::tick(){
    util_timer* tmp = head->next;
    std::cout<<"timer tick\n";
    time_t cur = time(nullptr); // 获取当前时间
    while(tmp != tail){
        if(cur < tmp->expire){
            break;
        }
        // 调用计时器的回调函数
        tmp->cb_func(tmp->user_data);
        // 删除该节点
        util_timer* next = tmp->next;
        tmp->prev->next = next;
        next->prev = tmp->prev;
        delete tmp;
        tmp = next;
    }
}

void sort_list_timer::insert(util_timer* tmp, util_timer* timer){
    while(tmp != tail && tmp->expire <= timer->expire){
        tmp = tmp->next;
    }
    util_timer* p = tmp->prev;
    timer->next = tmp;
    tmp->prev = timer;
    p->next = timer;
    timer->prev = p;
}


util_timer* sort_list_timer::begin(){
    return head;
}

util_timer* sort_list_timer::end(){
    return tail;
}