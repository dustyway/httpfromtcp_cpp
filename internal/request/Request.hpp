#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <string>

namespace ParserState {
    enum State {
        Init,
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
    RequestLine requestLine;

    Request();

    // Parse data incrementally - can be called multiple times
    // Returns number of bytes consumed, or -1 on error
    // Sets errorMsg if provided and an error occurs
    int parse(const std::string& data, std::string* errorMsg);

    // Check if parsing is complete (either done or error)
    bool done() const;

    // Check if parsing ended in error
    bool error() const;

    static const char* ERROR_MALFORMED_REQUEST_LINE;
    static const char* ERROR_REQUEST_IN_ERROR_STATE;

private:
    ParserState::State state;

    static const char* SEPARATOR;

    // Parse request line from buffer
    // Returns bytes consumed (0 if incomplete, -1 on error)
    static int parseRequestLine(const std::string& buffer,
                                RequestLine* rl,
                                std::string* errorMsg);
};

#endif
