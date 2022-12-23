#ifndef SQLCONNPOOL_H
#define SQLCONNPOOL_H

#include <queue>
#include <mutex>
#include <thread>
#include <string>
#include <semaphore.h>
#include <mysql/mysql.h>
#include "../log/log.h"

class SqlConnPool
{
public:
    SqlConnPool();
    ~SqlConnPool();
    void Init(const char *host, int port, const char *user, const char *password, const char *DataBaseName, int ConnSize);
    static SqlConnPool *Instance(); // // 创建一个static对象(单例),用于对象间通信
    MYSQL *GetConn();
    void FreeConn(MYSQL *sql);
    int GetFreeConnCount();
    void ClosePool();

private:
    int m_MAX_CONN;
    int m_UseCount;
    int m_FreeCount;
    std::queue<MYSQL *> m_ConnQue;
    std::mutex m_mtx;
    sem_t m_SemID;
};
#endif
