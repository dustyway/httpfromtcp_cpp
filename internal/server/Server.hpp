#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <stdint.h>
#include "Response.hpp"

class Request;

struct HandlerError {
    Response::StatusCode statusCode;
    std::string message;
};

// Handler writes to responseBody on success, fills error on failure.
// Returns true if there was an error.
typedef bool (*Handler)(std::string& responseBody, const Request& req, HandlerError& error);

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
