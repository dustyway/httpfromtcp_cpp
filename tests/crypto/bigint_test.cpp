#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_random.hpp>
#include <catch2/generators/catch_generators_adapters.hpp>
#include <cstring>
#include <openssl/bn.h>

#include "BigInt.hpp"
#include "Sha256.hpp"

using Crypto::BigInt;

TEST_CASE("BigInt basic construction", "[crypto][bigint]") {
    CHECK(BigInt().isZero());
    CHECK(BigInt(0).isZero());
    CHECK(!BigInt(1).isZero());
    CHECK(!BigInt(0xFFFFFFFFu).isZero());
}

TEST_CASE("BigInt comparison", "[crypto][bigint]") {
    CHECK(BigInt(0) == BigInt(0));
    CHECK(BigInt(42) == BigInt(42));
    CHECK(BigInt(1) < BigInt(2));
    CHECK(BigInt(2) > BigInt(1));
    CHECK(BigInt(1) != BigInt(2));
    CHECK(BigInt(5) <= BigInt(5));
    CHECK(BigInt(5) >= BigInt(5));
}

TEST_CASE("BigInt addition", "[crypto][bigint]") {
    CHECK(BigInt(0) + BigInt(0) == BigInt(0));
    CHECK(BigInt(1) + BigInt(1) == BigInt(2));
    CHECK(BigInt(100) + BigInt(200) == BigInt(300));

    // Carry across limb boundary
    BigInt a(0xFFFFFFFFu);
    BigInt b(1);
    BigInt sum = a + b;
    CHECK(sum == BigInt::fromHex("100000000"));
}

TEST_CASE("BigInt subtraction", "[crypto][bigint]") {
    CHECK(BigInt(5) - BigInt(3) == BigInt(2));
    CHECK(BigInt(100) - BigInt(100) == BigInt(0));

    // Borrow across limb boundary
    BigInt a = BigInt::fromHex("100000000");
    BigInt b(1);
    CHECK(a - b == BigInt(0xFFFFFFFFu));
}

TEST_CASE("BigInt multiplication", "[crypto][bigint]") {
    CHECK(BigInt(0) * BigInt(123) == BigInt(0));
    CHECK(BigInt(1) * BigInt(123) == BigInt(123));
    CHECK(BigInt(6) * BigInt(7) == BigInt(42));

    // Multi-limb
    BigInt a = BigInt::fromHex("FFFFFFFF");
    BigInt b = BigInt::fromHex("FFFFFFFF");
    CHECK(a * b == BigInt::fromHex("fffffffe00000001"));
}

TEST_CASE("BigInt division and modulo", "[crypto][bigint]") {
    CHECK(BigInt(10) / BigInt(3) == BigInt(3));
    CHECK(BigInt(10) % BigInt(3) == BigInt(1));
    CHECK(BigInt(100) / BigInt(10) == BigInt(10));
    CHECK(BigInt(100) % BigInt(10) == BigInt(0));
    CHECK(BigInt(7) / BigInt(10) == BigInt(0));
    CHECK(BigInt(7) % BigInt(10) == BigInt(7));
}

TEST_CASE("BigInt shift operations", "[crypto][bigint]") {
    CHECK(BigInt(1) << 0 == BigInt(1));
    CHECK(BigInt(1) << 1 == BigInt(2));
    CHECK(BigInt(1) << 32 == BigInt::fromHex("100000000"));
    CHECK(BigInt(0xFF) << 8 == BigInt(0xFF00));

    CHECK(BigInt(4) >> 1 == BigInt(2));
    CHECK(BigInt(1) >> 1 == BigInt(0));
    CHECK(BigInt::fromHex("100000000") >> 32 == BigInt(1));
}

TEST_CASE("BigInt bit operations", "[crypto][bigint]") {
    BigInt val(0xA5);
    CHECK(val.getBit(0) == true);   // 1
    CHECK(val.getBit(1) == false);  // 0
    CHECK(val.getBit(2) == true);   // 1
    CHECK(val.getBit(5) == true);   // 1
    CHECK(val.getBit(7) == true);   // 1
    CHECK(val.getBit(8) == false);

    CHECK(BigInt(0).bitLength() == 0);
    CHECK(BigInt(1).bitLength() == 1);
    CHECK(BigInt(2).bitLength() == 2);
    CHECK(BigInt(255).bitLength() == 8);
    CHECK(BigInt(256).bitLength() == 9);
}

TEST_CASE("BigInt byte import/export roundtrip", "[crypto][bigint]") {
    const unsigned char data[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef};
    BigInt val = BigInt::fromBytes(data, 8);
    CHECK(val == BigInt::fromHex("0123456789abcdef"));

    unsigned char out[8];
    val.toBytes(out, 8);
    CHECK(std::memcmp(out, data, 8) == 0);
}

TEST_CASE("BigInt hex conversion roundtrip", "[crypto][bigint]") {
    CHECK(BigInt::fromHex("0").isZero());
    CHECK(BigInt::fromHex("ff") == BigInt(255));
    CHECK(BigInt::fromHex("DEADBEEF") == BigInt::fromHex("deadbeef"));

    BigInt val = BigInt::fromHex("123456789abcdef0");
    CHECK(val.toHex() == "123456789abcdef0");
}

TEST_CASE("BigInt modPow small values", "[crypto][bigint]") {
    // 2^10 mod 1000 = 1024 mod 1000 = 24
    CHECK(BigInt::modPow(BigInt(2), BigInt(10), BigInt(1000)) == BigInt(24));
    // 3^0 mod 7 = 1
    CHECK(BigInt::modPow(BigInt(3), BigInt(0), BigInt(7)) == BigInt(1));
    // 5^3 mod 13 = 125 mod 13 = 8
    CHECK(BigInt::modPow(BigInt(5), BigInt(3), BigInt(13)) == BigInt(8));
    // 7^256 mod 13: Fermat's little theorem: 7^12 ≡ 1 (mod 13), 256 = 21*12+4, 7^4 = 2401 mod 13 = 9
    CHECK(BigInt::modPow(BigInt(7), BigInt(256), BigInt(13)) == BigInt(9));
}

TEST_CASE("BigInt modPow matches OpenSSL BN_mod_exp", "[crypto][bigint]") {
    // Use fixed RSA-sized test with known values
    // Generate a pseudo-random 256-bit base, exponent, modulus
    unsigned int seed = GENERATE(take(20, random(0u, 0xFFFFFFFFu)));
    unsigned int state = seed;

    unsigned char baseBytes[32], expBytes[32], modBytes[32];
    for (int i = 0; i < 32; i++) {
        state = state * 1103515245u + 12345u;
        baseBytes[i] = static_cast<unsigned char>(state >> 16);
        state = state * 1103515245u + 12345u;
        expBytes[i] = static_cast<unsigned char>(state >> 16);
        state = state * 1103515245u + 12345u;
        modBytes[i] = static_cast<unsigned char>(state >> 16);
    }
    // Ensure modulus is odd and nonzero (set MSB and LSB)
    modBytes[0] |= 0x80;
    modBytes[31] |= 0x01;

    BigInt base = BigInt::fromBytes(baseBytes, 32);
    BigInt exp = BigInt::fromBytes(expBytes, 32);
    BigInt mod = BigInt::fromBytes(modBytes, 32);

    BigInt result = BigInt::modPow(base, exp, mod);

    // Cross-validate with OpenSSL
    BIGNUM* bnBase = BN_bin2bn(baseBytes, 32, NULL);
    BIGNUM* bnExp = BN_bin2bn(expBytes, 32, NULL);
    BIGNUM* bnMod = BN_bin2bn(modBytes, 32, NULL);
    BIGNUM* bnResult = BN_new();
    BN_CTX* ctx = BN_CTX_new();
    BN_mod_exp(bnResult, bnBase, bnExp, bnMod, ctx);

    unsigned char osslBytes[32];
    std::memset(osslBytes, 0, 32);
    int osslLen = BN_bn2bin(bnResult, osslBytes + (32 - BN_num_bytes(bnResult)));

    unsigned char ourBytes[32];
    result.toBytes(ourBytes, 32);

    CAPTURE(seed);
    CHECK(std::memcmp(ourBytes, osslBytes, 32) == 0);

    BN_free(bnBase);
    BN_free(bnExp);
    BN_free(bnMod);
    BN_free(bnResult);
    BN_CTX_free(ctx);
}

TEST_CASE("BigInt large byte roundtrip", "[crypto][bigint]") {
    unsigned char data[256];
    for (int i = 0; i < 256; i++) data[i] = (unsigned char)i;
    data[0] = 0x01; // ensure leading nonzero

    BigInt val = BigInt::fromBytes(data, 256);
    unsigned char out[256];
    val.toBytes(out, 256);
    CHECK(std::memcmp(out, data, 256) == 0);
}
