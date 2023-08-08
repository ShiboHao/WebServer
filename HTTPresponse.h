#pragma once

#include <unordered_map>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <assert.h>

#include "buffer.h"


class HTTPresponse {
public:
    HTTPresponse();
    ~HTTPresponse();

    void init(const std::string& srcDir, std::string& path, bool isKeepAlive=false, int code=-1);
    void makeResponse(Buffer& Buffer);
    void unmapFile();
    inline char* file();
    inline size_t fileLen() const;
    void errorContent(Buffer& Buffer, std::string message);
    inline int getCode() const { return code; }

private:
    void addStateLine(Buffer& buffer);
    void addResponseHeader(Buffer& buffer);
    void addResponseContext(Buffer& buffer);

    void errorHTML();
    std::string getFileType();

    int code;
    bool isKeepAlive;

    std::string path;
    std::string srcDir;

    char* mmFile;
    struct stat mmFileStat;

    static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;
    static const std::unordered_map<int, std::string> CODE_STATUS;
    static const std::unordered_map<int, std::string> CODE_PATH;

};