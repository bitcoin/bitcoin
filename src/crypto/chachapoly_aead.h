// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CRYPTO_CHACHAPOLY_AEAD_H
#define BITCOIN_CRYPTO_CHACHAPOLY_AEAD_H

#include <crypto/chacha.h>

#define CHACHA_KEYLEN 32 /* 2 x 256 bit keys */

struct chachapolyaead_ctx {
  struct chacha_ctx main_ctx, header_ctx;
};

int chacha20poly1305_init(struct chachapolyaead_ctx *cpctx, const uint8_t *key,
                          int keylen);
int chacha20poly1305_crypt(struct chachapolyaead_ctx *ctx, uint32_t seqnr,
                           uint8_t *dest, const uint8_t *src, uint32_t len,
                           uint32_t aadlen, int is_encrypt);

// extracts the LE 24bit (3byte) length from the AAD and puts it into a uint32_t (host endianness)
int chacha20poly1305_get_length24(struct chachapolyaead_ctx *ctx,
                                uint32_t *len_out, uint32_t seqnr,
                                const uint8_t *ciphertext);
#endif /* BITCOIN_CRYPTO_CHACHAPOLY_AEAD_H */
