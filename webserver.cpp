#include "webserver.h"


WebServer::WebServer(int _port, int _trigMode, int _timeoutMS, bool _optLinger, int _threadNum):
    port(port),
    openLinger(_optLinger),
    timeoutMS(timeoutMS),
    isClose(false),
    timer(new TimerManager()),
    threadpool(new ThreadPool(_threadNum)),
    epoller(new Epoller())
{
    srcDir = getcwd(nullptr, 256);  // 当前绝对工作目录
    assert(srcDir);
    strncat(srcDir, "/resources/", 16);

    HTTPconnection::userCount = 0;
    HTTPconnection::srcDir = srcDir;

    initEventMode(_trigMode);
    if (!initSocket())
        isClose = true;
}


WebServer::~WebServer() {
    close(listenFd);
    isClose = true;
    free(srcDir);
}


void WebServer::initEventMode(int _trigMode) {
    listenEvent = EPOLLRDHUP;
    connectionEvent = EPOLLONESHOT | EPOLLRDHUP;
    switch (_trigMode) {
    case 0:
        break;
    case 1:
        connectionEvent |= EPOLLET;
        break;
    case 2:
        listenEvent |= EPOLLET;
        break;
    case 3:
        listenEvent |= EPOLLET;
        connectionEvent |= EPOLLET;
        break;
    default:
        listenEvent |= EPOLLET;
        connectionEvent |= EPOLLET;
        break;
    }
    HTTPconnection::isET = (connectionEvent & EPOLLET);
}


void WebServer::Start() {
    int timeMS = -1;    // epoll wait
    if (!isClose) {
        std::cout<<"============================";
        std::cout<<"WebServer, 启动!";
        std::cout<<"============================";
        std::cout<<std::endl;
    }
    while (!isClose) {
        if (timeoutMS > 0) {
            timeMS = timer->getNextHandle();
        }
        int eventCnt = epoller->wait(timeMS);
        for (int i = 0; i < eventCnt; ++i) {
            int fd = epoller->getEventFd(i);
            uint32_t events = epoller->getEvents(i);

            if (fd == listenFd) {
                std::cout << fd << " is listening" << std::endl;
                handleListen();
            }
            else if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                assert(users.count(fd) > 0);
                std::cout << fd << " is closed" << std::endl;
                closeConn(&users[fd]);
            }
            else if (events & EPOLLIN) {
                assert(users.count(fd) > 0);
                std::cout << fd << " is reading completely" << std::endl;
                handelRead(&users[fd]);
            }
            else if (events & EPOLLOUT) {
                assert(users.count(fd) > 0);
                std::cout << fd << " is writing completely" << std::endl;
                handleWrite(&users[fd]);
            }
            else {
                std::cout << "Un expected event" << std::endl;
            }
        }

    }
}


void WebServer::sendError(int fd, const char* info) {
    assert(fd > 0);
    int ret = send(fd, info, strlen(info), 0);
    if (ret < 0) {
        std::cout << fd << " error!" << std::endl;
    }
    close(fd);
}


void WebServer::closeConn(HTTPconnection* client) {
    assert(client);
    std::cout << "Client: " << client->getFd() << " close" << std::endl;
    epoller->delFd(client->getFd());
    client->closeHTTPConn();
}


void WebServer::addClientConnection(int fd, sockaddr_in addr) {
    assert(fd > 0);
    users[fd].initHTTPConn(fd, addr);
    if (timeoutMS > 0) {
        timer->addTimer(fd, timeoutMS, std::bind(
            &WebServer::closeConn, this, &users[fd]
        ));
    }
    epoller->addFd(fd, EPOLLIN | connectionEvent);
    setFdNonblock(fd);
}


void WebServer::handleListen() {
    sockaddr_in addr;
    socklen_t len = sizeof(addr);
    do {
        int fd = accept(listenFd, (sockaddr*)&addr, &len);
        if (fd <= 0)
            return;
        else if (HTTPconnection::userCount >= MAX_FD) {
            sendError(fd, "Server busy");
            return;
        }
        addClientConnection(fd, addr);
    } while (listenEvent & EPOLLET);
}


void WebServer::handelRead(HTTPconnection* client) {
    assert(client);
    extentTime(client);
    threadpool->submit(std::bind(
        &WebServer::onRead, this, client
    ));
}


void WebServer::handleWrite(HTTPconnection* client) {
    assert(client);
    extentTime(client);
    threadpool->submit(std::bind(
        &WebServer::onWrite, this, client
    ));
}


void WebServer::extentTime(HTTPconnection* client) {
    assert(client);
    if (timeoutMS > 0) {
        timer->update(client->getFd(), timeoutMS);
    }
}


void WebServer::onRead(HTTPconnection* client) {
    assert(client);
    int ret = -1;
    int readErrno = 0;
    ret = client->readBuffer(&readErrno);
    if (ret <= 0 && readErrno != EAGAIN) {
        closeConn(client);
        return;
    }
    onProcess(client);
}


void WebServer::onProcess(HTTPconnection* client) {
    if (client->handleHTTPConn()) {
        epoller->modFd(client->getFd(), connectionEvent | EPOLLOUT);
    }
    else {
        epoller->modFd(client->getFd(), connectionEvent | EPOLLIN);
    }
}


void WebServer::onWrite(HTTPconnection* client) {
    assert(client);
    int ret = -1;
    int writeErrno = 0;
    ret = client->writeBuffer(&writeErrno);
    if (client->writeBytes() == 0) {
        if (client->isKeepAlive()) {
            onProcess(client);
            return;
        }
    }
    else if (ret < 0) {
        if (writeErrno == EAGAIN) {
            epoller->modFd(client->getFd(), connectionEvent | EPOLLOUT);
            return;
        }
    }
    closeConn(client);
}


bool WebServer::initSocket() {
    int ret;
    sockaddr_in addr;
    if (port > 65535 || port < 1024) {
        std::cout << "port number error!" << std::endl;
        return false;
    }
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);
    linger optlinger = {0};
    if (openLinger) {
        optlinger.l_onoff = 1;
        optlinger.l_linger = 1;
    }

    listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd < 0) {
        std::cout << "Create socket error!" << std::endl;
        return false;
    }

    ret = setsockopt(listenFd, SOL_SOCKET, SO_LINGER, &optlinger, sizeof(optlinger));
    if (ret < 0) {
        close(listenFd);
        std::cout << "Initicial linger error!" << std::endl;
        return false;
    }

    int optval = 1;
    ret = setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int));
    if (ret == -1) {
        std::cout << "Set socket error!" << std::endl;
        close(listenFd);
        return false;
    }

    ret = bind(listenFd, (sockaddr*)&addr, sizeof(addr));
    if (ret < 0) {
        std::cout << "bind port " << port << " error!" << std::endl;
        close(listenFd);
        return false; 
    }

    ret = listen(listenFd, 6);
    if (ret < 0) {
        std::cout << "Listen port " << port << " error!" << std::endl;
        close(listenFd);
        return false;
    }   

    ret = epoller->addFd(listenFd, listenEvent | EPOLLIN);
    if (ret == 0) {
        std::cout << "Add listen " << listenFd << "error!" << std::endl;
        close(listenFd);
        return false;
    }

    std::cout << "Server port: " << port << std::endl;
    setFdNonblock(listenFd);
    
    return true;
}