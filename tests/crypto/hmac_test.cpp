#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_random.hpp>
#include <catch2/generators/catch_generators_adapters.hpp>
#include <string>
#include <cstring>
#include <openssl/hmac.h>

#include "Hmac.hpp"
#include "Sha256.hpp"

static std::string hmac_hex(const unsigned char* key, size_t keyLen,
                            const unsigned char* data, size_t dataLen) {
    unsigned char out[32];
    Crypto::hmacSha256(key, keyLen, data, dataLen, out);
    return Crypto::toHexStr(out, 32);
}

static std::string openssl_hmac_hex(const unsigned char* key, size_t keyLen,
                                    const unsigned char* data, size_t dataLen) {
    unsigned char out[32];
    unsigned int outLen = 0;
    HMAC(EVP_sha256(), key, static_cast<int>(keyLen), data, dataLen, out, &outLen);
    return Crypto::toHexStr(out, outLen);
}

// RFC 4231 test vectors
TEST_CASE("HMAC-SHA256 RFC 4231 test vectors", "[crypto][hmac]") {
    SECTION("Test Case 1") {
        unsigned char key[20];
        std::memset(key, 0x0b, 20);
        const unsigned char data[] = "Hi There";
        CHECK(hmac_hex(key, 20, data, 8)
              == "b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7");
    }

    SECTION("Test Case 2 - Jefe") {
        const unsigned char key[] = "Jefe";
        const unsigned char data[] = "what do ya want for nothing?";
        CHECK(hmac_hex(key, 4, data, 28)
              == "5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843");
    }

    SECTION("Test Case 3") {
        unsigned char key[20];
        std::memset(key, 0xaa, 20);
        unsigned char data[50];
        std::memset(data, 0xdd, 50);
        CHECK(hmac_hex(key, 20, data, 50)
              == "773ea91e36800e46854db8ebd09181a72959098b3ef8c122d9635514ced565fe");
    }

    SECTION("Test Case 4") {
        unsigned char key[25];
        for (int i = 0; i < 25; i++) key[i] = (unsigned char)(i + 1);
        unsigned char data[50];
        std::memset(data, 0xcd, 50);
        CHECK(hmac_hex(key, 25, data, 50)
              == "82558a389a443c0ea4cc819899f2083a85f0faa3e578f8077a2e3ff46729665b");
    }

    SECTION("Test Case 5 - truncated (check full)") {
        unsigned char key[20];
        std::memset(key, 0x0c, 20);
        const unsigned char data[] = "Test With Truncation";
        std::string full = hmac_hex(key, 20, data, 20);
        CHECK(full.substr(0, 32)
              == "a3b6167473100ee06e0c796c2955552b");
    }

    SECTION("Test Case 6 - long key") {
        unsigned char key[131];
        std::memset(key, 0xaa, 131);
        const unsigned char data[] = "Test Using Larger Than Block-Size Key - Hash Key First";
        CHECK(hmac_hex(key, 131, data, 54)
              == "60e431591ee0b67f0d8a26aacbf5b77f8e0bc6213728c5140546040f0ee37f54");
    }

    SECTION("Test Case 7 - long key and data") {
        unsigned char key[131];
        std::memset(key, 0xaa, 131);
        const unsigned char data[] =
            "This is a test using a larger than block-size key and a "
            "larger than block-size data. The key needs to be hashed "
            "before being used by the HMAC algorithm.";
        CHECK(hmac_hex(key, 131, data, 152)
              == "9b09ffa71b942fcb27635fbcd5b0e944bfdc63644f0713938a7f51535c3a35e2");
    }
}

TEST_CASE("HMAC-SHA256 matches OpenSSL for random inputs", "[crypto][hmac]") {
    unsigned int seed = GENERATE(take(100, random(0u, 0xFFFFFFFFu)));
    unsigned int state = seed;

    int keyLen = static_cast<int>((state * 1103515245u + 12345u) >> 16) % 256;
    state = state * 1103515245u + 12345u;
    int dataLen = static_cast<int>((state * 1103515245u + 12345u) >> 16) % 512;
    state = state * 1103515245u + 12345u;

    unsigned char key[256];
    for (int i = 0; i < keyLen; i++) {
        state = state * 1103515245u + 12345u;
        key[i] = static_cast<unsigned char>(state >> 16);
    }

    unsigned char data[512];
    for (int i = 0; i < dataLen; i++) {
        state = state * 1103515245u + 12345u;
        data[i] = static_cast<unsigned char>(state >> 16);
    }

    CAPTURE(seed, keyLen, dataLen);
    CHECK(hmac_hex(key, keyLen, data, dataLen)
          == openssl_hmac_hex(key, keyLen, data, dataLen));
}

TEST_CASE("HMAC-SHA256 edge cases", "[crypto][hmac]") {
    SECTION("empty key and data") {
        unsigned char dummy = 0;
        CHECK(hmac_hex(&dummy, 0, &dummy, 0)
              == openssl_hmac_hex(&dummy, 0, &dummy, 0));
    }

    SECTION("key exactly block size (64 bytes)") {
        unsigned char key[64];
        for (int i = 0; i < 64; i++) key[i] = (unsigned char)i;
        const unsigned char data[] = "test";
        CHECK(hmac_hex(key, 64, data, 4)
              == openssl_hmac_hex(key, 64, data, 4));
    }

    SECTION("key one byte over block size (65 bytes)") {
        unsigned char key[65];
        for (int i = 0; i < 65; i++) key[i] = (unsigned char)i;
        const unsigned char data[] = "test";
        CHECK(hmac_hex(key, 65, data, 4)
              == openssl_hmac_hex(key, 65, data, 4));
    }
}
