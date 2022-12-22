#ifndef SQLCONNPOOL_H
#define SQLCONNPOOL_H
#include <mysql/mysql.h>
#include <string>
#include <queue>
#include <mutex>
#include <semaphore.h>
#include <thread>
#include "../log/log.h"

class SqlConnPool
{
public:
    SqlConnPool();
    ~SqlConnPool();
    static SqlConnPool *Instance();
    MYSQL *GetConn();
    void FreeConn(MYSQL *conn);
    int GetFreeConnCount();
    void Init(const char *host, int port, const char *user, const char *pwd, const char *dbname, int connsize);
    void ClosePool();

private:
    int MAX_CONN;
    int m_usecount;
    int m_freecount;
    std::queue<MYSQL *> m_connque;
    std::mutex m_mtx;
    sem_t m_semid;
};
#endif