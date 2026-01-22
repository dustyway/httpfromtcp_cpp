#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <string>

class Request {
public:
    std::string method;
    std::string requestTarget;
    std::string httpVersion;

    static const char* ERROR_MALFORMED_REQUEST_LINE;

    // Parse a request from raw data
    // Returns NULL on error, sets errorMsg if provided
    static Request* parse(const std::string& data, std::string* errorMsg = NULL);

private:
    static const char* SEPARATOR;

    // Parse request line, returns false on error
    // Sets remaining to the rest of the buffer after the request line
    static bool parseRequestLine(const std::string& buffer,
                                 Request* req,
                                 std::string& remaining,
                                 std::string* errorMsg);
};


#endif