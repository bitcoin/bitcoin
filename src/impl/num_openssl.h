// Copyright (c) 2013 Pieter Wuille
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef _SECP256K1_NUM_REPR_IMPL_H_
#define _SECP256K1_NUM_REPR_IMPL_H_

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <openssl/bn.h>
#include <openssl/crypto.h>

#include "../num.h"

void static secp256k1_num_init(secp256k1_num_t *r) {
    BN_init(&r->bn);
}

void static secp256k1_num_free(secp256k1_num_t *r) {
    BN_free(&r->bn);
}

void static secp256k1_num_copy(secp256k1_num_t *r, const secp256k1_num_t *a) {
    BN_copy(&r->bn, &a->bn);
}

void static secp256k1_num_get_bin(unsigned char *r, unsigned int rlen, const secp256k1_num_t *a) {
    unsigned int size = BN_num_bytes(&a->bn);
    assert(size <= rlen);
    memset(r,0,rlen);
    BN_bn2bin(&a->bn, r + rlen - size);
}

void static secp256k1_num_set_bin(secp256k1_num_t *r, const unsigned char *a, unsigned int alen) {
    BN_bin2bn(a, alen, &r->bn);
}

void static secp256k1_num_set_int(secp256k1_num_t *r, int a) {
    BN_set_word(&r->bn, a < 0 ? -a : a);
    BN_set_negative(&r->bn, a < 0);
}

void static secp256k1_num_mod_inverse(secp256k1_num_t *r, const secp256k1_num_t *a, const secp256k1_num_t *m) {
    BN_CTX *ctx = BN_CTX_new();
    BN_mod_inverse(&r->bn, &a->bn, &m->bn, ctx);
    BN_CTX_free(ctx);
}

void static secp256k1_num_mod_mul(secp256k1_num_t *r, const secp256k1_num_t *a, const secp256k1_num_t *b, const secp256k1_num_t *m) {
    BN_CTX *ctx = BN_CTX_new();
    BN_mod_mul(&r->bn, &a->bn, &b->bn, &m->bn, ctx);
    BN_CTX_free(ctx);
}

int static secp256k1_num_cmp(const secp256k1_num_t *a, const secp256k1_num_t *b) {
    return BN_cmp(&a->bn, &b->bn);
}

void static secp256k1_num_add(secp256k1_num_t *r, const secp256k1_num_t *a, const secp256k1_num_t *b) {
    BN_add(&r->bn, &a->bn, &b->bn);
}

void static secp256k1_num_sub(secp256k1_num_t *r, const secp256k1_num_t *a, const secp256k1_num_t *b) {
    BN_sub(&r->bn, &a->bn, &b->bn);
}

void static secp256k1_num_mul(secp256k1_num_t *r, const secp256k1_num_t *a, const secp256k1_num_t *b) {
    BN_CTX *ctx = BN_CTX_new();
    BN_mul(&r->bn, &a->bn, &b->bn, ctx);
    BN_CTX_free(ctx);
}

void static secp256k1_num_div(secp256k1_num_t *r, const secp256k1_num_t *a, const secp256k1_num_t *b) {
    BN_CTX *ctx = BN_CTX_new();
    BN_div(&r->bn, NULL, &a->bn, &b->bn, ctx);
    BN_CTX_free(ctx);
}

void static secp256k1_num_mod(secp256k1_num_t *r, const secp256k1_num_t *m) {
    BN_CTX *ctx = BN_CTX_new();
    BN_nnmod(&r->bn, &r->bn, &m->bn, ctx);
    BN_CTX_free(ctx);
}

int static secp256k1_num_bits(const secp256k1_num_t *a) {
    return BN_num_bits(&a->bn);
}

int static secp256k1_num_shift(secp256k1_num_t *r, int bits) {
    int ret = BN_is_zero(&r->bn) ? 0 : r->bn.d[0] & ((1 << bits) - 1);
    BN_rshift(&r->bn, &r->bn, bits);
    return ret;
}

int static secp256k1_num_is_zero(const secp256k1_num_t *a) {
    return BN_is_zero(&a->bn);
}

int static secp256k1_num_is_odd(const secp256k1_num_t *a) {
    return BN_is_odd(&a->bn);
}

int static secp256k1_num_is_neg(const secp256k1_num_t *a) {
    return BN_is_negative(&a->bn);
}

int static secp256k1_num_get_bit(const secp256k1_num_t *a, int pos) {
    return BN_is_bit_set(&a->bn, pos);
}

void static secp256k1_num_inc(secp256k1_num_t *r) {
    BN_add_word(&r->bn, 1);
}

void static secp256k1_num_set_hex(secp256k1_num_t *r, const char *a, int alen) {
    char *str = (char*)malloc(alen+1);
    memcpy(str, a, alen);
    str[alen] = 0;
    BIGNUM *pbn = &r->bn;
    BN_hex2bn(&pbn, str);
    free(str);
}

void static secp256k1_num_get_hex(char *r, int rlen, const secp256k1_num_t *a) {
    char *str = BN_bn2hex(&a->bn);
    int len = strlen(str);
    assert(rlen >= len);
    for (int i=0; i<rlen-len; i++)
        r[i] = '0';
    memcpy(r+rlen-len, str, len);
    OPENSSL_free(str);
}

void static secp256k1_num_split(secp256k1_num_t *rl, secp256k1_num_t *rh, const secp256k1_num_t *a, int bits) {
    BN_copy(&rl->bn, &a->bn);
    BN_rshift(&rh->bn, &a->bn, bits);
    BN_mask_bits(&rl->bn, bits);
}

void static secp256k1_num_negate(secp256k1_num_t *r) {
    BN_set_negative(&r->bn, !BN_is_negative(&r->bn));
}

#endif
