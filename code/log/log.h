#ifndef LOG_H
#define LOG_H

#include <mutex>
#include <string>
#include <thread>
#include <sys/time.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <sys/stat.h> //mkdir
#include <vector>
#include "blockqueue.h"
#include "../buffer/buffer.h"

class Log
{
public:
    Log();
    ~Log();
    void Init(int level, const char *path = "./log", const char *suffix = ".log", int MaxQueueCapacity = 1024);
    int GetLevel();
    void SetLevel(int level);
    bool IsOpen();
    void Flush();
    void Write(int level, const char *format, ...);
    void AsyncWrite();
    static Log *Instance(); // 单例
    static void FlushLogThread();

private:
    void m_AppendLogLevelTitle(int level);
    virtual ~Log();
    static const int LOG_PATH_LEN = 256;
    static const int LOG_NAME_LEN = 256;
    static const int MAX_LINES = 60000;
    const char *m_path;
    const char *m_suffix;
    int m_LineCount;
    int m_Today;
    bool m_IsOpen;
    int m_level;
    FILE *m_fp;
    Buffer m_buffer;
    std::unique_ptr<BlockDeque<std::string>> m_deque;
    std::unique_ptr<std::thread> m_WriteThread;
    std::mutex m_mtx;
};
/*对外输出接口*/
void LOG_INFO(const char *format, ...)
{
    Log *log = Log::Instance();
    if (log->IsOpen() && log->GetLevel() <= 0)
    {
        log->Write(0, format);
        log->Flush();
    }
}
void LOG_DEBUG(const char *format, ...)
{
    Log *log = Log::Instance();
    if (log->IsOpen() && log->GetLevel() <= 1)
    {
        log->Write(1, format);
        log->Flush();
    }
}
void LOG_WARN(const char *format, ...)
{
    Log *log = Log::Instance();
    if (log->IsOpen() && log->GetLevel() <= 2)
    {
        log->Write(2, format);
        log->Flush();
    }
}
void LOG_ERROR(const char *format, ...)
{
    Log *log = Log::Instance();
    if (log->IsOpen() && log->GetLevel() <= 3)
    {
        log->Write(3, format);
        log->Flush();
    }
}
#endif
