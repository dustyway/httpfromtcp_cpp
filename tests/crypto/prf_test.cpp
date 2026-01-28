#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_random.hpp>
#include <catch2/generators/catch_generators_adapters.hpp>
#include <cstring>
#include <openssl/kdf.h>
#include <openssl/evp.h>

#include "Prf.hpp"
#include "Sha256.hpp"

static void openssl_tls12_prf(const unsigned char* secret, size_t secretLen,
                              const std::string& label,
                              const unsigned char* seed, size_t seedLen,
                              unsigned char* out, size_t outLen) {
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_TLS1_PRF, NULL);
    EVP_PKEY_derive_init(ctx);
    EVP_PKEY_CTX_set_tls1_prf_md(ctx, EVP_sha256());
    EVP_PKEY_CTX_set1_tls1_prf_secret(ctx, secret, static_cast<int>(secretLen));

    // OpenSSL takes label + seed concatenated as the "seed" parameter
    size_t combinedLen = label.size() + seedLen;
    unsigned char* combined = new unsigned char[combinedLen];
    std::memcpy(combined, label.data(), label.size());
    if (seedLen > 0) {
        std::memcpy(combined + label.size(), seed, seedLen);
    }
    EVP_PKEY_CTX_add1_tls1_prf_seed(ctx, combined, static_cast<int>(combinedLen));

    size_t deriveLen = outLen;
    EVP_PKEY_derive(ctx, out, &deriveLen);

    delete[] combined;
    EVP_PKEY_CTX_free(ctx);
}

TEST_CASE("TLS PRF basic output", "[crypto][prf]") {
    // Simple known-input test: verify we get deterministic output
    const unsigned char secret[] = "test secret";
    const unsigned char seed[] = "test seed";
    unsigned char out1[48], out2[48];

    Crypto::prf(secret, 11, "test label", seed, 9, out1, 48);
    Crypto::prf(secret, 11, "test label", seed, 9, out2, 48);

    CHECK(std::memcmp(out1, out2, 48) == 0);
}

TEST_CASE("TLS PRF matches OpenSSL", "[crypto][prf]") {
    SECTION("master secret derivation size") {
        const unsigned char secret[48] = {
            0x03, 0x01, 0x42, 0x53, 0x61, 0x72, 0x83, 0x94,
            0xa5, 0xb6, 0xc7, 0xd8, 0xe9, 0xfa, 0x0b, 0x1c,
            0x2d, 0x3e, 0x4f, 0x50, 0x61, 0x72, 0x83, 0x94,
            0xa5, 0xb6, 0xc7, 0xd8, 0xe9, 0xfa, 0x0b, 0x1c,
            0x2d, 0x3e, 0x4f, 0x50, 0x61, 0x72, 0x83, 0x94,
            0xa5, 0xb6, 0xc7, 0xd8, 0xe9, 0xfa, 0x0b, 0x1c
        };
        const unsigned char seed[64] = {
            0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
            0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
            0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
            0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20,
            0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
            0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30,
            0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38,
            0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40
        };

        unsigned char ourOut[48], osslOut[48];
        Crypto::prf(secret, 48, "master secret", seed, 64, ourOut, 48);
        openssl_tls12_prf(secret, 48, "master secret", seed, 64, osslOut, 48);
        CHECK(std::memcmp(ourOut, osslOut, 48) == 0);
    }

    SECTION("key expansion size") {
        const unsigned char secret[48] = {
            0xAB, 0xCD, 0xEF, 0x01, 0x23, 0x45, 0x67, 0x89,
            0xAB, 0xCD, 0xEF, 0x01, 0x23, 0x45, 0x67, 0x89,
            0xAB, 0xCD, 0xEF, 0x01, 0x23, 0x45, 0x67, 0x89,
            0xAB, 0xCD, 0xEF, 0x01, 0x23, 0x45, 0x67, 0x89,
            0xAB, 0xCD, 0xEF, 0x01, 0x23, 0x45, 0x67, 0x89,
            0xAB, 0xCD, 0xEF, 0x01, 0x23, 0x45, 0x67, 0x89
        };
        const unsigned char seed[64] = {0};

        unsigned char ourOut[72], osslOut[72];
        Crypto::prf(secret, 48, "key expansion", seed, 64, ourOut, 72);
        openssl_tls12_prf(secret, 48, "key expansion", seed, 64, osslOut, 72);
        CHECK(std::memcmp(ourOut, osslOut, 72) == 0);
    }
}

TEST_CASE("TLS PRF matches OpenSSL for random inputs", "[crypto][prf]") {
    unsigned int seed = GENERATE(take(30, random(0u, 0xFFFFFFFFu)));
    unsigned int state = seed;

    unsigned char secret[64];
    int secretLen = 32 + static_cast<int>((state * 1103515245u + 12345u) >> 16) % 32;
    state = state * 1103515245u + 12345u;
    for (int i = 0; i < secretLen; i++) {
        state = state * 1103515245u + 12345u;
        secret[i] = static_cast<unsigned char>(state >> 16);
    }

    unsigned char prfSeed[64];
    for (int i = 0; i < 64; i++) {
        state = state * 1103515245u + 12345u;
        prfSeed[i] = static_cast<unsigned char>(state >> 16);
    }

    int outLen = 32 + static_cast<int>((state * 1103515245u + 12345u) >> 16) % 96;
    state = state * 1103515245u + 12345u;

    unsigned char ourOut[128], osslOut[128];
    Crypto::prf(secret, secretLen, "test label", prfSeed, 64, ourOut, outLen);
    openssl_tls12_prf(secret, secretLen, "test label", prfSeed, 64, osslOut, outLen);

    CAPTURE(seed, secretLen, outLen);
    CHECK(std::memcmp(ourOut, osslOut, outLen) == 0);
}

TEST_CASE("TLS PRF different labels produce different output", "[crypto][prf]") {
    const unsigned char secret[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                                       0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10};
    const unsigned char prfSeed[8] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};

    unsigned char out1[32], out2[32];
    Crypto::prf(secret, 16, "label A", prfSeed, 8, out1, 32);
    Crypto::prf(secret, 16, "label B", prfSeed, 8, out2, 32);
    CHECK(std::memcmp(out1, out2, 32) != 0);
}
