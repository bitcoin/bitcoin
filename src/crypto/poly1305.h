/* $OpenBSD: poly1305.h,v 1.4 2014/05/02 03:27:54 djm Exp $ */

/*
 * Public Domain poly1305 from Andrew Moon
 * poly1305-donna-unrolled.c from https://github.com/floodyberry/poly1305-donna
 */

#ifndef BITCOIN_CRYPTO_POLY1305_H
#define BITCOIN_CRYPTO_POLY1305_H

#include <stdint.h>
#include <sys/types.h>

#define POLY1305_KEYLEN 32
#define POLY1305_TAGLEN 16

void poly1305_auth(uint8_t out[POLY1305_TAGLEN], const uint8_t *m, size_t inlen,
                   const uint8_t key[POLY1305_KEYLEN])
#if defined(__clang__)
    __attribute__((__bounded__(__minbytes__, 1, POLY1305_TAGLEN)))
    __attribute__((__bounded__(__buffer__, 2, 3)))
    __attribute__((__bounded__(__minbytes__, 4, POLY1305_KEYLEN)))
#endif
;

#endif /* BITCOIN_CRYPTO_POLY1305_H */
