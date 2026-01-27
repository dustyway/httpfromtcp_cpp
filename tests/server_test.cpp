#include <catch2/catch_test_macros.hpp>
#include <string>
#include <cstring>
#include <pthread.h>
#include <csignal>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "Server.hpp"
#include "Request.hpp"

#define TEST_PORT 18080

static bool handleRequest(std::string& responseBody, const Request& req, HandlerError& error) {
    if (req.getTarget() == "/yourproblem") {
        error.statusCode = Response::StatusBadRequest;
        error.message = "Your problem is not my problem\n";
        return true;
    } else if (req.getTarget() == "/myproblem") {
        error.statusCode = Response::StatusInternalServerError;
        error.message = "Woopsie, my bad\n";
        return true;
    }
    responseBody = "All good, frfr\n";
    return false;
}

static void* serverThread(void* arg) {
    Server* s = static_cast<Server*>(arg);
    s->run();
    return NULL;
}

static std::string sendRequest(uint16_t port, const std::string& path) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        return "";
    }

    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (connect(fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        close(fd);
        return "";
    }

    std::string request = "GET " + path + " HTTP/1.1\r\n"
                          "Host: localhost\r\n"
                          "Connection: close\r\n"
                          "\r\n";

    ssize_t written = write(fd, request.c_str(), request.size());
    if (written < 0) {
        close(fd);
        return "";
    }

    std::string response;
    char buf[4096];
    for (;;) {
        ssize_t n = read(fd, buf, sizeof(buf));
        if (n <= 0) {
            break;
        }
        response.append(buf, n);
    }

    close(fd);
    return response;
}

// RAII wrapper: starts the server in a pthread, tears it down via SIGTERM.
struct ServerGuard {
    Server* s;
    pthread_t tid;

    ServerGuard() : s(NULL) {
        // Block SIGINT/SIGTERM so only the server's signalfd picks them up.
        sigset_t mask;
        sigemptyset(&mask);
        sigaddset(&mask, SIGINT);
        sigaddset(&mask, SIGTERM);
        sigprocmask(SIG_BLOCK, &mask, NULL);

        // Drain any pending SIGTERM left from a previous test.
        struct timespec ts;
        ts.tv_sec = 0;
        ts.tv_nsec = 0;
        sigtimedwait(&mask, NULL, &ts);

        std::string err;
        s = Server::serve(TEST_PORT, handleRequest, err);
        if (!s) {
            return;
        }
        pthread_create(&tid, NULL, serverThread, s);
        usleep(50000);
    }

    ~ServerGuard() {
        if (s) {
            kill(getpid(), SIGTERM);
            pthread_join(tid, NULL);
            s->close();
            delete s;
        }
    }
};

TEST_CASE("GET / returns 200 OK", "[server]") {
    ServerGuard server;
    REQUIRE(server.s != NULL);

    std::string resp = sendRequest(TEST_PORT, "/");
    REQUIRE_FALSE(resp.empty());

    CHECK(resp.find("HTTP/1.1 200 OK\r\n") != std::string::npos);
    CHECK(resp.find("content-type: text/plain\r\n") != std::string::npos);
    CHECK(resp.find("content-length: 15\r\n") != std::string::npos);
    CHECK(resp.find("All good, frfr\n") != std::string::npos);
}

TEST_CASE("GET /yourproblem returns 400 Bad Request", "[server]") {
    ServerGuard server;
    REQUIRE(server.s != NULL);

    std::string resp = sendRequest(TEST_PORT, "/yourproblem");
    REQUIRE_FALSE(resp.empty());

    CHECK(resp.find("HTTP/1.1 400 Bad Request\r\n") != std::string::npos);
    CHECK(resp.find("content-type: text/plain\r\n") != std::string::npos);
    CHECK(resp.find("content-length: 31\r\n") != std::string::npos);
    CHECK(resp.find("Your problem is not my problem\n") != std::string::npos);
}

TEST_CASE("GET /myproblem returns 500 Internal Server Error", "[server]") {
    ServerGuard server;
    REQUIRE(server.s != NULL);

    std::string resp = sendRequest(TEST_PORT, "/myproblem");
    REQUIRE_FALSE(resp.empty());

    CHECK(resp.find("HTTP/1.1 500 Internal Server Error\r\n") != std::string::npos);
    CHECK(resp.find("content-type: text/plain\r\n") != std::string::npos);
    CHECK(resp.find("content-length: 16\r\n") != std::string::npos);
    CHECK(resp.find("Woopsie, my bad\n") != std::string::npos);
}
