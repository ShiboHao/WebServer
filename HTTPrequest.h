/*
** HTTP request
** 
** request line     method space URL space version \r\n
** request header   segment: value \r\n
**                  ...
** \r\n
** request body
*/



#pragma once

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <regex>

#include "buffer.h"

class HTTPrequest {
public:
    enum PARSE_STATE {
        REQUEST_LINE,
        HEADERS,
        BODY,
        FINISH
    };

    enum HTTP_CODE {
        NO_REQUEST = 0,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDENT_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION
    };

    HTTPrequest() { init(); }
    ~HTTPrequest() = default;

    void init();
    
    bool parse(Buffer& Buff);   // parse HTTP req

    inline std::string getPath() const;
    inline std::string& getPath ();
    inline std::string getMethod() const;
    inline std::string getVersion() const;
    std::string getPost(const std::string& key) const;
    std::string getPost(const char* key) const;

    bool isKeepAlive() const;

private:
    bool parseRequestLine(const std::string& line);
    void parseRequestHeader(const std::string& line);
    void parseDataBody(const std::string& line);

    void parsePath();
    void parsePost();

    static int convertHex(char ch);

    PARSE_STATE state;
    std::string method;
    std::string path;
    std::string version;
    std::string body;
    std::unordered_map<std::string, std::string> header;
    std::unordered_map<std::string, std::string> post;

    static const std::unordered_set<std::string> DEFAULT_HTML;

};