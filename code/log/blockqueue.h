#ifndef BLOCKQUEUE_H
#define BLOCKQUEUE_H

#include <mutex>
#include <condition_variable>
#include <sys/time.h>
#include <deque>
#include <assert.h>

template <class T>
class BlockDeque
{
public:
    BlockDeque(size_t MaxCapacity = 800);
    ~BlockDeque();
    bool Empty();
    bool Full();
    void Clear();
    void Close();
    void Push_Back(const T &item);
    void Push_Front(const T &item);
    bool Pop(T &item);
    void Flush();
    size_t Size();
    size_t Capacity();
    T Front();
    T Back();

private:
    std::deque<T> m_deque;
    std::mutex m_mtx;
    bool m_isclose;
    size_t m_capacity;
    std::condition_variable m_CondConsumer;
    std::condition_variable m_CondProducer;
};

template <class T>
BlockDeque<T>::BlockDeque(size_t MaxCapacity)
{
    assert(MaxCapacity > 0);
    m_capacity = MaxCapacity;
    m_isclose = false;
}

template <class T>
BlockDeque<T>::~BlockDeque()
{
    Close();
}

template <class T>
bool BlockDeque<T>::Empty()
{
    std::lock_guard<std::mutex> locker(m_mtx);
    return m_deque.empty();
}

template <class T>
bool BlockDeque<T>::Full()
{
    std::lock_guard<std::mutex> locker(m_mtx);
    return m_deque.size() >= m_capacity;
}

template <class T>
void BlockDeque<T>::Clear()
{
    std::lock_guard<std::mutex> locker(m_mtx);
    m_deque.clear();
}

template <class T>
void BlockDeque<T>::Close()
{
    {
        std::lock_guard<std::mutex> locker(m_mtx);
        m_deque.clear();
        m_isclose = true;
    }
    m_CondConsumer.notify_all();
    m_CondProducer.notify_all();
}

template <class T>
void BlockDeque<T>::Push_Back(const T &item)
{
    std::lock_guard<std::mutex> locker(m_mtx);
    whlie(m_deque.size() >= m_capacity)
    {
        m_CondProducer.wait(locker);
    }
    m_deque.push_back(item);
    m_CondConsumer.notify_one();
}

template <class T>
void BlockDeque<T>::Push_Front(const T &item)
{
    std::lock_guard<std::mutex> locker(m_mtx);
    while (m_deque.size() >= m_capacity)
    {
        m_CondProducer.wait();
    }
    m_deque.push_front(item);
    m_CondConsumer.notify_one();
}

template <class T>
bool BlockDeque<T>::Pop(T &item)
{
    std::lock_guard<std::mutex> locker(m_mtx);
    while (m_deque.empty())
    {
        m_CondConsumer.wait(locker);
        if (m_isclose)
        {
            return false;
        }
    }
    item = m_deque.front();
    m_deque.pop_front();
    m_CondProducer.notify_one();
    return true;
}

template <class T>
void BlockDeque<T>::Flush()
{
    m_CondConsumer.notify_one();
}

template <class T>
size_t BlockDeque<T>::Size()
{
    std::lock_guard<std::mutex> locker(m_mtx);
    return m_deque.size();
}

template <class T>
size_t BlockDeque<T>::Capacity()
{
    std::lock_guard<std::mutex> locker(m_mtx);
    return m_capacity();
}

template <class T>
T BlockDeque<T>::Front()
{
    std::lock_guard<std::mutex> locker(m_mtx);
    return m_deque.front();
}

template <class T>
T BlockDeque<T>::Back()
{
    std::lock_guard<std::mutex> locker(m_mtx);
    return m_deque.back();
}

#endif

