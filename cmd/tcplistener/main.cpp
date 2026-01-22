#include <cstring>
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "LineReader.hpp"

#define PORT 42069

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

        LineReader reader(conn);
        std::string line;
        while (reader.getLine(line)) {
            std::cout << line << std::endl;
        }

        std::cout << "Connection to " << client_ip << ":" << client_port << " closed" << std::endl;
        close(conn);
    }
    close(listener);
    return 0;
}
