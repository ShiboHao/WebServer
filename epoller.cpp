#include "epoller.h"


Epoller::Epoller(int maxEvent) :
    epollerFd(epoll_create(512)),
    events(maxEvent)
{
    assert(epollerFd >= 0 && events.size() > 0);
}


Epoller::~Epoller() {
    close(epollerFd);
}


bool Epoller::addFd(int fd, uint32_t events) {
    if (fd < 0)
        return false;

    epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = events;

    return 0 == epoll_ctl(epollerFd, EPOLL_CTL_ADD, fd, &ev);
}


bool Epoller::modFd(int fd, uint32_t events) {
    if (fd < 0)
        return false;

    epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = events;

    return 0 == epoll_ctl(epollerFd, EPOLL_CTL_MOD, fd, &ev);
}


bool Epoller::delFd(int fd) {
    if (fd < 0)
        return false;
    
    epoll_event ev = {0};
    
    return 0 == epoll_ctl(epollerFd, EPOLL_CTL_DEL, fd, &ev);
}


int Epoller::wait(int timeoutMS) {
    return epoll_wait(epollerFd, &events[0], static_cast<int>(events.size()), timeoutMS);
}


int Epoller::getEventFd(size_t i) const {
    assert(i < events.size() && i >= 0);
    
    return events[i].data.fd;
}


uint32_t Epoller::getEvents(size_t i) const {
    assert(i < events.size() && i >= 0);

    return events[i].events; 
}