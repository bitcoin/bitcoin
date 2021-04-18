// Copyright (c) 2020 The Widecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef WIDECOIN_CRYPTO_SHA3_H
#define WIDECOIN_CRYPTO_SHA3_H

#include <span.h>

#include <stdint.h>
#include <stdlib.h>

//! The Keccak-f[1600] transform.
void KeccakF(uint64_t (&st)[25]);

class SHA3_256
{
private:
    uint64_t m_state[25] = {0};
    unsigned char m_buffer[8];
    unsigned m_bufsize = 0;
    unsigned m_pos = 0;

    //! Sponge rate in bits.
    static constexpr unsigned RATE_BITS = 1088;

    //! Sponge rate expressed as a multiple of the buffer size.
    static constexpr unsigned RATE_BUFFERS = RATE_BITS / (8 * sizeof(m_buffer));

    static_assert(RATE_BITS % (8 * sizeof(m_buffer)) == 0, "Rate must be a multiple of 8 bytes");

public:
    static constexpr size_t OUTPUT_SIZE = 32;

    SHA3_256() {}
    SHA3_256& Write(Span<const unsigned char> data);
    SHA3_256& Finalize(Span<unsigned char> output);
    SHA3_256& Reset();
};

#endif // WIDECOIN_CRYPTO_SHA3_H
