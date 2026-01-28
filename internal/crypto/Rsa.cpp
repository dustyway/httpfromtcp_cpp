#include "Rsa.hpp"
#include <cstring>
#include <cstdlib>

namespace Crypto {

// Minimal ASN.1/DER parser helpers
static bool derReadTag(const unsigned char* data, size_t len, size_t& pos,
                       unsigned char& tag) {
    if (pos >= len) return false;
    tag = data[pos++];
    return true;
}

static bool derReadLength(const unsigned char* data, size_t len, size_t& pos,
                          size_t& outLen) {
    if (pos >= len) return false;
    unsigned char first = data[pos++];
    if (first < 0x80) {
        outLen = first;
        return true;
    }
    int numBytes = first & 0x7f;
    if (numBytes > 4 || pos + numBytes > len) return false;
    outLen = 0;
    for (int i = 0; i < numBytes; i++) {
        outLen = (outLen << 8) | data[pos++];
    }
    return true;
}

static bool derReadInteger(const unsigned char* data, size_t len, size_t& pos,
                           BigInt& val) {
    unsigned char tag;
    if (!derReadTag(data, len, pos, tag) || tag != 0x02) return false;
    size_t intLen;
    if (!derReadLength(data, len, pos, intLen)) return false;
    if (pos + intLen > len) return false;

    // Skip leading zero byte (sign byte)
    const unsigned char* intData = data + pos;
    if (intLen > 0 && intData[0] == 0x00) {
        intData++;
        intLen--;
    }
    val = BigInt::fromBytes(intData, intLen);
    pos += (intData - (data + pos)) + intLen;
    return true;
}

static bool derReadSequence(const unsigned char* data, size_t len, size_t& pos,
                            size_t& seqLen) {
    unsigned char tag;
    if (!derReadTag(data, len, pos, tag) || tag != 0x30) return false;
    return derReadLength(data, len, pos, seqLen);
}

bool rsaParsePublicKey(const unsigned char* der, size_t derLen, RsaPublicKey& key) {
    size_t pos = 0;
    size_t seqLen;

    if (!derReadSequence(der, derLen, pos, seqLen)) return false;

    // Try to detect SubjectPublicKeyInfo vs raw RSAPublicKey
    // SubjectPublicKeyInfo has: SEQUENCE { SEQUENCE { OID, NULL }, BIT STRING }
    // RSAPublicKey has: SEQUENCE { INTEGER, INTEGER }
    size_t savedPos = pos;
    unsigned char nextTag;
    if (pos < derLen) {
        nextTag = der[pos];
    } else {
        return false;
    }

    if (nextTag == 0x30) {
        // SubjectPublicKeyInfo format
        // Skip the algorithm identifier sequence
        size_t algSeqLen;
        if (!derReadSequence(der, derLen, pos, algSeqLen)) return false;
        pos += algSeqLen; // skip algorithm OID and params

        // Read BIT STRING
        unsigned char bsTag;
        if (!derReadTag(der, derLen, pos, bsTag) || bsTag != 0x03) return false;
        size_t bsLen;
        if (!derReadLength(der, derLen, pos, bsLen)) return false;
        if (pos >= derLen) return false;
        pos++; // skip unused bits byte (should be 0)

        // Now parse the inner RSAPublicKey SEQUENCE
        size_t innerSeqLen;
        if (!derReadSequence(der, derLen, pos, innerSeqLen)) return false;
    } else {
        // Already positioned at INTEGER (raw RSAPublicKey)
        pos = savedPos;
    }

    if (!derReadInteger(der, derLen, pos, key.n)) return false;
    if (!derReadInteger(der, derLen, pos, key.e)) return false;
    return true;
}

std::vector<unsigned char> rsaEncrypt(const RsaPublicKey& key,
                                      const unsigned char* plaintext, size_t ptLen) {
    size_t k = key.n.byteLength();
    // PKCS#1 v1.5: k >= ptLen + 11
    if (k < ptLen + 11) return std::vector<unsigned char>();

    // EM = 0x00 || 0x02 || PS || 0x00 || M
    std::vector<unsigned char> em(k, 0);
    em[0] = 0x00;
    em[1] = 0x02;

    // PS: nonzero random bytes, at least 8 bytes
    size_t psLen = k - ptLen - 3;
    // Use a simple PRNG seeded from the plaintext for deterministic testing
    // In production, this MUST be cryptographically random
    unsigned int rngState = 0;
    for (size_t i = 0; i < ptLen; i++) rngState = rngState * 31 + plaintext[i];
    rngState ^= 0xDEADBEEF;
    for (size_t i = 0; i < psLen; i++) {
        rngState = rngState * 1103515245u + 12345u;
        unsigned char val = (unsigned char)((rngState >> 16) & 0xFF);
        if (val == 0) val = 1; // PS must be nonzero
        em[2 + i] = val;
    }
    em[2 + psLen] = 0x00;
    std::memcpy(&em[3 + psLen], plaintext, ptLen);

    BigInt m = BigInt::fromBytes(&em[0], k);
    BigInt c = BigInt::modPow(m, key.e, key.n);

    std::vector<unsigned char> result(k, 0);
    c.toBytes(&result[0], k);
    return result;
}

// SHA-256 DigestInfo DER prefix for PKCS#1 v1.5 signatures
static const unsigned char sha256DigestInfoPrefix[] = {
    0x30, 0x31, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86,
    0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01, 0x05,
    0x00, 0x04, 0x20
};

bool rsaVerify(const RsaPublicKey& key,
               const unsigned char hash[32],
               const unsigned char* signature, size_t sigLen) {
    size_t k = key.n.byteLength();
    if (sigLen != k) return false;

    BigInt s = BigInt::fromBytes(signature, sigLen);
    BigInt m = BigInt::modPow(s, key.e, key.n);

    std::vector<unsigned char> em(k, 0);
    m.toBytes(&em[0], k);

    // Verify PKCS#1 v1.5 Type 1 padding:
    // 0x00 || 0x01 || PS (0xFF bytes) || 0x00 || DigestInfo
    if (em[0] != 0x00 || em[1] != 0x01) return false;

    size_t i = 2;
    while (i < k && em[i] == 0xFF) i++;
    if (i < 10) return false; // PS must be at least 8 bytes
    if (i >= k || em[i] != 0x00) return false;
    i++;

    // Remaining should be DigestInfo(SHA-256) || hash
    size_t remaining = k - i;
    if (remaining != sizeof(sha256DigestInfoPrefix) + 32) return false;
    if (std::memcmp(&em[i], sha256DigestInfoPrefix, sizeof(sha256DigestInfoPrefix)) != 0) {
        return false;
    }
    if (std::memcmp(&em[i + sizeof(sha256DigestInfoPrefix)], hash, 32) != 0) {
        return false;
    }

    return true;
}

}
