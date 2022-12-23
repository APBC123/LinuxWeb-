#include "sqlconnpool.h"

SqlConnPool::SqlConnPool()
{
    m_UseCount = 0;
    m_FreeCount = 0;
}

SqlConnPool::~SqlConnPool()
{
    ClosePool();
}

SqlConnPool *SqlConnPool::Instance()
{
    static SqlConnPool ConPool;
    return &ConPool;
}

void SqlConnPool::Init(const char *host, int port, const char *user, const char *password, const char *DataBaseName, int Connsize = 8)
{
    for (int i = 0; i < Connsize; i++)
    {
        MYSQL *sql = nullptr;
        sql = mysql_init(sql);
        if (!sql)
        {
            LOG_ERROR("MySql init error!");
            assert(sql);
        }
        sql = mysql_real_connect(sql, host, user, password, DataBaseName, port, nullptr, 0);
        if (!sql)
        {
            LOG_ERROR("MySql Connect error!");
        }
        m_ConnQue.push(sql);
    }
    m_MAX_CONN = Connsize;
    sem_init(&m_SemID, 0, m_MAX_CONN);
}

MYSQL *SqlConnPool::GetConn()
{
    MYSQL *sql = nullptr;
    if (m_ConnQue.empty())
    {
        LOG_WARN("SqlConnPool busy!");
        sem_wait(&m_SemID);
        {
            std::lock_guard<std::mutex> locker(m_mtx);
            sql = m_ConnQue.front();
            m_ConnQue.pop();
        }
        return sql;
    }
}

void SqlConnPool::ClosePool()
{
    std::lock_guard<std::mutex> locker(m_mtx);
    while (!m_ConnQue.empty())
    {
        auto item = m_ConnQue.front();
        m_ConnQue.pop();
        mysql_close(item);
    }
    mysql_library_end();
}

void SqlConnPool::FreeConn(MYSQL *sql)
{
    assert(sql);
    std::lock_guard<std::mutex> locker(m_mtx);
    m_ConnQue.push(sql);
    sem_post(&m_SemID);
}

int SqlConnPool::GetFreeConnCount()
{
    std::lock_guard<std::mutex> locker(m_mtx);
    return m_ConnQue.size();
}
