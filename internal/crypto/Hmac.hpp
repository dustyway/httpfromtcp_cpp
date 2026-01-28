#ifndef HMAC_HPP
#define HMAC_HPP

#include <cstddef>

namespace Crypto {

void hmacSha256(const unsigned char* key, size_t keyLen,
                const unsigned char* data, size_t dataLen,
                unsigned char out[32]);

}

#endif
