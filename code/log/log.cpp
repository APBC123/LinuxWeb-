#include "log.h"

Log::Log()
{
    m_LineCount = 0;
    m_WriteThread = nullptr;
    m_deque = nullptr;
    m_Today = 0;
    m_fp = nullptr;
}

Log::~Log()
{
    if (m_WriteThread && m_WriteThread->joinable())
    {
        while (!m_deque->Empty())
        {
            m_deque->Flush();
        }
        m_deque->Close();
        m_WriteThread->join();
    }
    if (m_fp)
    {
        std::lock_guard<std::mutex> locker(m_mtx);
        Flush();
        fclose(m_fp);
    }
}

int Log::GetLevel()
{
    std::lock_guard<std::mutex> locker(m_mtx);
    return m_level;
}

void Log::SetLevel(int level)
{
    std::lock_guard<std::mutex> locker(m_mtx);
    m_level = level;
}

void Log::m_AppendLogLevelTitle(int level)
{
    switch (level)
    {
    case 0:
        m_buffer.Append("[debug]:");
        break;
    case 1:
        m_buffer.Append("[info]:");
        break;
    case 2:
        m_buffer.Append("[warn]:");
        break;
    case 3:
        m_buffer.Append("[error]:");
        break;
    default:
        m_buffer.Append("[info]:");
        break;
    }
}

void Log::Flush()
{
    m_deque->Flush();
    fflush(m_fp);
}

void Log::AsyncWrite()
{
    std::string str = "";
    while (m_deque->Pop(str))
    {
        std::lock_guard<std::mutex> locker(m_mtx);
        fputs(str.c_str(), m_fp);
    }
}

bool Log::IsOpen()
{
    return m_IsOpen;
}

Log *Log::Instance()
{
    static Log inst;
    return &inst;
}
void Log::FlushLogThread()
{
    Log::Instance()->AsyncWrite();
}

void Log::Init(int level = 1, const char *path, const char *suffix, int MaxQueueSize)
{
    m_IsOpen = true;
    m_level = level;
    assert(MaxQueueSize > 0);
    if (!m_deque)
    {
        std::unique_ptr<BlockDeque<std::string>> NewDeque(new BlockDeque<std::string>);
        m_deque = std::move(NewDeque);
        std::unique_ptr<std::thread> NewThread(new std::thread(FlushLogThread));
        m_WriteThread = std::move(NewThread);
    }
    m_LineCount = 0;
    time_t timer = time(nullptr);
    struct tm *SysTime = localtime(&timer);
    struct tm t = *SysTime;
    m_path = path;
    m_suffix = suffix;
    char FileName[LOG_NAME_LEN] = {0};
    snprintf(FileName, LOG_NAME_LEN - 1, "%s/%04d_%02d_%02d%s", m_path, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, m_suffix);
    m_Today = t.tm_mday;
    {
        std::lock_guard<std::mutex> locker(m_mtx);
        m_buffer.RetrieveAll();
        if (m_fp)
        {
            Flush();
            fclose(m_fp);
        }
        m_fp = fopen(FileName, "a"); // FileName文件有可能不存在
        if (m_fp == nullptr)
        {
            mkdir(m_path, 0777); // 第二个参数是目录的模式，如果是0777，表示文件所有者、文件所有者所在的组的*用户、*所有用户，都有权限进行读、写、执行的操作。
            m_fp = fopen(FileName, "a");
        }
        assert(m_fp != nullptr);
    }
}

void Log::Write(int level, const char *format, ...)
{
    struct timeval now = {0, 0};
    gettimeofday(&now, nullptr);
    time_t TSec = now.tv_sec;
    struct tm *SysTime = localtime(&TSec);
    struct tm t = *SysTime;
    va_list VaList;
    /*写入日志日期 日志行数*/
    if (m_Today != t.tm_mday || (m_LineCount && (m_LineCount == MAX_LINES)))
    {
        std::unique_lock<std::mutex> locker(m_mtx);
        locker.unlock();
        char NewFile[LOG_NAME_LEN];
        char tail[36] = {0};
        snprintf(tail, 36, "%04d_%02d_%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);
        if (m_Today != t.tm_mday)
        {
            snprintf(NewFile, LOG_NAME_LEN - 72, "%s/%s%s", m_path, tail, m_suffix);
            m_Today = t.tm_mday;
            m_LineCount = 0;
        }
        else
        {
            snprintf(NewFile, LOG_NAME_LEN - 72, "%s/%s-%d%s", m_path, tail, (m_LineCount / MAX_LINES), m_suffix);
        }

        locker.lock();
        Flush();
        fclose(m_fp);
        m_fp = fopen(NewFile, "a");
        assert(m_fp != nullptr);
    }

    {
        std::unique_lock<std::mutex> locker(m_mtx);
        m_LineCount++;
        int n = snprintf(m_buffer.BeginWrite(), 128, "%d-%02d-%02d %02d:%02d:%02d.%06ld ",
                         t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                         t.tm_hour, t.tm_min, t.tm_sec, now.tv_usec);
        m_buffer.HasWritten(n);
        m_AppendLogLevelTitle(level);
        va_start(VaList, format);
        int m = vsnprintf(m_buffer.BeginWrite(), m_buffer.WritableBytes(), format, VaList);
        va_end(VaList);
        m_buffer.HasWritten(m);
        m_buffer.Append("\n\0", 2);
        if (m_deque && !m_deque->Full())
        {
            m_deque->Push_Back(m_buffer.RetrieveAllToStr());
        }
        else
        {
            fputs(m_buffer.Peek(), m_fp);
        }
        m_buffer.RetrieveAll();
    }
}
