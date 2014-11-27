/**********************************************************************
 * Copyright (c) 2014 Pieter Wuille                                   *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef _SECP256K1_SCALAR_IMPL_H_
#define _SECP256K1_SCALAR_IMPL_H_

#include <string.h>

#include "group.h"
#include "scalar.h"

#if defined HAVE_CONFIG_H
#include "libsecp256k1-config.h"
#endif

#if defined(USE_SCALAR_4X64)
#include "scalar_4x64_impl.h"
#elif defined(USE_SCALAR_8X32)
#include "scalar_8x32_impl.h"
#else
#error "Please select scalar implementation"
#endif

typedef struct {
    secp256k1_num_t order;
#ifdef USE_ENDOMORPHISM
    secp256k1_num_t a1b2, b1, a2;
#endif
} secp256k1_scalar_consts_t;

static const secp256k1_scalar_consts_t *secp256k1_scalar_consts = NULL;

static void secp256k1_scalar_start(void) {
    if (secp256k1_scalar_consts != NULL)
        return;

    /* Allocate. */
    secp256k1_scalar_consts_t *ret = (secp256k1_scalar_consts_t*)malloc(sizeof(secp256k1_scalar_consts_t));

    static const unsigned char secp256k1_scalar_consts_order[] = {
        0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
        0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFE,
        0xBA,0xAE,0xDC,0xE6,0xAF,0x48,0xA0,0x3B,
        0xBF,0xD2,0x5E,0x8C,0xD0,0x36,0x41,0x41
    };
    secp256k1_num_set_bin(&ret->order, secp256k1_scalar_consts_order, sizeof(secp256k1_scalar_consts_order));
#ifdef USE_ENDOMORPHISM
    static const unsigned char secp256k1_scalar_consts_a1b2[] = {
        0x30,0x86,0xd2,0x21,0xa7,0xd4,0x6b,0xcd,
        0xe8,0x6c,0x90,0xe4,0x92,0x84,0xeb,0x15
    };
    static const unsigned char secp256k1_scalar_consts_b1[] = {
        0xe4,0x43,0x7e,0xd6,0x01,0x0e,0x88,0x28,
        0x6f,0x54,0x7f,0xa9,0x0a,0xbf,0xe4,0xc3
    };
    static const unsigned char secp256k1_scalar_consts_a2[] = {
        0x01,
        0x14,0xca,0x50,0xf7,0xa8,0xe2,0xf3,0xf6,
        0x57,0xc1,0x10,0x8d,0x9d,0x44,0xcf,0xd8
    };

    secp256k1_num_set_bin(&ret->a1b2, secp256k1_scalar_consts_a1b2, sizeof(secp256k1_scalar_consts_a1b2));
    secp256k1_num_set_bin(&ret->a2, secp256k1_scalar_consts_a2, sizeof(secp256k1_scalar_consts_a2));
    secp256k1_num_set_bin(&ret->b1, secp256k1_scalar_consts_b1, sizeof(secp256k1_scalar_consts_b1));
#endif

    /* Set the global pointer. */
    secp256k1_scalar_consts = ret;
}

static void secp256k1_scalar_stop(void) {
    if (secp256k1_scalar_consts == NULL)
        return;

    secp256k1_scalar_consts_t *c = (secp256k1_scalar_consts_t*)secp256k1_scalar_consts;
    secp256k1_scalar_consts = NULL;
    free(c);
}

static void secp256k1_scalar_get_num(secp256k1_num_t *r, const secp256k1_scalar_t *a) {
    unsigned char c[32];
    secp256k1_scalar_get_b32(c, a);
    secp256k1_num_set_bin(r, c, 32);
}

static void secp256k1_scalar_order_get_num(secp256k1_num_t *r) {
    *r = secp256k1_scalar_consts->order;
}

static void secp256k1_scalar_inverse(secp256k1_scalar_t *r, const secp256k1_scalar_t *x) {
    /* First compute x ^ (2^N - 1) for some values of N. */
    secp256k1_scalar_t x2, x3, x4, x6, x7, x8, x15, x30, x60, x120, x127;

    secp256k1_scalar_sqr(&x2,  x);
    secp256k1_scalar_mul(&x2, &x2,  x);

    secp256k1_scalar_sqr(&x3, &x2);
    secp256k1_scalar_mul(&x3, &x3,  x);

    secp256k1_scalar_sqr(&x4, &x3);
    secp256k1_scalar_mul(&x4, &x4,  x);

    secp256k1_scalar_sqr(&x6, &x4);
    secp256k1_scalar_sqr(&x6, &x6);
    secp256k1_scalar_mul(&x6, &x6, &x2);

    secp256k1_scalar_sqr(&x7, &x6);
    secp256k1_scalar_mul(&x7, &x7,  x);

    secp256k1_scalar_sqr(&x8, &x7);
    secp256k1_scalar_mul(&x8, &x8,  x);

    secp256k1_scalar_sqr(&x15, &x8);
    for (int i=0; i<6; i++)
        secp256k1_scalar_sqr(&x15, &x15);
    secp256k1_scalar_mul(&x15, &x15, &x7);

    secp256k1_scalar_sqr(&x30, &x15);
    for (int i=0; i<14; i++)
        secp256k1_scalar_sqr(&x30, &x30);
    secp256k1_scalar_mul(&x30, &x30, &x15);

    secp256k1_scalar_sqr(&x60, &x30);
    for (int i=0; i<29; i++)
        secp256k1_scalar_sqr(&x60, &x60);
    secp256k1_scalar_mul(&x60, &x60, &x30);

    secp256k1_scalar_sqr(&x120, &x60);
    for (int i=0; i<59; i++)
        secp256k1_scalar_sqr(&x120, &x120);
    secp256k1_scalar_mul(&x120, &x120, &x60);

    secp256k1_scalar_sqr(&x127, &x120);
    for (int i=0; i<6; i++)
        secp256k1_scalar_sqr(&x127, &x127);
    secp256k1_scalar_mul(&x127, &x127, &x7);

    /* Then accumulate the final result (t starts at x127). */
    secp256k1_scalar_t *t = &x127;
    for (int i=0; i<2; i++) /* 0 */
        secp256k1_scalar_sqr(t, t);
    secp256k1_scalar_mul(t, t, x); /* 1 */
    for (int i=0; i<4; i++) /* 0 */
        secp256k1_scalar_sqr(t, t);
    secp256k1_scalar_mul(t, t, &x3); /* 111 */
    for (int i=0; i<2; i++) /* 0 */
        secp256k1_scalar_sqr(t, t);
    secp256k1_scalar_mul(t, t, x); /* 1 */
    for (int i=0; i<2; i++) /* 0 */
        secp256k1_scalar_sqr(t, t);
    secp256k1_scalar_mul(t, t, x); /* 1 */
    for (int i=0; i<2; i++) /* 0 */
        secp256k1_scalar_sqr(t, t);
    secp256k1_scalar_mul(t, t, x); /* 1 */
    for (int i=0; i<4; i++) /* 0 */
        secp256k1_scalar_sqr(t, t);
    secp256k1_scalar_mul(t, t, &x3); /* 111 */
    for (int i=0; i<3; i++) /* 0 */
        secp256k1_scalar_sqr(t, t);
    secp256k1_scalar_mul(t, t, &x2); /* 11 */
    for (int i=0; i<4; i++) /* 0 */
        secp256k1_scalar_sqr(t, t);
    secp256k1_scalar_mul(t, t, &x3); /* 111 */
    for (int i=0; i<5; i++) /* 00 */
        secp256k1_scalar_sqr(t, t);
    secp256k1_scalar_mul(t, t, &x3); /* 111 */
    for (int i=0; i<4; i++) /* 00 */
        secp256k1_scalar_sqr(t, t);
    secp256k1_scalar_mul(t, t, &x2); /* 11 */
    for (int i=0; i<2; i++) /* 0 */
        secp256k1_scalar_sqr(t, t);
    secp256k1_scalar_mul(t, t, x); /* 1 */
    for (int i=0; i<2; i++) /* 0 */
        secp256k1_scalar_sqr(t, t);
    secp256k1_scalar_mul(t, t, x); /* 1 */
    for (int i=0; i<5; i++) /* 0 */
        secp256k1_scalar_sqr(t, t);
    secp256k1_scalar_mul(t, t, &x4); /* 1111 */
    for (int i=0; i<2; i++) /* 0 */
        secp256k1_scalar_sqr(t, t);
    secp256k1_scalar_mul(t, t, x); /* 1 */
    for (int i=0; i<3; i++) /* 00 */
        secp256k1_scalar_sqr(t, t);
    secp256k1_scalar_mul(t, t, x); /* 1 */
    for (int i=0; i<4; i++) /* 000 */
        secp256k1_scalar_sqr(t, t);
    secp256k1_scalar_mul(t, t, x); /* 1 */
    for (int i=0; i<2; i++) /* 0 */
        secp256k1_scalar_sqr(t, t);
    secp256k1_scalar_mul(t, t, x); /* 1 */
    for (int i=0; i<10; i++) /* 0000000 */
        secp256k1_scalar_sqr(t, t);
    secp256k1_scalar_mul(t, t, &x3); /* 111 */
    for (int i=0; i<4; i++) /* 0 */
        secp256k1_scalar_sqr(t, t);
    secp256k1_scalar_mul(t, t, &x3); /* 111 */
    for (int i=0; i<9; i++) /* 0 */
        secp256k1_scalar_sqr(t, t);
    secp256k1_scalar_mul(t, t, &x8); /* 11111111 */
    for (int i=0; i<2; i++) /* 0 */
        secp256k1_scalar_sqr(t, t);
    secp256k1_scalar_mul(t, t, x); /* 1 */
    for (int i=0; i<3; i++) /* 00 */
        secp256k1_scalar_sqr(t, t);
    secp256k1_scalar_mul(t, t, x); /* 1 */
    for (int i=0; i<3; i++) /* 00 */
        secp256k1_scalar_sqr(t, t);
    secp256k1_scalar_mul(t, t, x); /* 1 */
    for (int i=0; i<5; i++) /* 0 */
        secp256k1_scalar_sqr(t, t);
    secp256k1_scalar_mul(t, t, &x4); /* 1111 */
    for (int i=0; i<2; i++) /* 0 */
        secp256k1_scalar_sqr(t, t);
    secp256k1_scalar_mul(t, t, x); /* 1 */
    for (int i=0; i<5; i++) /* 000 */
        secp256k1_scalar_sqr(t, t);
    secp256k1_scalar_mul(t, t, &x2); /* 11 */
    for (int i=0; i<4; i++) /* 00 */
        secp256k1_scalar_sqr(t, t);
    secp256k1_scalar_mul(t, t, &x2); /* 11 */
    for (int i=0; i<2; i++) /* 0 */
        secp256k1_scalar_sqr(t, t);
    secp256k1_scalar_mul(t, t, x); /* 1 */
    for (int i=0; i<8; i++) /* 000000 */
        secp256k1_scalar_sqr(t, t);
    secp256k1_scalar_mul(t, t, &x2); /* 11 */
    for (int i=0; i<3; i++) /* 0 */
        secp256k1_scalar_sqr(t, t);
    secp256k1_scalar_mul(t, t, &x2); /* 11 */
    for (int i=0; i<3; i++) /* 00 */
        secp256k1_scalar_sqr(t, t);
    secp256k1_scalar_mul(t, t, x); /* 1 */
    for (int i=0; i<6; i++) /* 00000 */
        secp256k1_scalar_sqr(t, t);
    secp256k1_scalar_mul(t, t, x); /* 1 */
    for (int i=0; i<8; i++) /* 00 */
        secp256k1_scalar_sqr(t, t);
    secp256k1_scalar_mul(r, t, &x6); /* 111111 */
}

static void secp256k1_scalar_inverse_var(secp256k1_scalar_t *r, const secp256k1_scalar_t *x) {
#if defined(USE_SCALAR_INV_BUILTIN)
    secp256k1_scalar_inverse(r, x);
#elif defined(USE_SCALAR_INV_NUM)
    unsigned char b[32];
    secp256k1_scalar_get_b32(b, x);
    secp256k1_num_t n;
    secp256k1_num_set_bin(&n, b, 32);
    secp256k1_num_mod_inverse(&n, &n, &secp256k1_scalar_consts->order);
    secp256k1_num_get_bin(b, 32, &n);
    secp256k1_scalar_set_b32(r, b, NULL);
#else
#error "Please select scalar inverse implementation"
#endif
}

#ifdef USE_ENDOMORPHISM
static void secp256k1_scalar_split_lambda_var(secp256k1_scalar_t *r1, secp256k1_scalar_t *r2, const secp256k1_scalar_t *a) {
    unsigned char b[32];
    secp256k1_scalar_get_b32(b, a);
    secp256k1_num_t na;
    secp256k1_num_set_bin(&na, b, 32);

    secp256k1_num_t rn1, rn2;

    const secp256k1_scalar_consts_t *c = secp256k1_scalar_consts;
    secp256k1_num_t bnc1, bnc2, bnt1, bnt2, bnn2;

    secp256k1_num_copy(&bnn2, &c->order);
    secp256k1_num_shift(&bnn2, 1);

    secp256k1_num_mul(&bnc1, &na, &c->a1b2);
    secp256k1_num_add(&bnc1, &bnc1, &bnn2);
    secp256k1_num_div(&bnc1, &bnc1, &c->order);

    secp256k1_num_mul(&bnc2, &na, &c->b1);
    secp256k1_num_add(&bnc2, &bnc2, &bnn2);
    secp256k1_num_div(&bnc2, &bnc2, &c->order);

    secp256k1_num_mul(&bnt1, &bnc1, &c->a1b2);
    secp256k1_num_mul(&bnt2, &bnc2, &c->a2);
    secp256k1_num_add(&bnt1, &bnt1, &bnt2);
    secp256k1_num_sub(&rn1, &na, &bnt1);
    secp256k1_num_mul(&bnt1, &bnc1, &c->b1);
    secp256k1_num_mul(&bnt2, &bnc2, &c->a1b2);
    secp256k1_num_sub(&rn2, &bnt1, &bnt2);

    secp256k1_num_get_bin(b, 32, &rn1);
    secp256k1_scalar_set_b32(r1, b, NULL);
    if (secp256k1_num_is_neg(&rn1)) {
        secp256k1_scalar_negate(r1, r1);
    }
    secp256k1_num_get_bin(b, 32, &rn2);
    secp256k1_scalar_set_b32(r2, b, NULL);
    if (secp256k1_num_is_neg(&rn2)) {
        secp256k1_scalar_negate(r2, r2);
    }
}
#endif

#endif
