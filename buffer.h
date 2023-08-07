#pragma once

#include <vector>
#include <iostream>
#include <cstring>
#include <atomic>
#include <unistd.h>
#include <sys/uio.h>
#include <assert.h>


class Buffer {
public:
    Buffer(int initBufferSize=1024);
    ~Buffer() = default;

    inline size_t writeableBytes() const;  // 缓冲区中可读
    inline size_t readableBytes() const;   // 缓冲区中可写
    inline size_t readBytes() const;       // 缓冲区中已读取字节数

    inline const char* curReadPtr() const;
    inline const char* curWritePtrConst() const;
    inline char* curWritePtr();

    void updateReadPtr(size_t len);
    void updateReadPtrUntilEnd(const char* end);
    void updateWritePtr(size_t len);
    void initPtr();

    void ensureWriteable(size_t len);

    void append(const char* str, size_t len);
    void append(const std::string& str);
    void append(const void* data, size_t len);
    void append(const Buffer& buffer);

    ssize_t readFd(int fd, int* Errno);
    ssize_t writeFd(int fd, int* Errno);

    std::string AlltoStr();

    void printContent();

private:
    inline char* BeginPtr();
    inline const char* BeginPtr() const;

    void allocateSpace(size_t len); // 扩容

    std::vector<char> buffer;
    std::atomic<std::size_t> readPos;
    std::atomic<std::size_t> writePos;

};