#ifndef BLOCK_QUEUE
#define BLOCK_QUEUE

#include<queue>
#include<mutex>
#include<condition_variable>
#include<assert.h>
#include<random>
#include<iostream>
#include<atomic>
#include<thread>

using std::cout;
using std::endl;

template<typename T>
class block_queue
{
public:
    block_queue(int size = 1000)
    {
        assert(size > 0);
        capacity = size;
        _stopped = false;
    }

    ~block_queue() {
        stop();
    }
    
    void stop()
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            _stopped = true;
        }
        // 唤醒阻塞的线程
        cv_full.notify_all();
        cv_empty.notify_all();
        // std::cout<<"["<<std::this_thread::get_id()<<"]"<<std::endl;
    }

    bool available()
    {
        return !stopped() || !empty();
    }
    
    bool empty()
    {
        return m_queue.empty();
    }

    bool full()
    {
        return m_queue.size() == capacity;
    }

    // 压入数据
    bool push(T& value)
    {
        append(value);
        return true;
    }

    // 压入数据
    bool push(T&& value)
    {
        append(std::forward<T>(value));
        return true;
    }

    // 弹出数据
    bool pop(T& ret)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        cv_empty.wait(lock, [this](){return _stopped || !this->empty();});
        if(_stopped){
            return true;
        }
        bool is_full = full();
        ret = m_queue.front();
        cout<<"thread id: "<<std::this_thread::get_id()<<" has consumer a log\n";
        m_queue.pop();
        lock.unlock();

        if(is_full){
            cout<<"thread id: "<<std::this_thread::get_id()<<" notify one product thread\n";
            cv_full.notify_one();
        }
        return true;
    }

private:

    void append(T&& value){
        std::unique_lock<std::mutex> lock(m_mutex);
        cv_full.wait(lock, [this](){return _stopped || !this->full();});
        if(_stopped){
            return; // 如果主线程已停止直接返回
        }
        cout<<"thread id: "<<std::this_thread::get_id()<<" has product a log\n";
        bool is_empty = empty();
        m_queue.push(std::forward<T>(value));
        lock.unlock();
        if(is_empty){
            cout<<"thread id: "<<std::this_thread::get_id()<<" notify one consumer thread\n";
            cv_empty.notify_one(); // 如果插入元素前为空唤醒写入日志线程
        }
    }

    bool stopped()
    {
        return _stopped;
    }

private:
    bool _stopped;
    int capacity;
    T front;
    T back;
    std::queue<T> m_queue;
    std::mutex m_mutex;
    std::condition_variable cv_full, cv_empty;
};

#endif