#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <string>
#include "Headers.hpp"

namespace ParserState {
    enum State {
        Init,
        Headers,
        Done,
        Error
    };
}

struct RequestLine {
    std::string method;
    std::string requestTarget;
    std::string httpVersion;
};

class Request {
public:
    Request();

    const std::string& getMethod() const;
    const std::string& getTarget() const;
    const std::string& getHttpVersion() const;
    const ::Headers& getHeaders() const;
    void forEachHeader(void (*callback)(const std::string&, const std::string&, void*), void* userData) const;

    static const char* ERROR_MALFORMED_REQUEST_LINE;
    static const char* ERROR_REQUEST_IN_ERROR_STATE;

    // Read and parse a request from a socket
    // Returns NULL on error, sets errorMsg if provided
    static Request* requestFromSocket(int socketFd, std::string* errorMsg);

private:
    RequestLine requestLine;
    ::Headers headers;
    ParserState::State state;

    static const char* SEPARATOR;

    // Parse data incrementally - can be called multiple times
    // Returns number of bytes consumed, or -1 on error
    // Sets errorMsg if provided and an error occurs
    int parse(const std::string& data, std::string* errorMsg);

    // Check if parsing is complete (either done or error)
    bool done() const;

    // Check if parsing ended in error
    bool error() const;

    // Parse request line from buffer
    // Returns bytes consumed (0 if incomplete, -1 on error)
    static int parseRequestLine(const std::string& buffer,
                                RequestLine* rl,
                                std::string* errorMsg);
};

#endif
