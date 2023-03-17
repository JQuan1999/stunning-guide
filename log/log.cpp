#include"log.h"

Log::Log()
{
    m_count = 0;
    m_is_async = false;
    m_buf = nullptr;
    m_bq = nullptr;
    m_path = nullptr;
    m_suffix = nullptr;
}


Log::~Log()
{   
    close();
}

void Log::close()
{
    is_closed = 1;
    if(m_bq)
    {
        delete m_bq;
        m_bq = nullptr; 
    }
    if(m_buf){
        delete m_buf;
        m_buf = nullptr;
    }
    if(m_path){
        delete[] m_path;
        m_path = nullptr;
    }
    if(m_suffix){
        delete[] m_suffix;
        m_suffix = nullptr;
    }
    if(fout){fout.flush();fout.close();}
}

bool Log::is_open()
{
    return !is_closed;
}

int Log::get_level()
{
    return m_level;
}

/*
path：保存log日志文件的路径, suffix：文件后缀, split_lines：单个日志写入的最大行数, max_queue_size: 阻塞队列的容量
*/
bool Log::init(int level, const char* path, const char* suffix, int split_lines, int max_queue_size)
{
    // 如果设置了max_queue_size，则设置为异步
    m_level = level;

    m_path = new char[256];
    memset(m_path, 0, sizeof(m_path));
    if(path == nullptr){
        getcwd(m_path, sizeof(m_path));
    }else{
        strcat(m_path, path);
    }

    m_suffix = new char[10];
    memset(m_suffix, 0, sizeof(m_suffix));
    if(suffix == nullptr){
        suffix = ".log";
    }
    strcat(m_suffix, suffix);

    m_buf = new buffer();
    m_buf->delAll();

    if(max_queue_size >= 1)
    {
        m_is_async = true;
        m_bq = new block_queue<std::string>(max_queue_size);
        std::thread(flush_log_thread).detach(); // 写入日志文件线程
    }else{
        m_is_async = false;
    }

    is_closed = 0; // 开启
    m_split_lines = split_lines;
    m_count = 0; // 行计数

    time_t t = time(nullptr);
    struct tm *sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;
    m_today = my_tm.tm_mday;

    char log_full_name[256] = {0};
    snprintf(log_full_name, 256 - 1, "%s/%04d_%2d_%02d%s", m_path, my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, m_suffix); // 文件名
    fout = std::ofstream(log_full_name, std::ofstream::app);
    if(!fout.is_open())
    {
        mkdir(m_path, 0777);
        fout = std::ofstream(log_full_name, std::ofstream::app);
    }
    assert(fout.is_open());
    return true;
}

Log* Log::getInstance()
{
    static Log obj;
    return &obj;
}


void Log::flush_log_thread()
{
    Log::getInstance()->async_write_log();
}


void Log::async_write_log()
{
    std::string single_log;
    // 从阻塞队列中取出一个日志string，写入文件
    while(m_bq->pop(single_log) && !is_closed)
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        // cout<<"single_log = "<<single_log<<endl;
        fout<<single_log;
    }
}

void Log::write_log(int level, const char* format, ...)
{
    struct timeval now = {0, 0};
    gettimeofday(&now, nullptr);
    time_t t = now.tv_sec;
    struct tm* sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;
    char s[16] = {0};
    switch (level)
    {
    case 0:
        strcpy(s, "[debug]:");
        break;
    case 1:
        strcpy(s, "[info]:");
        break;
    case 2:
        strcpy(s, "[warn]:");
        break;
    case 3:
        strcpy(s, "[erro]:");
        break;
    default:
        strcpy(s, "[info]:");
        break;
    }

    // 每天写入新的日志文件或达到最大行数后新写一个日志文件
    if (m_today != my_tm.tm_mday || (m_count && m_count % m_split_lines == 0)) //everyday log
    {
        m_mutex.lock();
        char new_log[256] = {0};
        fout.flush();
        fout.close();

        char tail[36] = {0};
        snprintf(tail, 36, "%04d_%02d_%02d", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday);
        
        if (m_today != my_tm.tm_mday)
        {
            snprintf(new_log, 255, "%s/%s%s", m_path, tail, m_suffix);
            m_today = my_tm.tm_mday;
            m_count = 0;
        }
        else
        {
            snprintf(new_log, 255, "%s/%s-%d%s", m_path, tail, m_count / m_split_lines, m_suffix);
        }

        fout = std::ofstream(new_log, std::ofstream::app);
        m_mutex.unlock();
    }

    m_mutex.lock();
    m_count++;
    //写入的具体时间内容格式
    int n = snprintf(m_buf->beginWrite(), 48, "%d-%02d-%02d %02d:%02d:%02d.%06ld %s ",
                     my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday,
                     my_tm.tm_hour, my_tm.tm_min, my_tm.tm_sec, now.tv_usec, s);
    m_buf->hasWrite(n);

    va_list valst;
    va_start(valst, format);
    int m = vsnprintf(m_buf->beginWrite(), m_buf->writeAbleBytes(), format, valst);
    va_end(valst);
    m_buf->hasWrite(m);

    m_buf->append("\n", 1);

    // 异步写
    if (m_is_async && m_bq && !m_bq->full())
    {
        m_bq->push(std::string(m_buf->peek(), m_buf->end()));
        m_buf->delAll();
    }
    // 同步写
    else
    {
        fout<<m_buf;
    }
    m_mutex.unlock();
}

//强制刷新写入流缓冲区
void Log::flush()
{
    m_mutex.lock();
    fout.flush();
    m_mutex.unlock();
}