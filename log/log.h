#ifndef LOG_H
#define LOG_H

#include <assert.h>
#include <string>
#include <mutex>
#include <thread>
#include <string.h>
#include <sys/time.h>
#include <stdarg.h>
#include <sys/stat.h>
#include "blockqueue.h"
#include "../buffer/buffer.h"

class Log
{
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
};

#define LOG_BASE(level, format, ...)                   \
    do                                                 \
    {                                                  \
        Log *log = Log::instance();                    \
        if (log->isopen() && log->getlevel() <= level) \
        {                                              \
            log->write(level, format, ##__VA_ARGS__);  \
            log->flush();                              \
        }                                              \
    } while (0);

#define LOG_DEBUG(format, ...)             \
    do                                     \
    {                                      \
        LOG_BASE(0, format, ##__VA_ARGS__) \
    } while (0);
#define LOG_INFO(format, ...)              \
    do                                     \
    {                                      \
        LOG_BASE(1, format, ##__VA_ARGS__) \
    } while (0);
#define LOG_WARN(format, ...)              \
    do                                     \
    {                                      \
        LOG_BASE(2, format, ##__VA_ARGS__) \
    } while (0);
#define LOG_ERROR(format, ...)             \
    do                                     \
    {                                      \
        LOG_BASE(3, format, ##__VA_ARGS__) \
    } while (0);

#endif