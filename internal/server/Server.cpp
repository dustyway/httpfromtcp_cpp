#include "Server.hpp"
#include "Request.hpp"
#include <cstring>
#include <cerrno>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <signal.h>
#include <unistd.h>

Server::Server() : closed(false), listenerFd(-1), epollFd(-1), handler(NULL) {}

Server::~Server() {
    if (listenerFd >= 0) {
        ::close(listenerFd);
    }
    if (epollFd >= 0) {
        ::close(epollFd);
    }
}

void Server::runConnection(int conn) {
    Response::Writer w(conn);

    std::string parseErr;
    Request* req = Request::requestFromSocket(conn, parseErr);
    if (req == NULL) {
        w.writeStatusLine(Response::StatusBadRequest);
        w.writeHeaders(Response::getDefaultHeaders(0));
        ::close(conn);
        return;
    }

    handler(w, *req);

    delete req;
    ::close(conn);
}

void Server::run() {
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    sigprocmask(SIG_BLOCK, &mask, NULL);

    int sfd = signalfd(-1, &mask, 0);
    if (sfd < 0) {
        return;
    }

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = listenerFd;
    epoll_ctl(epollFd, EPOLL_CTL_ADD, listenerFd, &ev);

    ev.events = EPOLLIN;
    ev.data.fd = sfd;
    epoll_ctl(epollFd, EPOLL_CTL_ADD, sfd, &ev);

    struct epoll_event events[16];

    while (!closed) {
        int n = epoll_wait(epollFd, events, 16, -1);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }

        for (int i = 0; i < n; i++) {
            if (events[i].data.fd == sfd) {
                closed = true;
                break;
            } else if (events[i].data.fd == listenerFd) {
                int conn = accept(listenerFd, NULL, NULL);
                if (conn >= 0) {
                    runConnection(conn);
                }
            }
        }
    }

    ::close(sfd);
}

Server* Server::serve(uint16_t port, Handler h, std::string& errorMsg) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        errorMsg = std::string("socket error: ") + std::strerror(errno);
        return NULL;
    }

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        errorMsg = std::string("bind error: ") + std::strerror(errno);
        ::close(fd);
        return NULL;
    }

    if (listen(fd, 5) < 0) {
        errorMsg = std::string("listen error: ") + std::strerror(errno);
        ::close(fd);
        return NULL;
    }

    int epfd = epoll_create(1);
    if (epfd < 0) {
        errorMsg = std::string("epoll_create error: ") + std::strerror(errno);
        ::close(fd);
        return NULL;
    }

    Server* s = new Server();
    s->listenerFd = fd;
    s->epollFd = epfd;
    s->handler = h;

    return s;
}

void Server::close() {
    closed = true;
}
