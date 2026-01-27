#include <iostream>
#include <sstream>
#include <cstdio>
#include "Server.hpp"
#include "Request.hpp"
#include "Sha256.hpp"

#define PORT 42069

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

static bool isSafePath(const std::string& path) {
    for (size_t i = 0; i < path.size(); ++i) {
        char c = path[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '/' || c == '.' ||
              c == '-' || c == '_')) {
            return false;
        }
    }
    return !path.empty();
}

static bool startsWith(const std::string& str, const std::string& prefix) {
    if (str.size() < prefix.size()) return false;
    return str.compare(0, prefix.size(), prefix) == 0;
}

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
    } else if (startsWith(req.getTarget(), "/httpbin/")) {
        std::string target = req.getTarget();
        std::string httpbinPath = target.substr(9); // after "/httpbin/"

        if (!isSafePath(httpbinPath)) {
            body = BODY_500;
            bodyLen = sizeof(BODY_500) - 1;
            status = Response::StatusInternalServerError;
        } else {
            std::string cmd = "curl -s https://httpbin.org/" + httpbinPath;
            FILE* pipe = popen(cmd.c_str(), "r");
            if (pipe == NULL) {
                body = BODY_500;
                bodyLen = sizeof(BODY_500) - 1;
                status = Response::StatusInternalServerError;
            } else {
                w.writeStatusLine(Response::StatusOk);
                h.remove("content-length");
                h.set("transfer-encoding", "chunked");
                h.replace("content-type", "text/plain");
                h.set("Trailer", "X-Content-SHA256");
                h.set("Trailer", "X-Content-Length");
                w.writeHeaders(h);

                std::string fullBody;
                char data[32];
                for (;;) {
                    size_t n = fread(data, 1, sizeof(data), pipe);
                    if (n == 0) {
                        break;
                    }
                    fullBody.append(data, n);
                    w.writeChunkedBody(data, n);
                }
                pclose(pipe);

                w.writeBody("0\r\n", 3);

                unsigned char hash[32];
                Crypto::sha256(fullBody, hash);
                Headers trailers;
                trailers.set("X-Content-SHA256", Crypto::toHexStr(hash, 32));
                std::ostringstream toss;
                toss << fullBody.size();
                trailers.set("X-Content-Length", toss.str());
                w.writeHeaders(trailers);
                w.writeBody("\r\n\r\n", 4);
            }
        }
    }

    std::ostringstream oss;
    oss << bodyLen;
    h.replace("Content-Length", oss.str());
    h.replace("Content-Type", "text/html");
    w.writeStatusLine(status);
    w.writeHeaders(h);
    w.writeBody(body, bodyLen);
}

int main() {
    std::string errorMsg;
    Server* s = Server::serve(PORT, handleRequest, errorMsg);
    if (s == NULL) {
        std::cerr << "Error starting server: " << errorMsg << std::endl;
        return 1;
    }

    std::cout << "Server started on port " << PORT << std::endl;

    s->run();

    s->close();
    delete s;

    std::cout << "Server gracefully stopped" << std::endl;
    return 0;
}
