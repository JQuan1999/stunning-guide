#ifndef LOG
#define LOG

#include<cstdio>
#include<sys/stat.h>
#include<string>
#include<cstring>
#include<fstream>
#include<mutex>
#include<thread>
#include<time.h>
#include<sys/time.h>
#include<stdarg.h>
#include"block_queue.h"
#include "../buffer/buffer.h"

#define BUF_SIZE 128
#define PATH_SIZE 256

class Log
{
public:
    // 采用局部变量懒汉模式
    static Log* getInstance();

    static void flush_log_thread();

    // 可选择的参数有日志文件、日志记录级别、日志缓冲区大小、最大行数以及最长日志条队列 
    bool init(int level, const char* path = nullptr, const char* suffix = ".log", 
    int split_lines = 500000, int max_queue_size = 0);

    void write_log(int level, const char* format, ...);
    void flush();
    void close();
    bool is_open();
    int get_level();

private:
    Log();
    ~Log();
    
    void async_write_log();

private:
    block_queue<std::string>* m_bq; // 阻塞队列
    buffer* m_buf; // 写入数据的buf
    char* m_path;
    char* m_suffix;
    int m_level;
    int m_split_lines; // 日志最大行数
    int m_count; // 日志行数记录
    int m_today; // 按天分类
    std::ofstream fout; // 输出log文件的fstream对象
    bool m_is_async; // 是否同步标志位
    int is_closed;
    std::mutex m_mutex;
};

#define LOG_BASE(level, format, ...) \
    do {\
        Log* log = Log::getInstance();\
        if (log->is_open() && log->get_level() <= level) {\
            log->write_log(level, format, ##__VA_ARGS__); \
            log->flush();\
        }\
    } while(0);

#define LOG_DEBUG(format, ...) do {LOG_BASE(0, format, ##__VA_ARGS__)} while(0);
#define LOG_INFO(format, ...) do {LOG_BASE(1, format, ##__VA_ARGS__)} while(0);
#define LOG_WARN(format, ...) do {LOG_BASE(2, format, ##__VA_ARGS__)} while(0);
#define LOG_ERROR(format, ...) do {LOG_BASE(3, format, ##__VA_ARGS__)} while(0);


#endif