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

#ifndef USE_NUM_NONE
static void secp256k1_scalar_get_num(secp256k1_num_t *r, const secp256k1_scalar_t *a) {
    unsigned char c[32];
    secp256k1_scalar_get_b32(c, a);
    secp256k1_num_set_bin(r, c, 32);
}

/** secp256k1 curve order, see secp256k1_ecdsa_const_order_as_fe in ecdsa_impl.h */
static void secp256k1_scalar_order_get_num(secp256k1_num_t *r) {
    static const unsigned char order[32] = {
        0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
        0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFE,
        0xBA,0xAE,0xDC,0xE6,0xAF,0x48,0xA0,0x3B,
        0xBF,0xD2,0x5E,0x8C,0xD0,0x36,0x41,0x41
    };
    secp256k1_num_set_bin(r, order, 32);
}
#endif

static void secp256k1_scalar_inverse(secp256k1_scalar_t *r, const secp256k1_scalar_t *x) {
    secp256k1_scalar_t *t;
    int i;
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
    for (i = 0; i < 6; i++) {
        secp256k1_scalar_sqr(&x15, &x15);
    }
    secp256k1_scalar_mul(&x15, &x15, &x7);

    secp256k1_scalar_sqr(&x30, &x15);
    for (i = 0; i < 14; i++) {
        secp256k1_scalar_sqr(&x30, &x30);
    }
    secp256k1_scalar_mul(&x30, &x30, &x15);

    secp256k1_scalar_sqr(&x60, &x30);
    for (i = 0; i < 29; i++) {
        secp256k1_scalar_sqr(&x60, &x60);
    }
    secp256k1_scalar_mul(&x60, &x60, &x30);

    secp256k1_scalar_sqr(&x120, &x60);
    for (i = 0; i < 59; i++) {
        secp256k1_scalar_sqr(&x120, &x120);
    }
    secp256k1_scalar_mul(&x120, &x120, &x60);

    secp256k1_scalar_sqr(&x127, &x120);
    for (i = 0; i < 6; i++) {
        secp256k1_scalar_sqr(&x127, &x127);
    }
    secp256k1_scalar_mul(&x127, &x127, &x7);

    /* Then accumulate the final result (t starts at x127). */
    t = &x127;
    for (i = 0; i < 2; i++) { /* 0 */
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, x); /* 1 */
    for (i = 0; i < 4; i++) { /* 0 */
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, &x3); /* 111 */
    for (i = 0; i < 2; i++) { /* 0 */
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, x); /* 1 */
    for (i = 0; i < 2; i++) { /* 0 */
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, x); /* 1 */
    for (i = 0; i < 2; i++) { /* 0 */
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, x); /* 1 */
    for (i = 0; i < 4; i++) { /* 0 */
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, &x3); /* 111 */
    for (i = 0; i < 3; i++) { /* 0 */
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, &x2); /* 11 */
    for (i = 0; i < 4; i++) { /* 0 */
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, &x3); /* 111 */
    for (i = 0; i < 5; i++) { /* 00 */
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, &x3); /* 111 */
    for (i = 0; i < 4; i++) { /* 00 */
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, &x2); /* 11 */
    for (i = 0; i < 2; i++) { /* 0 */
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, x); /* 1 */
    for (i = 0; i < 2; i++) { /* 0 */
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, x); /* 1 */
    for (i = 0; i < 5; i++) { /* 0 */
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, &x4); /* 1111 */
    for (i = 0; i < 2; i++) { /* 0 */
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, x); /* 1 */
    for (i = 0; i < 3; i++) { /* 00 */
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, x); /* 1 */
    for (i = 0; i < 4; i++) { /* 000 */
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, x); /* 1 */
    for (i = 0; i < 2; i++) { /* 0 */
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, x); /* 1 */
    for (i = 0; i < 10; i++) { /* 0000000 */
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, &x3); /* 111 */
    for (i = 0; i < 4; i++) { /* 0 */
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, &x3); /* 111 */
    for (i = 0; i < 9; i++) { /* 0 */
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, &x8); /* 11111111 */
    for (i = 0; i < 2; i++) { /* 0 */
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, x); /* 1 */
    for (i = 0; i < 3; i++) { /* 00 */
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, x); /* 1 */
    for (i = 0; i < 3; i++) { /* 00 */
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, x); /* 1 */
    for (i = 0; i < 5; i++) { /* 0 */
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, &x4); /* 1111 */
    for (i = 0; i < 2; i++) { /* 0 */
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, x); /* 1 */
    for (i = 0; i < 5; i++) { /* 000 */
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, &x2); /* 11 */
    for (i = 0; i < 4; i++) { /* 00 */
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, &x2); /* 11 */
    for (i = 0; i < 2; i++) { /* 0 */
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, x); /* 1 */
    for (i = 0; i < 8; i++) { /* 000000 */
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, &x2); /* 11 */
    for (i = 0; i < 3; i++) { /* 0 */
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, &x2); /* 11 */
    for (i = 0; i < 3; i++) { /* 00 */
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, x); /* 1 */
    for (i = 0; i < 6; i++) { /* 00000 */
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, x); /* 1 */
    for (i = 0; i < 8; i++) { /* 00 */
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(r, t, &x6); /* 111111 */
}

static void secp256k1_scalar_inverse_var(secp256k1_scalar_t *r, const secp256k1_scalar_t *x) {
#if defined(USE_SCALAR_INV_BUILTIN)
    secp256k1_scalar_inverse(r, x);
#elif defined(USE_SCALAR_INV_NUM)
    unsigned char b[32];
    secp256k1_num_t n, m;
    secp256k1_scalar_get_b32(b, x);
    secp256k1_num_set_bin(&n, b, 32);
    secp256k1_scalar_order_get_num(&m);
    secp256k1_num_mod_inverse(&n, &n, &m);
    secp256k1_num_get_bin(b, 32, &n);
    secp256k1_scalar_set_b32(r, b, NULL);
#else
#error "Please select scalar inverse implementation"
#endif
}

#ifdef USE_ENDOMORPHISM
/**
 * The Secp256k1 curve has an endomorphism, where lambda * (x, y) = (beta * x, y), where
 * lambda is {0x53,0x63,0xad,0x4c,0xc0,0x5c,0x30,0xe0,0xa5,0x26,0x1c,0x02,0x88,0x12,0x64,0x5a,
 *            0x12,0x2e,0x22,0xea,0x20,0x81,0x66,0x78,0xdf,0x02,0x96,0x7c,0x1b,0x23,0xbd,0x72}
 *
 * "Guide to Elliptic Curve Cryptography" (Hankerson, Menezes, Vanstone) gives an algorithm
 * (algorithm 3.74) to find k1 and k2 given k, such that k1 + k2 * lambda == k mod n, and k1
 * and k2 have a small size.
 * It relies on constants a1, b1, a2, b2. These constants for the value of lambda above are:
 *
 * - a1 =      {0x30,0x86,0xd2,0x21,0xa7,0xd4,0x6b,0xcd,0xe8,0x6c,0x90,0xe4,0x92,0x84,0xeb,0x15}
 * - b1 =     -{0xe4,0x43,0x7e,0xd6,0x01,0x0e,0x88,0x28,0x6f,0x54,0x7f,0xa9,0x0a,0xbf,0xe4,0xc3}
 * - a2 = {0x01,0x14,0xca,0x50,0xf7,0xa8,0xe2,0xf3,0xf6,0x57,0xc1,0x10,0x8d,0x9d,0x44,0xcf,0xd8}
 * - b2 =      {0x30,0x86,0xd2,0x21,0xa7,0xd4,0x6b,0xcd,0xe8,0x6c,0x90,0xe4,0x92,0x84,0xeb,0x15}
 *
 * The algorithm then computes c1 = round(b1 * k / n) and c2 = round(b2 * k / n), and gives
 * k1 = k - (c1*a1 + c2*a2) and k2 = -(c1*b1 + c2*b2). Instead, we use modular arithmetic, and
 * compute k1 as k - k2 * lambda, avoiding the need for constants a1 and a2.
 *
 * g1, g2 are precomputed constants used to replace division with a rounded multiplication
 * when decomposing the scalar for an endomorphism-based point multiplication.
 *
 * The possibility of using precomputed estimates is mentioned in "Guide to Elliptic Curve
 * Cryptography" (Hankerson, Menezes, Vanstone) in section 3.5.
 *
 * The derivation is described in the paper "Efficient Software Implementation of Public-Key
 * Cryptography on Sensor Networks Using the MSP430X Microcontroller" (Gouvea, Oliveira, Lopez),
 * Section 4.3 (here we use a somewhat higher-precision estimate):
 * d = a1*b2 - b1*a2
 * g1 = round((2^272)*b2/d)
 * g2 = round((2^272)*b1/d)
 *
 * (Note that 'd' is also equal to the curve order here because [a1,b1] and [a2,b2] are found
 * as outputs of the Extended Euclidean Algorithm on inputs 'order' and 'lambda').
 *
 * The function below splits a in r1 and r2, such that r1 + lambda * r2 == a (mod order).
 */

static void secp256k1_scalar_split_lambda_var(secp256k1_scalar_t *r1, secp256k1_scalar_t *r2, const secp256k1_scalar_t *a) {
    secp256k1_scalar_t c1, c2;
    static const secp256k1_scalar_t minus_lambda = SECP256K1_SCALAR_CONST(
        0xAC9C52B3UL, 0x3FA3CF1FUL, 0x5AD9E3FDUL, 0x77ED9BA4UL,
        0xA880B9FCUL, 0x8EC739C2UL, 0xE0CFC810UL, 0xB51283CFUL
    );
    static const secp256k1_scalar_t minus_b1 = SECP256K1_SCALAR_CONST(
        0x00000000UL, 0x00000000UL, 0x00000000UL, 0x00000000UL,
        0xE4437ED6UL, 0x010E8828UL, 0x6F547FA9UL, 0x0ABFE4C3UL
    );
    static const secp256k1_scalar_t minus_b2 = SECP256K1_SCALAR_CONST(
        0xFFFFFFFFUL, 0xFFFFFFFFUL, 0xFFFFFFFFUL, 0xFFFFFFFEUL,
        0x8A280AC5UL, 0x0774346DUL, 0xD765CDA8UL, 0x3DB1562CUL
    );
    static const secp256k1_scalar_t g1 = SECP256K1_SCALAR_CONST(
        0x00000000UL, 0x00000000UL, 0x00000000UL, 0x00003086UL,
        0xD221A7D4UL, 0x6BCDE86CUL, 0x90E49284UL, 0xEB153DABUL
    );
    static const secp256k1_scalar_t g2 = SECP256K1_SCALAR_CONST(
        0x00000000UL, 0x00000000UL, 0x00000000UL, 0x0000E443UL,
        0x7ED6010EUL, 0x88286F54UL, 0x7FA90ABFUL, 0xE4C42212UL
    );
    VERIFY_CHECK(r1 != a);
    VERIFY_CHECK(r2 != a);
    secp256k1_scalar_mul_shift_var(&c1, a, &g1, 272);
    secp256k1_scalar_mul_shift_var(&c2, a, &g2, 272);
    secp256k1_scalar_mul(&c1, &c1, &minus_b1);
    secp256k1_scalar_mul(&c2, &c2, &minus_b2);
    secp256k1_scalar_add(r2, &c1, &c2);
    secp256k1_scalar_mul(r1, r2, &minus_lambda);
    secp256k1_scalar_add(r1, r1, a);
}
#endif

#endif
