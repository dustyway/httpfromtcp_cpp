#include "Request.hpp"

const char* Request::ERROR_MALFORMED_REQUEST_LINE = "malformed request-line";
const char* Request::ERROR_REQUEST_IN_ERROR_STATE = "request in error state";
const char* Request::SEPARATOR = "\r\n";

Request::Request() : state(ParserState::Init) {}

bool Request::done() const {
    return state == ParserState::Done || state == ParserState::Error;
}

bool Request::error() const {
    return state == ParserState::Error;
}

int Request::parseRequestLine(const std::string& buffer,
                              RequestLine* rl,
                              std::string* errorMsg) {
    std::string::size_type idx = buffer.find(SEPARATOR);
    if (idx == std::string::npos) {
        return 0; // Need more data
    }

    std::string startLine = buffer.substr(0, idx);
    int bytesRead = static_cast<int>(idx) + 2; // Include \r\n

    // Split by spaces - need exactly 3 parts
    std::string::size_type firstSpace = startLine.find(' ');
    if (firstSpace == std::string::npos) {
        if (errorMsg) *errorMsg = ERROR_MALFORMED_REQUEST_LINE;
        return -1;
    }

    std::string::size_type secondSpace = startLine.find(' ', firstSpace + 1);
    if (secondSpace == std::string::npos) {
        if (errorMsg) *errorMsg = ERROR_MALFORMED_REQUEST_LINE;
        return -1;
    }

    // Check no more spaces after secondSpace
    if (startLine.find(' ', secondSpace + 1) != std::string::npos) {
        if (errorMsg) *errorMsg = ERROR_MALFORMED_REQUEST_LINE;
        return -1;
    }

    rl->method = startLine.substr(0, firstSpace);
    rl->requestTarget = startLine.substr(firstSpace + 1, secondSpace - firstSpace - 1);
    std::string httpVersionFull = startLine.substr(secondSpace + 1);

    // Validate HTTP version format: HTTP/1.1
    std::string::size_type slashPos = httpVersionFull.find('/');
    if (slashPos == std::string::npos) {
        if (errorMsg) *errorMsg = ERROR_MALFORMED_REQUEST_LINE;
        return -1;
    }

    std::string httpPrefix = httpVersionFull.substr(0, slashPos);
    std::string httpVersion = httpVersionFull.substr(slashPos + 1);

    if (httpPrefix != "HTTP" || httpVersion != "1.1") {
        if (errorMsg) *errorMsg = ERROR_MALFORMED_REQUEST_LINE;
        return -1;
    }

    rl->httpVersion = httpVersion;
    return bytesRead;
}

int Request::parse(const std::string& data, std::string* errorMsg) {
    int totalRead = 0;

    while (true) {
        switch (state) {
            case ParserState::Error:
                if (errorMsg) *errorMsg = ERROR_REQUEST_IN_ERROR_STATE;
                return -1;

            case ParserState::Init: {
                int n = parseRequestLine(data.substr(totalRead), &requestLine, errorMsg);
                if (n < 0) {
                    state = ParserState::Error;
                    return -1;
                }
                if (n == 0) {
                    return totalRead; // Need more data
                }
                totalRead += n;
                state = ParserState::Done;
                break;
            }

            case ParserState::Done:
                return totalRead;
        }
    }
}
