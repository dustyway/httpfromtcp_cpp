#include "Hmac.hpp"
#include "Sha256.hpp"
#include <cstring>

void Crypto::hmacSha256(const unsigned char* key, size_t keyLen,
                        const unsigned char* data, size_t dataLen,
                        unsigned char out[32]) {
    static const size_t BLOCK_SIZE = 64;

    unsigned char keyBlock[64];
    std::memset(keyBlock, 0, BLOCK_SIZE);

    if (keyLen > BLOCK_SIZE) {
        sha256(key, keyLen, keyBlock);
    } else {
        std::memcpy(keyBlock, key, keyLen);
    }

    unsigned char iPad[64];
    unsigned char oPad[64];
    for (size_t i = 0; i < BLOCK_SIZE; i++) {
        iPad[i] = keyBlock[i] ^ 0x36;
        oPad[i] = keyBlock[i] ^ 0x5c;
    }

    // inner hash: SHA256(iPad || data)
    unsigned char* inner = new unsigned char[BLOCK_SIZE + dataLen];
    std::memcpy(inner, iPad, BLOCK_SIZE);
    if (dataLen > 0) {
        std::memcpy(inner + BLOCK_SIZE, data, dataLen);
    }
    unsigned char innerHash[32];
    sha256(inner, BLOCK_SIZE + dataLen, innerHash);
    delete[] inner;

    // outer hash: SHA256(oPad || innerHash)
    unsigned char outer[64 + 32];
    std::memcpy(outer, oPad, BLOCK_SIZE);
    std::memcpy(outer + BLOCK_SIZE, innerHash, 32);
    sha256(outer, BLOCK_SIZE + 32, out);
}
