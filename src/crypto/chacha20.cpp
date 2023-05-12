// Copyright (c) 2017-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Based on the public domain implementation 'merged' by D. J. Bernstein
// See https://cr.yp.to/chacha.html.

#include <crypto/common.h>
#include <crypto/chacha20.h>

#include <algorithm>
#include <string.h>

constexpr static inline uint32_t rotl32(uint32_t v, int c) { return (v << c) | (v >> (32 - c)); }

#define QUARTERROUND(a,b,c,d) \
  a += b; d = rotl32(d ^ a, 16); \
  c += d; b = rotl32(b ^ c, 12); \
  a += b; d = rotl32(d ^ a, 8); \
  c += d; b = rotl32(b ^ c, 7);

#define REPEAT10(a) do { {a}; {a}; {a}; {a}; {a}; {a}; {a}; {a}; {a}; {a}; } while(0)

void ChaCha20Aligned::SetKey32(const unsigned char* k)
{
    input[0] = ReadLE32(k + 0);
    input[1] = ReadLE32(k + 4);
    input[2] = ReadLE32(k + 8);
    input[3] = ReadLE32(k + 12);
    input[4] = ReadLE32(k + 16);
    input[5] = ReadLE32(k + 20);
    input[6] = ReadLE32(k + 24);
    input[7] = ReadLE32(k + 28);
    input[8] = 0;
    input[9] = 0;
    input[10] = 0;
    input[11] = 0;
}

ChaCha20Aligned::ChaCha20Aligned()
{
    memset(input, 0, sizeof(input));
}

ChaCha20Aligned::ChaCha20Aligned(const unsigned char* key32)
{
    SetKey32(key32);
}

void ChaCha20Aligned::SetIV(uint64_t iv)
{
    input[10] = iv;
    input[11] = iv >> 32;
}

void ChaCha20Aligned::Seek64(uint64_t pos)
{
    input[8] = pos;
    input[9] = pos >> 32;
}

inline void ChaCha20Aligned::Keystream64(unsigned char* c, size_t blocks)
{
    uint32_t x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15;
    uint32_t j4, j5, j6, j7, j8, j9, j10, j11, j12, j13, j14, j15;

    if (!blocks) return;

    j4 = input[0];
    j5 = input[1];
    j6 = input[2];
    j7 = input[3];
    j8 = input[4];
    j9 = input[5];
    j10 = input[6];
    j11 = input[7];
    j12 = input[8];
    j13 = input[9];
    j14 = input[10];
    j15 = input[11];

    for (;;) {
        x0 = 0x61707865;
        x1 = 0x3320646e;
        x2 = 0x79622d32;
        x3 = 0x6b206574;
        x4 = j4;
        x5 = j5;
        x6 = j6;
        x7 = j7;
        x8 = j8;
        x9 = j9;
        x10 = j10;
        x11 = j11;
        x12 = j12;
        x13 = j13;
        x14 = j14;
        x15 = j15;

        // The 20 inner ChaCha20 rounds are unrolled here for performance.
        REPEAT10(
            QUARTERROUND( x0, x4, x8,x12);
            QUARTERROUND( x1, x5, x9,x13);
            QUARTERROUND( x2, x6,x10,x14);
            QUARTERROUND( x3, x7,x11,x15);
            QUARTERROUND( x0, x5,x10,x15);
            QUARTERROUND( x1, x6,x11,x12);
            QUARTERROUND( x2, x7, x8,x13);
            QUARTERROUND( x3, x4, x9,x14);
        );

        x0 += 0x61707865;
        x1 += 0x3320646e;
        x2 += 0x79622d32;
        x3 += 0x6b206574;
        x4 += j4;
        x5 += j5;
        x6 += j6;
        x7 += j7;
        x8 += j8;
        x9 += j9;
        x10 += j10;
        x11 += j11;
        x12 += j12;
        x13 += j13;
        x14 += j14;
        x15 += j15;

        ++j12;
        if (!j12) ++j13;

        WriteLE32(c + 0, x0);
        WriteLE32(c + 4, x1);
        WriteLE32(c + 8, x2);
        WriteLE32(c + 12, x3);
        WriteLE32(c + 16, x4);
        WriteLE32(c + 20, x5);
        WriteLE32(c + 24, x6);
        WriteLE32(c + 28, x7);
        WriteLE32(c + 32, x8);
        WriteLE32(c + 36, x9);
        WriteLE32(c + 40, x10);
        WriteLE32(c + 44, x11);
        WriteLE32(c + 48, x12);
        WriteLE32(c + 52, x13);
        WriteLE32(c + 56, x14);
        WriteLE32(c + 60, x15);

        if (blocks == 1) {
            input[8] = j12;
            input[9] = j13;
            return;
        }
        blocks -= 1;
        c += 64;
    }
}

inline void ChaCha20Aligned::Crypt64(const unsigned char* m, unsigned char* c, size_t blocks)
{
    uint32_t x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15;
    uint32_t j4, j5, j6, j7, j8, j9, j10, j11, j12, j13, j14, j15;

    if (!blocks) return;

    j4 = input[0];
    j5 = input[1];
    j6 = input[2];
    j7 = input[3];
    j8 = input[4];
    j9 = input[5];
    j10 = input[6];
    j11 = input[7];
    j12 = input[8];
    j13 = input[9];
    j14 = input[10];
    j15 = input[11];

    for (;;) {
        x0 = 0x61707865;
        x1 = 0x3320646e;
        x2 = 0x79622d32;
        x3 = 0x6b206574;
        x4 = j4;
        x5 = j5;
        x6 = j6;
        x7 = j7;
        x8 = j8;
        x9 = j9;
        x10 = j10;
        x11 = j11;
        x12 = j12;
        x13 = j13;
        x14 = j14;
        x15 = j15;

        // The 20 inner ChaCha20 rounds are unrolled here for performance.
        REPEAT10(
            QUARTERROUND( x0, x4, x8,x12);
            QUARTERROUND( x1, x5, x9,x13);
            QUARTERROUND( x2, x6,x10,x14);
            QUARTERROUND( x3, x7,x11,x15);
            QUARTERROUND( x0, x5,x10,x15);
            QUARTERROUND( x1, x6,x11,x12);
            QUARTERROUND( x2, x7, x8,x13);
            QUARTERROUND( x3, x4, x9,x14);
        );

        x0 += 0x61707865;
        x1 += 0x3320646e;
        x2 += 0x79622d32;
        x3 += 0x6b206574;
        x4 += j4;
        x5 += j5;
        x6 += j6;
        x7 += j7;
        x8 += j8;
        x9 += j9;
        x10 += j10;
        x11 += j11;
        x12 += j12;
        x13 += j13;
        x14 += j14;
        x15 += j15;

        x0 ^= ReadLE32(m + 0);
        x1 ^= ReadLE32(m + 4);
        x2 ^= ReadLE32(m + 8);
        x3 ^= ReadLE32(m + 12);
        x4 ^= ReadLE32(m + 16);
        x5 ^= ReadLE32(m + 20);
        x6 ^= ReadLE32(m + 24);
        x7 ^= ReadLE32(m + 28);
        x8 ^= ReadLE32(m + 32);
        x9 ^= ReadLE32(m + 36);
        x10 ^= ReadLE32(m + 40);
        x11 ^= ReadLE32(m + 44);
        x12 ^= ReadLE32(m + 48);
        x13 ^= ReadLE32(m + 52);
        x14 ^= ReadLE32(m + 56);
        x15 ^= ReadLE32(m + 60);

        ++j12;
        if (!j12) ++j13;

        WriteLE32(c + 0, x0);
        WriteLE32(c + 4, x1);
        WriteLE32(c + 8, x2);
        WriteLE32(c + 12, x3);
        WriteLE32(c + 16, x4);
        WriteLE32(c + 20, x5);
        WriteLE32(c + 24, x6);
        WriteLE32(c + 28, x7);
        WriteLE32(c + 32, x8);
        WriteLE32(c + 36, x9);
        WriteLE32(c + 40, x10);
        WriteLE32(c + 44, x11);
        WriteLE32(c + 48, x12);
        WriteLE32(c + 52, x13);
        WriteLE32(c + 56, x14);
        WriteLE32(c + 60, x15);

        if (blocks == 1) {
            input[8] = j12;
            input[9] = j13;
            return;
        }
        blocks -= 1;
        c += 64;
        m += 64;
    }
}

void ChaCha20::Keystream(unsigned char* c, size_t bytes)
{
    if (!bytes) return;
    if (m_bufleft) {
        unsigned reuse = std::min<size_t>(m_bufleft, bytes);
        memcpy(c, m_buffer + 64 - m_bufleft, reuse);
        m_bufleft -= reuse;
        bytes -= reuse;
        c += reuse;
    }
    if (bytes >= 64) {
        size_t blocks = bytes / 64;
        m_aligned.Keystream64(c, blocks);
        c += blocks * 64;
        bytes -= blocks * 64;
    }
    if (bytes) {
        m_aligned.Keystream64(m_buffer, 1);
        memcpy(c, m_buffer, bytes);
        m_bufleft = 64 - bytes;
    }
}

void ChaCha20::Crypt(const unsigned char* m, unsigned char* c, size_t bytes)
{
    if (!bytes) return;
    if (m_bufleft) {
        unsigned reuse = std::min<size_t>(m_bufleft, bytes);
        for (unsigned i = 0; i < reuse; i++) {
            c[i] = m[i] ^ m_buffer[64 - m_bufleft + i];
        }
        m_bufleft -= reuse;
        bytes -= reuse;
        c += reuse;
        m += reuse;
    }
    if (bytes >= 64) {
        size_t blocks = bytes / 64;
        m_aligned.Crypt64(m, c, blocks);
        c += blocks * 64;
        m += blocks * 64;
        bytes -= blocks * 64;
    }
    if (bytes) {
        m_aligned.Keystream64(m_buffer, 1);
        for (unsigned i = 0; i < bytes; i++) {
            c[i] = m[i] ^ m_buffer[i];
        }
        m_bufleft = 64 - bytes;
    }
}
