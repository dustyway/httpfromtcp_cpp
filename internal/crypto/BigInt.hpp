#ifndef BIGINT_HPP
#define BIGINT_HPP

#include <cstddef>
#include <vector>
#include <string>

namespace Crypto {

class BigInt {
public:
    BigInt();
    explicit BigInt(unsigned int val);

    static BigInt fromBytes(const unsigned char* data, size_t len);
    void toBytes(unsigned char* out, size_t len) const;
    size_t byteLength() const;

    static BigInt fromHex(const std::string& hex);
    std::string toHex() const;

    bool isZero() const;
    int compare(const BigInt& other) const;

    bool operator==(const BigInt& other) const;
    bool operator!=(const BigInt& other) const;
    bool operator<(const BigInt& other) const;
    bool operator<=(const BigInt& other) const;
    bool operator>(const BigInt& other) const;
    bool operator>=(const BigInt& other) const;

    BigInt operator+(const BigInt& other) const;
    BigInt operator-(const BigInt& other) const;
    BigInt operator*(const BigInt& other) const;
    BigInt operator/(const BigInt& other) const;
    BigInt operator%(const BigInt& other) const;

    BigInt operator<<(int bits) const;
    BigInt operator>>(int bits) const;

    bool getBit(size_t n) const;
    size_t bitLength() const;

    static BigInt modPow(const BigInt& base, const BigInt& exp, const BigInt& mod);

private:
    // Little-endian 32-bit limbs
    std::vector<unsigned int> limbs;

    void trim();
    static void divmod(const BigInt& a, const BigInt& b, BigInt& q, BigInt& r);
};

}

#endif
