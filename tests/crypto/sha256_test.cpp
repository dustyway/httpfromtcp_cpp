#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_random.hpp>
#include <string>
#include <openssl/sha.h>

#include "Sha256.hpp"
#include "catch2/generators/catch_generators_adapters.hpp"

static std::string openssl_sha256(const std::string& input) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(input.c_str()),
           input.size(), hash);
    return Crypto::toHexStr(hash, SHA256_DIGEST_LENGTH);
}

static std::string our_sha256(const std::string& input) {
    unsigned char hash[32];
    Crypto::sha256(input, hash);
    return Crypto::toHexStr(hash, 32);
}

TEST_CASE("SHA256 known vectors", "[crypto][sha256]") {
    CHECK(our_sha256("")
          == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    CHECK(our_sha256("abc")
          == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    CHECK(our_sha256("hello world")
          == "b94d27b9934d3e08a52e52d7da7dabfac484efe37a5380ee9088f7ace2efcde9");
    CHECK(our_sha256("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq")
          == "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
}

TEST_CASE("SHA256 matches OpenSSL for random inputs", "[crypto][sha256]") {
    unsigned int seed = GENERATE(take(100, random(0u, 0xFFFFFFFFu)));
    int len = static_cast<int>(seed % 512);

    std::string input(len, '\0');
    unsigned int state = seed;
    for (int i = 0; i < len; i++) {
        state = state * 1103515245u + 12345u;
        input[i] = static_cast<char>(state >> 16);
    }

    CAPTURE(seed, len);
    CHECK(our_sha256(input) == openssl_sha256(input));
}

TEST_CASE("SHA256 matches OpenSSL at block boundaries", "[crypto][sha256]") {
    int len = GENERATE(0, 1, 55, 56, 57, 63, 64, 65, 119, 120, 127, 128, 129, 256);

    unsigned int state = static_cast<unsigned int>(len) * 2654435761u;
    std::string input(len, '\0');
    for (int i = 0; i < len; i++) {
        state = state * 1103515245u + 12345u;
        input[i] = static_cast<char>(state >> 16);
    }

    CAPTURE(len);
    CHECK(our_sha256(input) == openssl_sha256(input));
}

TEST_CASE("toHexStr produces correct output", "[crypto][sha256]") {
    unsigned char bytes[] = {0x00, 0x01, 0x0a, 0x0f, 0x10, 0xff, 0xab, 0xcd};
    CHECK(Crypto::toHexStr(bytes, 8) == "00010a0f10ffabcd");
    CHECK(Crypto::toHexStr(bytes, 0).empty());
    CHECK(Crypto::toHexStr(bytes, 1) == "00");
}
