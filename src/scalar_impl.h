// Copyright (c) 2014 Pieter Wuille
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef _SECP256K1_SCALAR_IMPL_H_
#define _SECP256K1_SCALAR_IMPL_H_

#include <string.h>

#include "scalar.h"

#include "group.h"

void static secp256k1_scalar_init(secp256k1_scalar_t *r) {
    secp256k1_num_init(&r->n);
}

void static secp256k1_scalar_clear(secp256k1_scalar_t *r) {
    secp256k1_num_clear(&r->n);
}

void static secp256k1_scalar_free(secp256k1_scalar_t *r) {
    secp256k1_num_free(&r->n);
}

int static secp256k1_scalar_get_bits(const secp256k1_scalar_t *a, int offset, int count) {
    return secp256k1_num_get_bits(&a->n, offset, count);
}

void static secp256k1_scalar_set_b32(secp256k1_scalar_t *r, const unsigned char *bin, int *overflow) {
    secp256k1_num_set_bin(&r->n, bin, 32);
    if (overflow) {
        *overflow = secp256k1_num_cmp(&r->n, &secp256k1_ge_consts->order) >= 0;
    }
    secp256k1_num_mod(&r->n, &secp256k1_ge_consts->order);
}

void static secp256k1_scalar_get_b32(unsigned char *bin, const secp256k1_scalar_t* a) {
    secp256k1_num_get_bin(bin, 32, &a->n);
}

void static secp256k1_scalar_add(secp256k1_scalar_t *r, const secp256k1_scalar_t *a, const secp256k1_scalar_t *b) {
    secp256k1_num_add(&r->n, &a->n, &b->n);
    secp256k1_num_mod(&r->n, &secp256k1_ge_consts->order);
}

void static secp256k1_scalar_mul(secp256k1_scalar_t *r, const secp256k1_scalar_t *a, const secp256k1_scalar_t *b) {
    secp256k1_num_mod_mul(&r->n, &a->n, &b->n, &secp256k1_ge_consts->order);
}

void static secp256k1_scalar_inverse(secp256k1_scalar_t *r, const secp256k1_scalar_t *a) {
    secp256k1_num_mod_inverse(&r->n, &a->n, &secp256k1_ge_consts->order);
}

void static secp256k1_scalar_negate(secp256k1_scalar_t *r, const secp256k1_scalar_t *a) {
    secp256k1_num_sub(&r->n, &secp256k1_ge_consts->order, &a->n);
    secp256k1_num_mod(&r->n, &secp256k1_ge_consts->order);
}

int static secp256k1_scalar_is_zero(const secp256k1_scalar_t *a) {
    return secp256k1_num_is_zero(&a->n);
}

int static secp256k1_scalar_is_one(const secp256k1_scalar_t *a) {
    return secp256k1_num_bits(&a->n) == 1;
}

int static secp256k1_scalar_is_high(const secp256k1_scalar_t *a) {
    return secp256k1_num_cmp(&a->n, &secp256k1_ge_consts->half_order) > 0;
}

void static secp256k1_scalar_get_num(secp256k1_num_t *r, const secp256k1_scalar_t *a) {
    unsigned char c[32];
    secp256k1_num_get_bin(c, 32, &a->n);
    secp256k1_num_set_bin(r, c, 32);
}

#endif
