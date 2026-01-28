#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_random.hpp>
#include <catch2/generators/catch_generators_adapters.hpp>
#include <cstring>
#include <openssl/evp.h>

#include "Aes.hpp"
#include "Sha256.hpp"

static void openssl_aes128_ecb_encrypt(const unsigned char key[16],
                                       const unsigned char in[16],
                                       unsigned char out[16]) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(ctx, EVP_aes_128_ecb(), NULL, key, NULL);
    EVP_CIPHER_CTX_set_padding(ctx, 0);
    int outLen = 0;
    EVP_EncryptUpdate(ctx, out, &outLen, in, 16);
    EVP_CIPHER_CTX_free(ctx);
}

static void openssl_aes128_ecb_decrypt(const unsigned char key[16],
                                       const unsigned char in[16],
                                       unsigned char out[16]) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    EVP_DecryptInit_ex(ctx, EVP_aes_128_ecb(), NULL, key, NULL);
    EVP_CIPHER_CTX_set_padding(ctx, 0);
    int outLen = 0;
    EVP_DecryptUpdate(ctx, out, &outLen, in, 16);
    EVP_CIPHER_CTX_free(ctx);
}

// FIPS 197 Appendix B test vector
TEST_CASE("AES-128 FIPS 197 Appendix B", "[crypto][aes]") {
    const unsigned char key[16] = {
        0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
        0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
    };
    const unsigned char plaintext[16] = {
        0x32, 0x43, 0xf6, 0xa8, 0x88, 0x5a, 0x30, 0x8d,
        0x31, 0x31, 0x98, 0xa2, 0xe0, 0x37, 0x07, 0x34
    };
    const unsigned char expected[16] = {
        0x39, 0x25, 0x84, 0x1d, 0x02, 0xdc, 0x09, 0xfb,
        0xdc, 0x11, 0x85, 0x97, 0x19, 0x6a, 0x0b, 0x32
    };

    unsigned char ciphertext[16];
    Crypto::aes128Encrypt(key, plaintext, ciphertext);
    CHECK(std::memcmp(ciphertext, expected, 16) == 0);

    unsigned char decrypted[16];
    Crypto::aes128Decrypt(key, ciphertext, decrypted);
    CHECK(std::memcmp(decrypted, plaintext, 16) == 0);
}

// NIST SP 800-38A F.1.1 ECB-AES128.Encrypt
TEST_CASE("AES-128 NIST SP 800-38A ECB", "[crypto][aes]") {
    const unsigned char key[16] = {
        0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
        0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
    };

    SECTION("Block 1") {
        const unsigned char pt[16] = {
            0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
            0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a
        };
        const unsigned char ct[16] = {
            0x3a, 0xd7, 0x7b, 0xb4, 0x0d, 0x7a, 0x36, 0x60,
            0xa8, 0x9e, 0xca, 0xf3, 0x24, 0x66, 0xef, 0x97
        };
        unsigned char out[16];
        Crypto::aes128Encrypt(key, pt, out);
        CHECK(std::memcmp(out, ct, 16) == 0);
    }

    SECTION("Block 2") {
        const unsigned char pt[16] = {
            0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c,
            0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51
        };
        const unsigned char ct[16] = {
            0xf5, 0xd3, 0xd5, 0x85, 0x03, 0xb9, 0x69, 0x9d,
            0xe7, 0x85, 0x89, 0x5a, 0x96, 0xfd, 0xba, 0xaf
        };
        unsigned char out[16];
        Crypto::aes128Encrypt(key, pt, out);
        CHECK(std::memcmp(out, ct, 16) == 0);
    }
}

TEST_CASE("AES-128 encrypt-decrypt roundtrip matches OpenSSL", "[crypto][aes]") {
    unsigned int seed = GENERATE(take(100, random(0u, 0xFFFFFFFFu)));
    unsigned int state = seed;

    unsigned char key[16];
    unsigned char pt[16];
    for (int i = 0; i < 16; i++) {
        state = state * 1103515245u + 12345u;
        key[i] = static_cast<unsigned char>(state >> 16);
        state = state * 1103515245u + 12345u;
        pt[i] = static_cast<unsigned char>(state >> 16);
    }

    unsigned char our_ct[16], ossl_ct[16];
    Crypto::aes128Encrypt(key, pt, our_ct);
    openssl_aes128_ecb_encrypt(key, pt, ossl_ct);

    CAPTURE(seed);
    CHECK(std::memcmp(our_ct, ossl_ct, 16) == 0);

    unsigned char our_dec[16], ossl_dec[16];
    Crypto::aes128Decrypt(key, our_ct, our_dec);
    openssl_aes128_ecb_decrypt(key, ossl_ct, ossl_dec);

    CHECK(std::memcmp(our_dec, pt, 16) == 0);
    CHECK(std::memcmp(ossl_dec, pt, 16) == 0);
}
