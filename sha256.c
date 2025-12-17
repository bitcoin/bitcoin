/* Parts translated from Bitcoin Core's C++ project:
 * src/crypto/sha256.cpp commit eb7daf4d600eeb631427c018a984a77a34aca66e
 *
 * Copyright (c) 2014-2018 The Bitcoin Core developers
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */
#include "sha256.h"

#include <limits.h>
#include "simplicity_assert.h"

/* Multiplying a uint32_t by 1U promotes a value's type to the wider of unsigned int and uint32_t,
 * avoiding any possible issues with signed integer promotions causing havoc with unsigned modular arithmetic.
 */
static inline uint32_t Ch(uint32_t x, uint32_t y, uint32_t z) { return z ^ (x & (y ^ z)); }
static inline uint32_t Maj(uint32_t x, uint32_t y, uint32_t z) { return (x & y) | (z & (x | y)); }
static inline uint32_t Sigma0(uint32_t x) { return (x >> 2 | 1U * x << 30) ^ (x >> 13 | 1U * x << 19) ^ (x >> 22 | 1U * x << 10); }
static inline uint32_t Sigma1(uint32_t x) { return (x >> 6 | 1U * x << 26) ^ (x >> 11 | 1U * x << 21) ^ (x >> 25 | 1U * x << 7); }
static inline uint32_t sigma0(uint32_t x) { return (x >> 7 | 1U * x << 25) ^ (x >> 18 | 1U * x << 14) ^ (x >> 3); }
static inline uint32_t sigma1(uint32_t x) { return (x >> 17 | 1U * x << 15) ^ (x >> 19 | 1U * x << 13) ^ (x >> 10); }

/* One round of SHA-256.
 *
 * Precondition: NULL != d
 *               NULL != h
 *               d != h
 */
static inline void Round(uint32_t a, uint32_t b, uint32_t c, uint32_t* d, uint32_t e, uint32_t f, uint32_t g, uint32_t* h, uint32_t k) {
    uint32_t t1 = 1U * *h + Sigma1(e) + Ch(e, f, g) + k;
    uint32_t t2 = 1U * Sigma0(a) + Maj(a, b, c);
    *d = 1U * *d + t1;
    *h = 1U * t1 + t2;
}

/* Given a 256-bit 's' and a 512-bit 'chunk', then 's' becomes the value of the SHA-256 compression function ("added" to the original 's' value).
 *
 * Precondition: uint32_t s[8];
 *               uint32_t chunk[16]
 */
static void sha256_compression_portable(uint32_t* s, const uint32_t* chunk) {
    uint32_t a = s[0], b = s[1], c = s[2], d = s[3], e = s[4], f = s[5], g = s[6], h = s[7];
    uint32_t w0, w1, w2, w3, w4, w5, w6, w7, w8, w9, w10, w11, w12, w13, w14, w15;

    Round(a, b, c, &d, e, f, g, &h, 0x428a2f98U + (w0 = chunk[0]));
    Round(h, a, b, &c, d, e, f, &g, 0x71374491U + (w1 = chunk[1]));
    Round(g, h, a, &b, c, d, e, &f, 0xb5c0fbcfU + (w2 = chunk[2]));
    Round(f, g, h, &a, b, c, d, &e, 0xe9b5dba5U + (w3 = chunk[3]));
    Round(e, f, g, &h, a, b, c, &d, 0x3956c25bU + (w4 = chunk[4]));
    Round(d, e, f, &g, h, a, b, &c, 0x59f111f1U + (w5 = chunk[5]));
    Round(c, d, e, &f, g, h, a, &b, 0x923f82a4U + (w6 = chunk[6]));
    Round(b, c, d, &e, f, g, h, &a, 0xab1c5ed5U + (w7 = chunk[7]));
    Round(a, b, c, &d, e, f, g, &h, 0xd807aa98U + (w8 = chunk[8]));
    Round(h, a, b, &c, d, e, f, &g, 0x12835b01U + (w9 = chunk[9]));
    Round(g, h, a, &b, c, d, e, &f, 0x243185beU + (w10 = chunk[10]));
    Round(f, g, h, &a, b, c, d, &e, 0x550c7dc3U + (w11 = chunk[11]));
    Round(e, f, g, &h, a, b, c, &d, 0x72be5d74U + (w12 = chunk[12]));
    Round(d, e, f, &g, h, a, b, &c, 0x80deb1feU + (w13 = chunk[13]));
    Round(c, d, e, &f, g, h, a, &b, 0x9bdc06a7U + (w14 = chunk[14]));
    Round(b, c, d, &e, f, g, h, &a, 0xc19bf174U + (w15 = chunk[15]));

    Round(a, b, c, &d, e, f, g, &h, 0xe49b69c1U + (w0 = 1U * w0 + sigma1(w14) + w9 + sigma0(w1)));
    Round(h, a, b, &c, d, e, f, &g, 0xefbe4786U + (w1 = 1U * w1 + sigma1(w15) + w10 + sigma0(w2)));
    Round(g, h, a, &b, c, d, e, &f, 0x0fc19dc6U + (w2 = 1U * w2 + sigma1(w0) + w11 + sigma0(w3)));
    Round(f, g, h, &a, b, c, d, &e, 0x240ca1ccU + (w3 = 1U * w3 + sigma1(w1) + w12 + sigma0(w4)));
    Round(e, f, g, &h, a, b, c, &d, 0x2de92c6fU + (w4 = 1U * w4 + sigma1(w2) + w13 + sigma0(w5)));
    Round(d, e, f, &g, h, a, b, &c, 0x4a7484aaU + (w5 = 1U * w5 + sigma1(w3) + w14 + sigma0(w6)));
    Round(c, d, e, &f, g, h, a, &b, 0x5cb0a9dcU + (w6 = 1U * w6 + sigma1(w4) + w15 + sigma0(w7)));
    Round(b, c, d, &e, f, g, h, &a, 0x76f988daU + (w7 = 1U * w7 + sigma1(w5) + w0 + sigma0(w8)));
    Round(a, b, c, &d, e, f, g, &h, 0x983e5152U + (w8 = 1U * w8 + sigma1(w6) + w1 + sigma0(w9)));
    Round(h, a, b, &c, d, e, f, &g, 0xa831c66dU + (w9 = 1U * w9 + sigma1(w7) + w2 + sigma0(w10)));
    Round(g, h, a, &b, c, d, e, &f, 0xb00327c8U + (w10 = 1U * w10 + sigma1(w8) + w3 + sigma0(w11)));
    Round(f, g, h, &a, b, c, d, &e, 0xbf597fc7U + (w11 = 1U * w11 + sigma1(w9) + w4 + sigma0(w12)));
    Round(e, f, g, &h, a, b, c, &d, 0xc6e00bf3U + (w12 = 1U * w12 + sigma1(w10) + w5 + sigma0(w13)));
    Round(d, e, f, &g, h, a, b, &c, 0xd5a79147U + (w13 = 1U * w13 + sigma1(w11) + w6 + sigma0(w14)));
    Round(c, d, e, &f, g, h, a, &b, 0x06ca6351U + (w14 = 1U * w14 + sigma1(w12) + w7 + sigma0(w15)));
    Round(b, c, d, &e, f, g, h, &a, 0x14292967U + (w15 = 1U * w15 + sigma1(w13) + w8 + sigma0(w0)));

    Round(a, b, c, &d, e, f, g, &h, 0x27b70a85U + (w0 = 1U * w0 + sigma1(w14) + w9 + sigma0(w1)));
    Round(h, a, b, &c, d, e, f, &g, 0x2e1b2138U + (w1 = 1U * w1 + sigma1(w15) + w10 + sigma0(w2)));
    Round(g, h, a, &b, c, d, e, &f, 0x4d2c6dfcU + (w2 = 1U * w2 + sigma1(w0) + w11 + sigma0(w3)));
    Round(f, g, h, &a, b, c, d, &e, 0x53380d13U + (w3 = 1U * w3 + sigma1(w1) + w12 + sigma0(w4)));
    Round(e, f, g, &h, a, b, c, &d, 0x650a7354U + (w4 = 1U * w4 + sigma1(w2) + w13 + sigma0(w5)));
    Round(d, e, f, &g, h, a, b, &c, 0x766a0abbU + (w5 = 1U * w5 + sigma1(w3) + w14 + sigma0(w6)));
    Round(c, d, e, &f, g, h, a, &b, 0x81c2c92eU + (w6 = 1U * w6 + sigma1(w4) + w15 + sigma0(w7)));
    Round(b, c, d, &e, f, g, h, &a, 0x92722c85U + (w7 = 1U * w7 + sigma1(w5) + w0 + sigma0(w8)));
    Round(a, b, c, &d, e, f, g, &h, 0xa2bfe8a1U + (w8 = 1U * w8 + sigma1(w6) + w1 + sigma0(w9)));
    Round(h, a, b, &c, d, e, f, &g, 0xa81a664bU + (w9 = 1U * w9 + sigma1(w7) + w2 + sigma0(w10)));
    Round(g, h, a, &b, c, d, e, &f, 0xc24b8b70U + (w10 = 1U * w10 + sigma1(w8) + w3 + sigma0(w11)));
    Round(f, g, h, &a, b, c, d, &e, 0xc76c51a3U + (w11 = 1U * w11 + sigma1(w9) + w4 + sigma0(w12)));
    Round(e, f, g, &h, a, b, c, &d, 0xd192e819U + (w12 = 1U * w12 + sigma1(w10) + w5 + sigma0(w13)));
    Round(d, e, f, &g, h, a, b, &c, 0xd6990624U + (w13 = 1U * w13 + sigma1(w11) + w6 + sigma0(w14)));
    Round(c, d, e, &f, g, h, a, &b, 0xf40e3585U + (w14 = 1U * w14 + sigma1(w12) + w7 + sigma0(w15)));
    Round(b, c, d, &e, f, g, h, &a, 0x106aa070U + (w15 = 1U * w15 + sigma1(w13) + w8 + sigma0(w0)));

    Round(a, b, c, &d, e, f, g, &h, 0x19a4c116U + (w0 = 1U * w0 + sigma1(w14) + w9 + sigma0(w1)));
    Round(h, a, b, &c, d, e, f, &g, 0x1e376c08U + (w1 = 1U * w1 + sigma1(w15) + w10 + sigma0(w2)));
    Round(g, h, a, &b, c, d, e, &f, 0x2748774cU + (w2 = 1U * w2 + sigma1(w0) + w11 + sigma0(w3)));
    Round(f, g, h, &a, b, c, d, &e, 0x34b0bcb5U + (w3 = 1U * w3 + sigma1(w1) + w12 + sigma0(w4)));
    Round(e, f, g, &h, a, b, c, &d, 0x391c0cb3U + (w4 = 1U * w4 + sigma1(w2) + w13 + sigma0(w5)));
    Round(d, e, f, &g, h, a, b, &c, 0x4ed8aa4aU + (w5 = 1U * w5 + sigma1(w3) + w14 + sigma0(w6)));
    Round(c, d, e, &f, g, h, a, &b, 0x5b9cca4fU + (w6 = 1U * w6 + sigma1(w4) + w15 + sigma0(w7)));
    Round(b, c, d, &e, f, g, h, &a, 0x682e6ff3U + (w7 = 1U * w7 + sigma1(w5) + w0 + sigma0(w8)));
    Round(a, b, c, &d, e, f, g, &h, 0x748f82eeU + (w8 = 1U * w8 + sigma1(w6) + w1 + sigma0(w9)));
    Round(h, a, b, &c, d, e, f, &g, 0x78a5636fU + (w9 = 1U * w9 + sigma1(w7) + w2 + sigma0(w10)));
    Round(g, h, a, &b, c, d, e, &f, 0x84c87814U + (w10 = 1U * w10 + sigma1(w8) + w3 + sigma0(w11)));
    Round(f, g, h, &a, b, c, d, &e, 0x8cc70208U + (w11 = 1U * w11 + sigma1(w9) + w4 + sigma0(w12)));
    Round(e, f, g, &h, a, b, c, &d, 0x90befffaU + (w12 = 1U * w12 + sigma1(w10) + w5 + sigma0(w13)));
    Round(d, e, f, &g, h, a, b, &c, 0xa4506cebU + (w13 = 1U * w13 + sigma1(w11) + w6 + sigma0(w14)));
    Round(c, d, e, &f, g, h, a, &b, 0xbef9a3f7U + (1U * w14 + sigma1(w12) + w7 + sigma0(w15)));
    Round(b, c, d, &e, f, g, h, &a, 0xc67178f2U + (1U * w15 + sigma1(w13) + w8 + sigma0(w0)));

    s[0] = 1U * s[0] + a;
    s[1] = 1U * s[1] + b;
    s[2] = 1U * s[2] + c;
    s[3] = 1U * s[3] + d;
    s[4] = 1U * s[4] + e;
    s[5] = 1U * s[5] + f;
    s[6] = 1U * s[6] + g;
    s[7] = 1U * s[7] + h;
}

void (*simplicity_sha256_compression)(uint32_t* midstate, const uint32_t* block) = sha256_compression_portable;

/* For information purposes only.
 * Returns true if the sha256_compression implementation has been optimized for the CPU.
 * Otherwise returns false.
 */
bool simplicity_sha256_compression_is_optimized(void) {
  return sha256_compression_portable != simplicity_sha256_compression;
};

/* Given a SHA-256 midstate, 'h', of 'len / 512' blocks, and
 * a 'block' with 'len % 512' bits set and with the remaining bits set to 0,
 * finalize the SHA-256 computation by adding SHA-256 padding and set 'h' to the resulting SHA-256 hash.
 *
 * Precondition: uint32_t h[8];
 *               uint32_t block[16];
 */
static void sha256_end(uint32_t* h, uint32_t* block, const uint_fast64_t len) {
  block[len / 32 % 16] |= (uint32_t)1 << (31 - len % 32);
  if (448 <= len % 512) {
    simplicity_sha256_compression(h, block);
    memset(block, 0, sizeof(uint32_t[14]));
  }
  block[14] = (uint32_t)(len >> 32);
  block[15] = (uint32_t)len;
  simplicity_sha256_compression(h, block);
}

/* Compute the SHA-256 hash, 'h', of the bitstring represented by 's'.
 *
 * Precondition: uint32_t h[8];
 *               '*s' is a valid bitstring;
 *               's->len < 2^64;
 */
void simplicity_sha256_bitstring(uint32_t* h, const bitstring* s) {
  /* This static assert should never fail if uint32_t exists.
   * But for more certainty, we note that the correctness of this implementation depends on CHAR_BIT being no more than 32.
   */
  static_assert(CHAR_BIT <= 32, "CHAR_BIT has to be less than 32 for uint32_t to even exist.");

  uint32_t block[16] = { 0 };
  size_t count = 0;
  sha256_iv(h);
  if (s->len) {
    block[0] = s->arr[s->offset / CHAR_BIT];
    if (s->len < CHAR_BIT - s->offset % CHAR_BIT) {
      /* s->len is so short that we don't even use a whole char.
       * Zero out the low bits.
       */
      block[0] = block[0] >> (CHAR_BIT - s->offset % CHAR_BIT - s->len)
                          << (CHAR_BIT - s->offset % CHAR_BIT - s->len);
      count = s->len;
    } else {
      count = CHAR_BIT - s->offset % CHAR_BIT;
    }
    block[0] = 1U * block[0] << (32 - CHAR_BIT + s->offset % CHAR_BIT);

    while (count < s->len) {
      unsigned char ch = s->arr[(s->offset + count)/CHAR_BIT];
      size_t delta = CHAR_BIT;
      if (s->len - count < CHAR_BIT) {
        delta = s->len - count;
        /* Zero out any extra low bits that 'ch' may have. */
        ch = (unsigned char)(ch >> (CHAR_BIT - delta) << (CHAR_BIT - delta));
      }

      if (count / 32 != (count + CHAR_BIT) / 32) {
        /* The next character from s->arr straddles (or almost straddles) the boundary of two elements of the block array. */
        block[count / 32 % 16] |= (uint32_t)((uint_fast32_t)ch >> (count + CHAR_BIT) % 32);
        if (count / 512 != (count + delta) / 512) {
          simplicity_sha256_compression(h, block);
          memset(block, 0, sizeof(uint32_t[16]));
        }
      }
      if ((count + CHAR_BIT) % 32) {
        block[(count + CHAR_BIT) / 32 % 16] |= (uint32_t)(1U * (uint_fast32_t)ch << (32 - (count + CHAR_BIT) % 32));
      }
      count += delta;
    }
  }
  simplicity_assert(count == s->len);
  sha256_end(h, block, s->len);
}

#ifndef NO_SHA_NI_FLAG
#include "sha256_x86.inc"
#endif
