#ifndef AES_HPP
#define AES_HPP

namespace Crypto {

void aes128Encrypt(const unsigned char key[16],
                   const unsigned char in[16],
                   unsigned char out[16]);

void aes128Decrypt(const unsigned char key[16],
                   const unsigned char in[16],
                   unsigned char out[16]);

}

#endif
