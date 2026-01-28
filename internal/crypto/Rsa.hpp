#ifndef RSA_HPP
#define RSA_HPP

#include "BigInt.hpp"
#include <cstddef>
#include <vector>

namespace Crypto {

struct RsaPublicKey {
    BigInt n; // modulus
    BigInt e; // public exponent
};

// Parse a DER-encoded RSA public key (PKCS#1 RSAPublicKey or SubjectPublicKeyInfo)
bool rsaParsePublicKey(const unsigned char* der, size_t derLen, RsaPublicKey& key);

// PKCS#1 v1.5 encryption (Type 2 padding)
// Returns ciphertext bytes, or empty vector on error.
std::vector<unsigned char> rsaEncrypt(const RsaPublicKey& key,
                                      const unsigned char* plaintext, size_t ptLen);

// PKCS#1 v1.5 signature verification (Type 1 padding)
// hash is the SHA-256 digest (32 bytes), signature is the raw RSA signature.
bool rsaVerify(const RsaPublicKey& key,
               const unsigned char hash[32],
               const unsigned char* signature, size_t sigLen);

}

#endif
