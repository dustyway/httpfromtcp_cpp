#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_random.hpp>
#include <catch2/generators/catch_generators_adapters.hpp>
#include <cstring>
#include <openssl/evp.h>

#include "Gcm.hpp"
#include "Sha256.hpp"

static bool openssl_gcm_encrypt(const unsigned char key[16],
                                const unsigned char* iv, size_t ivLen,
                                const unsigned char* aad, size_t aadLen,
                                const unsigned char* pt, size_t ptLen,
                                unsigned char* ct, unsigned char tag[16]) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(ctx, EVP_aes_128_gcm(), NULL, NULL, NULL);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, static_cast<int>(ivLen), NULL);
    EVP_EncryptInit_ex(ctx, NULL, NULL, key, iv);
    int outLen = 0;
    if (aadLen > 0) {
        EVP_EncryptUpdate(ctx, NULL, &outLen, aad, static_cast<int>(aadLen));
    }
    if (ptLen > 0) {
        EVP_EncryptUpdate(ctx, ct, &outLen, pt, static_cast<int>(ptLen));
    }
    EVP_EncryptFinal_ex(ctx, ct + outLen, &outLen);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag);
    EVP_CIPHER_CTX_free(ctx);
    return true;
}

static bool openssl_gcm_decrypt(const unsigned char key[16],
                                const unsigned char* iv, size_t ivLen,
                                const unsigned char* aad, size_t aadLen,
                                const unsigned char* ct, size_t ctLen,
                                unsigned char* pt, const unsigned char tag[16]) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    EVP_DecryptInit_ex(ctx, EVP_aes_128_gcm(), NULL, NULL, NULL);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, static_cast<int>(ivLen), NULL);
    EVP_DecryptInit_ex(ctx, NULL, NULL, key, iv);
    int outLen = 0;
    if (aadLen > 0) {
        EVP_DecryptUpdate(ctx, NULL, &outLen, aad, static_cast<int>(aadLen));
    }
    if (ctLen > 0) {
        EVP_DecryptUpdate(ctx, pt, &outLen, ct, static_cast<int>(ctLen));
    }
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, const_cast<unsigned char*>(tag));
    int ret = EVP_DecryptFinal_ex(ctx, pt + outLen, &outLen);
    EVP_CIPHER_CTX_free(ctx);
    return ret > 0;
}

// NIST SP 800-38D Test Case 1: zero-length plaintext, zero-length AAD
TEST_CASE("GCM NIST test case 1 - empty pt, empty aad", "[crypto][gcm]") {
    const unsigned char key[16] = {0};
    const unsigned char iv[12] = {0};
    unsigned char ct[1];
    unsigned char tag[16];

    Crypto::gcmEncrypt(key, iv, 12, NULL, 0, NULL, 0, ct, tag);

    const unsigned char expectedTag[16] = {
        0x58, 0xe2, 0xfc, 0xce, 0xfa, 0x7e, 0x30, 0x61,
        0x36, 0x7f, 0x1d, 0x57, 0xa4, 0xe7, 0x45, 0x5a
    };
    CHECK(std::memcmp(tag, expectedTag, 16) == 0);
}

// NIST SP 800-38D Test Case 2: 16-byte plaintext, zero-length AAD
TEST_CASE("GCM NIST test case 2 - 16B pt, empty aad", "[crypto][gcm]") {
    const unsigned char key[16] = {0};
    const unsigned char iv[12] = {0};
    const unsigned char pt[16] = {0};
    const unsigned char expectedCt[16] = {
        0x03, 0x88, 0xda, 0xce, 0x60, 0xb6, 0xa3, 0x92,
        0xf3, 0x28, 0xc2, 0xb9, 0x71, 0xb2, 0xfe, 0x78
    };
    const unsigned char expectedTag[16] = {
        0xab, 0x6e, 0x47, 0xd4, 0x2c, 0xec, 0x13, 0xbd,
        0xf5, 0x3a, 0x67, 0xb2, 0x12, 0x57, 0xbd, 0xdf
    };

    unsigned char ct[16];
    unsigned char tag[16];
    Crypto::gcmEncrypt(key, iv, 12, NULL, 0, pt, 16, ct, tag);

    CHECK(std::memcmp(ct, expectedCt, 16) == 0);
    CHECK(std::memcmp(tag, expectedTag, 16) == 0);

    // Decrypt and verify
    unsigned char decrypted[16];
    CHECK(Crypto::gcmDecrypt(key, iv, 12, NULL, 0, ct, 16, decrypted, tag) == true);
    CHECK(std::memcmp(decrypted, pt, 16) == 0);
}

// NIST SP 800-38D Test Case 3
TEST_CASE("GCM NIST test case 3", "[crypto][gcm]") {
    const unsigned char key[16] = {
        0xfe, 0xff, 0xe9, 0x92, 0x86, 0x65, 0x73, 0x1c,
        0x6d, 0x6a, 0x8f, 0x94, 0x67, 0x30, 0x83, 0x08
    };
    const unsigned char iv[12] = {
        0xca, 0xfe, 0xba, 0xbe, 0xfa, 0xce, 0xdb, 0xad,
        0xde, 0xca, 0xf8, 0x88
    };
    const unsigned char pt[64] = {
        0xd9, 0x31, 0x32, 0x25, 0xf8, 0x84, 0x06, 0xe5,
        0xa5, 0x59, 0x09, 0xc5, 0xaf, 0xf5, 0x26, 0x9a,
        0x86, 0xa7, 0xa9, 0x53, 0x15, 0x34, 0xf7, 0xda,
        0x2e, 0x4c, 0x30, 0x3d, 0x8a, 0x31, 0x8a, 0x72,
        0x1c, 0x3c, 0x0c, 0x95, 0x95, 0x68, 0x09, 0x53,
        0x2f, 0xcf, 0x0e, 0x24, 0x49, 0xa6, 0xb5, 0x25,
        0xb1, 0x6a, 0xed, 0xf5, 0xaa, 0x0d, 0xe6, 0x57,
        0xba, 0x63, 0x7b, 0x39, 0x1a, 0xaf, 0xd2, 0x55
    };
    const unsigned char expectedCt[64] = {
        0x42, 0x83, 0x1e, 0xc2, 0x21, 0x77, 0x74, 0x24,
        0x4b, 0x72, 0x21, 0xb7, 0x84, 0xd0, 0xd4, 0x9c,
        0xe3, 0xaa, 0x21, 0x2f, 0x2c, 0x02, 0xa4, 0xe0,
        0x35, 0xc1, 0x7e, 0x23, 0x29, 0xac, 0xa1, 0x2e,
        0x21, 0xd5, 0x14, 0xb2, 0x54, 0x66, 0x93, 0x1c,
        0x7d, 0x8f, 0x6a, 0x5a, 0xac, 0x84, 0xaa, 0x05,
        0x1b, 0xa3, 0x0b, 0x39, 0x6a, 0x0a, 0xac, 0x97,
        0x3d, 0x58, 0xe0, 0x91, 0x47, 0x3f, 0x59, 0x85
    };
    const unsigned char expectedTag[16] = {
        0x4d, 0x5c, 0x2a, 0xf3, 0x27, 0xcd, 0x64, 0xa6,
        0x2c, 0xf3, 0x5a, 0xbd, 0x2b, 0xa6, 0xfa, 0xb4
    };

    unsigned char ct[64];
    unsigned char tag[16];
    Crypto::gcmEncrypt(key, iv, 12, NULL, 0, pt, 64, ct, tag);

    CHECK(std::memcmp(ct, expectedCt, 64) == 0);
    CHECK(std::memcmp(tag, expectedTag, 16) == 0);
}

// NIST SP 800-38D Test Case 4 (same key/IV as TC3, with AAD)
TEST_CASE("GCM NIST test case 4 - with AAD", "[crypto][gcm]") {
    const unsigned char key[16] = {
        0xfe, 0xff, 0xe9, 0x92, 0x86, 0x65, 0x73, 0x1c,
        0x6d, 0x6a, 0x8f, 0x94, 0x67, 0x30, 0x83, 0x08
    };
    const unsigned char iv[12] = {
        0xca, 0xfe, 0xba, 0xbe, 0xfa, 0xce, 0xdb, 0xad,
        0xde, 0xca, 0xf8, 0x88
    };
    const unsigned char aad[20] = {
        0xfe, 0xed, 0xfa, 0xce, 0xde, 0xad, 0xbe, 0xef,
        0xfe, 0xed, 0xfa, 0xce, 0xde, 0xad, 0xbe, 0xef,
        0xab, 0xad, 0xda, 0xd2
    };
    const unsigned char pt[60] = {
        0xd9, 0x31, 0x32, 0x25, 0xf8, 0x84, 0x06, 0xe5,
        0xa5, 0x59, 0x09, 0xc5, 0xaf, 0xf5, 0x26, 0x9a,
        0x86, 0xa7, 0xa9, 0x53, 0x15, 0x34, 0xf7, 0xda,
        0x2e, 0x4c, 0x30, 0x3d, 0x8a, 0x31, 0x8a, 0x72,
        0x1c, 0x3c, 0x0c, 0x95, 0x95, 0x68, 0x09, 0x53,
        0x2f, 0xcf, 0x0e, 0x24, 0x49, 0xa6, 0xb5, 0x25,
        0xb1, 0x6a, 0xed, 0xf5, 0xaa, 0x0d, 0xe6, 0x57,
        0xba, 0x63, 0x7b, 0x39
    };
    const unsigned char expectedCt[60] = {
        0x42, 0x83, 0x1e, 0xc2, 0x21, 0x77, 0x74, 0x24,
        0x4b, 0x72, 0x21, 0xb7, 0x84, 0xd0, 0xd4, 0x9c,
        0xe3, 0xaa, 0x21, 0x2f, 0x2c, 0x02, 0xa4, 0xe0,
        0x35, 0xc1, 0x7e, 0x23, 0x29, 0xac, 0xa1, 0x2e,
        0x21, 0xd5, 0x14, 0xb2, 0x54, 0x66, 0x93, 0x1c,
        0x7d, 0x8f, 0x6a, 0x5a, 0xac, 0x84, 0xaa, 0x05,
        0x1b, 0xa3, 0x0b, 0x39, 0x6a, 0x0a, 0xac, 0x97,
        0x3d, 0x58, 0xe0, 0x91
    };
    const unsigned char expectedTag[16] = {
        0x5b, 0xc9, 0x4f, 0xbc, 0x32, 0x21, 0xa5, 0xdb,
        0x94, 0xfa, 0xe9, 0x5a, 0xe7, 0x12, 0x1a, 0x47
    };

    unsigned char ct[60];
    unsigned char tag[16];
    Crypto::gcmEncrypt(key, iv, 12, aad, 20, pt, 60, ct, tag);

    CHECK(std::memcmp(ct, expectedCt, 60) == 0);
    CHECK(std::memcmp(tag, expectedTag, 16) == 0);

    // Decrypt roundtrip
    unsigned char decrypted[60];
    CHECK(Crypto::gcmDecrypt(key, iv, 12, aad, 20, ct, 60, decrypted, tag) == true);
    CHECK(std::memcmp(decrypted, pt, 60) == 0);
}

TEST_CASE("GCM tag mismatch rejection", "[crypto][gcm]") {
    const unsigned char key[16] = {0};
    const unsigned char iv[12] = {0};
    const unsigned char pt[16] = {0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
                                   0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50};
    unsigned char ct[16];
    unsigned char tag[16];
    Crypto::gcmEncrypt(key, iv, 12, NULL, 0, pt, 16, ct, tag);

    // Flip one bit in the tag
    unsigned char badTag[16];
    std::memcpy(badTag, tag, 16);
    badTag[0] ^= 0x01;

    unsigned char decrypted[16];
    CHECK(Crypto::gcmDecrypt(key, iv, 12, NULL, 0, ct, 16, decrypted, badTag) == false);

    // Flip one bit in the ciphertext
    unsigned char badCt[16];
    std::memcpy(badCt, ct, 16);
    badCt[0] ^= 0x01;
    CHECK(Crypto::gcmDecrypt(key, iv, 12, NULL, 0, badCt, 16, decrypted, tag) == false);
}

TEST_CASE("GCM matches OpenSSL for random inputs", "[crypto][gcm]") {
    unsigned int seed = GENERATE(take(50, random(0u, 0xFFFFFFFFu)));
    unsigned int state = seed;

    unsigned char key[16];
    unsigned char iv[12];
    for (int i = 0; i < 16; i++) {
        state = state * 1103515245u + 12345u;
        key[i] = static_cast<unsigned char>(state >> 16);
    }
    for (int i = 0; i < 12; i++) {
        state = state * 1103515245u + 12345u;
        iv[i] = static_cast<unsigned char>(state >> 16);
    }

    state = state * 1103515245u + 12345u;
    int aadLen = static_cast<int>((state >> 16) % 64);
    state = state * 1103515245u + 12345u;
    int ptLen = static_cast<int>((state >> 16) % 128);

    unsigned char aad[64];
    for (int i = 0; i < aadLen; i++) {
        state = state * 1103515245u + 12345u;
        aad[i] = static_cast<unsigned char>(state >> 16);
    }
    unsigned char pt[128];
    for (int i = 0; i < ptLen; i++) {
        state = state * 1103515245u + 12345u;
        pt[i] = static_cast<unsigned char>(state >> 16);
    }

    unsigned char ourCt[128], osslCt[128];
    unsigned char ourTag[16], osslTag[16];

    Crypto::gcmEncrypt(key, iv, 12, aad, aadLen, pt, ptLen, ourCt, ourTag);
    openssl_gcm_encrypt(key, iv, 12, aad, aadLen, pt, ptLen, osslCt, osslTag);

    CAPTURE(seed, aadLen, ptLen);
    CHECK(std::memcmp(ourCt, osslCt, ptLen) == 0);
    CHECK(std::memcmp(ourTag, osslTag, 16) == 0);

    // Verify our decrypt works on OpenSSL-encrypted data
    unsigned char decrypted[128];
    CHECK(Crypto::gcmDecrypt(key, iv, 12, aad, aadLen, osslCt, ptLen, decrypted, osslTag) == true);
    CHECK(std::memcmp(decrypted, pt, ptLen) == 0);
}
