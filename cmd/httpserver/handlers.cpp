#include "handlers.hpp"
#include "Request.hpp"
#include "Sha256.hpp"
#include <cstdio>
#include <sstream>

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

static void sendHtml(Response::Writer& w, Response::StatusCode status,
                     const char* body, size_t bodyLen) {
    Headers h = Response::getDefaultHeaders(0);
    std::ostringstream oss;
    oss << bodyLen;
    h.replace("Content-Length", oss.str());
    h.replace("Content-Type", "text/html");
    w.writeStatusLine(status);
    w.writeHeaders(h);
    w.writeBody(body, bodyLen);
}

void handleDefault(Response::Writer& w, const Request&) {
    sendHtml(w, Response::StatusOk, BODY_200, sizeof(BODY_200) - 1);
}

void handle400(Response::Writer& w, const Request&) {
    sendHtml(w, Response::StatusBadRequest, BODY_400, sizeof(BODY_400) - 1);
}

void handle500(Response::Writer& w, const Request&) {
    sendHtml(w, Response::StatusInternalServerError, BODY_500, sizeof(BODY_500) - 1);
}

void VideoHandler::handle(Response::Writer& w, const Request&) {
    FILE* f = std::fopen(path.c_str(), "rb");
    if (f == NULL) {
        sendHtml(w, Response::StatusInternalServerError, BODY_500, sizeof(BODY_500) - 1);
        return;
    }

    Headers h = Response::getDefaultHeaders(0);
    h.remove("content-length");
    h.set("transfer-encoding", "chunked");
    h.replace("content-type", "video/mp4");
    w.writeStatusLine(Response::StatusOk);
    w.writeHeaders(h);

    char buf[1024];
    for (;;) {
        size_t n = std::fread(buf, 1, sizeof(buf), f);
        if (n > 0) {
            w.writeChunkedBody(buf, n);
        }
        if (n < sizeof(buf)) {
            break;
        }
    }
    w.writeChunkedBodyDone();
    std::fclose(f);
}

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

void handleHttpbin(Response::Writer& w, const Request& req) {
    std::string target = req.getTarget();
    std::string httpbinPath = target.substr(9); // after "/httpbin/"

    if (!isSafePath(httpbinPath)) {
        sendHtml(w, Response::StatusInternalServerError, BODY_500, sizeof(BODY_500) - 1);
        return;
    }

    std::string cmd = "curl -s https://httpbin.org/" + httpbinPath;
    FILE* pipe = popen(cmd.c_str(), "r");
    if (pipe == NULL) {
        sendHtml(w, Response::StatusInternalServerError, BODY_500, sizeof(BODY_500) - 1);
        return;
    }

    Headers h = Response::getDefaultHeaders(0);
    h.remove("content-length");
    h.set("transfer-encoding", "chunked");
    h.replace("content-type", "text/plain");
    h.set("Trailer", "X-Content-SHA256");
    h.set("Trailer", "X-Content-Length");
    w.writeStatusLine(Response::StatusOk);
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
