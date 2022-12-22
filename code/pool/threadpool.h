#ifndef THREADPOOL_H
#define THREADPOOL_H
#include <mutex>
#include <condition_variable>
#include <thread>
#include <queue>
#include <assert.h>
#include <functional>

class ThreadPool
{
public:
    ThreadPool(size_t ThreadCount);
    ~ThreadPool();
    template <class T>
    void AddTask(T &task);

private:
    struct Pool
    {
        std::mutex m_mtx;
        std::condition_variable cond;
        bool m_IsClosed;
        std::queue<std::function<void()>> tasks;
    };
    std::shared_ptr<Pool> m_pool;
};

ThreadPool::ThreadPool(size_t ThreadCount = 8)
{
    m_pool = std::make_shared<Pool>();
    for (size_t i = 0; i < ThreadCount; i++)
    {
        std::thread([pool = m_pool]
                    {
            std::unique_lock<std::mutex> locker(pool->m_mtx);
            while(true)
            {
                if(!pool->tasks.empty())
                {
                    auto task = pool->tasks.front();
                    pool->tasks.pop();
                    locker.unlock();
                    task();
                    locker.lock();
                }
                else if(pool->m_IsClosed)
                {
                    break;
                }
                else
                {
                    pool->cond.wait(locker);
                }
            } })
            .detach();
    }
}

ThreadPool::~ThreadPool()
{
    if (m_pool)
    {
        {
            std::lock_guard<std::mutex> locker(m_pool->m_mtx);
            m_pool->m_IsClosed = true;
        }
        m_pool->cond.notify_all();
    }
}

template <class T>
void AddTask(T &task)
{
    {
        std::lock_guard<std::mutex> locker(m_pool->m_mtx);
        m_pool->tasks.emplace_back(task);
    }
    m_pool->cond.notify_one();
}
#endif
