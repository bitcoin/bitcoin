// Copyright (c) 2019-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CRYPTO_POLY1305_H
#define BITCOIN_CRYPTO_POLY1305_H

#include <cstdlib>
#include <stdint.h>

#define POLY1305_KEYLEN 32
#define POLY1305_TAGLEN 16
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

void poly1305_auth(unsigned char out[POLY1305_TAGLEN], const unsigned char *m, size_t inlen,
    const unsigned char key[POLY1305_KEYLEN]);

#endif // BITCOIN_CRYPTO_POLY1305_H
