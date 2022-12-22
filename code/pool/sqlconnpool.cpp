#include "sqlconnpool.h"
using namespace std;
SqlConnPool::SqlConnPool()
{
    m_usecount = 0;
    m_freecount = 0;
}

SqlConnPool::~SqlConnPool()
{
    ClosePool();
}

void SqlConnPool::ClosePool()
{
    lock_guard<mutex> locker(m_mtx);
    while (!m_connque.empty())
    {
        auto item = m_connque.front();
        m_connque.pop();
        mysql_close(item);
    }
    mysql_library_end();
}

int SqlConnPool::GetFreeConnCount()
{
    lock_guard<mutex> locker(m_mtx);
    return m_connque.size();
}

MYSQL *SqlConnPool::GetConn()
{
    MYSQL *sql = nullptr;
    if (m_connque.empty())
    {
        LOG_WARN("SqlConnPool busy!");
        return nullptr;
    }
    sem_wait(&m_semid);
    {
        lock_guard<mutex> locker(m_mtx);
        sql = m_connque.front();
        m_connque.pop();
    }
    return sql;
}

void SqlConnPool::Init(const char *host, int port, const char *user, const char *pwd, const char *dbname, int connsize = 10)
{
    assert(connsize > 0);
    for (int i = 0; i < connsize; i++)
    {
        MYSQL *sql = nullptr;
        sql = mysql_init(sql);
        if (!sql)
        {
            LOG_ERROR("MySql init error");
            assert(sql);
        }
        sql = mysql_real_connect(sql, host, user, pwd, dbname, port, nullptr, 0);
        if (!sql)
        {
            LOG_ERROR("MySql Connect error!");
        }
        m_connque.push(sql);
    }
    MAX_CONN = connsize;
    sem_init(&m_semid, 0, MAX_CONN);
}

void SqlConnPool::FreeConn(MYSQL *sql)
{
    assert(sql);
    lock_guard<mutex> locker(m_mtx);
    m_connque.push(sql);
    sem_post(&m_semid);
}

SqlConnPool *SqlConnPool::Instance()
{
    static SqlConnPool connpool;
    return &connpool;
}