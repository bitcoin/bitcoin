// Copyright (c) 2019 The Widecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Based on the public domain implementation by Andrew Moon
// poly1305-donna-unrolled.c from https://github.com/floodyberry/poly1305-donna

#include <crypto/common.h>
#include <crypto/poly1305.h>

#include <string.h>

#define mul32x32_64(a,b) ((uint64_t)(a) * (b))

void poly1305_auth(unsigned char out[POLY1305_TAGLEN], const unsigned char *m, size_t inlen, const unsigned char key[POLY1305_KEYLEN]) {
    uint32_t t0,t1,t2,t3;
    uint32_t h0,h1,h2,h3,h4;
    uint32_t r0,r1,r2,r3,r4;
    uint32_t s1,s2,s3,s4;
    uint32_t b, nb;
    size_t j;
    uint64_t t[5];
    uint64_t f0,f1,f2,f3;
    uint64_t g0,g1,g2,g3,g4;
    uint64_t c;
    unsigned char mp[16];

    /* clamp key */
    t0 = ReadLE32(key+0);
    t1 = ReadLE32(key+4);
    t2 = ReadLE32(key+8);
    t3 = ReadLE32(key+12);

    /* precompute multipliers */
    r0 = t0 & 0x3ffffff; t0 >>= 26; t0 |= t1 << 6;
    r1 = t0 & 0x3ffff03; t1 >>= 20; t1 |= t2 << 12;
    r2 = t1 & 0x3ffc0ff; t2 >>= 14; t2 |= t3 << 18;
    r3 = t2 & 0x3f03fff; t3 >>= 8;
    r4 = t3 & 0x00fffff;

    s1 = r1 * 5;
    s2 = r2 * 5;
    s3 = r3 * 5;
    s4 = r4 * 5;

    /* init state */
    h0 = 0;
    h1 = 0;
    h2 = 0;
    h3 = 0;
    h4 = 0;

    /* full blocks */
    if (inlen < 16) goto poly1305_donna_atmost15bytes;
poly1305_donna_16bytes:
    m += 16;
    inlen -= 16;

    t0 = ReadLE32(m-16);
    t1 = ReadLE32(m-12);
    t2 = ReadLE32(m-8);
    t3 = ReadLE32(m-4);

    h0 += t0 & 0x3ffffff;
    h1 += ((((uint64_t)t1 << 32) | t0) >> 26) & 0x3ffffff;
    h2 += ((((uint64_t)t2 << 32) | t1) >> 20) & 0x3ffffff;
    h3 += ((((uint64_t)t3 << 32) | t2) >> 14) & 0x3ffffff;
    h4 += (t3 >> 8) | (1 << 24);


poly1305_donna_mul:
    t[0]  = mul32x32_64(h0,r0) + mul32x32_64(h1,s4) + mul32x32_64(h2,s3) + mul32x32_64(h3,s2) + mul32x32_64(h4,s1);
    t[1]  = mul32x32_64(h0,r1) + mul32x32_64(h1,r0) + mul32x32_64(h2,s4) + mul32x32_64(h3,s3) + mul32x32_64(h4,s2);
    t[2]  = mul32x32_64(h0,r2) + mul32x32_64(h1,r1) + mul32x32_64(h2,r0) + mul32x32_64(h3,s4) + mul32x32_64(h4,s3);
    t[3]  = mul32x32_64(h0,r3) + mul32x32_64(h1,r2) + mul32x32_64(h2,r1) + mul32x32_64(h3,r0) + mul32x32_64(h4,s4);
    t[4]  = mul32x32_64(h0,r4) + mul32x32_64(h1,r3) + mul32x32_64(h2,r2) + mul32x32_64(h3,r1) + mul32x32_64(h4,r0);

                    h0 = (uint32_t)t[0] & 0x3ffffff; c =           (t[0] >> 26);
    t[1] += c;      h1 = (uint32_t)t[1] & 0x3ffffff; b = (uint32_t)(t[1] >> 26);
    t[2] += b;      h2 = (uint32_t)t[2] & 0x3ffffff; b = (uint32_t)(t[2] >> 26);
    t[3] += b;      h3 = (uint32_t)t[3] & 0x3ffffff; b = (uint32_t)(t[3] >> 26);
    t[4] += b;      h4 = (uint32_t)t[4] & 0x3ffffff; b = (uint32_t)(t[4] >> 26);
    h0 += b * 5;

    if (inlen >= 16) goto poly1305_donna_16bytes;

    /* final bytes */
poly1305_donna_atmost15bytes:
    if (!inlen) goto poly1305_donna_finish;

    for (j = 0; j < inlen; j++) mp[j] = m[j];
    mp[j++] = 1;
    for (; j < 16; j++) mp[j] = 0;
    inlen = 0;

    t0 = ReadLE32(mp+0);
    t1 = ReadLE32(mp+4);
    t2 = ReadLE32(mp+8);
    t3 = ReadLE32(mp+12);

    h0 += t0 & 0x3ffffff;
    h1 += ((((uint64_t)t1 << 32) | t0) >> 26) & 0x3ffffff;
    h2 += ((((uint64_t)t2 << 32) | t1) >> 20) & 0x3ffffff;
    h3 += ((((uint64_t)t3 << 32) | t2) >> 14) & 0x3ffffff;
    h4 += (t3 >> 8);

    goto poly1305_donna_mul;

poly1305_donna_finish:
                 b = h0 >> 26; h0 = h0 & 0x3ffffff;
    h1 +=     b; b = h1 >> 26; h1 = h1 & 0x3ffffff;
    h2 +=     b; b = h2 >> 26; h2 = h2 & 0x3ffffff;
    h3 +=     b; b = h3 >> 26; h3 = h3 & 0x3ffffff;
    h4 +=     b; b = h4 >> 26; h4 = h4 & 0x3ffffff;
    h0 += b * 5; b = h0 >> 26; h0 = h0 & 0x3ffffff;
    h1 +=     b;

    g0 = h0 + 5; b = g0 >> 26; g0 &= 0x3ffffff;
    g1 = h1 + b; b = g1 >> 26; g1 &= 0x3ffffff;
    g2 = h2 + b; b = g2 >> 26; g2 &= 0x3ffffff;
    g3 = h3 + b; b = g3 >> 26; g3 &= 0x3ffffff;
    g4 = h4 + b - (1 << 26);

    b = (g4 >> 31) - 1;
    nb = ~b;
    h0 = (h0 & nb) | (g0 & b);
    h1 = (h1 & nb) | (g1 & b);
    h2 = (h2 & nb) | (g2 & b);
    h3 = (h3 & nb) | (g3 & b);
    h4 = (h4 & nb) | (g4 & b);

    f0 = ((h0      ) | (h1 << 26)) + (uint64_t)ReadLE32(&key[16]);
    f1 = ((h1 >>  6) | (h2 << 20)) + (uint64_t)ReadLE32(&key[20]);
    f2 = ((h2 >> 12) | (h3 << 14)) + (uint64_t)ReadLE32(&key[24]);
    f3 = ((h3 >> 18) | (h4 <<  8)) + (uint64_t)ReadLE32(&key[28]);

    WriteLE32(&out[ 0], f0); f1 += (f0 >> 32);
    WriteLE32(&out[ 4], f1); f2 += (f1 >> 32);
    WriteLE32(&out[ 8], f2); f3 += (f2 >> 32);
    WriteLE32(&out[12], f3);
}
