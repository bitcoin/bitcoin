// Copyright (c) 2019 The Widecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef WIDECOIN_CRYPTO_POLY1305_H
#define WIDECOIN_CRYPTO_POLY1305_H

#include <stdint.h>
#include <stdlib.h>

#define POLY1305_KEYLEN 32
#define POLY1305_TAGLEN 16

void poly1305_auth(unsigned char out[POLY1305_TAGLEN], const unsigned char *m, size_t inlen,
    const unsigned char key[POLY1305_KEYLEN]);

#endif // WIDECOIN_CRYPTO_POLY1305_H
