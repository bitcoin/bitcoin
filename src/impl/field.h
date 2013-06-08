// Copyright (c) 2013 Pieter Wuille
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef _SECP256K1_FIELD_IMPL_H_
#define _SECP256K1_FIELD_IMPL_H_

#if defined(USE_FIELD_GMP)
#include "field_gmp.h"
#elif defined(USE_FIELD_10X26)
#include "field_10x26.h"
#elif defined(USE_FIELD_5X52)
#include "field_5x52.h"
#elif defined(USE_FIELD_5X64)
#include "field_5x64.h"
#else
#error "Please select field implementation"
#endif

void static secp256k1_fe_get_hex(char *r, int *rlen, const secp256k1_fe_t *a) {
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

void static secp256k1_fe_set_hex(secp256k1_fe_t *r, const char *a, int alen) {
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
    secp256k1_fe_set_b32(r, tmp);
}

void static secp256k1_fe_sqrt(secp256k1_fe_t *r, const secp256k1_fe_t *a) {
    // calculate a^p, with p={15,780,1022,1023}
    secp256k1_fe_t a2; secp256k1_fe_sqr(&a2, a);
    secp256k1_fe_t a3; secp256k1_fe_mul(&a3, &a2, a);
    secp256k1_fe_t a6; secp256k1_fe_sqr(&a6, &a3);
    secp256k1_fe_t a12; secp256k1_fe_sqr(&a12, &a6);
    secp256k1_fe_t a15; secp256k1_fe_mul(&a15, &a12, &a3);
    secp256k1_fe_t a30; secp256k1_fe_sqr(&a30, &a15);
    secp256k1_fe_t a60; secp256k1_fe_sqr(&a60, &a30);
    secp256k1_fe_t a120; secp256k1_fe_sqr(&a120, &a60);
    secp256k1_fe_t a240; secp256k1_fe_sqr(&a240, &a120);
    secp256k1_fe_t a255; secp256k1_fe_mul(&a255, &a240, &a15);
    secp256k1_fe_t a510; secp256k1_fe_sqr(&a510, &a255);
    secp256k1_fe_t a750; secp256k1_fe_mul(&a750, &a510, &a240);
    secp256k1_fe_t a780; secp256k1_fe_mul(&a780, &a750, &a30);
    secp256k1_fe_t a1020; secp256k1_fe_sqr(&a1020, &a510);
    secp256k1_fe_t a1022; secp256k1_fe_mul(&a1022, &a1020, &a2);
    secp256k1_fe_t a1023; secp256k1_fe_mul(&a1023, &a1022, a);
    secp256k1_fe_t x = a15;
    for (int i=0; i<21; i++) {
        for (int j=0; j<10; j++) secp256k1_fe_sqr(&x, &x);
        secp256k1_fe_mul(&x, &x, &a1023);
    }
    for (int j=0; j<10; j++) secp256k1_fe_sqr(&x, &x);
    secp256k1_fe_mul(&x, &x, &a1022);
    for (int i=0; i<2; i++) {
        for (int j=0; j<10; j++) secp256k1_fe_sqr(&x, &x);
        secp256k1_fe_mul(&x, &x, &a1023);
    }
    for (int j=0; j<10; j++) secp256k1_fe_sqr(&x, &x);
    secp256k1_fe_mul(r, &x, &a780);
}

void static secp256k1_fe_inv(secp256k1_fe_t *r, const secp256k1_fe_t *a) {
    // calculate a^p, with p={45,63,1019,1023}
    secp256k1_fe_t a2; secp256k1_fe_sqr(&a2, a);
    secp256k1_fe_t a3; secp256k1_fe_mul(&a3, &a2, a);
    secp256k1_fe_t a4; secp256k1_fe_sqr(&a4, &a2);
    secp256k1_fe_t a5; secp256k1_fe_mul(&a5, &a4, a);
    secp256k1_fe_t a10; secp256k1_fe_sqr(&a10, &a5);
    secp256k1_fe_t a11; secp256k1_fe_mul(&a11, &a10, a);
    secp256k1_fe_t a21; secp256k1_fe_mul(&a21, &a11, &a10);
    secp256k1_fe_t a42; secp256k1_fe_sqr(&a42, &a21);
    secp256k1_fe_t a45; secp256k1_fe_mul(&a45, &a42, &a3);
    secp256k1_fe_t a63; secp256k1_fe_mul(&a63, &a42, &a21);
    secp256k1_fe_t a126; secp256k1_fe_sqr(&a126, &a63);
    secp256k1_fe_t a252; secp256k1_fe_sqr(&a252, &a126);
    secp256k1_fe_t a504; secp256k1_fe_sqr(&a504, &a252);
    secp256k1_fe_t a1008; secp256k1_fe_sqr(&a1008, &a504);
    secp256k1_fe_t a1019; secp256k1_fe_mul(&a1019, &a1008, &a11);
    secp256k1_fe_t a1023; secp256k1_fe_mul(&a1023, &a1019, &a4);
    secp256k1_fe_t x = a63;
    for (int i=0; i<21; i++) {
        for (int j=0; j<10; j++) secp256k1_fe_sqr(&x, &x);
        secp256k1_fe_mul(&x, &x, &a1023);
    }
    for (int j=0; j<10; j++) secp256k1_fe_sqr(&x, &x);
    secp256k1_fe_mul(&x, &x, &a1019);
    for (int i=0; i<2; i++) {
        for (int j=0; j<10; j++) secp256k1_fe_sqr(&x, &x);
        secp256k1_fe_mul(&x, &x, &a1023);
    }
    for (int j=0; j<10; j++) secp256k1_fe_sqr(&x, &x);
    secp256k1_fe_mul(r, &x, &a45);
}

void static secp256k1_fe_inv_var(secp256k1_fe_t *r, const secp256k1_fe_t *a) {
#if defined(USE_FIELD_INV_BUILTIN)
    secp256k1_fe_inv(r, a);
#elif defined(USE_FIELD_INV_NUM)
    unsigned char b[32];
    secp256k1_fe_t c = *a;
    secp256k1_fe_normalize(&c);
    secp256k1_fe_get_b32(b, &c);
    secp256k1_num_t n; 
    secp256k1_num_init(&n);
    secp256k1_num_set_bin(&n, b, 32);
    secp256k1_num_mod_inverse(&n, &n, &secp256k1_fe_consts->p);
    secp256k1_num_get_bin(b, 32, &n);
    secp256k1_num_free(&n);
    secp256k1_fe_set_b32(r, b);
#else
#error "Please select field inverse implementation"
#endif
}

void static secp256k1_fe_start(void) {
    static const unsigned char secp256k1_fe_consts_p[] = {
        0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
        0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
        0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
        0xFF,0xFF,0xFF,0xFE,0xFF,0xFF,0xFC,0x2F
    };
    if (secp256k1_fe_consts == NULL) {
        secp256k1_fe_inner_start();
        secp256k1_fe_consts_t *ret = (secp256k1_fe_consts_t*)malloc(sizeof(secp256k1_fe_consts_t));
        secp256k1_num_init(&ret->p);
        secp256k1_num_set_bin(&ret->p, secp256k1_fe_consts_p, sizeof(secp256k1_fe_consts_p));
        secp256k1_fe_consts = ret;
    }
}

void static secp256k1_fe_stop(void) {
    if (secp256k1_fe_consts != NULL) {
        secp256k1_fe_consts_t *c = (secp256k1_fe_consts_t*)secp256k1_fe_consts;
        secp256k1_num_free(&c->p);
        free((void*)c);
        secp256k1_fe_consts = NULL;
        secp256k1_fe_inner_stop();
    }
}

#endif
