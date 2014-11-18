/**********************************************************************
 * Copyright (c) 2014 Pieter Wuille                                   *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef _SECP256K1_SCALAR_IMPL_H_
#define _SECP256K1_SCALAR_IMPL_H_

#include <string.h>

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

static void secp256k1_scalar_get_num(secp256k1_num_t *r, const secp256k1_scalar_t *a) {
    unsigned char c[32];
    secp256k1_scalar_get_b32(c, a);
    secp256k1_num_set_bin(r, c, 32);
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

#endif
