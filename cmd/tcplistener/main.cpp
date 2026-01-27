#include <cstring>
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "Request.hpp"

#define PORT 42069

struct HeaderPrinter {
    void operator()(const std::string& name, const std::string& value) const {
        std::cout << "- " << name << ": " << value << std::endl;
    }
};

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0) {
        std::cerr << "error listening for TCP traffic: " << strerror(errno) << std::endl;
        return 1;
    }

    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    if (bind(listener, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "error listening for TCP traffic: " << strerror(errno) << std::endl;
        close(listener);
        return 1;
    }

    if (listen(listener, 5) < 0) {
        std::cerr << "error listening for TCP traffic: " << strerror(errno) << std::endl;
        close(listener);
        return 1;
    }

    std::cout << "Listening for TCP traffic on :" << PORT << std::endl;

    while (true) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int conn = accept(listener, reinterpret_cast<struct sockaddr*>(&client_addr), &client_len);

        if (conn < 0) {
            std::cerr << "error: " << strerror(errno) << std::endl;
            close(listener);
            return 1;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        int client_port = ntohs(client_addr.sin_port);

        std::cout << "Accepted connection from " << client_ip << ":" << client_port << std::endl;

        std::string errorMsg;
        Request* r = Request::requestFromSocket(conn, errorMsg);
        if (r == NULL) {
            std::cerr << "error: " << errorMsg << std::endl;
            close(conn);
            continue;
        }

        std::cout << "Request line:" << std::endl;
        std::cout << "- Method: " << r->getMethod() << std::endl;
        std::cout << "- Target: " << r->getTarget() << std::endl;
        std::cout << "- Version: " << r->getHttpVersion() << std::endl;
        std::cout << "Headers:" << std::endl;
        r->forEachHeader(HeaderPrinter());

        delete r;
        close(conn);
    }
    close(listener);
    return 0;
}
