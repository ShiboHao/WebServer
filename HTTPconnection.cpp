#include "HTTPconnection.h"


const char* HTTPconnection::srcDir;
std::atomic<int> HTTPconnection::userCount;
bool HTTPconnection::isET;


HTTPconnection::HTTPconnection():
    fd(-1),
    addr{0},
    isClose(true)
{}


HTTPconnection::~HTTPconnection() {
    closeHTTPConn();
}


void HTTPconnection::initHTTPConn(int _fd, const sockaddr_in& _addr) {
    assert(_fd > 0);
    ++userCount;
    addr = _addr;
    fd = _fd;
    writeBuffers.initPtr();
    readBuffers.initPtr();
    isClose = false; 
}


void HTTPconnection::closeHTTPConn() {
    response.unmapFile();
    if (isClose == false) {
        isClose = true;
        --userCount;
        close(fd);
    }
}


int HTTPconnection::getFd() const {
    return fd;
}


sockaddr_in HTTPconnection::getAddr() const {
    return addr;
}


const char* HTTPconnection::getIP() const {
    return inet_ntoa(addr.sin_addr);
}


int HTTPconnection::getPort() const {
    return addr.sin_port;
}


ssize_t HTTPconnection::readBuffer(int* saveErrno) {
    ssize_t len = -1;
    do {
        len = readBuffers.readFd(fd, saveErrno);
        if (len <= 0) 
            break;
    } while (isET);
    return len;
}


ssize_t HTTPconnection::writeBuffer(int* saveErrno) {
    ssize_t len = -1;
    do {
        len = writev(fd, iov, iovCnt);
        if (len <= 0) {
            *saveErrno = errno;
            break;
        }
        if (iov[0].iov_len + iov[1].iov_len == 0)
            break;
        else if (static_cast<size_t>(len) > iov[0].iov_len) {
            iov[1].iov_base = reinterpret_cast<uint8_t*>(iov[1].iov_base) + (len - iov[0].iov_len);
            iov[1].iov_len -= (len - iov[0].iov_len);
            if (iov[0].iov_len) {
                writeBuffers.initPtr();
                iov[0].iov_len = 0;
            }
        }
        else {
            iov[0].iov_base = reinterpret_cast<uint8_t*>(iov[0].iov_base) + len;
            iov[0].iov_len -= len;
            writeBuffers.updateReadPtr(len);
        }
    } while (isET || writeBytes() > 10240);
    return len;
}


bool HTTPconnection::handleHTTPConn() {
    request.init();
    if (readBuffers.readableBytes() <= 0) {
        std::cout << "read buffer is empty" << std::endl;
        return false;
    }
    else if (request.parse(readBuffers)) {
        response.init(srcDir, request.getPath(), request.isKeepAlive(), 200);
    }
    else {
        std::cout << "400" << std::endl;
        readBuffers.printContent();
        response.init(srcDir, request.getPath(), false, 400);
    }

    response.makeResponse(writeBuffers);

    iov[0].iov_base = const_cast<char*>(writeBuffers.curReadPtr());
    iov[0].iov_len = writeBuffers.readableBytes();
    iovCnt = 1;

    if (response.fileLen() > 0 && response.file()) {
        iov[1].iov_base = response.file();
        iov[1].iov_len = response.fileLen();
        iovCnt = 2;
    }

    return true;
}