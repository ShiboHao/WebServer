#include "HTTPrequest.h"


const std::unordered_set<std::string> HTTPrequest::DEFAULT_HTML {
    "/index",
    "/welcome",
    "/video",
    "/picture"
};


void HTTPrequest::init() {
    method = path = version = body = "";
    state = REQUEST_LINE;
    header.clear();
    post.clear();
}


bool HTTPrequest::isKeepAlive() const {
    if (header.count("Connection") == 1)
        return header.find("Connection")->second == "keep-alive" && version == "1.1";
    return false;
}


bool HTTPrequest::parse(Buffer& buff) {
    const char CRLF[] = "\r\n";
    if (buff.readableBytes() <= 0) {
        return false;
    }

    std::cout << "====================parse====================" << std::endl;
    buff.printContent();
    std::cout << "====================parse====================" << std::endl;
    
    while (buff.readableBytes() && state != FINISH) {
        const char* lineEnd = std::search(buff.curReadPtr(), buff.curWritePtrConst(), CRLF, CRLF + 2);
        std::string line(buff.curReadPtr(), lineEnd);

        switch (state)
        {
        case REQUEST_LINE:
            std::cout << "REQUEST: " << line << std::endl;
            if (!parseRequestLine(line))
                return false;
            parsePath();
            break;
        case HEADERS:
            std::cout << "HEADERS: " << line << std::endl;
            parseRequestHeader(line);
            if (buff.readableBytes() <= 2) {
                state = FINISH;
            }
            break;
        case BODY:
            std::cout << "BODY: " << line << std::endl;
            parseDataBody(line);
            break;

        default:
            break;
        }
        
        if (lineEnd == buff.curWritePtr())
            break;
        
        buff.updateReadPtrUntilEnd(lineEnd + 2);
    }

    return true;
}


void HTTPrequest::parsePath() {
    if (path == "/")
        path = "/index.html";
    else {
        for (auto &item: DEFAULT_HTML) {
            if (item == path) {
                path += ".html";
                break;
            }
        }
    }
}


bool HTTPrequest::parseRequestLine(const std::string& line) {
    std::regex pattern("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    std::smatch subMatch;
    if (regex_match(line, subMatch, pattern)) {
        method = subMatch[1];
        path = subMatch[2];
        version = subMatch[3];
        state = HEADERS;
        return true;
    }
    return false;
}


void HTTPrequest::parseRequestHeader(const std::string& line) {
    std::regex pattern("^([^:]*): ?(.*)$");
    std::smatch subMatch;
    if (regex_match(line, subMatch, pattern)) {
        header[subMatch[1]] = subMatch[2];
    }
    else {
        state = BODY;
    }
}

void HTTPrequest::parseDataBody(const std::string& line) {
    body = line;
    parsePost();
    state = FINISH;
}


int HTTPrequest::convertHex(char ch) {
    if(ch >= 'A' && ch <= 'F') return ch -'A' + 10;
    if(ch >= 'a' && ch <= 'f') return ch -'a' + 10;
    return ch;
}


void HTTPrequest::parsePost() {
    if (method == "POST" && header["Content-Type"] == "application/x-www-form-urlencoded") {
        if (body.size() == 0)
            return;

        std::string key, value;
        int num = 0;
        int n = body.size();
        int i = 0, j = 0;

        for (;i < n; ++i) {
            char ch = body[i];
            switch (ch) {
            case '=':
                key = body.substr(j, i - j);
                j = i + 1;
                break;
            case '+':
                body[i] = ' ';
                break;
            case '%':
                num = convertHex(body[i + 1] * 16 + convertHex(body[i + 2]));
                body[i + 2] = num % 10 + '0';
                body[i + 1] = num / 10 + '0';
                i += 2;
                break;
            case '&':
                value = body.substr(j, i - j);
                j = i + 1;
                post[key] = value;
                break;
            default:
                break;
            }
        }
        assert(j <= i);
        if (post.count(key) == 0 && j < i) {
            value = body.substr(j, i - j);
            post[key] = value;    
        }
    }
}


std::string HTTPrequest::getPath() const {
    return path;
}


std::string& HTTPrequest::getPath(){
    return path;
}


std::string HTTPrequest::getMethod() const {
    return method;
}


std::string HTTPrequest::getVersion() const {
    return version;
}


std::string HTTPrequest::getPost(const std::string& key) const {
    assert(key != "");
    if (post.count(key) == 1) {
        return post.find(key)->second;
    }
    return "";
}


std::string HTTPrequest::getPost(const char* key) const {
    assert(key != nullptr);
    if (post.count(key) == 1) {
        return post.find(key)->second;
    }
    return "";
}