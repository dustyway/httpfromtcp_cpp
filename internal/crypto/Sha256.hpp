#ifndef SHA256_HPP
#define SHA256_HPP

#include <string>

namespace Crypto {

void sha256(const std::string& input, unsigned char hash[32]);

std::string toHexStr(const unsigned char* bytes, size_t len);

}

#endif
