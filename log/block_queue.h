#ifndef BLOCKQUEUE_H
#define BLOCKQUEUE_H

#include <mutex>
#include <deque>
#include <condition_variable>
#include <sys/time.h>

template <class T>
class BlockDeque
{
public:
    explicit BlockDeque(size_t MaxCapacity = 1000);
    ~BlockDeque();
    void clear();
    bool empty();
    bool full();
    void close();
    size_t size();
    size_t capacity();
    T front();
    T back();
    void push_back(const T &item);
    void push_front(const T &item);
    bool pop(T &item);
    bool pop(T &item, int timeout);
    void flush();

private:
    std::deque<T> m_dep;
    size_t m_capacity;
    std::mutex m_mtx;
    bool m_isclose;
    std::condition_variable m_condconsumer;
    std::condition_variable m_condproducer;
};

template <class T>
BlockDeque<T>::BlockDeque(size_t MaxCapacity)
{
    m_capacity = MaxCapacity;
    assert(MaxCapacity > 0);
    m_isclose = false;
}

template <class T>
BlockDeque<T>::~BlockDeque()
{
    close();
}

template <class T>
void BlockDeque<T>::clear()
{
    std::lock_guard<std::mutex> locker(m_mtx);
    m_dep.clear();
}

template <class T>
bool BlockDeque<T>::empty()
{
    std::lock_guard<std::mutex> locker(m_mtx);
    return m_dep.empty();
}

template <class T>
bool BlockDeque<T>::full()
{
    std::lock_guard<std::mutex> locker(m_mtx);
    if (m_deq.size() >= m_capacity)
        return true;
    return false;
}

template <class T>
void BlockDeque<T>::close()
{
    {
        std::lock_guard<std::mutex> locker(m_mtx);
        m_dep.clear();
        m_isclose = true;
    }
    m_condconsumer.notify_all(); //通知属于所有该实例的线程
    m_condproducer.notify_all();
}

template <class T>
size_t BlockDeque<T>::size()
{
    std::lock_guard<std::mutex> locker(m_mtx);
    return m_dep.size();
}

template <class T>
size_t BlockDeque<T>::capacity()
{
    std::lock_guard<std::mutex> locker(m_mtx);
    return m_capacity;
}

template <class T>
T BlockDeque<T>::front()
{
    std::lock_guard<std::mutex> locker(m_mtx);
    return m_dep.front();
}

template <class T>
T BlockDeque<T>::back()
{
    std::lock_guard<std::mutex> locker(m_mtx);
    return m_dep.back();
}

template <class T>
void BlockDeque<T>::push_back(const T &item)
{
    std::unique_lock<std::mutex> locker(m_mtx);
    while (m_dep.size() >= m_capacity)
    {
        m_condproducer.wait(locker); //阻塞队列已满，此线程进入等待状态
    }
    m_deq.push_back(item);
    m_condconsumer.notify_one();
}

template <class T>
void BlockDeque<T>::push_front(const T &item)
{
    std::unique_lock<std::mutex> locker(m_mtx);
    while (m_dep.size() >= m_capacity)
    {
        m_condproducer.wait(locker); //阻塞队列已满，此线程进入等待状态
    }
    m_deq.push_front(item);
    m_condconsumer.notify_one();
}

template <class T>
bool BlockDeque<T>::pop(T &item)
{
    std::unique_lock<std::mutex> locker(m_mtx);
    while (m_dep.empty())
    {
        m_condconsumer.wait(locker);
        if (m_isclose)
            return false;
    }
    item = m_dep.front();
    m_dep.pop_front();
    m_condproducer.notify_one();
    return true;
}

template <class T>
bool BlockDeque<T>::pop(T &item, int timeout)
{
    std::unique_lock<std::mutex> locker(m_mtx);
    while (m_dep.empty())
    {
        if (m_condconsumer.wait_for(locker, std::chrono::seconds(timeout)) == std::cv_status::timeout)
        {
            return false;
        }
        if (m_isclose)
        {
            return false;
        }
    }
    item = m_deq.front();
    m_dep.pop();
    m_condproducer.notify_one();
    return true;
}

template <class T>
void BlockDeque<T>::flush() //与pop(T &item)配合使用的，避免线程过长时间处于阻塞状态
{
    m_condconsumer.notify_one();
}
#endif
