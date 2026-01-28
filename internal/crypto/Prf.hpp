#ifndef PRF_HPP
#define PRF_HPP

#include <cstddef>
#include <string>

namespace Crypto {

void prf(const unsigned char* secret, size_t secretLen,
         const std::string& label,
         const unsigned char* seed, size_t seedLen,
         unsigned char* out, size_t outLen);

}

#endif
