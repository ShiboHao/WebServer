#pragma once

#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <vector>
#include <errno.h>

class Epoller {
public:
    explicit Epoller(int maxEvent=1024);
    ~Epoller();

    // 
    bool addFd(int fd, uint32_t events);
    bool modFd(int fd, uint32_t events);
    bool delFd(int fd);

    // timewait=-1: 阻塞直至就绪; 
    // timewait=0: 立即返回
    // timewait>0: 阻塞timeout, 或就绪
    int wait(int timewait=-1);  

    int getEventFd(size_t i) const;
    uint32_t getEvents(size_t i) const;

private:
    int epollerFd; // 唯一标记
    std::vector<struct epoll_event> events; // 就绪事件
    
};