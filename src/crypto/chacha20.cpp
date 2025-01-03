// Copyright (c) 2017-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Based on the public domain implementation 'merged' by D. J. Bernstein
// See https://cr.yp.to/chacha.html.

#include <crypto/common.h>
#include <crypto/chacha20.h>
#include <support/cleanse.h>
#include <span.h>

#include <algorithm>
#include <bit>
#include <string.h>

#define QUARTERROUND(a,b,c,d) \
  a += b; d = std::rotl(d ^ a, 16); \
  c += d; b = std::rotl(b ^ c, 12); \
  a += b; d = std::rotl(d ^ a, 8); \
  c += d; b = std::rotl(b ^ c, 7);

#define REPEAT10(a) do { {a}; {a}; {a}; {a}; {a}; {a}; {a}; {a}; {a}; {a}; } while(0)

void ChaCha20Aligned::SetKey(std::span<const std::byte> key) noexcept
{
    assert(key.size() == KEYLEN);
    input[0] = ReadLE32(key.data() + 0);
    input[1] = ReadLE32(key.data() + 4);
    input[2] = ReadLE32(key.data() + 8);
    input[3] = ReadLE32(key.data() + 12);
    input[4] = ReadLE32(key.data() + 16);
    input[5] = ReadLE32(key.data() + 20);
    input[6] = ReadLE32(key.data() + 24);
    input[7] = ReadLE32(key.data() + 28);
    input[8] = 0;
    input[9] = 0;
    input[10] = 0;
    input[11] = 0;
}

ChaCha20Aligned::~ChaCha20Aligned()
{
    memory_cleanse(input, sizeof(input));
}

ChaCha20Aligned::ChaCha20Aligned(std::span<const std::byte> key) noexcept
{
    SetKey(key);
}

void ChaCha20Aligned::Seek(Nonce96 nonce, uint32_t block_counter) noexcept
{
    input[8] = block_counter;
    input[9] = nonce.first;
    input[10] = nonce.second;
    input[11] = nonce.second >> 32;
}

inline void ChaCha20Aligned::Keystream(std::span<std::byte> output) noexcept
{
    std::byte* c = output.data();
    size_t blocks = output.size() / BLOCKLEN;
    assert(blocks * BLOCKLEN == output.size());

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
        c += BLOCKLEN;
    }
}

inline void ChaCha20Aligned::Crypt(std::span<const std::byte> in_bytes, std::span<std::byte> out_bytes) noexcept
{
    assert(in_bytes.size() == out_bytes.size());
    const std::byte* m = in_bytes.data();
    std::byte* c = out_bytes.data();
    size_t blocks = out_bytes.size() / BLOCKLEN;
    assert(blocks * BLOCKLEN == out_bytes.size());

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
        c += BLOCKLEN;
        m += BLOCKLEN;
    }
}

void ChaCha20::Keystream(std::span<std::byte> out) noexcept
{
    if (out.empty()) return;
    if (m_bufleft) {
        unsigned reuse = std::min<size_t>(m_bufleft, out.size());
        std::copy(m_buffer.end() - m_bufleft, m_buffer.end() - m_bufleft + reuse, out.begin());
        m_bufleft -= reuse;
        out = out.subspan(reuse);
    }
    if (out.size() >= m_aligned.BLOCKLEN) {
        size_t blocks = out.size() / m_aligned.BLOCKLEN;
        m_aligned.Keystream(out.first(blocks * m_aligned.BLOCKLEN));
        out = out.subspan(blocks * m_aligned.BLOCKLEN);
    }
    if (!out.empty()) {
        m_aligned.Keystream(m_buffer);
        std::copy(m_buffer.begin(), m_buffer.begin() + out.size(), out.begin());
        m_bufleft = m_aligned.BLOCKLEN - out.size();
    }
}

void ChaCha20::Crypt(std::span<const std::byte> input, std::span<std::byte> output) noexcept
{
    assert(input.size() == output.size());

    if (!input.size()) return;
    if (m_bufleft) {
        unsigned reuse = std::min<size_t>(m_bufleft, input.size());
        for (unsigned i = 0; i < reuse; i++) {
            output[i] = input[i] ^ m_buffer[m_aligned.BLOCKLEN - m_bufleft + i];
        }
        m_bufleft -= reuse;
        output = output.subspan(reuse);
        input = input.subspan(reuse);
    }
    if (input.size() >= m_aligned.BLOCKLEN) {
        size_t blocks = input.size() / m_aligned.BLOCKLEN;
        m_aligned.Crypt(input.first(blocks * m_aligned.BLOCKLEN), output.first(blocks * m_aligned.BLOCKLEN));
        output = output.subspan(blocks * m_aligned.BLOCKLEN);
        input = input.subspan(blocks * m_aligned.BLOCKLEN);
    }
    if (!input.empty()) {
        m_aligned.Keystream(m_buffer);
        for (unsigned i = 0; i < input.size(); i++) {
            output[i] = input[i] ^ m_buffer[i];
        }
        m_bufleft = m_aligned.BLOCKLEN - input.size();
    }
}

ChaCha20::~ChaCha20()
{
    memory_cleanse(m_buffer.data(), m_buffer.size());
}

void ChaCha20::SetKey(std::span<const std::byte> key) noexcept
{
    m_aligned.SetKey(key);
    m_bufleft = 0;
    memory_cleanse(m_buffer.data(), m_buffer.size());
}

FSChaCha20::FSChaCha20(std::span<const std::byte> key, uint32_t rekey_interval) noexcept :
    m_chacha20(key), m_rekey_interval(rekey_interval)
{
    assert(key.size() == KEYLEN);
}

void FSChaCha20::Crypt(std::span<const std::byte> input, std::span<std::byte> output) noexcept
{
    assert(input.size() == output.size());

    // Invoke internal stream cipher for actual encryption/decryption.
    m_chacha20.Crypt(input, output);

    // Rekey after m_rekey_interval encryptions/decryptions.
    if (++m_chunk_counter == m_rekey_interval) {
        // Get new key from the stream cipher.
        std::byte new_key[KEYLEN];
        m_chacha20.Keystream(new_key);
        // Update its key.
        m_chacha20.SetKey(new_key);
        // Wipe the key (a copy remains inside m_chacha20, where it'll be wiped on the next rekey
        // or on destruction).
        memory_cleanse(new_key, sizeof(new_key));
        // Set the nonce for the new section of output.
        m_chacha20.Seek({0, ++m_rekey_counter}, 0);
        // Reset the chunk counter.
        m_chunk_counter = 0;
    }
}
