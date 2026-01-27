#include <catch2/catch_test_macros.hpp>
#include <string>
#include <cstring>
#include <sstream>
#include <pthread.h>
#include <csignal>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "Server.hpp"
#include "Request.hpp"

#define TEST_PORT 18080

static const char BODY_200[] =
    "<html>\n"
    "  <head>\n"
    "    <title>200 OK</title>\n"
    "  </head>\n"
    "  <body>\n"
    "    <h1>Success!</h1>\n"
    "    <p>Your request was an absolute banger.</p>\n"
    "  </body>\n"
    "</html>";

static const char BODY_400[] =
    "<html>\n"
    "  <head>\n"
    "    <title>400 Bad Request</title>\n"
    "  </head>\n"
    "  <body>\n"
    "    <h1>Bad Request</h1>\n"
    "    <p>Your request honestly kinda sucked.</p>\n"
    "  </body>\n"
    "</html>";

static const char BODY_500[] =
    "<html>\n"
    "  <head>\n"
    "    <title>500 Internal Server Error</title>\n"
    "  </head>\n"
    "  <body>\n"
    "    <h1>Internal Server Error</h1>\n"
    "    <p>Okay, you know what? This one is on me.</p>\n"
    "  </body>\n"
    "</html>";

static void handleRequest(Response::Writer& w, const Request& req) {
    Headers h = Response::getDefaultHeaders(0);
    const char* body = BODY_200;
    size_t bodyLen = sizeof(BODY_200) - 1;
    Response::StatusCode status = Response::StatusOk;

    if (req.getTarget() == "/yourproblem") {
        body = BODY_400;
        bodyLen = sizeof(BODY_400) - 1;
        status = Response::StatusBadRequest;
    } else if (req.getTarget() == "/myproblem") {
        body = BODY_500;
        bodyLen = sizeof(BODY_500) - 1;
        status = Response::StatusInternalServerError;
    }

    std::ostringstream oss;
    oss << bodyLen;
    w.writeStatusLine(status);
    h.replace("Content-Length", oss.str());
    h.replace("Content-Type", "text/html");
    w.writeHeaders(h);
    w.writeBody(body, bodyLen);
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
    CHECK(resp.find("content-type: text/html\r\n") != std::string::npos);
    CHECK(resp.find(BODY_200) != std::string::npos);
}

TEST_CASE("GET /yourproblem returns 400 Bad Request", "[server]") {
    ServerGuard server;
    REQUIRE(server.s != NULL);

    std::string resp = sendRequest(TEST_PORT, "/yourproblem");
    REQUIRE_FALSE(resp.empty());

    CHECK(resp.find("HTTP/1.1 400 Bad Request\r\n") != std::string::npos);
    CHECK(resp.find("content-type: text/html\r\n") != std::string::npos);
    CHECK(resp.find(BODY_400) != std::string::npos);
}

TEST_CASE("GET /myproblem returns 500 Internal Server Error", "[server]") {
    ServerGuard server;
    REQUIRE(server.s != NULL);

    std::string resp = sendRequest(TEST_PORT, "/myproblem");
    REQUIRE_FALSE(resp.empty());

    CHECK(resp.find("HTTP/1.1 500 Internal Server Error\r\n") != std::string::npos);
    CHECK(resp.find("content-type: text/html\r\n") != std::string::npos);
    CHECK(resp.find(BODY_500) != std::string::npos);
}
