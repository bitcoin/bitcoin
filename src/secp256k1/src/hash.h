/***********************************************************************
 * Copyright (c) 2014 Pieter Wuille                                    *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_HASH_H
#define SECP256K1_HASH_H

#include <stdlib.h>
#include <stdint.h>

typedef struct {
    secp256k1_sha256_compression_function fn_sha256_compression;
} secp256k1_hash_ctx;

static void secp256k1_hash_ctx_init(secp256k1_hash_ctx *hash_ctx);

typedef struct {
    uint32_t s[8];
    unsigned char buf[64];
    uint64_t bytes;
} secp256k1_sha256;

static void secp256k1_sha256_initialize(secp256k1_sha256 *hash);
/* Initialize a SHA256 hash state with a precomputed midstate.
 * The byte counter must be a multiple of 64, i.e., there must be no unwritten
 * bytes in the buffer. */
static void secp256k1_sha256_initialize_midstate(secp256k1_sha256 *hash, uint64_t bytes, const uint32_t state[8]);
static void secp256k1_sha256_write(const secp256k1_hash_ctx *hash_ctx, secp256k1_sha256 *hash, const unsigned char *data, size_t size);
static void secp256k1_sha256_finalize(const secp256k1_hash_ctx *hash_ctx, secp256k1_sha256 *hash, unsigned char *out32);
static void secp256k1_sha256_clear(secp256k1_sha256 *hash);

typedef struct {
    secp256k1_sha256 inner, outer;
} secp256k1_hmac_sha256;

static void secp256k1_hmac_sha256_initialize(const secp256k1_hash_ctx *hash_ctx, secp256k1_hmac_sha256 *hash, const unsigned char *key, size_t size);
static void secp256k1_hmac_sha256_write(const secp256k1_hash_ctx *hash_ctx, secp256k1_hmac_sha256 *hash, const unsigned char *data, size_t size);
static void secp256k1_hmac_sha256_finalize(const secp256k1_hash_ctx *hash_ctx, secp256k1_hmac_sha256 *hash, unsigned char *out32);
static void secp256k1_hmac_sha256_clear(secp256k1_hmac_sha256 *hash);

typedef struct {
    unsigned char v[32];
    unsigned char k[32];
    int retry;
} secp256k1_rfc6979_hmac_sha256;

static void secp256k1_rfc6979_hmac_sha256_initialize(const secp256k1_hash_ctx *hash_ctx, secp256k1_rfc6979_hmac_sha256 *rng, const unsigned char *key, size_t keylen);
static void secp256k1_rfc6979_hmac_sha256_generate(const secp256k1_hash_ctx *hash_ctx, secp256k1_rfc6979_hmac_sha256 *rng, unsigned char *out, size_t outlen);
static void secp256k1_rfc6979_hmac_sha256_finalize(secp256k1_rfc6979_hmac_sha256 *rng);
static void secp256k1_rfc6979_hmac_sha256_clear(secp256k1_rfc6979_hmac_sha256 *rng);

#endif /* SECP256K1_HASH_H */
