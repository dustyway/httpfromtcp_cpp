#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <stdint.h>
#include "Response.hpp"

class Request;

typedef void (*Handler)(Response::Writer& w, const Request& req);

class Server {
public:
    static Server* serve(uint16_t port, Handler handler, std::string& errorMsg);

    void run();
    void close();

    ~Server();

private:
    Server();

    bool closed;
    int listenerFd;
    int epollFd;
    Handler handler;

    void runConnection(int conn);
};

#endif
