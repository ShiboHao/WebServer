/*
** 定时器，清理过期  
*/



#pragma once 

#include <queue>
#include <deque>
#include <unordered_map>
#include <ctime>
#include <chrono>
#include <functional>
#include <memory>
#include <assert.h>

#include "HTTPconnection.h"


typedef std::function<void()> TimeoutCallBack;
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds MS;
typedef Clock::time_point TimeStamp;


class TimerNode {
public:
    int id;
    TimeStamp expire;   
    TimeoutCallBack cb; // callback func 删除定时器关闭HTTP conn

    bool operator<(const TimerNode& t) {
        return expire < t.expire;
    }
};


class TimerManager {
    typedef std::shared_ptr<TimerNode> SP_TimerNode;

public:
    TimerManager() { heap.reserve(64); }
    ~TimerManager() { clear(); }

    void addTimer(int id, int timeout, const TimeoutCallBack& cb);
    void handle_expired_event();    // 处理过期定时器
    int getNextHandle();            // 下次处理时间

    void update(int id, int timeout);  
    void work(int id);

    void pop();
    void clear();

private:
    void del(size_t i);
    void siftup(size_t i);
    bool siftdown(size_t index, size_t n);
    void swapNode(size_t i, size_t j);

    std::vector<TimerNode> heap;
    std::unordered_map<int, size_t> ref;    // fd对应的定时器在heap中的位置
    
};