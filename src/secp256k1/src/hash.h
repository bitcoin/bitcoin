/**********************************************************************
 * Copyright (c) 2014 Pieter Wuille                                   *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef _SECP256K1_HASH_
#define _SECP256K1_HASH_

#include <stdlib.h>
#include <stdint.h>

typedef struct {
    uint32_t s[8];
    uint32_t buf[16]; /* In big endian */
    size_t bytes;
} secp256k1_sha256_t;

static void secp256k1_sha256_initialize(secp256k1_sha256_t *hash);
static void secp256k1_sha256_write(secp256k1_sha256_t *hash, const unsigned char *data, size_t size);
static void secp256k1_sha256_finalize(secp256k1_sha256_t *hash, unsigned char *out32);

typedef struct {
    secp256k1_sha256_t inner, outer;
} secp256k1_hmac_sha256_t;

static void secp256k1_hmac_sha256_initialize(secp256k1_hmac_sha256_t *hash, const unsigned char *key, size_t size);
static void secp256k1_hmac_sha256_write(secp256k1_hmac_sha256_t *hash, const unsigned char *data, size_t size);
static void secp256k1_hmac_sha256_finalize(secp256k1_hmac_sha256_t *hash, unsigned char *out32);

typedef struct {
    unsigned char v[32];
    unsigned char k[32];
    int retry;
} secp256k1_rfc6979_hmac_sha256_t;

static void secp256k1_rfc6979_hmac_sha256_initialize(secp256k1_rfc6979_hmac_sha256_t *rng, const unsigned char *key, size_t keylen);
static void secp256k1_rfc6979_hmac_sha256_generate(secp256k1_rfc6979_hmac_sha256_t *rng, unsigned char *out, size_t outlen);
static void secp256k1_rfc6979_hmac_sha256_finalize(secp256k1_rfc6979_hmac_sha256_t *rng);

#endif
