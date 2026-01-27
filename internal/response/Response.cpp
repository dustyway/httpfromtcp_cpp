#include "Response.hpp"
#include <cstring>
#include <sstream>
#include <unistd.h>

static void appendHeader(const std::string& name, const std::string& value, void* userData) {
    std::string* buf = static_cast<std::string*>(userData);
    *buf += name + ": " + value + "\r\n";
}

Headers* Response::getDefaultHeaders(int contentLen) {
    Headers* h = new Headers();
    std::ostringstream oss;
    oss << contentLen;
    h->set("Content-Length", oss.str());
    h->set("Connection", "close");
    h->set("Content-Type", "text/plain");
    return h;
}

bool Response::writeStatusLine(int fd, StatusCode statusCode) {
    const char* statusLine = NULL;
    switch (statusCode) {
        case StatusOk:
            statusLine = "HTTP/1.1 200 OK\r\n";
            break;
        case StatusBadRequest:
            statusLine = "HTTP/1.1 400 Bad Request\r\n";
            break;
        case StatusInternalServerError:
            statusLine = "HTTP/1.1 500 Internal Server Error\r\n";
            break;
        default:
            return false;
    }
    ssize_t n = ::write(fd, statusLine, std::strlen(statusLine));
    return n == static_cast<ssize_t>(std::strlen(statusLine));
}

bool Response::writeHeaders(int fd, const Headers& h) {
    std::string buf;
    h.forEach(appendHeader, &buf);
    buf += "\r\n";
    ssize_t n = ::write(fd, buf.c_str(), buf.size());
    return n == static_cast<ssize_t>(buf.size());
}
