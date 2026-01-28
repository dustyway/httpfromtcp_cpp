#include "Prf.hpp"
#include "Hmac.hpp"
#include <cstring>

// TLS 1.2 PRF per RFC 5246 Section 5:
// PRF(secret, label, seed) = P_SHA256(secret, label + seed)
// P_hash(secret, seed) = HMAC_hash(secret, A(1) + seed) +
//                        HMAC_hash(secret, A(2) + seed) + ...
// A(0) = seed
// A(i) = HMAC_hash(secret, A(i-1))

void Crypto::prf(const unsigned char* secret, size_t secretLen,
                 const std::string& label,
                 const unsigned char* seed, size_t seedLen,
                 unsigned char* out, size_t outLen) {
    // Construct label + seed
    size_t labelLen = label.size();
    size_t combinedLen = labelLen + seedLen;
    unsigned char* combined = new unsigned char[combinedLen];
    std::memcpy(combined, label.data(), labelLen);
    if (seedLen > 0) {
        std::memcpy(combined + labelLen, seed, seedLen);
    }

    // A(0) = combined (label + seed)
    // A(1) = HMAC(secret, A(0))
    unsigned char A[32]; // A(i), always 32 bytes (SHA-256 output)
    hmacSha256(secret, secretLen, combined, combinedLen, A);

    size_t generated = 0;
    while (generated < outLen) {
        // P_hash chunk = HMAC(secret, A(i) + seed)
        unsigned char* aConcat = new unsigned char[32 + combinedLen];
        std::memcpy(aConcat, A, 32);
        std::memcpy(aConcat + 32, combined, combinedLen);

        unsigned char chunk[32];
        hmacSha256(secret, secretLen, aConcat, 32 + combinedLen, chunk);
        delete[] aConcat;

        size_t toCopy = outLen - generated;
        if (toCopy > 32) toCopy = 32;
        std::memcpy(out + generated, chunk, toCopy);
        generated += toCopy;

        // A(i+1) = HMAC(secret, A(i))
        unsigned char nextA[32];
        hmacSha256(secret, secretLen, A, 32, nextA);
        std::memcpy(A, nextA, 32);
    }

    delete[] combined;
}
