#include "Request.hpp"

const char* Request::ERROR_MALFORMED_REQUEST_LINE = "malformed request-line";
const char* Request::SEPARATOR = "\r\n";

bool Request::parseRequestLine(const std::string& buffer,
                               Request* req,
                               std::string& remaining,
                               std::string* errorMsg) {
    std::string::size_type idx = buffer.find(SEPARATOR);
    if (idx == std::string::npos) {
        remaining = buffer;
        return false;
    }

    std::string startLine = buffer.substr(0, idx);
    remaining = buffer.substr(idx + 2); // Skip \r\n

    // Split by spaces - need exactly 3 parts
    std::string::size_type firstSpace = startLine.find(' ');
    if (firstSpace == std::string::npos) {
        if (errorMsg) *errorMsg = ERROR_MALFORMED_REQUEST_LINE;
        return false;
    }

    std::string::size_type secondSpace = startLine.find(' ', firstSpace + 1);
    if (secondSpace == std::string::npos) {
        if (errorMsg) *errorMsg = ERROR_MALFORMED_REQUEST_LINE;
        return false;
    }

    // Check no more spaces after secondSpace
    if (startLine.find(' ', secondSpace + 1) != std::string::npos) {
        if (errorMsg) *errorMsg = ERROR_MALFORMED_REQUEST_LINE;
        return false;
    }

    req->method = startLine.substr(0, firstSpace);
    req->requestTarget = startLine.substr(firstSpace + 1, secondSpace - firstSpace - 1);
    std::string httpVersionFull = startLine.substr(secondSpace + 1);

    // Validate HTTP version format: HTTP/1.1
    std::string::size_type slashPos = httpVersionFull.find('/');
    if (slashPos == std::string::npos) {
        if (errorMsg) *errorMsg = ERROR_MALFORMED_REQUEST_LINE;
        return false;
    }

    std::string httpPrefix = httpVersionFull.substr(0, slashPos);
    std::string httpVersion = httpVersionFull.substr(slashPos + 1);

    if (httpPrefix != "HTTP" || httpVersion != "1.1") {
        if (errorMsg) *errorMsg = ERROR_MALFORMED_REQUEST_LINE;
        return false;
    }

    req->httpVersion = httpVersion;
    return true;
}

Request* Request::parse(const std::string& data, std::string* errorMsg) {
    Request* req = new Request();
    std::string remaining;

    if (!parseRequestLine(data, req, remaining, errorMsg)) {
        delete req;
        return NULL;
    }

    return req;
}
