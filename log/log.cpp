#include "log.h"

void Log::init(int level = 1, const char *path, const char *suffix, int maxqueuecapacity)
{
    m_isopen = true;
    m_level = level;
    if (maxqueuecapacity > 0)
    {
        m_isasync = true;
        if (!m_deque)
        {
            std::unique_ptr<BlockDeque<std::string>> newdeque(new BlockDeque<std::string>);
            m_deque = move(newdeque);
            std::unique_ptr<std::thread> newthread(new std::thread(flushlogthread));
            m_writethread = move(newthread);
        }
    }
    else
    {
        m_isasync = false; //阻塞队列无容量，无法进行异步写日志
    }

    m_linecount = 0;
    time_t timer = time(nullptr);
    struct tm *systime = localtime(&timer);
    struct tm t = *systime;
    m_path = path;
    m_suffix = suffix;
    char filename[LOG_NAME_LEN] = {0};
    snprintf(filename, LOG_NAME_LEN - 1, "%s/%04d_%02d_%02d%s", m_path, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, m_suffix);
    m_today = t.tm_mday;
    {
        std::lock_guard<std::mutex> locker(m_mtx);
        m_buff.retrieveall();
        if (m_fp)
        {
            flush();
            fclose(m_fp);
        }
        m_fp = fopen(filename, "a");
        if (m_fp == nullptr)
        {
            mkdir(m_path, 0777); //创建新目录，0777(8进制)表示root权限
            m_fp = fopen(filename, "a");
        }
        assert(m_fp != nullptr);
    }
}

Log *Log::instance()
{
    static Log inst;
    return &inst;
}

void Log::flushlogthread()
{
    Log::instance()->asyncwrite();
}

void Log::write(int level, const char *format, ...)
{
    struct timeval now = {0, 0};
    gettimeofday(&now, nullptr);
    time_t tsec = now.tv_sec;
    struct tm *systime = localtime(&tsec);
    struct tm t = *systime;
    va_list valist;
    if (m_today != t.tm_mday || (m_linecount && (m_linecount % MAX_LINES == 0)))
    {
        std::unique_lock<std::mutex> locker(m_mtx);
        locker.unlock();
        char newfile[LOG_NAME_LEN];
        char tail[36] = {0};
        snprintf(tail, 36, "%04d_%02d_%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);
        if (m_today != t.tm_mday)
        {
            snprintf(newfile, LOG_NAME_LEN - 72, "%s/%s%s", m_path, tail, m_suffix);
            m_today = t.tm_mday;
            m_linecount = 0;
        }
        else
        {
            snprintf(newfile, LOG_NAME_LEN - 72, "%s/%s-%d%s", m_path, tail, (m_linecount / MAX_LINES), m_suffix);
        }
        locker.lock();
        flush();
        fclose(m_fp);
        m_fp = fopen(newfile, "a");
        assert(m_fp != nullptr);
    }
    {
        std::unique_lock<std::mutex> locker(m_mtx);
        m_linecount++;
        int n = snprintf(m_buff.beginwrite(), 128, "%d-%02d-%02d %02d:%02d:%02d.%06ld ", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, now.tv_usec);
        m_buffer.hashwritten(n);
        AppendLogLevelTitle(level);
        va_start(valist, format);
        int m = vsprintf(m_buff.beginwrite(), m_buff.writablebytes(), format, valist);
        va_end(valist);
        m_buff.hashwritten(m);
        m_buff.append("\n\0", 2);
        if (m_isasync && m_deque->full())
        {
            m_deque->push_back(m_buff.retrievealltostr());
        }
        else
        {
            fputs(m_buff.peek(), m_fp);
        }
        m_buff.retrieveall();
    }
}

void Log::flush()
{
    if (m_isasync)
    {
        m_deque->flush();
    }
    fflush(m_fp); //将m_fp的输出缓冲区数据写入文件中(实现随写随存)
}

int Log::getlevel()
{
    std::lock_guard<std::mutex> locker(m_mtx);
    return m_level;
}

void Log::setlevel(int level)
{
    std::lock_guard<std::mutex> locker(m_mtx);
    m_level = level;
}

bool Log::isopen()
{
    return m_isopen;
}

Log::Log()
{
    m_linecount = 0;
    m_today = 0;
    m_isasync = false;
    m_writethread = nullptr;
    m_deque = nullptr;
    m_fp = nullptr;
}

Log::~Log()
{
    if (m_writethread && m_writethread->joinable())
    {
        while (!m_deque->empty())
        {
            m_deque->flush();
        }
        m_deque->close();
        m_writethread->join();
    }
    if (m_fp)
    {
        std::lock_guard<std::mutex> locker(m_mtx);
        flush();
        fclose(m_fp);
    }
}

void Log::AppendLogLevelTitle(int level)
{
    switch (level)
    {
    case 0:
        m_buff.append("[debug]: ", 9);
        break;
    case 1:
        m_buff.append("[info] : ", 9);
        break;
    case 2:
        m_buff.append("[warn] : ", 9);
        break;
    case 3:
        m_buff.append("[error]: ", 9);
        break;
    default:
        m_buff.append("[info] : ", 9);
        break;
    }
}

void Log::asyncwrite()
{
    std::string str = "";
    while (m_deque->pop(str))
    {
        std::lock_guard<std::mutex> locker(m_mtx);
        fputs(str.c_str(), m_fp);
    }
}
/*
public:
    void init(int level, const char *path = "./log", const char *suffix = ".log", int maxqueuecapacity = 1024);
    static Log *instance();
    static void flushlogthread();
    void write(int level, const char *format, ...);
    void flush();
    int getlevel();
    void setlevel(int level);
    bool isopen();

private:
    Log();
    void AppendLogLevelTitle(int level);
    virtual ~Log();
    void asyncwrite();
    static const int LOG_PATH_LEN = 256;
    static const int LOG_NAME_LEN = 256;
    static const int MAX_LINES = 50000;
    const char *m_path;
    const char *m_suffix;
    int m_MAX_LINES;
    int m_linecount;
    int m_today; //日志日期
    int m_level;
    bool m_isopen;
    bool m_isasync; //异步
    Buffer m_buffer;
    FILE *m_fp;
    std::unique_ptr<BlockDeque<std::string>> m_deque;
    std::unique_ptr<std::thread> m_writethread;
    std::mutex m_mtx;
*/
