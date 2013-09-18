// Copyright (c) 2013 Pieter Wuille
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef _SECP256K1_GROUP_IMPL_H_
#define _SECP256K1_GROUP_IMPL_H_

#include <string.h>

#include "../num.h"
#include "../field.h"
#include "../group.h"

void static secp256k1_ge_set_infinity(secp256k1_ge_t *r) {
    r->infinity = 1;
}

void static secp256k1_ge_set_xy(secp256k1_ge_t *r, const secp256k1_fe_t *x, const secp256k1_fe_t *y) {
    r->infinity = 0;
    r->x = *x;
    r->y = *y;
}

int static secp256k1_ge_is_infinity(const secp256k1_ge_t *a) {
    return a->infinity;
}

void static secp256k1_ge_neg(secp256k1_ge_t *r, const secp256k1_ge_t *a) {
    r->infinity = a->infinity;
    r->x = a->x;
    r->y = a->y;
    secp256k1_fe_normalize(&r->y);
    secp256k1_fe_negate(&r->y, &r->y, 1);
}

void static secp256k1_ge_get_hex(char *r, int *rlen, const secp256k1_ge_t *a) {
    char cx[65]; int lx=65;
    char cy[65]; int ly=65;
    secp256k1_fe_get_hex(cx, &lx, &a->x);
    secp256k1_fe_get_hex(cy, &ly, &a->y);
    lx = strlen(cx);
    ly = strlen(cy);
    int len = lx + ly + 3 + 1;
    if (*rlen < len) {
        *rlen = len;
        return;
    }
    *rlen = len;
    r[0] = '(';
    memcpy(r+1, cx, lx);
    r[1+lx] = ',';
    memcpy(r+2+lx, cy, ly);
    r[2+lx+ly] = ')';
    r[3+lx+ly] = 0;
}

void static secp256k1_ge_set_gej(secp256k1_ge_t *r, secp256k1_gej_t *a) {
    secp256k1_fe_inv_var(&a->z, &a->z);
    secp256k1_fe_t z2; secp256k1_fe_sqr(&z2, &a->z);
    secp256k1_fe_t z3; secp256k1_fe_mul(&z3, &a->z, &z2);
    secp256k1_fe_mul(&a->x, &a->x, &z2);
    secp256k1_fe_mul(&a->y, &a->y, &z3);
    secp256k1_fe_set_int(&a->z, 1);
    r->infinity = a->infinity;
    r->x = a->x;
    r->y = a->y;
}

void static secp256k1_gej_set_infinity(secp256k1_gej_t *r) {
    r->infinity = 1;
}

void static secp256k1_gej_set_xy(secp256k1_gej_t *r, const secp256k1_fe_t *x, const secp256k1_fe_t *y) {
    r->infinity = 0;
    r->x = *x;
    r->y = *y;
    secp256k1_fe_set_int(&r->z, 1);
}

void static secp256k1_ge_set_xo(secp256k1_ge_t *r, const secp256k1_fe_t *x, int odd) {
    r->x = *x;
    secp256k1_fe_t x2; secp256k1_fe_sqr(&x2, x);
    secp256k1_fe_t x3; secp256k1_fe_mul(&x3, x, &x2);
    r->infinity = 0;
    secp256k1_fe_t c; secp256k1_fe_set_int(&c, 7);
    secp256k1_fe_add(&c, &x3);
    secp256k1_fe_sqrt(&r->y, &c);
    secp256k1_fe_normalize(&r->y);
    if (secp256k1_fe_is_odd(&r->y) != odd)
        secp256k1_fe_negate(&r->y, &r->y, 1);
}

void static secp256k1_gej_set_ge(secp256k1_gej_t *r, const secp256k1_ge_t *a) {
   r->infinity = a->infinity;
   r->x = a->x;
   r->y = a->y;
   secp256k1_fe_set_int(&r->z, 1);
}

void static secp256k1_gej_get_x(secp256k1_fe_t *r, const secp256k1_gej_t *a) {
    secp256k1_fe_t zi2; secp256k1_fe_inv_var(&zi2, &a->z); secp256k1_fe_sqr(&zi2, &zi2);
    secp256k1_fe_mul(r, &a->x, &zi2);
}

void static secp256k1_gej_neg(secp256k1_gej_t *r, const secp256k1_gej_t *a) {
    r->infinity = a->infinity;
    r->x = a->x;
    r->y = a->y;
    r->z = a->z;
    secp256k1_fe_normalize(&r->y);
    secp256k1_fe_negate(&r->y, &r->y, 1);
}

int static secp256k1_gej_is_infinity(const secp256k1_gej_t *a) {
    return a->infinity;
}

int static secp256k1_gej_is_valid(const secp256k1_gej_t *a) {
    if (a->infinity)
        return 0;
    // y^2 = x^3 + 7
    // (Y/Z^3)^2 = (X/Z^2)^3 + 7
    // Y^2 / Z^6 = X^3 / Z^6 + 7
    // Y^2 = X^3 + 7*Z^6
    secp256k1_fe_t y2; secp256k1_fe_sqr(&y2, &a->y);
    secp256k1_fe_t x3; secp256k1_fe_sqr(&x3, &a->x); secp256k1_fe_mul(&x3, &x3, &a->x);
    secp256k1_fe_t z2; secp256k1_fe_sqr(&z2, &a->z);
    secp256k1_fe_t z6; secp256k1_fe_sqr(&z6, &z2); secp256k1_fe_mul(&z6, &z6, &z2);
    secp256k1_fe_mul_int(&z6, 7);
    secp256k1_fe_add(&x3, &z6);
    secp256k1_fe_normalize(&y2);
    secp256k1_fe_normalize(&x3);
    return secp256k1_fe_equal(&y2, &x3);
}

int static secp256k1_ge_is_valid(const secp256k1_ge_t *a) {
    if (a->infinity)
        return 0;
    // y^2 = x^3 + 7
    secp256k1_fe_t y2; secp256k1_fe_sqr(&y2, &a->y);
    secp256k1_fe_t x3; secp256k1_fe_sqr(&x3, &a->x); secp256k1_fe_mul(&x3, &x3, &a->x);
    secp256k1_fe_t c; secp256k1_fe_set_int(&c, 7);
    secp256k1_fe_add(&x3, &c);
    secp256k1_fe_normalize(&y2);
    secp256k1_fe_normalize(&x3);
    return secp256k1_fe_equal(&y2, &x3);
}

void static secp256k1_gej_double(secp256k1_gej_t *r, const secp256k1_gej_t *a) {
    secp256k1_fe_t t5 = a->y;
    secp256k1_fe_normalize(&t5);
    if (a->infinity || secp256k1_fe_is_zero(&t5)) {
        r->infinity = 1;
        return;
    }

    secp256k1_fe_t t1,t2,t3,t4;
    secp256k1_fe_mul(&r->z, &t5, &a->z);
    secp256k1_fe_mul_int(&r->z, 2);       // Z' = 2*Y*Z (2)
    secp256k1_fe_sqr(&t1, &a->x);
    secp256k1_fe_mul_int(&t1, 3);         // T1 = 3*X^2 (3)
    secp256k1_fe_sqr(&t2, &t1);           // T2 = 9*X^4 (1)
    secp256k1_fe_sqr(&t3, &t5);
    secp256k1_fe_mul_int(&t3, 2);         // T3 = 2*Y^2 (2)
    secp256k1_fe_sqr(&t4, &t3);
    secp256k1_fe_mul_int(&t4, 2);         // T4 = 8*Y^4 (2)
    secp256k1_fe_mul(&t3, &a->x, &t3);    // T3 = 2*X*Y^2 (1)
    r->x = t3;
    secp256k1_fe_mul_int(&r->x, 4);       // X' = 8*X*Y^2 (4)
    secp256k1_fe_negate(&r->x, &r->x, 4); // X' = -8*X*Y^2 (5)
    secp256k1_fe_add(&r->x, &t2);         // X' = 9*X^4 - 8*X*Y^2 (6)
    secp256k1_fe_negate(&t2, &t2, 1);     // T2 = -9*X^4 (2)
    secp256k1_fe_mul_int(&t3, 6);         // T3 = 12*X*Y^2 (6)
    secp256k1_fe_add(&t3, &t2);           // T3 = 12*X*Y^2 - 9*X^4 (8)
    secp256k1_fe_mul(&r->y, &t1, &t3);    // Y' = 36*X^3*Y^2 - 27*X^6 (1)
    secp256k1_fe_negate(&t2, &t4, 2);     // T2 = -8*Y^4 (3)
    secp256k1_fe_add(&r->y, &t2);         // Y' = 36*X^3*Y^2 - 27*X^6 - 8*Y^4 (4)
    r->infinity = 0;
}

void static secp256k1_gej_add(secp256k1_gej_t *r, const secp256k1_gej_t *a, const secp256k1_gej_t *b) {
    if (a->infinity) {
        *r = *b;
        return;
    }
    if (b->infinity) {
        *r = *a;
        return;
    }
    r->infinity = 0;
    secp256k1_fe_t z22; secp256k1_fe_sqr(&z22, &b->z);
    secp256k1_fe_t z12; secp256k1_fe_sqr(&z12, &a->z);
    secp256k1_fe_t u1; secp256k1_fe_mul(&u1, &a->x, &z22);
    secp256k1_fe_t u2; secp256k1_fe_mul(&u2, &b->x, &z12);
    secp256k1_fe_t s1; secp256k1_fe_mul(&s1, &a->y, &z22); secp256k1_fe_mul(&s1, &s1, &b->z);
    secp256k1_fe_t s2; secp256k1_fe_mul(&s2, &b->y, &z12); secp256k1_fe_mul(&s2, &s2, &a->z);
    secp256k1_fe_normalize(&u1);
    secp256k1_fe_normalize(&u2);
    if (secp256k1_fe_equal(&u1, &u2)) {
        secp256k1_fe_normalize(&s1);
        secp256k1_fe_normalize(&s2);
        if (secp256k1_fe_equal(&s1, &s2)) {
            secp256k1_gej_double(r, a);
        } else {
            r->infinity = 1;
        }
        return;
    }
    secp256k1_fe_t h; secp256k1_fe_negate(&h, &u1, 1); secp256k1_fe_add(&h, &u2);
    secp256k1_fe_t i; secp256k1_fe_negate(&i, &s1, 1); secp256k1_fe_add(&i, &s2);
    secp256k1_fe_t i2; secp256k1_fe_sqr(&i2, &i);
    secp256k1_fe_t h2; secp256k1_fe_sqr(&h2, &h);
    secp256k1_fe_t h3; secp256k1_fe_mul(&h3, &h, &h2);
    secp256k1_fe_mul(&r->z, &a->z, &b->z); secp256k1_fe_mul(&r->z, &r->z, &h);
    secp256k1_fe_t t; secp256k1_fe_mul(&t, &u1, &h2);
    r->x = t; secp256k1_fe_mul_int(&r->x, 2); secp256k1_fe_add(&r->x, &h3); secp256k1_fe_negate(&r->x, &r->x, 3); secp256k1_fe_add(&r->x, &i2);
    secp256k1_fe_negate(&r->y, &r->x, 5); secp256k1_fe_add(&r->y, &t); secp256k1_fe_mul(&r->y, &r->y, &i);
    secp256k1_fe_mul(&h3, &h3, &s1); secp256k1_fe_negate(&h3, &h3, 1);
    secp256k1_fe_add(&r->y, &h3);
}

void static secp256k1_gej_add_ge(secp256k1_gej_t *r, const secp256k1_gej_t *a, const secp256k1_ge_t *b) {
    if (a->infinity) {
        r->infinity = b->infinity;
        r->x = b->x;
        r->y = b->y;
        secp256k1_fe_set_int(&r->z, 1);
        return;
    }
    if (b->infinity) {
        *r = *a;
        return;
    }
    r->infinity = 0;
    secp256k1_fe_t z12; secp256k1_fe_sqr(&z12, &a->z);
    secp256k1_fe_t u1 = a->x; secp256k1_fe_normalize(&u1);
    secp256k1_fe_t u2; secp256k1_fe_mul(&u2, &b->x, &z12);
    secp256k1_fe_t s1 = a->y; secp256k1_fe_normalize(&s1);
    secp256k1_fe_t s2; secp256k1_fe_mul(&s2, &b->y, &z12); secp256k1_fe_mul(&s2, &s2, &a->z);
    secp256k1_fe_normalize(&u1);
    secp256k1_fe_normalize(&u2);
    if (secp256k1_fe_equal(&u1, &u2)) {
        secp256k1_fe_normalize(&s1);
        secp256k1_fe_normalize(&s2);
        if (secp256k1_fe_equal(&s1, &s2)) {
            secp256k1_gej_double(r, a);
        } else {
            r->infinity = 1;
        }
        return;
    }
    secp256k1_fe_t h; secp256k1_fe_negate(&h, &u1, 1); secp256k1_fe_add(&h, &u2);
    secp256k1_fe_t i; secp256k1_fe_negate(&i, &s1, 1); secp256k1_fe_add(&i, &s2);
    secp256k1_fe_t i2; secp256k1_fe_sqr(&i2, &i);
    secp256k1_fe_t h2; secp256k1_fe_sqr(&h2, &h);
    secp256k1_fe_t h3; secp256k1_fe_mul(&h3, &h, &h2);
    r->z = a->z; secp256k1_fe_mul(&r->z, &r->z, &h);
    secp256k1_fe_t t; secp256k1_fe_mul(&t, &u1, &h2);
    r->x = t; secp256k1_fe_mul_int(&r->x, 2); secp256k1_fe_add(&r->x, &h3); secp256k1_fe_negate(&r->x, &r->x, 3); secp256k1_fe_add(&r->x, &i2);
    secp256k1_fe_negate(&r->y, &r->x, 5); secp256k1_fe_add(&r->y, &t); secp256k1_fe_mul(&r->y, &r->y, &i);
    secp256k1_fe_mul(&h3, &h3, &s1); secp256k1_fe_negate(&h3, &h3, 1);
    secp256k1_fe_add(&r->y, &h3);
}

void static secp256k1_gej_get_hex(char *r, int *rlen, const secp256k1_gej_t *a) {
    secp256k1_gej_t c = *a;
    secp256k1_ge_t t; secp256k1_ge_set_gej(&t, &c);
    secp256k1_ge_get_hex(r, rlen, &t);
}

void static secp256k1_gej_mul_lambda(secp256k1_gej_t *r, const secp256k1_gej_t *a) {
    const secp256k1_fe_t *beta = &secp256k1_ge_consts->beta;
    *r = *a;
    secp256k1_fe_mul(&r->x, &r->x, beta);
}

void static secp256k1_gej_split_exp(secp256k1_num_t *r1, secp256k1_num_t *r2, const secp256k1_num_t *a) {
    const secp256k1_ge_consts_t *c = secp256k1_ge_consts;
    secp256k1_num_t bnc1, bnc2, bnt1, bnt2, bnn2;

    secp256k1_num_init(&bnc1);
    secp256k1_num_init(&bnc2);
    secp256k1_num_init(&bnt1);
    secp256k1_num_init(&bnt2);
    secp256k1_num_init(&bnn2);

    secp256k1_num_copy(&bnn2, &c->order);
    secp256k1_num_shift(&bnn2, 1);

    secp256k1_num_mul(&bnc1, a, &c->a1b2);
    secp256k1_num_add(&bnc1, &bnc1, &bnn2);
    secp256k1_num_div(&bnc1, &bnc1, &c->order);

    secp256k1_num_mul(&bnc2, a, &c->b1);
    secp256k1_num_add(&bnc2, &bnc2, &bnn2);
    secp256k1_num_div(&bnc2, &bnc2, &c->order);

    secp256k1_num_mul(&bnt1, &bnc1, &c->a1b2);
    secp256k1_num_mul(&bnt2, &bnc2, &c->a2);
    secp256k1_num_add(&bnt1, &bnt1, &bnt2);
    secp256k1_num_sub(r1, a, &bnt1);
    secp256k1_num_mul(&bnt1, &bnc1, &c->b1);
    secp256k1_num_mul(&bnt2, &bnc2, &c->a1b2);
    secp256k1_num_sub(r2, &bnt1, &bnt2);

    secp256k1_num_free(&bnc1);
    secp256k1_num_free(&bnc2);
    secp256k1_num_free(&bnt1);
    secp256k1_num_free(&bnt2);
    secp256k1_num_free(&bnn2);
}


void static secp256k1_ge_start(void) {
    static const unsigned char secp256k1_ge_consts_order[] = {
        0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
        0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFE,
        0xBA,0xAE,0xDC,0xE6,0xAF,0x48,0xA0,0x3B,
        0xBF,0xD2,0x5E,0x8C,0xD0,0x36,0x41,0x41
    };
    static const unsigned char secp256k1_ge_consts_g_x[] = {
        0x79,0xBE,0x66,0x7E,0xF9,0xDC,0xBB,0xAC,
        0x55,0xA0,0x62,0x95,0xCE,0x87,0x0B,0x07,
        0x02,0x9B,0xFC,0xDB,0x2D,0xCE,0x28,0xD9,
        0x59,0xF2,0x81,0x5B,0x16,0xF8,0x17,0x98
    };
    static const unsigned char secp256k1_ge_consts_g_y[] = {
        0x48,0x3A,0xDA,0x77,0x26,0xA3,0xC4,0x65,
        0x5D,0xA4,0xFB,0xFC,0x0E,0x11,0x08,0xA8,
        0xFD,0x17,0xB4,0x48,0xA6,0x85,0x54,0x19,
        0x9C,0x47,0xD0,0x8F,0xFB,0x10,0xD4,0xB8
    };
    // properties of secp256k1's efficiently computable endomorphism
    static const unsigned char secp256k1_ge_consts_lambda[] = {
        0x53,0x63,0xad,0x4c,0xc0,0x5c,0x30,0xe0,
        0xa5,0x26,0x1c,0x02,0x88,0x12,0x64,0x5a,
        0x12,0x2e,0x22,0xea,0x20,0x81,0x66,0x78,
        0xdf,0x02,0x96,0x7c,0x1b,0x23,0xbd,0x72
    };
    static const unsigned char secp256k1_ge_consts_beta[] = {
        0x7a,0xe9,0x6a,0x2b,0x65,0x7c,0x07,0x10,
        0x6e,0x64,0x47,0x9e,0xac,0x34,0x34,0xe9,
        0x9c,0xf0,0x49,0x75,0x12,0xf5,0x89,0x95,
        0xc1,0x39,0x6c,0x28,0x71,0x95,0x01,0xee
    };
    static const unsigned char secp256k1_ge_consts_a1b2[] = {
        0x30,0x86,0xd2,0x21,0xa7,0xd4,0x6b,0xcd,
        0xe8,0x6c,0x90,0xe4,0x92,0x84,0xeb,0x15
    };
    static const unsigned char secp256k1_ge_consts_b1[] = {
        0xe4,0x43,0x7e,0xd6,0x01,0x0e,0x88,0x28,
        0x6f,0x54,0x7f,0xa9,0x0a,0xbf,0xe4,0xc3
    };
    static const unsigned char secp256k1_ge_consts_a2[] = {
        0x01,
        0x14,0xca,0x50,0xf7,0xa8,0xe2,0xf3,0xf6,
        0x57,0xc1,0x10,0x8d,0x9d,0x44,0xcf,0xd8
    };
    if (secp256k1_ge_consts == NULL) {
        secp256k1_ge_consts_t *ret = (secp256k1_ge_consts_t*)malloc(sizeof(secp256k1_ge_consts_t));
        secp256k1_num_init(&ret->order);
        secp256k1_num_init(&ret->half_order);
        secp256k1_num_init(&ret->lambda);
        secp256k1_num_init(&ret->a1b2);
        secp256k1_num_init(&ret->a2);
        secp256k1_num_init(&ret->b1);
        secp256k1_num_set_bin(&ret->order,  secp256k1_ge_consts_order,  sizeof(secp256k1_ge_consts_order));
        secp256k1_num_set_bin(&ret->lambda, secp256k1_ge_consts_lambda, sizeof(secp256k1_ge_consts_lambda));
        secp256k1_num_set_bin(&ret->a1b2,   secp256k1_ge_consts_a1b2,   sizeof(secp256k1_ge_consts_a1b2));
        secp256k1_num_set_bin(&ret->a2,     secp256k1_ge_consts_a2,     sizeof(secp256k1_ge_consts_a2));
        secp256k1_num_set_bin(&ret->b1,     secp256k1_ge_consts_b1,     sizeof(secp256k1_ge_consts_b1));
        secp256k1_num_copy(&ret->half_order, &ret->order);
        secp256k1_num_shift(&ret->half_order, 1);
        secp256k1_fe_set_b32(&ret->beta, secp256k1_ge_consts_beta);
        secp256k1_fe_t g_x, g_y;
        secp256k1_fe_set_b32(&g_x, secp256k1_ge_consts_g_x);
        secp256k1_fe_set_b32(&g_y, secp256k1_ge_consts_g_y);
        secp256k1_ge_set_xy(&ret->g, &g_x, &g_y);
        secp256k1_ge_consts = ret;
    }
}

void static secp256k1_ge_stop(void) {
    if (secp256k1_ge_consts != NULL) {
        secp256k1_ge_consts_t *c = (secp256k1_ge_consts_t*)secp256k1_ge_consts;
        secp256k1_num_free(&c->order);
        secp256k1_num_free(&c->half_order);
        secp256k1_num_free(&c->lambda);
        secp256k1_num_free(&c->a1b2);
        secp256k1_num_free(&c->a2);
        secp256k1_num_free(&c->b1);
        free((void*)c);
        secp256k1_ge_consts = NULL;
    }
}

#endif
