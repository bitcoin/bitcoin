// Copyright (c) 2019-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CRYPTO_POLY1305_H
#define BITCOIN_CRYPTO_POLY1305_H

#include <span.h>

#include <cassert>
#include <cstdlib>
#include <stdint.h>

#define POLY1305_BLOCK_SIZE 16

namespace poly1305_donna {

// Based on the public domain implementation by Andrew Moon
// poly1305-donna-32.h from https://github.com/floodyberry/poly1305-donna

typedef struct {
    uint32_t r[5];
    uint32_t h[5];
    uint32_t pad[4];
    size_t leftover;
    unsigned char buffer[POLY1305_BLOCK_SIZE];
    unsigned char final;
} poly1305_context;

void poly1305_init(poly1305_context *st, const unsigned char key[32]) noexcept;
void poly1305_update(poly1305_context *st, const unsigned char *m, size_t bytes) noexcept;
void poly1305_finish(poly1305_context *st, unsigned char mac[16]) noexcept;

}  // namespace poly1305_donna

/** C++ wrapper with std::byte Span interface around poly1305_donna code. */
class Poly1305
{
    poly1305_donna::poly1305_context m_ctx;

public:
    /** Length of the output produced by Finalize(). */
    static constexpr unsigned TAGLEN{16};

    /** Length of the keys expected by the constructor. */
    static constexpr unsigned KEYLEN{32};

    /** Construct a Poly1305 object with a given 32-byte key. */
    Poly1305(Span<const std::byte> key) noexcept
    {
        assert(key.size() == KEYLEN);
        poly1305_donna::poly1305_init(&m_ctx, UCharCast(key.data()));
    }

    /** Process message bytes. */
    Poly1305& Update(Span<const std::byte> msg) noexcept
    {
        poly1305_donna::poly1305_update(&m_ctx, UCharCast(msg.data()), msg.size());
        return *this;
    }

    /** Write authentication tag to 16-byte out. */
    void Finalize(Span<std::byte> out) noexcept
    {
        assert(out.size() == TAGLEN);
        poly1305_donna::poly1305_finish(&m_ctx, UCharCast(out.data()));
    }
};

#endif // BITCOIN_CRYPTO_POLY1305_H
