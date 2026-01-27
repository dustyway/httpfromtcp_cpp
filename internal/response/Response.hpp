#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include "Headers.hpp"

namespace Response {

    enum StatusCode {
        StatusOk = 200,
        StatusBadRequest = 400,
        StatusInternalServerError = 500
    };

    Headers getDefaultHeaders(int contentLen);

    class Writer {
    public:
        Writer(int fd);

        bool writeStatusLine(StatusCode statusCode);
        bool writeHeaders(const Headers& h);
        bool writeBody(const char* data, size_t len);
        bool writeChunkedBody(const char* data, size_t len);
        bool writeChunkedBodyDone();

    private:
        int fd;
    };

}

#endif
