#pragma once

#include <arpa/inet.h>  // sockaddr_in
#include <sys/uio.h>    // readv/writev
#include <iostream>
#include <sys/types.h>
#include <assert.h>
#include <atomic>

#include "buffer.h"
#include "HTTPrequest.h"
#include "HTTPresponse.h"

class HTTPconnection {
public:
    HTTPconnection();
    ~HTTPconnection();

    void initHTTPConn(int socketFd, const sockaddr_in& addr);

    // 连接对应的缓冲区读接口
    ssize_t readBuffer(int* saveErrno);
    ssize_t writeBuffer(int* saveErrno);

    void closeHTTPConn();
    // 解析request 生成response
    bool handleHTTPConn();

    inline const char* getIP() const;
    inline int getPort() const;
    inline int getFd() const;
    inline sockaddr_in getAddr() const;

    int writeBytes() { return iov[1].iov_len + iov[0].iov_len; }

    bool isKeepAlive() const { return request.isKeepAlive(); }

    static bool isET;
    static const char* srcDir;
    static std::atomic<int> userCount;

private:
    int fd; // HTTP conn 描述符
    struct sockaddr_in addr;
    bool isClose;

    int iovCnt;
    struct iovec iov[2];

    Buffer readBuffers;
    Buffer writeBuffers;

    HTTPrequest request;
    HTTPresponse response;
    
};