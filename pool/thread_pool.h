#ifndef THREAD_POOL
#define THREAD_POOL

#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <functional>

class thread_pool
{
public:
    thread_pool(size_t threadCount = 8)
    {   
        m_stop = false;
        for(size_t i = 0; i < threadCount; i++)
        {
            std::thread(worker, this).detach();
        }
    }

    // thread_pool(thread_pool& pool) = delete;
    // thread_pool& operator=(thread_pool& pool) = delete;

    ~thread_pool()
    {
        m_stop = true;
        m_cv.notify_all();
    }

    static void worker(void *arg)
    {
        thread_pool* pool = (thread_pool*)arg;
        pool->run();
    }

    
    bool append(std::function<void(void)> request)
    {
        {
            std::lock_guard<std::mutex> lock(m_metex);
            m_queue.push(request);
        }
        m_cv.notify_one();
        return true;
    }

    // 从队列中取出一个http请求进行处理
    void run()
    {
        std::unique_lock<std::mutex> lock(m_metex);
        while(!m_stop)
        {
            if(!m_queue.empty())
            {
                auto request = m_queue.front();
                m_queue.pop();
                lock.unlock();
                // request的处理不需要加锁
                // request->process();
                // lock.lock();
            }
            else{
                m_cv.wait(lock);
            }
        }
    }

private:
    bool m_stop;
    std::mutex m_metex;
    std::condition_variable m_cv;
    std::queue<std::function<void(void)>> m_queue;
};

#endif