#pragma once

#include <unordered_map>
#include <memory>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <error.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "epoller.h"
#include "timer.h"
#include "threadpool.h"
#include "HTTPconnection.h"

class WebServer {
public:
    WebServer(int port, int trigMode, int timeoutMS, bool optLinger, int threadNum);
    ~WebServer();

    void Start();

private:
    bool initSocket();

    void initEventMode(int trigMode);

    void addClientConnection(int fd, sockaddr_in addr);
    void closeConn(HTTPconnection* client);

    void handleListen();
    void handleWrite(HTTPconnection* client);
    void handelRead(HTTPconnection* client);

    void onRead(HTTPconnection* client);
    void onWrite(HTTPconnection* client);
    void onProcess(HTTPconnection* client);

    void sendError(int fd, const char* info);
    void extentTime(HTTPconnection* client);
    
    static constexpr int MAX_FD = 65536;
    static int setFdNonblock(int fd);

    int port;
    int timeoutMS;
    bool isClose;
    int listenFd;
    bool openLinger;
    char* srcDir;

    uint32_t listenEvent;
    uint32_t connectionEvent;

    std::unique_ptr<TimerManager> timer;
    std::unique_ptr<ThreadPool> threadpool;
    std::unique_ptr<Epoller> epoller;
    std::unordered_map<int, HTTPconnection> users;
};

int WebServer::setFdNonblock(int fd) {
    assert(fd > 0);
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
}