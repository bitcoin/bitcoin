/* $OpenBSD: chacha.h,v 1.4 2016/08/27 04:04:56 guenther Exp $ */

/*
chacha-merged.c version 20080118
D. J. Bernstein
Public domain.
*/

#ifndef BITCOIN_CRYPTO_CHACHA_H
#define BITCOIN_CRYPTO_CHACHA_H

#include <stdint.h>
#include <stdlib.h>

struct chacha_ctx {
  uint32_t input[16];
};

#define CHACHA_MINKEYLEN 16
#define CHACHA_NONCELEN 8
#define CHACHA_CTRLEN 8
#define CHACHA_STATELEN (CHACHA_NONCELEN + CHACHA_CTRLEN)
#define CHACHA_BLOCKLEN 64

void chacha_keysetup(struct chacha_ctx *x, const uint8_t *k, uint32_t kbits)
#if defined(__clang__)
    __attribute__((__bounded__(__minbytes__, 2, CHACHA_MINKEYLEN)))
#endif
;
void chacha_ivsetup(struct chacha_ctx *x, const uint8_t *iv, const uint8_t *ctr)
#if defined(__clang__)
    __attribute__((__bounded__(__minbytes__, 2, CHACHA_NONCELEN)))
    __attribute__((__bounded__(__minbytes__, 3, CHACHA_CTRLEN)))
#endif
;
void chacha_encrypt_bytes(struct chacha_ctx *x, const uint8_t *m, uint8_t *c,
                          uint32_t bytes)
#if defined(__clang__)
    __attribute__((__bounded__(__buffer__, 2, 4)))
    __attribute__((__bounded__(__buffer__, 3, 4)))
#endif
;

#endif /* BITCOIN_CRYPTO_CHACHA_H */
