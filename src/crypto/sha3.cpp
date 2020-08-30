// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Based on https://github.com/mjosaarinen/tiny_sha3/blob/master/sha3.c
// by Markku-Juhani O. Saarinen <mjos@iki.fi>

#include <crypto/sha3.h>
#include <crypto/common.h>
#include <span.h>

#include <algorithm>
#include <array> // For std::begin and std::end.

#include <stdint.h>

// Internal implementation code.
namespace
{
uint64_t Rotl(uint64_t x, int n) { return (x << n) | (x >> (64 - n)); }
} // namespace

void KeccakF(uint64_t (&st)[25])
{
    static constexpr uint64_t RNDC[24] = {
        0x0000000000000001, 0x0000000000008082, 0x800000000000808a, 0x8000000080008000,
        0x000000000000808b, 0x0000000080000001, 0x8000000080008081, 0x8000000000008009,
        0x000000000000008a, 0x0000000000000088, 0x0000000080008009, 0x000000008000000a,
        0x000000008000808b, 0x800000000000008b, 0x8000000000008089, 0x8000000000008003,
        0x8000000000008002, 0x8000000000000080, 0x000000000000800a, 0x800000008000000a,
        0x8000000080008081, 0x8000000000008080, 0x0000000080000001, 0x8000000080008008
    };
    static constexpr int ROTC[24] = {
        1,  3,  6,  10, 15, 21, 28, 36, 45, 55, 2,  14,
        27, 41, 56, 8,  25, 43, 62, 18, 39, 61, 20, 44
    };
    static constexpr int PILN[24] = {
        10, 7,  11, 17, 18, 3, 5,  16, 8,  21, 24, 4,
        15, 23, 19, 13, 12, 2, 20, 14, 22, 9,  6,  1
    };
    static constexpr int ROUNDS = 24;

    for (int round = 0; round < ROUNDS; ++round) {
        uint64_t bc[5], t;

        // Theta
        for (int i = 0; i < 5; i++) {
            bc[i] = st[i] ^ st[i + 5] ^ st[i + 10] ^ st[i + 15] ^ st[i + 20];
        }

        for (int i = 0; i < 5; i++) {
            t = bc[(i + 4) % 5] ^ Rotl(bc[(i + 1) % 5], 1);
            for (int j = 0; j < 25; j += 5) st[j + i] ^= t;
        }

        // Rho Pi
        t = st[1];
        for (int i = 0; i < 24; i++) {
            int j = PILN[i];
            bc[0] = st[j];
            st[j] = Rotl(t, ROTC[i]);
            t = bc[0];
        }

        // Chi
        for (int j = 0; j < 25; j += 5) {
            for (int i = 0; i < 5; i++) bc[i] = st[j + i];
            for (int i = 0; i < 5; i++) {
                st[j + i] ^= (~bc[(i + 1) % 5]) & bc[(i + 2) % 5];
            }
        }

        //  Iota
        st[0] ^= RNDC[round];
    }
}

SHA3_256& SHA3_256::Write(Span<const unsigned char> data)
{
    if (m_bufsize && m_bufsize + data.size() >= sizeof(m_buffer)) {
        // Fill the buffer and process it.
        std::copy(data.begin(), data.begin() + sizeof(m_buffer) - m_bufsize, m_buffer + m_bufsize);
        data = data.subspan(sizeof(m_buffer) - m_bufsize);
        m_state[m_pos++] ^= ReadLE64(m_buffer);
        m_bufsize = 0;
        if (m_pos == RATE_BUFFERS) {
            KeccakF(m_state);
            m_pos = 0;
        }
    }
    while (data.size() >= sizeof(m_buffer)) {
        // Process chunks directly from the buffer.
        m_state[m_pos++] ^= ReadLE64(data.data());
        data = data.subspan(8);
        if (m_pos == RATE_BUFFERS) {
            KeccakF(m_state);
            m_pos = 0;
        }
    }
    if (data.size()) {
        // Keep the remainder in the buffer.
        std::copy(data.begin(), data.end(), m_buffer + m_bufsize);
        m_bufsize += data.size();
    }
    return *this;
}

SHA3_256& SHA3_256::Finalize(Span<unsigned char> output)
{
    assert(output.size() == OUTPUT_SIZE);
    std::fill(m_buffer + m_bufsize, m_buffer + sizeof(m_buffer), 0);
    m_buffer[m_bufsize] ^= 0x06;
    m_state[m_pos] ^= ReadLE64(m_buffer);
    m_state[RATE_BUFFERS - 1] ^= 0x8000000000000000;
    KeccakF(m_state);
    for (unsigned i = 0; i < 4; ++i) {
        WriteLE64(output.data() + 8 * i, m_state[i]);
    }
    return *this;
}

SHA3_256& SHA3_256::Reset()
{
    m_bufsize = 0;
    m_pos = 0;
    std::fill(std::begin(m_state), std::end(m_state), 0);
    return *this;
}
