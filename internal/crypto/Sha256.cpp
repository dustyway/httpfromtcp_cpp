#include "Sha256.hpp"

static unsigned int rotr(unsigned int x, int n) {
    return (x >> n) | (x << (32 - n));
}

void Crypto::sha256(const unsigned char* data, size_t len, unsigned char hash[32]) {
    static const unsigned int k[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
        0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
        0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
        0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
        0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
        0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
        0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
        0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
        0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
    };

    size_t padded_len = len + 1;
    while (padded_len % 64 != 56) padded_len++;
    padded_len += 8;

    unsigned char* msg = new unsigned char[padded_len];
    for (size_t i = 0; i < padded_len; i++) msg[i] = 0;
    for (size_t i = 0; i < len; i++) msg[i] = data[i];
    msg[len] = 0x80;

    unsigned long long bit_len = (unsigned long long)len * 8;
    for (int i = 0; i < 8; i++) {
        msg[padded_len - 1 - i] = (unsigned char)(bit_len >> (i * 8));
    }

    unsigned int h0 = 0x6a09e667, h1 = 0xbb67ae85;
    unsigned int h2 = 0x3c6ef372, h3 = 0xa54ff53a;
    unsigned int h4 = 0x510e527f, h5 = 0x9b05688c;
    unsigned int h6 = 0x1f83d9ab, h7 = 0x5be0cd19;

    for (size_t offset = 0; offset < padded_len; offset += 64) {
        unsigned int w[64];
        for (int i = 0; i < 16; i++) {
            w[i] = ((unsigned int)msg[offset + i*4] << 24)
                 | ((unsigned int)msg[offset + i*4+1] << 16)
                 | ((unsigned int)msg[offset + i*4+2] << 8)
                 | ((unsigned int)msg[offset + i*4+3]);
        }
        for (int i = 16; i < 64; i++) {
            unsigned int s0 = rotr(w[i-15], 7) ^ rotr(w[i-15], 18) ^ (w[i-15] >> 3);
            unsigned int s1 = rotr(w[i-2], 17) ^ rotr(w[i-2], 19) ^ (w[i-2] >> 10);
            w[i] = w[i-16] + s0 + w[i-7] + s1;
        }

        unsigned int a = h0, b = h1, c = h2, d = h3;
        unsigned int e = h4, f = h5, g = h6, hh = h7;

        for (int i = 0; i < 64; i++) {
            unsigned int S1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
            unsigned int ch = (e & f) ^ ((~e) & g);
            unsigned int temp1 = hh + S1 + ch + k[i] + w[i];
            unsigned int S0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
            unsigned int maj = (a & b) ^ (a & c) ^ (b & c);
            unsigned int temp2 = S0 + maj;

            hh = g; g = f; f = e; e = d + temp1;
            d = c; c = b; b = a; a = temp1 + temp2;
        }

        h0 += a; h1 += b; h2 += c; h3 += d;
        h4 += e; h5 += f; h6 += g; h7 += hh;
    }

    delete[] msg;

    unsigned int hvals[8] = {h0, h1, h2, h3, h4, h5, h6, h7};
    for (int i = 0; i < 8; i++) {
        hash[i*4]   = (unsigned char)(hvals[i] >> 24);
        hash[i*4+1] = (unsigned char)(hvals[i] >> 16);
        hash[i*4+2] = (unsigned char)(hvals[i] >> 8);
        hash[i*4+3] = (unsigned char)(hvals[i]);
    }
}

void Crypto::sha256(const std::string& input, unsigned char hash[32]) {
    sha256(reinterpret_cast<const unsigned char*>(input.data()), input.size(), hash);
}

std::string Crypto::toHexStr(const unsigned char* bytes, size_t len) {
    static const char hex[] = "0123456789abcdef";
    std::string out;
    for (size_t i = 0; i < len; i++) {
        out += hex[bytes[i] >> 4];
        out += hex[bytes[i] & 0x0f];
    }
    return out;
}
