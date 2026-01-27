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
    bool writeStatusLine(int fd, StatusCode statusCode);
    bool writeHeaders(int fd, const Headers& h);

}

#endif
