#ifndef GCM_HPP
#define GCM_HPP

#include <cstddef>

namespace Crypto {

bool gcmEncrypt(const unsigned char key[16],
                const unsigned char* iv, size_t ivLen,
                const unsigned char* aad, size_t aadLen,
                const unsigned char* plaintext, size_t ptLen,
                unsigned char* ciphertext,
                unsigned char tag[16]);

bool gcmDecrypt(const unsigned char key[16],
                const unsigned char* iv, size_t ivLen,
                const unsigned char* aad, size_t aadLen,
                const unsigned char* ciphertext, size_t ctLen,
                unsigned char* plaintext,
                const unsigned char tag[16]);

}

#endif
