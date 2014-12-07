/**********************************************************************
 * Copyright (c) 2013, 2014 Pieter Wuille                             *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef _SECP256K1_FIELD_IMPL_H_
#define _SECP256K1_FIELD_IMPL_H_

#if defined HAVE_CONFIG_H
#include "libsecp256k1-config.h"
#endif

#include "util.h"

#if defined(USE_FIELD_GMP)
#include "field_gmp_impl.h"
#elif defined(USE_FIELD_10X26)
#include "field_10x26_impl.h"
#elif defined(USE_FIELD_5X52)
#include "field_5x52_impl.h"
#else
#error "Please select field implementation"
#endif

static void secp256k1_fe_get_hex(char *r, int *rlen, const secp256k1_fe_t *a) {
    if (*rlen < 65) {
        *rlen = 65;
        return;
    }
    *rlen = 65;
    unsigned char tmp[32];
    secp256k1_fe_t b = *a;
    secp256k1_fe_normalize(&b);
    secp256k1_fe_get_b32(tmp, &b);
    for (int i=0; i<32; i++) {
        static const char *c = "0123456789ABCDEF";
        r[2*i]   = c[(tmp[i] >> 4) & 0xF];
        r[2*i+1] = c[(tmp[i]) & 0xF];
    }
    r[64] = 0x00;
}

static int secp256k1_fe_set_hex(secp256k1_fe_t *r, const char *a, int alen) {
    unsigned char tmp[32] = {};
    static const int cvt[256] = {0, 0, 0, 0, 0, 0, 0,0,0,0,0,0,0,0,0,0,
                                 0, 0, 0, 0, 0, 0, 0,0,0,0,0,0,0,0,0,0,
                                 0, 0, 0, 0, 0, 0, 0,0,0,0,0,0,0,0,0,0,
                                 0, 1, 2, 3, 4, 5, 6,7,8,9,0,0,0,0,0,0,
                                 0,10,11,12,13,14,15,0,0,0,0,0,0,0,0,0,
                                 0, 0, 0, 0, 0, 0, 0,0,0,0,0,0,0,0,0,0,
                                 0,10,11,12,13,14,15,0,0,0,0,0,0,0,0,0,
                                 0, 0, 0, 0, 0, 0, 0,0,0,0,0,0,0,0,0,0,
                                 0, 0, 0, 0, 0, 0, 0,0,0,0,0,0,0,0,0,0,
                                 0, 0, 0, 0, 0, 0, 0,0,0,0,0,0,0,0,0,0,
                                 0, 0, 0, 0, 0, 0, 0,0,0,0,0,0,0,0,0,0,
                                 0, 0, 0, 0, 0, 0, 0,0,0,0,0,0,0,0,0,0,
                                 0, 0, 0, 0, 0, 0, 0,0,0,0,0,0,0,0,0,0,
                                 0, 0, 0, 0, 0, 0, 0,0,0,0,0,0,0,0,0,0,
                                 0, 0, 0, 0, 0, 0, 0,0,0,0,0,0,0,0,0,0,
                                 0, 0, 0, 0, 0, 0, 0,0,0,0,0,0,0,0,0,0};
    for (int i=0; i<32; i++) {
        if (alen > i*2)
            tmp[32 - alen/2 + i] = (cvt[(unsigned char)a[2*i]] << 4) + cvt[(unsigned char)a[2*i+1]];
    }
    return secp256k1_fe_set_b32(r, tmp);
}

static int secp256k1_fe_sqrt_var(secp256k1_fe_t *r, const secp256k1_fe_t *a) {

    /** The binary representation of (p + 1)/4 has 3 blocks of 1s, with lengths in
     *  { 2, 22, 223 }. Use an addition chain to calculate 2^n - 1 for each block:
     *  1, [2], 3, 6, 9, 11, [22], 44, 88, 176, 220, [223]
     */

    secp256k1_fe_t x2;
    secp256k1_fe_sqr(&x2, a);
    secp256k1_fe_mul(&x2, &x2, a);

    secp256k1_fe_t x3;
    secp256k1_fe_sqr(&x3, &x2);
    secp256k1_fe_mul(&x3, &x3, a);

    secp256k1_fe_t x6 = x3;
    for (int j=0; j<3; j++) secp256k1_fe_sqr(&x6, &x6);
    secp256k1_fe_mul(&x6, &x6, &x3);

    secp256k1_fe_t x9 = x6;
    for (int j=0; j<3; j++) secp256k1_fe_sqr(&x9, &x9);
    secp256k1_fe_mul(&x9, &x9, &x3);

    secp256k1_fe_t x11 = x9;
    for (int j=0; j<2; j++) secp256k1_fe_sqr(&x11, &x11);
    secp256k1_fe_mul(&x11, &x11, &x2);

    secp256k1_fe_t x22 = x11;
    for (int j=0; j<11; j++) secp256k1_fe_sqr(&x22, &x22);
    secp256k1_fe_mul(&x22, &x22, &x11);

    secp256k1_fe_t x44 = x22;
    for (int j=0; j<22; j++) secp256k1_fe_sqr(&x44, &x44);
    secp256k1_fe_mul(&x44, &x44, &x22);

    secp256k1_fe_t x88 = x44;
    for (int j=0; j<44; j++) secp256k1_fe_sqr(&x88, &x88);
    secp256k1_fe_mul(&x88, &x88, &x44);

    secp256k1_fe_t x176 = x88;
    for (int j=0; j<88; j++) secp256k1_fe_sqr(&x176, &x176);
    secp256k1_fe_mul(&x176, &x176, &x88);

    secp256k1_fe_t x220 = x176;
    for (int j=0; j<44; j++) secp256k1_fe_sqr(&x220, &x220);
    secp256k1_fe_mul(&x220, &x220, &x44);

    secp256k1_fe_t x223 = x220;
    for (int j=0; j<3; j++) secp256k1_fe_sqr(&x223, &x223);
    secp256k1_fe_mul(&x223, &x223, &x3);

    /* The final result is then assembled using a sliding window over the blocks. */

    secp256k1_fe_t t1 = x223;
    for (int j=0; j<23; j++) secp256k1_fe_sqr(&t1, &t1);
    secp256k1_fe_mul(&t1, &t1, &x22);
    for (int j=0; j<6; j++) secp256k1_fe_sqr(&t1, &t1);
    secp256k1_fe_mul(&t1, &t1, &x2);
    secp256k1_fe_sqr(&t1, &t1);
    secp256k1_fe_sqr(r, &t1);

    /* Check that a square root was actually calculated */

    secp256k1_fe_sqr(&t1, r);
    secp256k1_fe_negate(&t1, &t1, 1);
    secp256k1_fe_add(&t1, a);
    secp256k1_fe_normalize_var(&t1);
    return secp256k1_fe_is_zero(&t1);
}

static void secp256k1_fe_inv(secp256k1_fe_t *r, const secp256k1_fe_t *a) {

    /** The binary representation of (p - 2) has 5 blocks of 1s, with lengths in
     *  { 1, 2, 22, 223 }. Use an addition chain to calculate 2^n - 1 for each block:
     *  [1], [2], 3, 6, 9, 11, [22], 44, 88, 176, 220, [223]
     */

    secp256k1_fe_t x2;
    secp256k1_fe_sqr(&x2, a);
    secp256k1_fe_mul(&x2, &x2, a);

    secp256k1_fe_t x3;
    secp256k1_fe_sqr(&x3, &x2);
    secp256k1_fe_mul(&x3, &x3, a);

    secp256k1_fe_t x6 = x3;
    for (int j=0; j<3; j++) secp256k1_fe_sqr(&x6, &x6);
    secp256k1_fe_mul(&x6, &x6, &x3);

    secp256k1_fe_t x9 = x6;
    for (int j=0; j<3; j++) secp256k1_fe_sqr(&x9, &x9);
    secp256k1_fe_mul(&x9, &x9, &x3);

    secp256k1_fe_t x11 = x9;
    for (int j=0; j<2; j++) secp256k1_fe_sqr(&x11, &x11);
    secp256k1_fe_mul(&x11, &x11, &x2);

    secp256k1_fe_t x22 = x11;
    for (int j=0; j<11; j++) secp256k1_fe_sqr(&x22, &x22);
    secp256k1_fe_mul(&x22, &x22, &x11);

    secp256k1_fe_t x44 = x22;
    for (int j=0; j<22; j++) secp256k1_fe_sqr(&x44, &x44);
    secp256k1_fe_mul(&x44, &x44, &x22);

    secp256k1_fe_t x88 = x44;
    for (int j=0; j<44; j++) secp256k1_fe_sqr(&x88, &x88);
    secp256k1_fe_mul(&x88, &x88, &x44);

    secp256k1_fe_t x176 = x88;
    for (int j=0; j<88; j++) secp256k1_fe_sqr(&x176, &x176);
    secp256k1_fe_mul(&x176, &x176, &x88);

    secp256k1_fe_t x220 = x176;
    for (int j=0; j<44; j++) secp256k1_fe_sqr(&x220, &x220);
    secp256k1_fe_mul(&x220, &x220, &x44);

    secp256k1_fe_t x223 = x220;
    for (int j=0; j<3; j++) secp256k1_fe_sqr(&x223, &x223);
    secp256k1_fe_mul(&x223, &x223, &x3);

    /* The final result is then assembled using a sliding window over the blocks. */

    secp256k1_fe_t t1 = x223;
    for (int j=0; j<23; j++) secp256k1_fe_sqr(&t1, &t1);
    secp256k1_fe_mul(&t1, &t1, &x22);
    for (int j=0; j<5; j++) secp256k1_fe_sqr(&t1, &t1);
    secp256k1_fe_mul(&t1, &t1, a);
    for (int j=0; j<3; j++) secp256k1_fe_sqr(&t1, &t1);
    secp256k1_fe_mul(&t1, &t1, &x2);
    for (int j=0; j<2; j++) secp256k1_fe_sqr(&t1, &t1);
    secp256k1_fe_mul(r, a, &t1);
}

static void secp256k1_fe_inv_var(secp256k1_fe_t *r, const secp256k1_fe_t *a) {
#if defined(USE_FIELD_INV_BUILTIN)
    secp256k1_fe_inv(r, a);
#elif defined(USE_FIELD_INV_NUM)
    unsigned char b[32];
    secp256k1_fe_t c = *a;
    secp256k1_fe_normalize_var(&c);
    secp256k1_fe_get_b32(b, &c);
    secp256k1_num_t n;
    secp256k1_num_set_bin(&n, b, 32);
    secp256k1_num_mod_inverse(&n, &n, &secp256k1_fe_consts->p);
    secp256k1_num_get_bin(b, 32, &n);
    VERIFY_CHECK(secp256k1_fe_set_b32(r, b));
#else
#error "Please select field inverse implementation"
#endif
}

static void secp256k1_fe_inv_all_var(size_t len, secp256k1_fe_t r[len], const secp256k1_fe_t a[len]) {
    if (len < 1)
        return;

    VERIFY_CHECK((r + len <= a) || (a + len <= r));

    r[0] = a[0];

    size_t i = 0;
    while (++i < len) {
        secp256k1_fe_mul(&r[i], &r[i - 1], &a[i]);
    }

    secp256k1_fe_t u; secp256k1_fe_inv_var(&u, &r[--i]);

    while (i > 0) {
        int j = i--;
        secp256k1_fe_mul(&r[j], &r[i], &u);
        secp256k1_fe_mul(&u, &u, &a[j]);
    }

    r[0] = u;
}

static void secp256k1_fe_start(void) {
#ifndef USE_NUM_NONE
    static const unsigned char secp256k1_fe_consts_p[] = {
        0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
        0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
        0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
        0xFF,0xFF,0xFF,0xFE,0xFF,0xFF,0xFC,0x2F
    };
#endif
    if (secp256k1_fe_consts == NULL) {
        secp256k1_fe_inner_start();
        secp256k1_fe_consts_t *ret = (secp256k1_fe_consts_t*)checked_malloc(sizeof(secp256k1_fe_consts_t));
#ifndef USE_NUM_NONE
        secp256k1_num_set_bin(&ret->p, secp256k1_fe_consts_p, sizeof(secp256k1_fe_consts_p));
#endif
        secp256k1_fe_consts = ret;
    }
}

static void secp256k1_fe_stop(void) {
    if (secp256k1_fe_consts != NULL) {
        secp256k1_fe_consts_t *c = (secp256k1_fe_consts_t*)secp256k1_fe_consts;
        free((void*)c);
        secp256k1_fe_consts = NULL;
        secp256k1_fe_inner_stop();
    }
}

#endif
