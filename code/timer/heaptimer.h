#ifndef HEAP_TIMER_H
#define HEAP_TIMER_H
#include <queue>
#include <time.h>
#include <unordered_map>
#include <algorithm>
#include <arpa/inet.h>
#include <functional>
#include <assert.h>
#include <chrono>
#include "../log/log.h"

typedef std::function<void()> TimeoutCallBack;
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds MS;
typedef Clock::time_point TimeStamp;

struct TimerNode
{
    int id;
    TimeStamp expires;
    TimeoutCallBack cb;
    bool operator<(const TimerNode &t)
    {
        return expires < t.expires;
    }
};
class HeapTimer
{
public:
    HeapTimer() { m_heap.reserve(64); }
    ~HeapTimer() { Clear(); }
    void Adjust(int id, int newExpires);
    void Add(int id, int timeout, const TimeoutCallBack &cb);
    void DoWork(int id);
    void Clear();
    void Tick();
    void Pop();
    int GetNextTick();

private:
    void m_Del(size_t i);
    void m_Siftup(size_t i);
    bool m_Siftdown(size_t index, size_t n);
    void m_SwapNode(size_t i, size_t j);
    std::vector<TimerNode> m_heap;
    std::unordered_map<int, size_t> m_ref;
};
#endif
