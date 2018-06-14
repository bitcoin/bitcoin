// Copyright (c) 2014-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/sha256.h>
#include <crypto/common.h>

#include <assert.h>
#include <string.h>
#include <atomic>

#if defined(__x86_64__) || defined(__amd64__)
#if defined(USE_ASM)
#include <cpuid.h>
namespace sha256_sse4
{
void Transform(uint32_t* s, const unsigned char* chunk, size_t blocks);
}
#endif
#endif

namespace sha256d64_sse41
{
void Transform_4way(unsigned char* out, const unsigned char* in);
}

namespace sha256d64_avx2
{
void Transform_8way(unsigned char* out, const unsigned char* in);
}

// Internal implementation code.
namespace
{
/// Internal SHA-256 implementation.
namespace sha256
{
uint32_t inline Ch(uint32_t x, uint32_t y, uint32_t z) { return z ^ (x & (y ^ z)); }
uint32_t inline Maj(uint32_t x, uint32_t y, uint32_t z) { return (x & y) | (z & (x | y)); }
uint32_t inline Sigma0(uint32_t x) { return (x >> 2 | x << 30) ^ (x >> 13 | x << 19) ^ (x >> 22 | x << 10); }
uint32_t inline Sigma1(uint32_t x) { return (x >> 6 | x << 26) ^ (x >> 11 | x << 21) ^ (x >> 25 | x << 7); }
uint32_t inline sigma0(uint32_t x) { return (x >> 7 | x << 25) ^ (x >> 18 | x << 14) ^ (x >> 3); }
uint32_t inline sigma1(uint32_t x) { return (x >> 17 | x << 15) ^ (x >> 19 | x << 13) ^ (x >> 10); }

/** One round of SHA-256. */
void inline Round(uint32_t a, uint32_t b, uint32_t c, uint32_t& d, uint32_t e, uint32_t f, uint32_t g, uint32_t& h, uint32_t k)
{
    uint32_t t1 = h + Sigma1(e) + Ch(e, f, g) + k;
    uint32_t t2 = Sigma0(a) + Maj(a, b, c);
    d += t1;
    h = t1 + t2;
}

/** Initialize SHA-256 state. */
void inline Initialize(uint32_t* s)
{
    s[0] = 0x6a09e667ul;
    s[1] = 0xbb67ae85ul;
    s[2] = 0x3c6ef372ul;
    s[3] = 0xa54ff53aul;
    s[4] = 0x510e527ful;
    s[5] = 0x9b05688cul;
    s[6] = 0x1f83d9abul;
    s[7] = 0x5be0cd19ul;
}

/** Perform a number of SHA-256 transformations, processing 64-byte chunks. */
void Transform(uint32_t* s, const unsigned char* chunk, size_t blocks)
{
    while (blocks--) {
        uint32_t a = s[0], b = s[1], c = s[2], d = s[3], e = s[4], f = s[5], g = s[6], h = s[7];
        uint32_t w0, w1, w2, w3, w4, w5, w6, w7, w8, w9, w10, w11, w12, w13, w14, w15;

        Round(a, b, c, d, e, f, g, h, 0x428a2f98 + (w0 = ReadBE32(chunk + 0)));
        Round(h, a, b, c, d, e, f, g, 0x71374491 + (w1 = ReadBE32(chunk + 4)));
        Round(g, h, a, b, c, d, e, f, 0xb5c0fbcf + (w2 = ReadBE32(chunk + 8)));
        Round(f, g, h, a, b, c, d, e, 0xe9b5dba5 + (w3 = ReadBE32(chunk + 12)));
        Round(e, f, g, h, a, b, c, d, 0x3956c25b + (w4 = ReadBE32(chunk + 16)));
        Round(d, e, f, g, h, a, b, c, 0x59f111f1 + (w5 = ReadBE32(chunk + 20)));
        Round(c, d, e, f, g, h, a, b, 0x923f82a4 + (w6 = ReadBE32(chunk + 24)));
        Round(b, c, d, e, f, g, h, a, 0xab1c5ed5 + (w7 = ReadBE32(chunk + 28)));
        Round(a, b, c, d, e, f, g, h, 0xd807aa98 + (w8 = ReadBE32(chunk + 32)));
        Round(h, a, b, c, d, e, f, g, 0x12835b01 + (w9 = ReadBE32(chunk + 36)));
        Round(g, h, a, b, c, d, e, f, 0x243185be + (w10 = ReadBE32(chunk + 40)));
        Round(f, g, h, a, b, c, d, e, 0x550c7dc3 + (w11 = ReadBE32(chunk + 44)));
        Round(e, f, g, h, a, b, c, d, 0x72be5d74 + (w12 = ReadBE32(chunk + 48)));
        Round(d, e, f, g, h, a, b, c, 0x80deb1fe + (w13 = ReadBE32(chunk + 52)));
        Round(c, d, e, f, g, h, a, b, 0x9bdc06a7 + (w14 = ReadBE32(chunk + 56)));
        Round(b, c, d, e, f, g, h, a, 0xc19bf174 + (w15 = ReadBE32(chunk + 60)));

        Round(a, b, c, d, e, f, g, h, 0xe49b69c1 + (w0 += sigma1(w14) + w9 + sigma0(w1)));
        Round(h, a, b, c, d, e, f, g, 0xefbe4786 + (w1 += sigma1(w15) + w10 + sigma0(w2)));
        Round(g, h, a, b, c, d, e, f, 0x0fc19dc6 + (w2 += sigma1(w0) + w11 + sigma0(w3)));
        Round(f, g, h, a, b, c, d, e, 0x240ca1cc + (w3 += sigma1(w1) + w12 + sigma0(w4)));
        Round(e, f, g, h, a, b, c, d, 0x2de92c6f + (w4 += sigma1(w2) + w13 + sigma0(w5)));
        Round(d, e, f, g, h, a, b, c, 0x4a7484aa + (w5 += sigma1(w3) + w14 + sigma0(w6)));
        Round(c, d, e, f, g, h, a, b, 0x5cb0a9dc + (w6 += sigma1(w4) + w15 + sigma0(w7)));
        Round(b, c, d, e, f, g, h, a, 0x76f988da + (w7 += sigma1(w5) + w0 + sigma0(w8)));
        Round(a, b, c, d, e, f, g, h, 0x983e5152 + (w8 += sigma1(w6) + w1 + sigma0(w9)));
        Round(h, a, b, c, d, e, f, g, 0xa831c66d + (w9 += sigma1(w7) + w2 + sigma0(w10)));
        Round(g, h, a, b, c, d, e, f, 0xb00327c8 + (w10 += sigma1(w8) + w3 + sigma0(w11)));
        Round(f, g, h, a, b, c, d, e, 0xbf597fc7 + (w11 += sigma1(w9) + w4 + sigma0(w12)));
        Round(e, f, g, h, a, b, c, d, 0xc6e00bf3 + (w12 += sigma1(w10) + w5 + sigma0(w13)));
        Round(d, e, f, g, h, a, b, c, 0xd5a79147 + (w13 += sigma1(w11) + w6 + sigma0(w14)));
        Round(c, d, e, f, g, h, a, b, 0x06ca6351 + (w14 += sigma1(w12) + w7 + sigma0(w15)));
        Round(b, c, d, e, f, g, h, a, 0x14292967 + (w15 += sigma1(w13) + w8 + sigma0(w0)));

        Round(a, b, c, d, e, f, g, h, 0x27b70a85 + (w0 += sigma1(w14) + w9 + sigma0(w1)));
        Round(h, a, b, c, d, e, f, g, 0x2e1b2138 + (w1 += sigma1(w15) + w10 + sigma0(w2)));
        Round(g, h, a, b, c, d, e, f, 0x4d2c6dfc + (w2 += sigma1(w0) + w11 + sigma0(w3)));
        Round(f, g, h, a, b, c, d, e, 0x53380d13 + (w3 += sigma1(w1) + w12 + sigma0(w4)));
        Round(e, f, g, h, a, b, c, d, 0x650a7354 + (w4 += sigma1(w2) + w13 + sigma0(w5)));
        Round(d, e, f, g, h, a, b, c, 0x766a0abb + (w5 += sigma1(w3) + w14 + sigma0(w6)));
        Round(c, d, e, f, g, h, a, b, 0x81c2c92e + (w6 += sigma1(w4) + w15 + sigma0(w7)));
        Round(b, c, d, e, f, g, h, a, 0x92722c85 + (w7 += sigma1(w5) + w0 + sigma0(w8)));
        Round(a, b, c, d, e, f, g, h, 0xa2bfe8a1 + (w8 += sigma1(w6) + w1 + sigma0(w9)));
        Round(h, a, b, c, d, e, f, g, 0xa81a664b + (w9 += sigma1(w7) + w2 + sigma0(w10)));
        Round(g, h, a, b, c, d, e, f, 0xc24b8b70 + (w10 += sigma1(w8) + w3 + sigma0(w11)));
        Round(f, g, h, a, b, c, d, e, 0xc76c51a3 + (w11 += sigma1(w9) + w4 + sigma0(w12)));
        Round(e, f, g, h, a, b, c, d, 0xd192e819 + (w12 += sigma1(w10) + w5 + sigma0(w13)));
        Round(d, e, f, g, h, a, b, c, 0xd6990624 + (w13 += sigma1(w11) + w6 + sigma0(w14)));
        Round(c, d, e, f, g, h, a, b, 0xf40e3585 + (w14 += sigma1(w12) + w7 + sigma0(w15)));
        Round(b, c, d, e, f, g, h, a, 0x106aa070 + (w15 += sigma1(w13) + w8 + sigma0(w0)));

        Round(a, b, c, d, e, f, g, h, 0x19a4c116 + (w0 += sigma1(w14) + w9 + sigma0(w1)));
        Round(h, a, b, c, d, e, f, g, 0x1e376c08 + (w1 += sigma1(w15) + w10 + sigma0(w2)));
        Round(g, h, a, b, c, d, e, f, 0x2748774c + (w2 += sigma1(w0) + w11 + sigma0(w3)));
        Round(f, g, h, a, b, c, d, e, 0x34b0bcb5 + (w3 += sigma1(w1) + w12 + sigma0(w4)));
        Round(e, f, g, h, a, b, c, d, 0x391c0cb3 + (w4 += sigma1(w2) + w13 + sigma0(w5)));
        Round(d, e, f, g, h, a, b, c, 0x4ed8aa4a + (w5 += sigma1(w3) + w14 + sigma0(w6)));
        Round(c, d, e, f, g, h, a, b, 0x5b9cca4f + (w6 += sigma1(w4) + w15 + sigma0(w7)));
        Round(b, c, d, e, f, g, h, a, 0x682e6ff3 + (w7 += sigma1(w5) + w0 + sigma0(w8)));
        Round(a, b, c, d, e, f, g, h, 0x748f82ee + (w8 += sigma1(w6) + w1 + sigma0(w9)));
        Round(h, a, b, c, d, e, f, g, 0x78a5636f + (w9 += sigma1(w7) + w2 + sigma0(w10)));
        Round(g, h, a, b, c, d, e, f, 0x84c87814 + (w10 += sigma1(w8) + w3 + sigma0(w11)));
        Round(f, g, h, a, b, c, d, e, 0x8cc70208 + (w11 += sigma1(w9) + w4 + sigma0(w12)));
        Round(e, f, g, h, a, b, c, d, 0x90befffa + (w12 += sigma1(w10) + w5 + sigma0(w13)));
        Round(d, e, f, g, h, a, b, c, 0xa4506ceb + (w13 += sigma1(w11) + w6 + sigma0(w14)));
        Round(c, d, e, f, g, h, a, b, 0xbef9a3f7 + (w14 + sigma1(w12) + w7 + sigma0(w15)));
        Round(b, c, d, e, f, g, h, a, 0xc67178f2 + (w15 + sigma1(w13) + w8 + sigma0(w0)));

        s[0] += a;
        s[1] += b;
        s[2] += c;
        s[3] += d;
        s[4] += e;
        s[5] += f;
        s[6] += g;
        s[7] += h;
        chunk += 64;
    }
}

void TransformD64(unsigned char* out, const unsigned char* in)
{
    // Transform 1
    uint32_t a = 0x6a09e667ul;
    uint32_t b = 0xbb67ae85ul;
    uint32_t c = 0x3c6ef372ul;
    uint32_t d = 0xa54ff53aul;
    uint32_t e = 0x510e527ful;
    uint32_t f = 0x9b05688cul;
    uint32_t g = 0x1f83d9abul;
    uint32_t h = 0x5be0cd19ul;

    uint32_t w0, w1, w2, w3, w4, w5, w6, w7, w8, w9, w10, w11, w12, w13, w14, w15;

    Round(a, b, c, d, e, f, g, h, 0x428a2f98ul + (w0 = ReadBE32(in + 0)));
    Round(h, a, b, c, d, e, f, g, 0x71374491ul + (w1 = ReadBE32(in + 4)));
    Round(g, h, a, b, c, d, e, f, 0xb5c0fbcful + (w2 = ReadBE32(in + 8)));
    Round(f, g, h, a, b, c, d, e, 0xe9b5dba5ul + (w3 = ReadBE32(in + 12)));
    Round(e, f, g, h, a, b, c, d, 0x3956c25bul + (w4 = ReadBE32(in + 16)));
    Round(d, e, f, g, h, a, b, c, 0x59f111f1ul + (w5 = ReadBE32(in + 20)));
    Round(c, d, e, f, g, h, a, b, 0x923f82a4ul + (w6 = ReadBE32(in + 24)));
    Round(b, c, d, e, f, g, h, a, 0xab1c5ed5ul + (w7 = ReadBE32(in + 28)));
    Round(a, b, c, d, e, f, g, h, 0xd807aa98ul + (w8 = ReadBE32(in + 32)));
    Round(h, a, b, c, d, e, f, g, 0x12835b01ul + (w9 = ReadBE32(in + 36)));
    Round(g, h, a, b, c, d, e, f, 0x243185beul + (w10 = ReadBE32(in + 40)));
    Round(f, g, h, a, b, c, d, e, 0x550c7dc3ul + (w11 = ReadBE32(in + 44)));
    Round(e, f, g, h, a, b, c, d, 0x72be5d74ul + (w12 = ReadBE32(in + 48)));
    Round(d, e, f, g, h, a, b, c, 0x80deb1feul + (w13 = ReadBE32(in + 52)));
    Round(c, d, e, f, g, h, a, b, 0x9bdc06a7ul + (w14 = ReadBE32(in + 56)));
    Round(b, c, d, e, f, g, h, a, 0xc19bf174ul + (w15 = ReadBE32(in + 60)));
    Round(a, b, c, d, e, f, g, h, 0xe49b69c1ul + (w0 += sigma1(w14) + w9 + sigma0(w1)));
    Round(h, a, b, c, d, e, f, g, 0xefbe4786ul + (w1 += sigma1(w15) + w10 + sigma0(w2)));
    Round(g, h, a, b, c, d, e, f, 0x0fc19dc6ul + (w2 += sigma1(w0) + w11 + sigma0(w3)));
    Round(f, g, h, a, b, c, d, e, 0x240ca1ccul + (w3 += sigma1(w1) + w12 + sigma0(w4)));
    Round(e, f, g, h, a, b, c, d, 0x2de92c6ful + (w4 += sigma1(w2) + w13 + sigma0(w5)));
    Round(d, e, f, g, h, a, b, c, 0x4a7484aaul + (w5 += sigma1(w3) + w14 + sigma0(w6)));
    Round(c, d, e, f, g, h, a, b, 0x5cb0a9dcul + (w6 += sigma1(w4) + w15 + sigma0(w7)));
    Round(b, c, d, e, f, g, h, a, 0x76f988daul + (w7 += sigma1(w5) + w0 + sigma0(w8)));
    Round(a, b, c, d, e, f, g, h, 0x983e5152ul + (w8 += sigma1(w6) + w1 + sigma0(w9)));
    Round(h, a, b, c, d, e, f, g, 0xa831c66dul + (w9 += sigma1(w7) + w2 + sigma0(w10)));
    Round(g, h, a, b, c, d, e, f, 0xb00327c8ul + (w10 += sigma1(w8) + w3 + sigma0(w11)));
    Round(f, g, h, a, b, c, d, e, 0xbf597fc7ul + (w11 += sigma1(w9) + w4 + sigma0(w12)));
    Round(e, f, g, h, a, b, c, d, 0xc6e00bf3ul + (w12 += sigma1(w10) + w5 + sigma0(w13)));
    Round(d, e, f, g, h, a, b, c, 0xd5a79147ul + (w13 += sigma1(w11) + w6 + sigma0(w14)));
    Round(c, d, e, f, g, h, a, b, 0x06ca6351ul + (w14 += sigma1(w12) + w7 + sigma0(w15)));
    Round(b, c, d, e, f, g, h, a, 0x14292967ul + (w15 += sigma1(w13) + w8 + sigma0(w0)));
    Round(a, b, c, d, e, f, g, h, 0x27b70a85ul + (w0 += sigma1(w14) + w9 + sigma0(w1)));
    Round(h, a, b, c, d, e, f, g, 0x2e1b2138ul + (w1 += sigma1(w15) + w10 + sigma0(w2)));
    Round(g, h, a, b, c, d, e, f, 0x4d2c6dfcul + (w2 += sigma1(w0) + w11 + sigma0(w3)));
    Round(f, g, h, a, b, c, d, e, 0x53380d13ul + (w3 += sigma1(w1) + w12 + sigma0(w4)));
    Round(e, f, g, h, a, b, c, d, 0x650a7354ul + (w4 += sigma1(w2) + w13 + sigma0(w5)));
    Round(d, e, f, g, h, a, b, c, 0x766a0abbul + (w5 += sigma1(w3) + w14 + sigma0(w6)));
    Round(c, d, e, f, g, h, a, b, 0x81c2c92eul + (w6 += sigma1(w4) + w15 + sigma0(w7)));
    Round(b, c, d, e, f, g, h, a, 0x92722c85ul + (w7 += sigma1(w5) + w0 + sigma0(w8)));
    Round(a, b, c, d, e, f, g, h, 0xa2bfe8a1ul + (w8 += sigma1(w6) + w1 + sigma0(w9)));
    Round(h, a, b, c, d, e, f, g, 0xa81a664bul + (w9 += sigma1(w7) + w2 + sigma0(w10)));
    Round(g, h, a, b, c, d, e, f, 0xc24b8b70ul + (w10 += sigma1(w8) + w3 + sigma0(w11)));
    Round(f, g, h, a, b, c, d, e, 0xc76c51a3ul + (w11 += sigma1(w9) + w4 + sigma0(w12)));
    Round(e, f, g, h, a, b, c, d, 0xd192e819ul + (w12 += sigma1(w10) + w5 + sigma0(w13)));
    Round(d, e, f, g, h, a, b, c, 0xd6990624ul + (w13 += sigma1(w11) + w6 + sigma0(w14)));
    Round(c, d, e, f, g, h, a, b, 0xf40e3585ul + (w14 += sigma1(w12) + w7 + sigma0(w15)));
    Round(b, c, d, e, f, g, h, a, 0x106aa070ul + (w15 += sigma1(w13) + w8 + sigma0(w0)));
    Round(a, b, c, d, e, f, g, h, 0x19a4c116ul + (w0 += sigma1(w14) + w9 + sigma0(w1)));
    Round(h, a, b, c, d, e, f, g, 0x1e376c08ul + (w1 += sigma1(w15) + w10 + sigma0(w2)));
    Round(g, h, a, b, c, d, e, f, 0x2748774cul + (w2 += sigma1(w0) + w11 + sigma0(w3)));
    Round(f, g, h, a, b, c, d, e, 0x34b0bcb5ul + (w3 += sigma1(w1) + w12 + sigma0(w4)));
    Round(e, f, g, h, a, b, c, d, 0x391c0cb3ul + (w4 += sigma1(w2) + w13 + sigma0(w5)));
    Round(d, e, f, g, h, a, b, c, 0x4ed8aa4aul + (w5 += sigma1(w3) + w14 + sigma0(w6)));
    Round(c, d, e, f, g, h, a, b, 0x5b9cca4ful + (w6 += sigma1(w4) + w15 + sigma0(w7)));
    Round(b, c, d, e, f, g, h, a, 0x682e6ff3ul + (w7 += sigma1(w5) + w0 + sigma0(w8)));
    Round(a, b, c, d, e, f, g, h, 0x748f82eeul + (w8 += sigma1(w6) + w1 + sigma0(w9)));
    Round(h, a, b, c, d, e, f, g, 0x78a5636ful + (w9 += sigma1(w7) + w2 + sigma0(w10)));
    Round(g, h, a, b, c, d, e, f, 0x84c87814ul + (w10 += sigma1(w8) + w3 + sigma0(w11)));
    Round(f, g, h, a, b, c, d, e, 0x8cc70208ul + (w11 += sigma1(w9) + w4 + sigma0(w12)));
    Round(e, f, g, h, a, b, c, d, 0x90befffaul + (w12 += sigma1(w10) + w5 + sigma0(w13)));
    Round(d, e, f, g, h, a, b, c, 0xa4506cebul + (w13 += sigma1(w11) + w6 + sigma0(w14)));
    Round(c, d, e, f, g, h, a, b, 0xbef9a3f7ul + (w14 + sigma1(w12) + w7 + sigma0(w15)));
    Round(b, c, d, e, f, g, h, a, 0xc67178f2ul + (w15 + sigma1(w13) + w8 + sigma0(w0)));

    a += 0x6a09e667ul;
    b += 0xbb67ae85ul;
    c += 0x3c6ef372ul;
    d += 0xa54ff53aul;
    e += 0x510e527ful;
    f += 0x9b05688cul;
    g += 0x1f83d9abul;
    h += 0x5be0cd19ul;

    uint32_t t0 = a, t1 = b, t2 = c, t3 = d, t4 = e, t5 = f, t6 = g, t7 = h;

    // Transform 2
    Round(a, b, c, d, e, f, g, h, 0xc28a2f98ul);
    Round(h, a, b, c, d, e, f, g, 0x71374491ul);
    Round(g, h, a, b, c, d, e, f, 0xb5c0fbcful);
    Round(f, g, h, a, b, c, d, e, 0xe9b5dba5ul);
    Round(e, f, g, h, a, b, c, d, 0x3956c25bul);
    Round(d, e, f, g, h, a, b, c, 0x59f111f1ul);
    Round(c, d, e, f, g, h, a, b, 0x923f82a4ul);
    Round(b, c, d, e, f, g, h, a, 0xab1c5ed5ul);
    Round(a, b, c, d, e, f, g, h, 0xd807aa98ul);
    Round(h, a, b, c, d, e, f, g, 0x12835b01ul);
    Round(g, h, a, b, c, d, e, f, 0x243185beul);
    Round(f, g, h, a, b, c, d, e, 0x550c7dc3ul);
    Round(e, f, g, h, a, b, c, d, 0x72be5d74ul);
    Round(d, e, f, g, h, a, b, c, 0x80deb1feul);
    Round(c, d, e, f, g, h, a, b, 0x9bdc06a7ul);
    Round(b, c, d, e, f, g, h, a, 0xc19bf374ul);
    Round(a, b, c, d, e, f, g, h, 0x649b69c1ul);
    Round(h, a, b, c, d, e, f, g, 0xf0fe4786ul);
    Round(g, h, a, b, c, d, e, f, 0x0fe1edc6ul);
    Round(f, g, h, a, b, c, d, e, 0x240cf254ul);
    Round(e, f, g, h, a, b, c, d, 0x4fe9346ful);
    Round(d, e, f, g, h, a, b, c, 0x6cc984beul);
    Round(c, d, e, f, g, h, a, b, 0x61b9411eul);
    Round(b, c, d, e, f, g, h, a, 0x16f988faul);
    Round(a, b, c, d, e, f, g, h, 0xf2c65152ul);
    Round(h, a, b, c, d, e, f, g, 0xa88e5a6dul);
    Round(g, h, a, b, c, d, e, f, 0xb019fc65ul);
    Round(f, g, h, a, b, c, d, e, 0xb9d99ec7ul);
    Round(e, f, g, h, a, b, c, d, 0x9a1231c3ul);
    Round(d, e, f, g, h, a, b, c, 0xe70eeaa0ul);
    Round(c, d, e, f, g, h, a, b, 0xfdb1232bul);
    Round(b, c, d, e, f, g, h, a, 0xc7353eb0ul);
    Round(a, b, c, d, e, f, g, h, 0x3069bad5ul);
    Round(h, a, b, c, d, e, f, g, 0xcb976d5ful);
    Round(g, h, a, b, c, d, e, f, 0x5a0f118ful);
    Round(f, g, h, a, b, c, d, e, 0xdc1eeefdul);
    Round(e, f, g, h, a, b, c, d, 0x0a35b689ul);
    Round(d, e, f, g, h, a, b, c, 0xde0b7a04ul);
    Round(c, d, e, f, g, h, a, b, 0x58f4ca9dul);
    Round(b, c, d, e, f, g, h, a, 0xe15d5b16ul);
    Round(a, b, c, d, e, f, g, h, 0x007f3e86ul);
    Round(h, a, b, c, d, e, f, g, 0x37088980ul);
    Round(g, h, a, b, c, d, e, f, 0xa507ea32ul);
    Round(f, g, h, a, b, c, d, e, 0x6fab9537ul);
    Round(e, f, g, h, a, b, c, d, 0x17406110ul);
    Round(d, e, f, g, h, a, b, c, 0x0d8cd6f1ul);
    Round(c, d, e, f, g, h, a, b, 0xcdaa3b6dul);
    Round(b, c, d, e, f, g, h, a, 0xc0bbbe37ul);
    Round(a, b, c, d, e, f, g, h, 0x83613bdaul);
    Round(h, a, b, c, d, e, f, g, 0xdb48a363ul);
    Round(g, h, a, b, c, d, e, f, 0x0b02e931ul);
    Round(f, g, h, a, b, c, d, e, 0x6fd15ca7ul);
    Round(e, f, g, h, a, b, c, d, 0x521afacaul);
    Round(d, e, f, g, h, a, b, c, 0x31338431ul);
    Round(c, d, e, f, g, h, a, b, 0x6ed41a95ul);
    Round(b, c, d, e, f, g, h, a, 0x6d437890ul);
    Round(a, b, c, d, e, f, g, h, 0xc39c91f2ul);
    Round(h, a, b, c, d, e, f, g, 0x9eccabbdul);
    Round(g, h, a, b, c, d, e, f, 0xb5c9a0e6ul);
    Round(f, g, h, a, b, c, d, e, 0x532fb63cul);
    Round(e, f, g, h, a, b, c, d, 0xd2c741c6ul);
    Round(d, e, f, g, h, a, b, c, 0x07237ea3ul);
    Round(c, d, e, f, g, h, a, b, 0xa4954b68ul);
    Round(b, c, d, e, f, g, h, a, 0x4c191d76ul);

    w0 = t0 + a;
    w1 = t1 + b;
    w2 = t2 + c;
    w3 = t3 + d;
    w4 = t4 + e;
    w5 = t5 + f;
    w6 = t6 + g;
    w7 = t7 + h;

    // Transform 3
    a = 0x6a09e667ul;
    b = 0xbb67ae85ul;
    c = 0x3c6ef372ul;
    d = 0xa54ff53aul;
    e = 0x510e527ful;
    f = 0x9b05688cul;
    g = 0x1f83d9abul;
    h = 0x5be0cd19ul;

    Round(a, b, c, d, e, f, g, h, 0x428a2f98ul + w0);
    Round(h, a, b, c, d, e, f, g, 0x71374491ul + w1);
    Round(g, h, a, b, c, d, e, f, 0xb5c0fbcful + w2);
    Round(f, g, h, a, b, c, d, e, 0xe9b5dba5ul + w3);
    Round(e, f, g, h, a, b, c, d, 0x3956c25bul + w4);
    Round(d, e, f, g, h, a, b, c, 0x59f111f1ul + w5);
    Round(c, d, e, f, g, h, a, b, 0x923f82a4ul + w6);
    Round(b, c, d, e, f, g, h, a, 0xab1c5ed5ul + w7);
    Round(a, b, c, d, e, f, g, h, 0x5807aa98ul);
    Round(h, a, b, c, d, e, f, g, 0x12835b01ul);
    Round(g, h, a, b, c, d, e, f, 0x243185beul);
    Round(f, g, h, a, b, c, d, e, 0x550c7dc3ul);
    Round(e, f, g, h, a, b, c, d, 0x72be5d74ul);
    Round(d, e, f, g, h, a, b, c, 0x80deb1feul);
    Round(c, d, e, f, g, h, a, b, 0x9bdc06a7ul);
    Round(b, c, d, e, f, g, h, a, 0xc19bf274ul);
    Round(a, b, c, d, e, f, g, h, 0xe49b69c1ul + (w0 += sigma0(w1)));
    Round(h, a, b, c, d, e, f, g, 0xefbe4786ul + (w1 += 0xa00000ul + sigma0(w2)));
    Round(g, h, a, b, c, d, e, f, 0x0fc19dc6ul + (w2 += sigma1(w0) + sigma0(w3)));
    Round(f, g, h, a, b, c, d, e, 0x240ca1ccul + (w3 += sigma1(w1) + sigma0(w4)));
    Round(e, f, g, h, a, b, c, d, 0x2de92c6ful + (w4 += sigma1(w2) + sigma0(w5)));
    Round(d, e, f, g, h, a, b, c, 0x4a7484aaul + (w5 += sigma1(w3) + sigma0(w6)));
    Round(c, d, e, f, g, h, a, b, 0x5cb0a9dcul + (w6 += sigma1(w4) + 0x100ul + sigma0(w7)));
    Round(b, c, d, e, f, g, h, a, 0x76f988daul + (w7 += sigma1(w5) + w0 + 0x11002000ul));
    Round(a, b, c, d, e, f, g, h, 0x983e5152ul + (w8 = 0x80000000ul + sigma1(w6) + w1));
    Round(h, a, b, c, d, e, f, g, 0xa831c66dul + (w9 = sigma1(w7) + w2));
    Round(g, h, a, b, c, d, e, f, 0xb00327c8ul + (w10 = sigma1(w8) + w3));
    Round(f, g, h, a, b, c, d, e, 0xbf597fc7ul + (w11 = sigma1(w9) + w4));
    Round(e, f, g, h, a, b, c, d, 0xc6e00bf3ul + (w12 = sigma1(w10) + w5));
    Round(d, e, f, g, h, a, b, c, 0xd5a79147ul + (w13 = sigma1(w11) + w6));
    Round(c, d, e, f, g, h, a, b, 0x06ca6351ul + (w14 = sigma1(w12) + w7 + 0x400022ul));
    Round(b, c, d, e, f, g, h, a, 0x14292967ul + (w15 = 0x100ul + sigma1(w13) + w8 + sigma0(w0)));
    Round(a, b, c, d, e, f, g, h, 0x27b70a85ul + (w0 += sigma1(w14) + w9 + sigma0(w1)));
    Round(h, a, b, c, d, e, f, g, 0x2e1b2138ul + (w1 += sigma1(w15) + w10 + sigma0(w2)));
    Round(g, h, a, b, c, d, e, f, 0x4d2c6dfcul + (w2 += sigma1(w0) + w11 + sigma0(w3)));
    Round(f, g, h, a, b, c, d, e, 0x53380d13ul + (w3 += sigma1(w1) + w12 + sigma0(w4)));
    Round(e, f, g, h, a, b, c, d, 0x650a7354ul + (w4 += sigma1(w2) + w13 + sigma0(w5)));
    Round(d, e, f, g, h, a, b, c, 0x766a0abbul + (w5 += sigma1(w3) + w14 + sigma0(w6)));
    Round(c, d, e, f, g, h, a, b, 0x81c2c92eul + (w6 += sigma1(w4) + w15 + sigma0(w7)));
    Round(b, c, d, e, f, g, h, a, 0x92722c85ul + (w7 += sigma1(w5) + w0 + sigma0(w8)));
    Round(a, b, c, d, e, f, g, h, 0xa2bfe8a1ul + (w8 += sigma1(w6) + w1 + sigma0(w9)));
    Round(h, a, b, c, d, e, f, g, 0xa81a664bul + (w9 += sigma1(w7) + w2 + sigma0(w10)));
    Round(g, h, a, b, c, d, e, f, 0xc24b8b70ul + (w10 += sigma1(w8) + w3 + sigma0(w11)));
    Round(f, g, h, a, b, c, d, e, 0xc76c51a3ul + (w11 += sigma1(w9) + w4 + sigma0(w12)));
    Round(e, f, g, h, a, b, c, d, 0xd192e819ul + (w12 += sigma1(w10) + w5 + sigma0(w13)));
    Round(d, e, f, g, h, a, b, c, 0xd6990624ul + (w13 += sigma1(w11) + w6 + sigma0(w14)));
    Round(c, d, e, f, g, h, a, b, 0xf40e3585ul + (w14 += sigma1(w12) + w7 + sigma0(w15)));
    Round(b, c, d, e, f, g, h, a, 0x106aa070ul + (w15 += sigma1(w13) + w8 + sigma0(w0)));
    Round(a, b, c, d, e, f, g, h, 0x19a4c116ul + (w0 += sigma1(w14) + w9 + sigma0(w1)));
    Round(h, a, b, c, d, e, f, g, 0x1e376c08ul + (w1 += sigma1(w15) + w10 + sigma0(w2)));
    Round(g, h, a, b, c, d, e, f, 0x2748774cul + (w2 += sigma1(w0) + w11 + sigma0(w3)));
    Round(f, g, h, a, b, c, d, e, 0x34b0bcb5ul + (w3 += sigma1(w1) + w12 + sigma0(w4)));
    Round(e, f, g, h, a, b, c, d, 0x391c0cb3ul + (w4 += sigma1(w2) + w13 + sigma0(w5)));
    Round(d, e, f, g, h, a, b, c, 0x4ed8aa4aul + (w5 += sigma1(w3) + w14 + sigma0(w6)));
    Round(c, d, e, f, g, h, a, b, 0x5b9cca4ful + (w6 += sigma1(w4) + w15 + sigma0(w7)));
    Round(b, c, d, e, f, g, h, a, 0x682e6ff3ul + (w7 += sigma1(w5) + w0 + sigma0(w8)));
    Round(a, b, c, d, e, f, g, h, 0x748f82eeul + (w8 += sigma1(w6) + w1 + sigma0(w9)));
    Round(h, a, b, c, d, e, f, g, 0x78a5636ful + (w9 += sigma1(w7) + w2 + sigma0(w10)));
    Round(g, h, a, b, c, d, e, f, 0x84c87814ul + (w10 += sigma1(w8) + w3 + sigma0(w11)));
    Round(f, g, h, a, b, c, d, e, 0x8cc70208ul + (w11 += sigma1(w9) + w4 + sigma0(w12)));
    Round(e, f, g, h, a, b, c, d, 0x90befffaul + (w12 += sigma1(w10) + w5 + sigma0(w13)));
    Round(d, e, f, g, h, a, b, c, 0xa4506cebul + (w13 += sigma1(w11) + w6 + sigma0(w14)));
    Round(c, d, e, f, g, h, a, b, 0xbef9a3f7ul + (w14 + sigma1(w12) + w7 + sigma0(w15)));
    Round(b, c, d, e, f, g, h, a, 0xc67178f2ul + (w15 + sigma1(w13) + w8 + sigma0(w0)));

    // Output
    WriteBE32(out + 0, a + 0x6a09e667ul);
    WriteBE32(out + 4, b + 0xbb67ae85ul);
    WriteBE32(out + 8, c + 0x3c6ef372ul);
    WriteBE32(out + 12, d + 0xa54ff53aul);
    WriteBE32(out + 16, e + 0x510e527ful);
    WriteBE32(out + 20, f + 0x9b05688cul);
    WriteBE32(out + 24, g + 0x1f83d9abul);
    WriteBE32(out + 28, h + 0x5be0cd19ul);
}

} // namespace sha256

typedef void (*TransformType)(uint32_t*, const unsigned char*, size_t);
typedef void (*TransformD64Type)(unsigned char*, const unsigned char*);

template<TransformType tr>
void TransformD64Wrapper(unsigned char* out, const unsigned char* in)
{
    uint32_t s[8];
    static const unsigned char padding1[64] = {
        0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0,    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0,    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0,    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0
    };
    unsigned char buffer2[64] = {
        0,    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0,    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0,    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0
    };
    sha256::Initialize(s);
    tr(s, in, 1);
    tr(s, padding1, 1);
    WriteBE32(buffer2 + 0, s[0]);
    WriteBE32(buffer2 + 4, s[1]);
    WriteBE32(buffer2 + 8, s[2]);
    WriteBE32(buffer2 + 12, s[3]);
    WriteBE32(buffer2 + 16, s[4]);
    WriteBE32(buffer2 + 20, s[5]);
    WriteBE32(buffer2 + 24, s[6]);
    WriteBE32(buffer2 + 28, s[7]);
    sha256::Initialize(s);
    tr(s, buffer2, 1);
    WriteBE32(out + 0, s[0]);
    WriteBE32(out + 4, s[1]);
    WriteBE32(out + 8, s[2]);
    WriteBE32(out + 12, s[3]);
    WriteBE32(out + 16, s[4]);
    WriteBE32(out + 20, s[5]);
    WriteBE32(out + 24, s[6]);
    WriteBE32(out + 28, s[7]);
}

bool SelfTest(TransformType tr) {
    static const unsigned char in1[65] = {0, 0x80};
    static const unsigned char in2[129] = {
        0,
        32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 
        32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 
        0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0
    };
    static const uint32_t init[8] = {0x6a09e667ul, 0xbb67ae85ul, 0x3c6ef372ul, 0xa54ff53aul, 0x510e527ful, 0x9b05688cul, 0x1f83d9abul, 0x5be0cd19ul};
    static const uint32_t out1[8] = {0xe3b0c442ul, 0x98fc1c14ul, 0x9afbf4c8ul, 0x996fb924ul, 0x27ae41e4ul, 0x649b934cul, 0xa495991bul, 0x7852b855ul};
    static const uint32_t out2[8] = {0xce4153b0ul, 0x147c2a86ul, 0x3ed4298eul, 0xe0676bc8ul, 0x79fc77a1ul, 0x2abe1f49ul, 0xb2b055dful, 0x1069523eul};
    uint32_t buf[8];
    memcpy(buf, init, sizeof(buf));
    // Process nothing, and check we remain in the initial state.
    tr(buf, nullptr, 0);
    if (memcmp(buf, init, sizeof(buf))) return false;
    // Process the padded empty string (unaligned)
    tr(buf, in1 + 1, 1);
    if (memcmp(buf, out1, sizeof(buf))) return false;
    // Process 64 spaces (unaligned)
    memcpy(buf, init, sizeof(buf));
    tr(buf, in2 + 1, 2);
    if (memcmp(buf, out2, sizeof(buf))) return false;
    return true;
}

TransformType Transform = sha256::Transform;
TransformD64Type TransformD64 = sha256::TransformD64;
TransformD64Type TransformD64_4way = nullptr;
TransformD64Type TransformD64_8way = nullptr;

#if defined(USE_ASM) && (defined(__x86_64__) || defined(__amd64__) || defined(__i386__))
// We can't use cpuid.h's __get_cpuid as it does not support subleafs.
void inline cpuid(uint32_t leaf, uint32_t subleaf, uint32_t& a, uint32_t& b, uint32_t& c, uint32_t& d)
{
  __asm__ ("cpuid" : "=a"(a), "=b"(b), "=c"(c), "=d"(d) : "0"(leaf), "2"(subleaf));
}

/** Check whether the OS has enabled AVX registers. */
bool AVXEnabled()
{
    uint32_t a, d;
    __asm__("xgetbv" : "=a"(a), "=d"(d) : "c"(0));
    return (a & 6) == 6;
}
#endif
} // namespace


std::string SHA256AutoDetect()
{
    std::string ret = "standard";
#if defined(USE_ASM) && (defined(__x86_64__) || defined(__amd64__) || defined(__i386__))
    (void)AVXEnabled; // Silence unused warning (in case ENABLE_AVX2 is not defined)
    uint32_t eax, ebx, ecx, edx;
    cpuid(1, 0, eax, ebx, ecx, edx);
    if ((ecx >> 19) & 1) {
#if defined(__x86_64__) || defined(__amd64__)
        Transform = sha256_sse4::Transform;
        TransformD64 = TransformD64Wrapper<sha256_sse4::Transform>;
#endif
#if defined(ENABLE_SSE41) && !defined(BUILD_BITCOIN_INTERNAL)
        TransformD64_4way = sha256d64_sse41::Transform_4way;
        ret = "sse4(1way+4way)";
#if defined(ENABLE_AVX2) && !defined(BUILD_BITCOIN_INTERNAL)
        if (((ecx >> 27) & 1) && ((ecx >> 28) & 1)) { // XSAVE and AVX
            cpuid(7, 0, eax, ebx, ecx, edx);
            if ((ebx >> 5) & 1) { // AVX2 flag
                if (AVXEnabled()) { // OS has enabled AVX registers
                    TransformD64_8way = sha256d64_avx2::Transform_8way;
                    ret += ",avx2(8way)";
                }
            }
        }
#endif
#else
        ret = "sse4";
#endif
    }
#endif

    assert(SelfTest(Transform));
    return ret;
}

////// SHA-256

CSHA256::CSHA256() : bytes(0)
{
    sha256::Initialize(s);
}

CSHA256& CSHA256::Write(const unsigned char* data, size_t len)
{
    const unsigned char* end = data + len;
    size_t bufsize = bytes % 64;
    if (bufsize && bufsize + len >= 64) {
        // Fill the buffer, and process it.
        memcpy(buf + bufsize, data, 64 - bufsize);
        bytes += 64 - bufsize;
        data += 64 - bufsize;
        Transform(s, buf, 1);
        bufsize = 0;
    }
    if (end - data >= 64) {
        size_t blocks = (end - data) / 64;
        Transform(s, data, blocks);
        data += 64 * blocks;
        bytes += 64 * blocks;
    }
    if (end > data) {
        // Fill the buffer with what remains.
        memcpy(buf + bufsize, data, end - data);
        bytes += end - data;
    }
    return *this;
}

void CSHA256::Finalize(unsigned char hash[OUTPUT_SIZE])
{
    static const unsigned char pad[64] = {0x80};
    unsigned char sizedesc[8];
    WriteBE64(sizedesc, bytes << 3);
    Write(pad, 1 + ((119 - (bytes % 64)) % 64));
    Write(sizedesc, 8);
    WriteBE32(hash, s[0]);
    WriteBE32(hash + 4, s[1]);
    WriteBE32(hash + 8, s[2]);
    WriteBE32(hash + 12, s[3]);
    WriteBE32(hash + 16, s[4]);
    WriteBE32(hash + 20, s[5]);
    WriteBE32(hash + 24, s[6]);
    WriteBE32(hash + 28, s[7]);
}

CSHA256& CSHA256::Reset()
{
    bytes = 0;
    sha256::Initialize(s);
    return *this;
}

void SHA256D64(unsigned char* out, const unsigned char* in, size_t blocks)
{
    if (TransformD64_8way) {
        while (blocks >= 8) {
            TransformD64_8way(out, in);
            out += 256;
            in += 512;
            blocks -= 8;
        }
    }
    if (TransformD64_4way) {
        while (blocks >= 4) {
            TransformD64_4way(out, in);
            out += 128;
            in += 256;
            blocks -= 4;
        }
    }
    while (blocks) {
        TransformD64(out, in);
        out += 32;
        in += 64;
        --blocks;
    }
}
