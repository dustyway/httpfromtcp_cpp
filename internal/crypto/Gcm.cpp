#include "Gcm.hpp"
#include "Aes.hpp"
#include <cstring>

static void xorBlock(unsigned char* dst, const unsigned char* src, size_t len) {
    for (size_t i = 0; i < len; i++) dst[i] ^= src[i];
}

// GF(2^128) multiplication with the irreducible polynomial x^128 + x^7 + x^2 + x + 1.
// Both operands and result are 16-byte blocks in big-endian bit order.
static void gfMul(const unsigned char X[16], const unsigned char Y[16], unsigned char out[16]) {
    unsigned char V[16];
    std::memcpy(V, Y, 16);
    std::memset(out, 0, 16);

    for (int i = 0; i < 128; i++) {
        if (X[i / 8] & (1 << (7 - (i % 8)))) {
            xorBlock(out, V, 16);
        }
        unsigned char lsb = V[15] & 1;
        // right shift V by 1
        for (int j = 15; j > 0; j--) {
            V[j] = (V[j] >> 1) | (V[j-1] << 7);
        }
        V[0] >>= 1;
        if (lsb) {
            V[0] ^= 0xe1; // reduction polynomial: x^128 + x^7 + x^2 + x + 1
        }
    }
}

// GHASH: compute the GHASH over the given data using hash subkey H
static void ghash(const unsigned char H[16], const unsigned char* data, size_t dataLen,
                  unsigned char out[16]) {
    std::memset(out, 0, 16);
    size_t blocks = dataLen / 16;
    for (size_t i = 0; i < blocks; i++) {
        xorBlock(out, data + i * 16, 16);
        unsigned char tmp[16];
        gfMul(out, H, tmp);
        std::memcpy(out, tmp, 16);
    }
}

// Increment the rightmost 32 bits of a 16-byte counter block
static void incr(unsigned char block[16]) {
    for (int i = 15; i >= 12; i--) {
        if (++block[i] != 0) break;
    }
}

// GCTR: AES-CTR encryption/decryption
static void gctr(const unsigned char key[16], const unsigned char icb[16],
                 const unsigned char* input, size_t len, unsigned char* output) {
    if (len == 0) return;

    unsigned char cb[16];
    std::memcpy(cb, icb, 16);

    size_t fullBlocks = len / 16;
    for (size_t i = 0; i < fullBlocks; i++) {
        unsigned char ekey[16];
        Crypto::aes128Encrypt(key, cb, ekey);
        for (size_t j = 0; j < 16; j++) {
            output[i * 16 + j] = input[i * 16 + j] ^ ekey[j];
        }
        incr(cb);
    }

    size_t rem = len % 16;
    if (rem > 0) {
        unsigned char ekey[16];
        Crypto::aes128Encrypt(key, cb, ekey);
        for (size_t j = 0; j < rem; j++) {
            output[fullBlocks * 16 + j] = input[fullBlocks * 16 + j] ^ ekey[j];
        }
    }
}

// Build the GHASH input: pad AAD to 16-byte boundary, pad ciphertext to 16-byte boundary,
// then append 8-byte big-endian bit lengths of AAD and ciphertext.
static unsigned char* buildGhashInput(const unsigned char* aad, size_t aadLen,
                                      const unsigned char* ct, size_t ctLen,
                                      size_t* outLen) {
    size_t aadPadded = ((aadLen + 15) / 16) * 16;
    size_t ctPadded = ((ctLen + 15) / 16) * 16;
    size_t totalLen = aadPadded + ctPadded + 16;
    *outLen = totalLen;

    unsigned char* buf = new unsigned char[totalLen];
    std::memset(buf, 0, totalLen);
    if (aadLen > 0) std::memcpy(buf, aad, aadLen);
    if (ctLen > 0) std::memcpy(buf + aadPadded, ct, ctLen);

    // length block: bit lengths as 64-bit big-endian
    unsigned long long aadBits = (unsigned long long)aadLen * 8;
    unsigned long long ctBits = (unsigned long long)ctLen * 8;
    size_t off = aadPadded + ctPadded;
    for (int i = 7; i >= 0; i--) {
        buf[off + (7 - i)] = (unsigned char)(aadBits >> (i * 8));
        buf[off + 8 + (7 - i)] = (unsigned char)(ctBits >> (i * 8));
    }
    return buf;
}

bool Crypto::gcmEncrypt(const unsigned char key[16],
                        const unsigned char* iv, size_t ivLen,
                        const unsigned char* aad, size_t aadLen,
                        const unsigned char* plaintext, size_t ptLen,
                        unsigned char* ciphertext,
                        unsigned char tag[16]) {
    // Compute hash subkey H = AES_K(0^128)
    unsigned char H[16];
    unsigned char zeros[16];
    std::memset(zeros, 0, 16);
    aes128Encrypt(key, zeros, H);

    // Compute J0 (pre-counter block)
    unsigned char J0[16];
    if (ivLen == 12) {
        std::memcpy(J0, iv, 12);
        J0[12] = 0; J0[13] = 0; J0[14] = 0; J0[15] = 1;
    } else {
        size_t s = ((ivLen + 15) / 16) * 16;
        size_t ghashIvLen = s + 16;
        unsigned char* ghashIv = new unsigned char[ghashIvLen];
        std::memset(ghashIv, 0, ghashIvLen);
        std::memcpy(ghashIv, iv, ivLen);
        unsigned long long ivBits = (unsigned long long)ivLen * 8;
        for (int i = 7; i >= 0; i--) {
            ghashIv[ghashIvLen - 1 - i] = (unsigned char)(ivBits >> (i * 8));
        }
        ghash(H, ghashIv, ghashIvLen, J0);
        delete[] ghashIv;
    }

    // Encrypt: C = GCTR_K(inc32(J0), P)
    unsigned char J0inc[16];
    std::memcpy(J0inc, J0, 16);
    incr(J0inc);
    gctr(key, J0inc, plaintext, ptLen, ciphertext);

    // Compute tag: T = GCTR_K(J0, GHASH_H(A || 0* || C || 0* || len(A) || len(C)))
    size_t ghashDataLen = 0;
    unsigned char* ghashData = buildGhashInput(aad, aadLen, ciphertext, ptLen, &ghashDataLen);
    unsigned char S[16];
    ghash(H, ghashData, ghashDataLen, S);
    delete[] ghashData;

    gctr(key, J0, S, 16, tag);
    return true;
}

bool Crypto::gcmDecrypt(const unsigned char key[16],
                        const unsigned char* iv, size_t ivLen,
                        const unsigned char* aad, size_t aadLen,
                        const unsigned char* ciphertext, size_t ctLen,
                        unsigned char* plaintext,
                        const unsigned char tag[16]) {
    // Compute hash subkey H = AES_K(0^128)
    unsigned char H[16];
    unsigned char zeros[16];
    std::memset(zeros, 0, 16);
    aes128Encrypt(key, zeros, H);

    // Compute J0
    unsigned char J0[16];
    if (ivLen == 12) {
        std::memcpy(J0, iv, 12);
        J0[12] = 0; J0[13] = 0; J0[14] = 0; J0[15] = 1;
    } else {
        size_t s = ((ivLen + 15) / 16) * 16;
        size_t ghashIvLen = s + 16;
        unsigned char* ghashIv = new unsigned char[ghashIvLen];
        std::memset(ghashIv, 0, ghashIvLen);
        std::memcpy(ghashIv, iv, ivLen);
        unsigned long long ivBits = (unsigned long long)ivLen * 8;
        for (int i = 7; i >= 0; i--) {
            ghashIv[ghashIvLen - 1 - i] = (unsigned char)(ivBits >> (i * 8));
        }
        ghash(H, ghashIv, ghashIvLen, J0);
        delete[] ghashIv;
    }

    // Compute expected tag: T' = GCTR_K(J0, GHASH_H(A || 0* || C || 0* || len(A) || len(C)))
    size_t ghashDataLen = 0;
    unsigned char* ghashData = buildGhashInput(aad, aadLen, ciphertext, ctLen, &ghashDataLen);
    unsigned char S[16];
    ghash(H, ghashData, ghashDataLen, S);
    delete[] ghashData;

    unsigned char expectedTag[16];
    gctr(key, J0, S, 16, expectedTag);

    // Constant-time tag comparison
    unsigned char diff = 0;
    for (int i = 0; i < 16; i++) diff |= tag[i] ^ expectedTag[i];
    if (diff != 0) return false;

    // Decrypt: P = GCTR_K(inc32(J0), C)
    unsigned char J0inc[16];
    std::memcpy(J0inc, J0, 16);
    incr(J0inc);
    gctr(key, J0inc, ciphertext, ctLen, plaintext);

    return true;
}
