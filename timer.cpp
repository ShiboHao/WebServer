#include "timer.h"


void TimerManager::siftup(size_t i) {
    assert(i >= 0 && i < heap.size());

    size_t j = (i - 1) / 2;
    while (j >= 0) {
        if (heap[j] < heap[i])
            break;
        
        swapNode(i, j);
        i = j;
        j = (i - 1) / 2;
    }
}


void TimerManager::swapNode(size_t i, size_t j) {
    assert(i >= 0 && i < heap.size());
    assert(j >= 0 && j < heap.size());

    std::swap(heap[i], heap[j]);
    ref[heap[i].id] = i;
    ref[heap[j].id] = j;
}


bool TimerManager::siftdown(size_t index, size_t n) {
    assert(index >= 0 && index < heap.size());
    assert(n >= 0 && n <= heap.size());

    size_t i = index;
    size_t j = i * 2 + 1;
    while (j < n) {
        if (j + 1 < n && heap[j+1] < heap[j])
            ++j;
        if (heap[i] < heap[j])
            break;
        swapNode(i,j);
        i = j;
        j = i * 2 + 1;
    }

    return i > index;
}


void TimerManager::addTimer(int id, int timeout, const TimeoutCallBack& cb) {
    assert(id >= 0);

    size_t i;
    if (ref.count(id) == 0) {
        i = heap.size();
        ref[id] = i;
        heap.emplace_back(id, Clock::now() + MS(timeout), cb);
    }
    else {
        i = ref[id];
        heap[i].expire = Clock::now() + MS(timeout);
        heap[i].cb = cb;
        if (!siftdown(i, heap.size())) {
            siftup(i);
        }
    }
}


// 删除id， cb
void TimerManager::work(int id) {
    if (heap.empty() || ref.count(id) == 0) 
        return;
    
    size_t i = ref[id];
    TimerNode node = heap[i];
    node.cb();
    del(i);
}


void TimerManager::del(size_t index) {
    assert(!heap.empty() && index >= 0 && index < heap.size());

    size_t i = index;
    size_t n = heap.size() - 1;
    assert(i <= n);
    if (i < n) {
        swapNode(i, n);
        if (!siftdown(i, n)) {
            siftup(i);
        }
    }
    ref.erase(heap.back().id);
    heap.pop_back();
}


void TimerManager::update(int id, int timeout) {
    assert(!heap.empty() && ref.count(id) > 0);

    heap[ref[id]].expire = Clock::now() + MS(timeout);
    siftdown(ref[id], heap.size());
}


void TimerManager::handle_expired_event() {
    if (heap.empty()) 
        return;

    while (!heap.empty()) {
        TimerNode node = heap.front();
        if (std::chrono::duration_cast<MS>(node.expire - Clock::now()).count() > 0) 
            break;
        node.cb();
        pop();
    }
}


void TimerManager::pop() {
    assert(!heap.empty());

    del(0);
}


void TimerManager::clear() {
    ref.clear();
    heap.clear();
}


int TimerManager::getNextHandle() {
    handle_expired_event();
    
    size_t res = -1;
    if (!heap.empty()) {
        res = std::chrono::duration_cast<MS>(heap.front().expire - Clock::now()).count();
        if (res < 0)
            res = 0;
    }

    return res;
}