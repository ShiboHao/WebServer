#include "buffer.h"


Buffer::Buffer(int initBufferSize):
    buffer(initBufferSize),
    readPos(0),
    writePos(0)
{}


size_t Buffer::readableBytes() const {
    return writePos - readPos;
}


size_t Buffer::writeableBytes() const {
    return buffer.size() - writePos;
}


size_t Buffer::readBytes() const {
    return readPos;
}


const char* Buffer::curReadPtr() const {
    return BeginPtr() + readPos;
}


const char* Buffer::curWritePtrConst() const {
    return BeginPtr() + writePos;
}


char* Buffer::curWritePtr() {
    return BeginPtr() + writePos;
}


void Buffer::updateReadPtr(size_t len) {
    assert(len <= readableBytes());

    readPos += len;
}


void Buffer::updateReadPtrUntilEnd(const char* end) {
    assert(end >= curReadPtr());

    updateReadPtr(end - curReadPtr());
}


void Buffer::updateWritePtr(size_t len) {
    assert(len <= writeableBytes());

    writePos += len;
}


void Buffer::initPtr() {
    bzero(&buffer[0], buffer.size());   // set 0
    readPos = 0;
    writePos = 0;
}


void Buffer::allocateSpace(size_t len) {
    if (writeableBytes() + readableBytes() < len) {
        // 扩容
        buffer.resize(writePos + len + 1);
    }
    else {
        size_t readable = readableBytes();
        std::copy(BeginPtr() + readPos, BeginPtr() + writePos, BeginPtr());
        readPos = 0;
        writePos = readable;
        assert(readable == readableBytes());
    }
}


void Buffer::ensureWriteable(size_t len) {
    if (writeableBytes() < len) {
        allocateSpace(len);
    }
    assert(writeableBytes() >= len);
}


void Buffer::append(const char* str, size_t len) {
    assert(str);

    ensureWriteable(len);
    std::copy(str, str + len, curWritePtr());
    updateWritePtr(len);
}


void Buffer::append(const std::string& str) {
    append(str.data(), str.length());
}


void Buffer::append(const void* data, size_t len) {
    assert(data);
    append(static_cast<const char*>(data), len);
}


void Buffer::append(const Buffer& buffer) {
    append(buffer.curReadPtr(), buffer.readableBytes());
}


ssize_t Buffer::readFd(int fd, int* Errno) {
    char buff[65535];
    struct iovec iov[2];
    const size_t writeable = writeableBytes();

    iov[0].iov_base = BeginPtr() + writePos;
    iov[0].iov_len = writeable;
    iov[1].iov_base = buff;
    iov[1].iov_len = sizeof(buff);

    const ssize_t len = readv(fd, iov, 2);
    if (len < 0) {
        *Errno = errno;
    }
    else if (static_cast<size_t>(len) <= writeable) {
        writePos += len;
    }
    else {
        writePos = buffer.size();
        append(buff, len - writeable);
    }

    return len;
}


ssize_t Buffer::writeFd(int fd, int* Errno) {
    size_t readSize = readableBytes();
    ssize_t len = write(fd, curReadPtr(), readSize);
    if (len < 0) {
        *Errno = errno;
        return len;
    }
    readPos += len;
    return len;
}


std::string Buffer::AlltoStr() {
    std::string str(curReadPtr(), readableBytes());
    initPtr();
    return str;
}

void Buffer::printContent() {
    std::cout << "read pointer: " << readPos << ", write pointer: " << writePos << std::endl;
    for (int i = readPos; i <= writePos; ++i) {
        std::cout << buffer[i] << " ";
    }
    std::cout << std::endl;
}

char* Buffer::BeginPtr() {
    return &*buffer.begin();
}

const char* Buffer::BeginPtr() const {
    return &*buffer.begin();
}