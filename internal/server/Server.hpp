#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <stdint.h>
#include "RequestHandler.hpp"

class Server {
public:
    static Server* serve(uint16_t port, RequestHandler& handler, std::string& errorMsg);

    void run();
    void close();

    ~Server();

private:
    Server();

    bool closed;
    int listenerFd;
    int epollFd;
    RequestHandler* handler;

    void runConnection(int conn);
};

#endif
