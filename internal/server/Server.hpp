#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <stdint.h>

class Server {
public:
    // Create a server bound to the given port.
    // Returns NULL on error, sets errorMsg if provided.
    static Server* serve(uint16_t port, std::string* errorMsg);

    // Run the event loop. Blocks until SIGINT/SIGTERM is received.
    void run();

    // Close the server
    void close();

    ~Server();

private:
    Server();

    bool closed;
    int listenerFd;
    int epollFd;

    static void runConnection(int conn);
};

#endif
