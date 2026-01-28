#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/evp.h>
#include <openssl/bn.h>
#include <openssl/x509.h>

#include "Rsa.hpp"
#include "Sha256.hpp"

// Helper: generate an RSA keypair with OpenSSL and extract DER public key
struct TestKeyPair {
    EVP_PKEY* pkey;
    std::vector<unsigned char> pubDer;

    TestKeyPair() : pkey(NULL) {
        EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);
        EVP_PKEY_keygen_init(ctx);
        EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, 2048);
        EVP_PKEY_keygen(ctx, &pkey);
        EVP_PKEY_CTX_free(ctx);

        // Extract DER-encoded SubjectPublicKeyInfo
        int len = i2d_PUBKEY(pkey, NULL);
        pubDer.resize(len);
        unsigned char* p = &pubDer[0];
        i2d_PUBKEY(pkey, &p);
    }

    ~TestKeyPair() {
        if (pkey) EVP_PKEY_free(pkey);
    }
};

TEST_CASE("RSA parse DER public key", "[crypto][rsa]") {
    TestKeyPair kp;

    Crypto::RsaPublicKey pub;
    REQUIRE(Crypto::rsaParsePublicKey(&kp.pubDer[0], kp.pubDer.size(), pub));
    CHECK(pub.n.bitLength() >= 2047);
    CHECK(pub.n.bitLength() <= 2048);
    CHECK(pub.e == Crypto::BigInt(65537));
}

TEST_CASE("RSA PKCS1 v1.5 encrypt produces correct-length output", "[crypto][rsa]") {
    TestKeyPair kp;
    Crypto::RsaPublicKey pub;
    REQUIRE(Crypto::rsaParsePublicKey(&kp.pubDer[0], kp.pubDer.size(), pub));

    const unsigned char msg[] = "Hello, RSA!";
    std::vector<unsigned char> ct = Crypto::rsaEncrypt(pub, msg, 11);
    CHECK(ct.size() == 256); // 2048-bit key = 256 bytes

    // Decrypt with OpenSSL private key to verify
    EVP_PKEY_CTX* dctx = EVP_PKEY_CTX_new(kp.pkey, NULL);
    EVP_PKEY_decrypt_init(dctx);
    EVP_PKEY_CTX_set_rsa_padding(dctx, RSA_PKCS1_PADDING);
    size_t outLen = 256;
    unsigned char decrypted[256];
    int ret = EVP_PKEY_decrypt(dctx, decrypted, &outLen, &ct[0], ct.size());
    EVP_PKEY_CTX_free(dctx);

    REQUIRE(ret == 1);
    CHECK(outLen == 11);
    CHECK(std::memcmp(decrypted, msg, 11) == 0);
}

TEST_CASE("RSA PKCS1 v1.5 signature verification", "[crypto][rsa]") {
    TestKeyPair kp;
    Crypto::RsaPublicKey pub;
    REQUIRE(Crypto::rsaParsePublicKey(&kp.pubDer[0], kp.pubDer.size(), pub));

    // Sign with OpenSSL
    const unsigned char msg[] = "Test message for RSA signature verification";
    unsigned char hash[32];
    Crypto::sha256(msg, sizeof(msg) - 1, hash);

    EVP_PKEY_CTX* sctx = EVP_PKEY_CTX_new(kp.pkey, NULL);
    EVP_PKEY_sign_init(sctx);
    EVP_PKEY_CTX_set_rsa_padding(sctx, RSA_PKCS1_PADDING);
    EVP_PKEY_CTX_set_signature_md(sctx, EVP_sha256());

    size_t sigLen = 0;
    EVP_PKEY_sign(sctx, NULL, &sigLen, hash, 32);
    std::vector<unsigned char> sig(sigLen);
    EVP_PKEY_sign(sctx, &sig[0], &sigLen, hash, 32);
    EVP_PKEY_CTX_free(sctx);

    CHECK(Crypto::rsaVerify(pub, hash, &sig[0], sigLen) == true);

    // Tamper with hash — should fail
    unsigned char badHash[32];
    std::memcpy(badHash, hash, 32);
    badHash[0] ^= 0x01;
    CHECK(Crypto::rsaVerify(pub, badHash, &sig[0], sigLen) == false);

    // Tamper with signature — should fail
    std::vector<unsigned char> badSig(sig);
    badSig[0] ^= 0x01;
    CHECK(Crypto::rsaVerify(pub, hash, &badSig[0], badSig.size()) == false);
}

TEST_CASE("RSA rejects wrong signature length", "[crypto][rsa]") {
    TestKeyPair kp;
    Crypto::RsaPublicKey pub;
    REQUIRE(Crypto::rsaParsePublicKey(&kp.pubDer[0], kp.pubDer.size(), pub));

    unsigned char hash[32] = {0};
    unsigned char sig[128] = {0};
    CHECK(Crypto::rsaVerify(pub, hash, sig, 128) == false); // Wrong size for 2048-bit key
}
