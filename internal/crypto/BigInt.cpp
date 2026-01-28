#include "BigInt.hpp"
#include <algorithm>
#include <cstring>

namespace Crypto {

BigInt::BigInt() {}

BigInt::BigInt(unsigned int val) {
    if (val != 0) {
        limbs.push_back(val);
    }
}

void BigInt::trim() {
    while (!limbs.empty() && limbs.back() == 0) {
        limbs.pop_back();
    }
}

bool BigInt::isZero() const {
    return limbs.empty();
}

int BigInt::compare(const BigInt& other) const {
    if (limbs.size() != other.limbs.size()) {
        return limbs.size() < other.limbs.size() ? -1 : 1;
    }
    for (size_t i = limbs.size(); i > 0; i--) {
        if (limbs[i-1] != other.limbs[i-1]) {
            return limbs[i-1] < other.limbs[i-1] ? -1 : 1;
        }
    }
    return 0;
}

bool BigInt::operator==(const BigInt& other) const { return compare(other) == 0; }
bool BigInt::operator!=(const BigInt& other) const { return compare(other) != 0; }
bool BigInt::operator<(const BigInt& other) const { return compare(other) < 0; }
bool BigInt::operator<=(const BigInt& other) const { return compare(other) <= 0; }
bool BigInt::operator>(const BigInt& other) const { return compare(other) > 0; }
bool BigInt::operator>=(const BigInt& other) const { return compare(other) >= 0; }

BigInt BigInt::operator+(const BigInt& other) const {
    BigInt result;
    size_t n = std::max(limbs.size(), other.limbs.size());
    result.limbs.resize(n + 1, 0);
    unsigned long long carry = 0;
    for (size_t i = 0; i < n; i++) {
        unsigned long long a = (i < limbs.size()) ? limbs[i] : 0;
        unsigned long long b = (i < other.limbs.size()) ? other.limbs[i] : 0;
        unsigned long long sum = a + b + carry;
        result.limbs[i] = (unsigned int)(sum & 0xFFFFFFFFu);
        carry = sum >> 32;
    }
    result.limbs[n] = (unsigned int)carry;
    result.trim();
    return result;
}

BigInt BigInt::operator-(const BigInt& other) const {
    BigInt result;
    result.limbs.resize(limbs.size(), 0);
    long long borrow = 0;
    for (size_t i = 0; i < limbs.size(); i++) {
        long long a = limbs[i];
        long long b = (i < other.limbs.size()) ? (long long)other.limbs[i] : 0;
        long long diff = a - b - borrow;
        if (diff < 0) {
            diff += (1LL << 32);
            borrow = 1;
        } else {
            borrow = 0;
        }
        result.limbs[i] = (unsigned int)(diff & 0xFFFFFFFFu);
    }
    result.trim();
    return result;
}

BigInt BigInt::operator*(const BigInt& other) const {
    if (isZero() || other.isZero()) return BigInt();
    BigInt result;
    result.limbs.resize(limbs.size() + other.limbs.size(), 0);
    for (size_t i = 0; i < limbs.size(); i++) {
        unsigned long long carry = 0;
        for (size_t j = 0; j < other.limbs.size(); j++) {
            unsigned long long prod = (unsigned long long)limbs[i] * other.limbs[j]
                                   + result.limbs[i + j] + carry;
            result.limbs[i + j] = (unsigned int)(prod & 0xFFFFFFFFu);
            carry = prod >> 32;
        }
        result.limbs[i + other.limbs.size()] += (unsigned int)carry;
    }
    result.trim();
    return result;
}

void BigInt::divmod(const BigInt& a, const BigInt& b, BigInt& q, BigInt& r) {
    if (b.isZero()) return; // undefined
    if (a < b) {
        q = BigInt();
        r = a;
        return;
    }

    size_t aBits = a.bitLength();
    size_t bBits = b.bitLength();

    r = BigInt();
    q = BigInt();
    q.limbs.resize((aBits + 31) / 32, 0);

    for (size_t i = aBits; i > 0; i--) {
        // r = r << 1
        // shift left by 1
        unsigned int carry = 0;
        for (size_t j = 0; j < r.limbs.size(); j++) {
            unsigned int newCarry = r.limbs[j] >> 31;
            r.limbs[j] = (r.limbs[j] << 1) | carry;
            carry = newCarry;
        }
        if (carry) r.limbs.push_back(carry);

        // set bit 0 of r to bit (i-1) of a
        if (a.getBit(i - 1)) {
            if (r.limbs.empty()) r.limbs.push_back(0);
            r.limbs[0] |= 1;
        }

        if (r >= b) {
            r = r - b;
            size_t limbIdx = (i - 1) / 32;
            size_t bitIdx = (i - 1) % 32;
            q.limbs[limbIdx] |= (1u << bitIdx);
        }
    }
    q.trim();
    r.trim();
}

BigInt BigInt::operator/(const BigInt& other) const {
    BigInt q, r;
    divmod(*this, other, q, r);
    return q;
}

BigInt BigInt::operator%(const BigInt& other) const {
    BigInt q, r;
    divmod(*this, other, q, r);
    return r;
}

BigInt BigInt::operator<<(int bits) const {
    if (isZero() || bits == 0) return *this;
    int limbShift = bits / 32;
    int bitShift = bits % 32;

    BigInt result;
    result.limbs.resize(limbs.size() + limbShift + 1, 0);
    for (size_t i = 0; i < limbs.size(); i++) {
        unsigned long long val = (unsigned long long)limbs[i] << bitShift;
        result.limbs[i + limbShift] |= (unsigned int)(val & 0xFFFFFFFFu);
        result.limbs[i + limbShift + 1] |= (unsigned int)(val >> 32);
    }
    result.trim();
    return result;
}

BigInt BigInt::operator>>(int bits) const {
    if (isZero() || bits == 0) return *this;
    int limbShift = bits / 32;
    int bitShift = bits % 32;

    if ((size_t)limbShift >= limbs.size()) return BigInt();

    BigInt result;
    result.limbs.resize(limbs.size() - limbShift, 0);
    for (size_t i = 0; i < result.limbs.size(); i++) {
        result.limbs[i] = limbs[i + limbShift] >> bitShift;
        if (bitShift > 0 && i + limbShift + 1 < limbs.size()) {
            result.limbs[i] |= limbs[i + limbShift + 1] << (32 - bitShift);
        }
    }
    result.trim();
    return result;
}

bool BigInt::getBit(size_t n) const {
    size_t limbIdx = n / 32;
    size_t bitIdx = n % 32;
    if (limbIdx >= limbs.size()) return false;
    return (limbs[limbIdx] >> bitIdx) & 1;
}

size_t BigInt::bitLength() const {
    if (isZero()) return 0;
    size_t topLimb = limbs.size() - 1;
    unsigned int val = limbs[topLimb];
    size_t bits = topLimb * 32;
    while (val > 0) {
        bits++;
        val >>= 1;
    }
    return bits;
}

BigInt BigInt::modPow(const BigInt& base, const BigInt& exp, const BigInt& mod) {
    if (mod.isZero()) return BigInt();
    BigInt result(1);
    BigInt b = base % mod;
    size_t expBits = exp.bitLength();
    for (size_t i = expBits; i > 0; i--) {
        result = (result * result) % mod;
        if (exp.getBit(i - 1)) {
            result = (result * b) % mod;
        }
    }
    return result;
}

BigInt BigInt::fromBytes(const unsigned char* data, size_t len) {
    BigInt result;
    if (len == 0) return result;

    // data is big-endian
    size_t numLimbs = (len + 3) / 4;
    result.limbs.resize(numLimbs, 0);

    for (size_t i = 0; i < len; i++) {
        size_t bytePos = len - 1 - i;
        size_t limbIdx = i / 4;
        size_t byteInLimb = i % 4;
        result.limbs[limbIdx] |= ((unsigned int)data[bytePos]) << (byteInLimb * 8);
    }
    result.trim();
    return result;
}

void BigInt::toBytes(unsigned char* out, size_t len) const {
    std::memset(out, 0, len);
    for (size_t i = 0; i < limbs.size(); i++) {
        for (int b = 0; b < 4; b++) {
            size_t byteIdx = i * 4 + b;
            if (byteIdx >= len) break;
            size_t outIdx = len - 1 - byteIdx;
            out[outIdx] = (unsigned char)(limbs[i] >> (b * 8));
        }
    }
}

size_t BigInt::byteLength() const {
    return (bitLength() + 7) / 8;
}

BigInt BigInt::fromHex(const std::string& hex) {
    // Convert hex string to bytes, then use fromBytes
    size_t hexLen = hex.size();
    size_t byteLen = (hexLen + 1) / 2;
    unsigned char* bytes = new unsigned char[byteLen];
    std::memset(bytes, 0, byteLen);

    for (size_t i = 0; i < hexLen; i++) {
        unsigned char nibble;
        char c = hex[i];
        if (c >= '0' && c <= '9') nibble = (unsigned char)(c - '0');
        else if (c >= 'a' && c <= 'f') nibble = (unsigned char)(c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') nibble = (unsigned char)(c - 'A' + 10);
        else nibble = 0;

        size_t bytePos = (hexLen - 1 - i) / 2;
        size_t nibblePos = (hexLen - 1 - i) % 2;
        // bytePos from the right, but we want big-endian
        size_t outIdx = byteLen - 1 - bytePos;
        bytes[outIdx] |= nibble << (nibblePos * 4);
    }

    BigInt result = fromBytes(bytes, byteLen);
    delete[] bytes;
    return result;
}

std::string BigInt::toHex() const {
    if (isZero()) return "0";

    static const char hexChars[] = "0123456789abcdef";
    std::string result;

    size_t topLimb = limbs.size() - 1;
    // First limb without leading zeros
    unsigned int val = limbs[topLimb];
    bool started = false;
    for (int bit = 28; bit >= 0; bit -= 4) {
        unsigned char nibble = (unsigned char)((val >> bit) & 0xf);
        if (nibble != 0 || started) {
            result += hexChars[nibble];
            started = true;
        }
    }
    if (!started) result += '0';

    // Remaining limbs with leading zeros
    for (size_t i = topLimb; i > 0; i--) {
        val = limbs[i - 1];
        for (int bit = 28; bit >= 0; bit -= 4) {
            result += hexChars[(val >> bit) & 0xf];
        }
    }

    return result;
}

}
