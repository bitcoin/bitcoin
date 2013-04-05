#ifndef _SECP256K1_NUM_REPR_IMPL_H_
#define _SECP256K1_NUM_REPR_IMPL_H_

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <gmp.h>

#include "num.h"

typedef struct {
    int initialized;
    gmp_randstate_t rng;
} secp256k1_num_state_t;

static secp256k1_num_state_t secp256k1_num_state = {};

void static secp256k1_num_start(void) {
    if (secp256k1_num_state.initialized)
        return;
    secp256k1_num_state.initialized = 1;
    gmp_randinit_default(secp256k1_num_state.rng);
}

void static secp256k1_num_stop(void) {
    if (!secp256k1_num_state.initialized)
        return;
    secp256k1_num_state.initialized = 0;
    gmp_randclear(secp256k1_num_state.rng);
}

void static secp256k1_num_init(secp256k1_num_t *r) {
    mpz_init(r->bn);
}

void static secp256k1_num_free(secp256k1_num_t *r) {
    mpz_clear(r->bn);
}

void static secp256k1_num_copy(secp256k1_num_t *r, const secp256k1_num_t *a) {
    mpz_set(r->bn, a->bn);
}

void static secp256k1_num_get_bin(unsigned char *r, unsigned int rlen, const secp256k1_num_t *a) {
    unsigned int size = (mpz_sizeinbase(a->bn,2)+7)/8;
    assert(size <= rlen);
    memset(r,0,rlen);
    size_t count = 0;
    mpz_export(r + rlen - size, &count, 1, 1, 1, 0, a->bn);
    assert(count == 0 || size == count);
}

void static secp256k1_num_set_bin(secp256k1_num_t *r, const unsigned char *a, unsigned int alen) {
    mpz_import(r->bn, alen, 1, 1, 1, 0, a);
}

void static secp256k1_num_set_int(secp256k1_num_t *r, int a) {
    mpz_set_si(r->bn, a);
}

void static secp256k1_num_mod_inverse(secp256k1_num_t *r, const secp256k1_num_t *a, const secp256k1_num_t *m) {
    mpz_invert(r->bn, a->bn, m->bn);
}

void static secp256k1_num_mod_mul(secp256k1_num_t *r, const secp256k1_num_t *a, const secp256k1_num_t *b, const secp256k1_num_t *m) {
    mpz_mul(r->bn, a->bn, b->bn);
    mpz_mod(r->bn, r->bn, m->bn);
}

int static secp256k1_num_cmp(const secp256k1_num_t *a, const secp256k1_num_t *b) {
    return mpz_cmp(a->bn, b->bn);
}

void static secp256k1_num_add(secp256k1_num_t *r, const secp256k1_num_t *a, const secp256k1_num_t *b) {
    mpz_add(r->bn, a->bn, b->bn);
}

void static secp256k1_num_sub(secp256k1_num_t *r, const secp256k1_num_t *a, const secp256k1_num_t *b) {
    mpz_sub(r->bn, a->bn, b->bn);
}

void static secp256k1_num_mul(secp256k1_num_t *r, const secp256k1_num_t *a, const secp256k1_num_t *b) {
    mpz_mul(r->bn, a->bn, b->bn);
}

void static secp256k1_num_div(secp256k1_num_t *r, const secp256k1_num_t *a, const secp256k1_num_t *b) {
    mpz_tdiv_q(r->bn, a->bn, b->bn);
}

void static secp256k1_num_mod(secp256k1_num_t *r, const secp256k1_num_t *a, const secp256k1_num_t *b) {
    mpz_mod(r->bn, a->bn, b->bn);
}

int static secp256k1_num_bits(const secp256k1_num_t *a) {
    return mpz_sizeinbase(a->bn,2);
}

int static secp256k1_num_shift(secp256k1_num_t *r, int bits) {
    int ret = mpz_get_ui(r->bn) & ((1 << bits) - 1);
    mpz_fdiv_q_2exp(r->bn, r->bn, bits);
    return ret;
}

int static secp256k1_num_is_zero(const secp256k1_num_t *a) {
    return mpz_size(a->bn) == 0;
}

int static secp256k1_num_is_odd(const secp256k1_num_t *a) {
    return mpz_get_ui(a->bn) & 1;
}

int static secp256k1_num_is_neg(const secp256k1_num_t *a) {
    return mpz_sgn(a->bn) < 0;
}

int static secp256k1_num_get_bit(const secp256k1_num_t *a, int pos) {
    return mpz_tstbit(a->bn, pos);
}

void static secp256k1_num_inc(secp256k1_num_t *r) {
    mpz_add_ui(r->bn, r->bn, 1);
}

void static secp256k1_num_set_hex(secp256k1_num_t *r, const char *a, int alen) {
    char *str = (char*)malloc(alen+1);
    memcpy(str, a, alen);
    str[alen] = 0;
    mpz_set_str(r->bn, str, 16);
    free(str);
}

void static secp256k1_num_get_hex(char *r, int *rlen, const secp256k1_num_t *a) {
    int len = mpz_sizeinbase(a->bn, 16) + 2;
    if (*rlen < len) {
        *rlen = len;
        return;
    }
    mpz_get_str(r, 16, a->bn);
    *rlen = len;
}

void static secp256k1_num_split(secp256k1_num_t *rl, secp256k1_num_t *rh, const secp256k1_num_t *a, int bits) {
    mpz_t tmp;
    mpz_init_set_ui(tmp,1);
    mpz_mul_2exp(tmp, tmp, bits);
    mpz_sub_ui(tmp,tmp,1);
    mpz_and(rl->bn, a->bn, tmp);
    mpz_clear(tmp);
    mpz_fdiv_q_2exp(rh->bn, a->bn, bits);
}

void static secp256k1_num_negate(secp256k1_num_t *r) {
    mpz_neg(r->bn, r->bn);
}

void static secp256k1_num_set_rand(secp256k1_num_t *r, const secp256k1_num_t *a) {
    mpz_urandomm(r->bn, secp256k1_num_state.rng, a->bn);
}

#endif
