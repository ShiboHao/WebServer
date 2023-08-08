#include "HTTPresponse.h"


const std::unordered_map<std::string, std::string> HTTPresponse::SUFFIX_TYPE = {
    { ".html",  "text/html" },
    { ".xml",   "text/xml" },
    { ".xhtml", "application/xhtml+xml" },
    { ".txt",   "text/plain" },
    { ".rtf",   "application/rtf" },
    { ".pdf",   "application/pdf" },
    { ".word",  "application/nsword" },
    { ".png",   "image/png" },
    { ".gif",   "image/gif" },
    { ".jpg",   "image/jpeg" },
    { ".jpeg",  "image/jpeg" },
    { ".au",    "audio/basic" },
    { ".mpeg",  "video/mpeg" },
    { ".mpg",   "video/mpeg" },
    { ".avi",   "video/x-msvideo" },
    { ".gz",    "application/x-gzip" },
    { ".tar",   "application/x-tar" },
    { ".css",   "text/css "},
    { ".js",    "text/javascript "},
};



const std::unordered_map<int, std::string> HTTPresponse::CODE_STATUS = {
    { 200, "OK" },
    { 400, "Bad Request" },
    { 403, "Forbidden" },
    { 404, "Not Found" },
};


const std::unordered_map<int, std::string> HTTPresponse::CODE_PATH = {
    { 400, "/400.html" },
    { 403, "/403.html" },
    { 404, "/404.html" },
};


HTTPresponse::HTTPresponse():
    code(-1),
    path(""),
    srcDir(""),
    isKeepAlive(false),
    mmFile(nullptr),
    mmFileStat{0}
{}


HTTPresponse::~HTTPresponse() {
    unmapFile();
}


void HTTPresponse::init(const std::string& _srcDir, std::string& _path, bool _isKeepAlive, int _code) {
    assert(_srcDir != "");
    if (mmFile)
        unmapFile();
    code = _code;
    isKeepAlive = _isKeepAlive;
    path = _path;
    srcDir = _srcDir;
    mmFile = nullptr;
    mmFileStat = {0};
}


void HTTPresponse::makeResponse(Buffer& buffer) {
    if (stat((srcDir+path).data(), &mmFileStat) < 0 || S_ISDIR(mmFileStat.st_mode))    // stat 获取文件状态信息, 成功0，失败-1
        code = 404;
    else if (!(mmFileStat.st_mode & S_IROTH))   // S_IROTH 其他读
        code = 403;
    else if (code == -1)
        code = 200;
    
    errorHTML();
    addStateLine(buffer);
    addResponseHeader(buffer);
    addResponseContext(buffer);
}


char* HTTPresponse::file() {
    return mmFile;
}


size_t HTTPresponse::fileLen() const {
    return mmFileStat.st_size;
}


void HTTPresponse::errorHTML() {
    if (CODE_PATH.count(code) == 1) {
        path = CODE_PATH.find(code)->second;
        stat((srcDir + path).data(), &mmFileStat);
    }
}


void HTTPresponse::addStateLine(Buffer& buff) {
    std::string status;
    if (CODE_STATUS.count(code) == 1) {
        status = CODE_STATUS.find(code)->second;
    }
    else {
        code = 400;
        status = CODE_STATUS.find(400)->second;
    }
    buff.append("HTTP/1.1" + std::to_string(code) + " " + status + "\r\n");
}


void HTTPresponse::addResponseHeader(Buffer& buff) {
    buff.append("Connection: ");
    if (isKeepAlive) {
        buff.append("keep-alive\r\n");
        buff.append("keep-alive: max=6, timeout=120\r\n");
    }
    else {
        buff.append("close\r\n");
    }
    buff.append("Content-type: " + getFileType() + "\r\n");
}


void HTTPresponse::addResponseContext(Buffer& buff) {
    int srcFd = open((srcDir + path).data(), O_RDONLY);
    if (srcFd < 0) {
        errorContent(buff, "File NotFound!");
        return;
    }

    int* mmRet = (int*)mmap(0, mmFileStat.st_size, PROT_READ, MAP_PRIVATE, srcFd, 0);
    if (*mmRet == -1) {
        errorContent(buff, "File NotFound!");
        return;
    }
    mmFile = reinterpret_cast<char*>(mmRet);
    close(srcFd);
    buff.append("Content-length: " + std::to_string(mmFileStat.st_size) + "\r\n\r\n");
}


void HTTPresponse::unmapFile() {
    if (mmFile) {
        munmap(mmFile, mmFileStat.st_size);
        mmFile = nullptr;
    }
}


std::string HTTPresponse::getFileType() {
    std::string::size_type idx = path.find_last_of('.');
    if (idx == std::string::npos) {
        return "text/plain";
    }
    std::string suffix = path.substr(idx);
    if (SUFFIX_TYPE.count(suffix) == 1) {
        return SUFFIX_TYPE.find(suffix)->second;
    }
    return "text/plain";
}


void HTTPresponse::errorContent(Buffer& buff, std::string message) {
    std::string body;
    std::string status;
    body += "<html><title>Error</title>";
    body += "<body bgcolor=\"ffffff\">";
    if (CODE_STATUS.count(code) == 1) {
        status = CODE_STATUS.find(code)->second;
    }
    else {
        status = "Bad Request";
    }
    body += std::to_string(code) + " : " + status + "\n";
    body += "<p>" + message + "</p>";
    body += "<hr><em>TinyWebServer</em></body></html>";
    
    buff.append("Content-length: " + std::to_string(body.size()) + "\r\n\r\n");
    buff.append(body);
}