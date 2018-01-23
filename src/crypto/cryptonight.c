// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Modified for CPUminer by Lucas Jones

//#include "cpuminer-config.h"
//#include "miner.h"
#include <stdlib.h>

#include <crypto/oaes_lib.h>
#include <crypto/c_keccak.h>
#include <crypto/c_groestl.h>
#include <crypto/c_blake256.h>
#include <crypto/c_jh.h>
#include <crypto/c_skein.h>
#include <crypto/cryptonight.h>
#include <crypto/types.h>

struct cryptonight_ctx {
    uint8_t long_state[MEMORY];
    union cn_slow_hash_state state;
    uint8_t text[INIT_SIZE_BYTE];
    uint8_t a[AES_BLOCK_SIZE];
    uint8_t b[AES_BLOCK_SIZE];
    uint8_t c[AES_BLOCK_SIZE];
    oaes_ctx* aes_ctx;
};

static void do_blake_hash(const void* input, size_t len, char* output) {
    blake256_hash((uint8_t*)output, (const uint8_t*)input, len);
}

void do_groestl_hash(const void* input, size_t len, char* output) {
    groestl((const uint8_t*)input, len * 8, (uint8_t*)output);
}

static void do_jh_hash(const void* input, size_t len, char* output) {
    int r = jh_hash(HASH_SIZE * 8, (const uint8_t*)input, 8 * len, (uint8_t*)output);
}

static void do_skein_hash(const void* input, size_t len, char* output) {
    int r = skein_hash(8 * HASH_SIZE, (const uint8_t*)input, 8 * len, (uint8_t*)output);
}

void hash_permutation(union hash_state *state) {
  keccakf((uint64_t*)state, 24);
}

void hash_process(union hash_state *state, const uint8_t *buf, size_t count) {
  keccak1600(buf, count, (uint8_t*)state);
}

//extern int fast_aesb_single_round(const uint8_t *in, uint8_t*out, const uint8_t *expandedKey);
//extern void aesb_single_round(const uint8_t *in, uint8_t*out, const uint8_t *expandedKey);
void aesb_single_round(const uint8_t *in, uint8_t *out, uint8_t *expandedKey);
//extern void aesb_pseudo_round_mut(uint8_t *val, uint8_t *expandedKey);
void aesb_pseudo_round_mut(uint8_t *val, uint8_t *expandedKey);
//extern int fast_aesb_pseudo_round_mut(uint8_t *val, uint8_t *expandedKey);

static void (* const extra_hashes[4])(const void *, size_t, char *) = {
        do_blake_hash, do_groestl_hash, do_jh_hash, do_skein_hash
};

uint64_t mul128(uint64_t multiplier, uint64_t multiplicand, uint64_t* product_hi) {
  // multiplier   = ab = a * 2^32 + b
  // multiplicand = cd = c * 2^32 + d
  // ab * cd = a * c * 2^64 + (a * d + b * c) * 2^32 + b * d
  uint64_t a = hi_dword(multiplier);
  uint64_t b = lo_dword(multiplier);
  uint64_t c = hi_dword(multiplicand);
  uint64_t d = lo_dword(multiplicand);

  uint64_t ac = a * c;
  uint64_t ad = a * d;
  uint64_t bc = b * c;
  uint64_t bd = b * d;

  uint64_t adbc = ad + bc;
  uint64_t adbc_carry = adbc < ad ? 1 : 0;

  // multiplier * multiplicand = product_hi * 2^64 + product_lo
  uint64_t product_lo = bd + (adbc << 32);
  uint64_t product_lo_carry = product_lo < bd ? 1 : 0;
  *product_hi = ac + (adbc >> 32) + (adbc_carry << 32) + product_lo_carry;

  return product_lo;
}

static size_t e2i(const uint8_t* a) {
    return (*((uint64_t*) a) / AES_BLOCK_SIZE) & (MEMORY / AES_BLOCK_SIZE - 1);
}

static void mul(const uint8_t* a, const uint8_t* b, uint8_t* res) {
    ((uint64_t*) res)[1] = mul128(((uint64_t*) a)[0], ((uint64_t*) b)[0], (uint64_t*) res);
}

static void sum_half_blocks(uint8_t* a, const uint8_t* b) {
    ((uint64_t*) a)[0] += ((uint64_t*) b)[0];
    ((uint64_t*) a)[1] += ((uint64_t*) b)[1];
}

static void sum_half_blocks_dst(const uint8_t* a, const uint8_t* b, uint8_t* dst) {
    ((uint64_t*) dst)[0] = ((uint64_t*) a)[0] + ((uint64_t*) b)[0];
    ((uint64_t*) dst)[1] = ((uint64_t*) a)[1] + ((uint64_t*) b)[1];
}

static void mul_sum_dst(const uint8_t* a, const uint8_t* b, const uint8_t* c, uint8_t* dst) {
    ((uint64_t*) dst)[1] = mul128(((uint64_t*) a)[0], ((uint64_t*) b)[0], (uint64_t*) dst) + ((uint64_t*) c)[1];
    ((uint64_t*) dst)[0] += ((uint64_t*) c)[0];
}

static void mul_sum_xor_dst(const uint8_t* a, uint8_t* c, uint8_t* dst) {
    uint64_t hi, lo = mul128(((uint64_t*) a)[0], ((uint64_t*) dst)[0], &hi) + ((uint64_t*) c)[1];
    hi += ((uint64_t*) c)[0];

    ((uint64_t*) c)[0] = ((uint64_t*) dst)[0] ^ hi;
    ((uint64_t*) c)[1] = ((uint64_t*) dst)[1] ^ lo;
    ((uint64_t*) dst)[0] = hi;
    ((uint64_t*) dst)[1] = lo;
}

static void copy_block(uint8_t* dst, const uint8_t* src) {
    ((uint64_t*) dst)[0] = ((uint64_t*) src)[0];
    ((uint64_t*) dst)[1] = ((uint64_t*) src)[1];
}

static void xor_blocks(uint8_t* a, const uint8_t* b) {
    ((uint64_t*) a)[0] ^= ((uint64_t*) b)[0];
    ((uint64_t*) a)[1] ^= ((uint64_t*) b)[1];
}

static void xor_blocks_dst(const uint8_t* a, const uint8_t* b, uint8_t* dst) {
    ((uint64_t*) dst)[0] = ((uint64_t*) a)[0] ^ ((uint64_t*) b)[0];
    ((uint64_t*) dst)[1] = ((uint64_t*) a)[1] ^ ((uint64_t*) b)[1];
}

void cryptonight_hash_ctx(void* output, const void* input, size_t len, struct cryptonight_ctx* ctx) {
    size_t i, j;
    hash_process(&ctx->state.hs, (const uint8_t*) input, len);
    ctx->aes_ctx = (oaes_ctx*) oaes_alloc();
    memcpy(ctx->text, ctx->state.init, INIT_SIZE_BYTE);

    oaes_key_import_data(ctx->aes_ctx, ctx->state.hs.b, AES_KEY_SIZE);
    for (i = 0; likely(i < MEMORY); i += INIT_SIZE_BYTE) {
#undef RND
#define RND(p) aesb_pseudo_round_mut((uint8_t *)&ctx->text[AES_BLOCK_SIZE * p], (uint8_t *)ctx->aes_ctx->key->exp_data);
        RND(0);
        RND(1);
        RND(2);
        RND(3);
        RND(4);
        RND(5);
        RND(6);
        RND(7);
        memcpy(&ctx->long_state[i], ctx->text, INIT_SIZE_BYTE);
    }

    xor_blocks_dst(&ctx->state.k[0], &ctx->state.k[32], ctx->a);
    xor_blocks_dst(&ctx->state.k[16], &ctx->state.k[48], ctx->b);

    for (i = 0; likely(i < ITER / 4); ++i) {
        j = e2i(ctx->a) * AES_BLOCK_SIZE;
        aesb_single_round((const uint8_t *)&ctx->long_state[j], (uint8_t *)ctx->c, (uint8_t *)ctx->a);
        xor_blocks_dst(ctx->c, ctx->b, &ctx->long_state[j]);

        mul_sum_xor_dst(ctx->c, ctx->a, &ctx->long_state[e2i(ctx->c) * AES_BLOCK_SIZE]);

        j = e2i(ctx->a) * AES_BLOCK_SIZE;
        aesb_single_round((const uint8_t *)&ctx->long_state[j], (uint8_t *)ctx->b, (uint8_t *)ctx->a);
        xor_blocks_dst(ctx->b, ctx->c, &ctx->long_state[j]);

        mul_sum_xor_dst(ctx->b, ctx->a, &ctx->long_state[e2i(ctx->b) * AES_BLOCK_SIZE]);
    }

    memcpy(ctx->text, ctx->state.init, INIT_SIZE_BYTE);
    oaes_key_import_data(ctx->aes_ctx, &ctx->state.hs.b[32], AES_KEY_SIZE);
    for (i = 0; likely(i < MEMORY); i += INIT_SIZE_BYTE) {
#undef RND
#define RND(p) xor_blocks(&ctx->text[p * AES_BLOCK_SIZE], &ctx->long_state[i + p * AES_BLOCK_SIZE]); \
        aesb_pseudo_round_mut((uint8_t *)&ctx->text[p * AES_BLOCK_SIZE], (uint8_t *)ctx->aes_ctx->key->exp_data);
        RND(0);
        RND(1);
        RND(2);
        RND(3);
        RND(4);
        RND(5);
        RND(6);
        RND(7);
    }
    memcpy(ctx->state.init, ctx->text, INIT_SIZE_BYTE);
    hash_permutation(&ctx->state.hs);
    extra_hashes[ctx->state.hs.b[0] & 3](&ctx->state, 200, (char *)output);
    oaes_free((OAES_CTX **) &ctx->aes_ctx);
}

void cryptonight_hash(void* output, const void* input, size_t len) {
    struct cryptonight_ctx *ctx = (struct cryptonight_ctx*)malloc(sizeof(struct cryptonight_ctx));
    cryptonight_hash_ctx(output, input, len, ctx);
    free(ctx);
}
