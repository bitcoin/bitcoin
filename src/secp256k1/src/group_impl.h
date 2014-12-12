/**********************************************************************
 * Copyright (c) 2013, 2014 Pieter Wuille                             *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef _SECP256K1_GROUP_IMPL_H_
#define _SECP256K1_GROUP_IMPL_H_

#include <string.h>

#include "num.h"
#include "field.h"
#include "group.h"

static void secp256k1_ge_set_infinity(secp256k1_ge_t *r) {
    r->infinity = 1;
}

static void secp256k1_ge_set_xy(secp256k1_ge_t *r, const secp256k1_fe_t *x, const secp256k1_fe_t *y) {
    r->infinity = 0;
    r->x = *x;
    r->y = *y;
}

static int secp256k1_ge_is_infinity(const secp256k1_ge_t *a) {
    return a->infinity;
}

static void secp256k1_ge_neg(secp256k1_ge_t *r, const secp256k1_ge_t *a) {
    *r = *a;
    secp256k1_fe_normalize(&r->y);
    secp256k1_fe_negate(&r->y, &r->y, 1);
}

static void secp256k1_ge_neg_var(secp256k1_ge_t *r, const secp256k1_ge_t *a) {
    *r = *a;
    secp256k1_fe_normalize_var(&r->y);
    secp256k1_fe_negate(&r->y, &r->y, 1);
}

static void secp256k1_ge_get_hex(char *r, int *rlen, const secp256k1_ge_t *a) {
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

static void secp256k1_ge_set_gej(secp256k1_ge_t *r, secp256k1_gej_t *a) {
    r->infinity = a->infinity;
    secp256k1_fe_inv(&a->z, &a->z);
    secp256k1_fe_t z2; secp256k1_fe_sqr(&z2, &a->z);
    secp256k1_fe_t z3; secp256k1_fe_mul(&z3, &a->z, &z2);
    secp256k1_fe_mul(&a->x, &a->x, &z2);
    secp256k1_fe_mul(&a->y, &a->y, &z3);
    secp256k1_fe_set_int(&a->z, 1);
    r->x = a->x;
    r->y = a->y;
}

static void secp256k1_ge_set_gej_var(secp256k1_ge_t *r, secp256k1_gej_t *a) {
    r->infinity = a->infinity;
    if (a->infinity) {
        return;
    }
    secp256k1_fe_inv_var(&a->z, &a->z);
    secp256k1_fe_t z2; secp256k1_fe_sqr(&z2, &a->z);
    secp256k1_fe_t z3; secp256k1_fe_mul(&z3, &a->z, &z2);
    secp256k1_fe_mul(&a->x, &a->x, &z2);
    secp256k1_fe_mul(&a->y, &a->y, &z3);
    secp256k1_fe_set_int(&a->z, 1);
    r->x = a->x;
    r->y = a->y;
}

static void secp256k1_ge_set_all_gej_var(size_t len, secp256k1_ge_t r[len], const secp256k1_gej_t a[len]) {
    size_t count = 0;
    secp256k1_fe_t *az = checked_malloc(sizeof(secp256k1_fe_t) * len);
    for (size_t i=0; i<len; i++) {
        if (!a[i].infinity) {
            az[count++] = a[i].z;
        }
    }

    secp256k1_fe_t *azi = checked_malloc(sizeof(secp256k1_fe_t) * count);
    secp256k1_fe_inv_all_var(count, azi, az);
    free(az);

    count = 0;
    for (size_t i=0; i<len; i++) {
        r[i].infinity = a[i].infinity;
        if (!a[i].infinity) {
            secp256k1_fe_t *zi = &azi[count++];
            secp256k1_fe_t zi2; secp256k1_fe_sqr(&zi2, zi);
            secp256k1_fe_t zi3; secp256k1_fe_mul(&zi3, &zi2, zi);
            secp256k1_fe_mul(&r[i].x, &a[i].x, &zi2);
            secp256k1_fe_mul(&r[i].y, &a[i].y, &zi3);
        }
    }
    free(azi);
}

static void secp256k1_gej_set_infinity(secp256k1_gej_t *r) {
    r->infinity = 1;
    secp256k1_fe_set_int(&r->x, 0);
    secp256k1_fe_set_int(&r->y, 0);
    secp256k1_fe_set_int(&r->z, 0);
}

static void secp256k1_gej_set_xy(secp256k1_gej_t *r, const secp256k1_fe_t *x, const secp256k1_fe_t *y) {
    r->infinity = 0;
    r->x = *x;
    r->y = *y;
    secp256k1_fe_set_int(&r->z, 1);
}

static void secp256k1_gej_clear(secp256k1_gej_t *r) {
    r->infinity = 0;
    secp256k1_fe_clear(&r->x);
    secp256k1_fe_clear(&r->y);
    secp256k1_fe_clear(&r->z);
}

static void secp256k1_ge_clear(secp256k1_ge_t *r) {
    r->infinity = 0;
    secp256k1_fe_clear(&r->x);
    secp256k1_fe_clear(&r->y);
}

static int secp256k1_ge_set_xo_var(secp256k1_ge_t *r, const secp256k1_fe_t *x, int odd) {
    r->x = *x;
    secp256k1_fe_t x2; secp256k1_fe_sqr(&x2, x);
    secp256k1_fe_t x3; secp256k1_fe_mul(&x3, x, &x2);
    r->infinity = 0;
    secp256k1_fe_t c; secp256k1_fe_set_int(&c, 7);
    secp256k1_fe_add(&c, &x3);
    if (!secp256k1_fe_sqrt_var(&r->y, &c))
        return 0;
    secp256k1_fe_normalize_var(&r->y);
    if (secp256k1_fe_is_odd(&r->y) != odd)
        secp256k1_fe_negate(&r->y, &r->y, 1);
    return 1;
}

static void secp256k1_gej_set_ge(secp256k1_gej_t *r, const secp256k1_ge_t *a) {
   r->infinity = a->infinity;
   r->x = a->x;
   r->y = a->y;
   secp256k1_fe_set_int(&r->z, 1);
}

static void secp256k1_gej_get_x_var(secp256k1_fe_t *r, const secp256k1_gej_t *a) {
    secp256k1_fe_t zi2; secp256k1_fe_inv_var(&zi2, &a->z); secp256k1_fe_sqr(&zi2, &zi2);
    secp256k1_fe_mul(r, &a->x, &zi2);
}

static void secp256k1_gej_neg_var(secp256k1_gej_t *r, const secp256k1_gej_t *a) {
    r->infinity = a->infinity;
    r->x = a->x;
    r->y = a->y;
    r->z = a->z;
    secp256k1_fe_normalize_var(&r->y);
    secp256k1_fe_negate(&r->y, &r->y, 1);
}

static int secp256k1_gej_is_infinity(const secp256k1_gej_t *a) {
    return a->infinity;
}

static int secp256k1_gej_is_valid_var(const secp256k1_gej_t *a) {
    if (a->infinity)
        return 0;
    /** y^2 = x^3 + 7
     *  (Y/Z^3)^2 = (X/Z^2)^3 + 7
     *  Y^2 / Z^6 = X^3 / Z^6 + 7
     *  Y^2 = X^3 + 7*Z^6
     */
    secp256k1_fe_t y2; secp256k1_fe_sqr(&y2, &a->y);
    secp256k1_fe_t x3; secp256k1_fe_sqr(&x3, &a->x); secp256k1_fe_mul(&x3, &x3, &a->x);
    secp256k1_fe_t z2; secp256k1_fe_sqr(&z2, &a->z);
    secp256k1_fe_t z6; secp256k1_fe_sqr(&z6, &z2); secp256k1_fe_mul(&z6, &z6, &z2);
    secp256k1_fe_mul_int(&z6, 7);
    secp256k1_fe_add(&x3, &z6);
    secp256k1_fe_normalize_var(&y2);
    secp256k1_fe_normalize_var(&x3);
    return secp256k1_fe_equal(&y2, &x3);
}

static int secp256k1_ge_is_valid_var(const secp256k1_ge_t *a) {
    if (a->infinity)
        return 0;
    /* y^2 = x^3 + 7 */
    secp256k1_fe_t y2; secp256k1_fe_sqr(&y2, &a->y);
    secp256k1_fe_t x3; secp256k1_fe_sqr(&x3, &a->x); secp256k1_fe_mul(&x3, &x3, &a->x);
    secp256k1_fe_t c; secp256k1_fe_set_int(&c, 7);
    secp256k1_fe_add(&x3, &c);
    secp256k1_fe_normalize_var(&y2);
    secp256k1_fe_normalize_var(&x3);
    return secp256k1_fe_equal(&y2, &x3);
}

static void secp256k1_gej_double_var(secp256k1_gej_t *r, const secp256k1_gej_t *a) {
    // For secp256k1, 2Q is infinity if and only if Q is infinity. This is because if 2Q = infinity,
    // Q must equal -Q, or that Q.y == -(Q.y), or Q.y is 0. For a point on y^2 = x^3 + 7 to have
    // y=0, x^3 must be -7 mod p. However, -7 has no cube root mod p.
    r->infinity = a->infinity;
    if (r->infinity) {
        return;
    }

    secp256k1_fe_t t1,t2,t3,t4;
    secp256k1_fe_mul(&r->z, &a->z, &a->y);
    secp256k1_fe_mul_int(&r->z, 2);       /* Z' = 2*Y*Z (2) */
    secp256k1_fe_sqr(&t1, &a->x);
    secp256k1_fe_mul_int(&t1, 3);         /* T1 = 3*X^2 (3) */
    secp256k1_fe_sqr(&t2, &t1);           /* T2 = 9*X^4 (1) */
    secp256k1_fe_sqr(&t3, &a->y);
    secp256k1_fe_mul_int(&t3, 2);         /* T3 = 2*Y^2 (2) */
    secp256k1_fe_sqr(&t4, &t3);
    secp256k1_fe_mul_int(&t4, 2);         /* T4 = 8*Y^4 (2) */
    secp256k1_fe_mul(&t3, &t3, &a->x);    /* T3 = 2*X*Y^2 (1) */
    r->x = t3;
    secp256k1_fe_mul_int(&r->x, 4);       /* X' = 8*X*Y^2 (4) */
    secp256k1_fe_negate(&r->x, &r->x, 4); /* X' = -8*X*Y^2 (5) */
    secp256k1_fe_add(&r->x, &t2);         /* X' = 9*X^4 - 8*X*Y^2 (6) */
    secp256k1_fe_negate(&t2, &t2, 1);     /* T2 = -9*X^4 (2) */
    secp256k1_fe_mul_int(&t3, 6);         /* T3 = 12*X*Y^2 (6) */
    secp256k1_fe_add(&t3, &t2);           /* T3 = 12*X*Y^2 - 9*X^4 (8) */
    secp256k1_fe_mul(&r->y, &t1, &t3);    /* Y' = 36*X^3*Y^2 - 27*X^6 (1) */
    secp256k1_fe_negate(&t2, &t4, 2);     /* T2 = -8*Y^4 (3) */
    secp256k1_fe_add(&r->y, &t2);         /* Y' = 36*X^3*Y^2 - 27*X^6 - 8*Y^4 (4) */
}

static void secp256k1_gej_add_var(secp256k1_gej_t *r, const secp256k1_gej_t *a, const secp256k1_gej_t *b) {
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
    secp256k1_fe_normalize_var(&u1);
    secp256k1_fe_normalize_var(&u2);
    if (secp256k1_fe_equal(&u1, &u2)) {
        secp256k1_fe_normalize_var(&s1);
        secp256k1_fe_normalize_var(&s2);
        if (secp256k1_fe_equal(&s1, &s2)) {
            secp256k1_gej_double_var(r, a);
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

static void secp256k1_gej_add_ge_var(secp256k1_gej_t *r, const secp256k1_gej_t *a, const secp256k1_ge_t *b) {
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
    secp256k1_fe_t u1 = a->x;
    secp256k1_fe_t u2; secp256k1_fe_mul(&u2, &b->x, &z12);
    secp256k1_fe_t s1 = a->y; secp256k1_fe_normalize_var(&s1);
    secp256k1_fe_t s2; secp256k1_fe_mul(&s2, &b->y, &z12); secp256k1_fe_mul(&s2, &s2, &a->z);
    secp256k1_fe_normalize_var(&u1);
    secp256k1_fe_normalize_var(&u2);
    if (secp256k1_fe_equal(&u1, &u2)) {
        secp256k1_fe_normalize_var(&s2);
        if (secp256k1_fe_equal(&s1, &s2)) {
            secp256k1_gej_double_var(r, a);
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

static void secp256k1_gej_add_ge(secp256k1_gej_t *r, const secp256k1_gej_t *a, const secp256k1_ge_t *b) {
    VERIFY_CHECK(!b->infinity);
    VERIFY_CHECK(a->infinity == 0 || a->infinity == 1);

    /** In:
     *    Eric Brier and Marc Joye, Weierstrass Elliptic Curves and Side-Channel Attacks.
     *    In D. Naccache and P. Paillier, Eds., Public Key Cryptography, vol. 2274 of Lecture Notes in Computer Science, pages 335-345. Springer-Verlag, 2002.
     *  we find as solution for a unified addition/doubling formula:
     *    lambda = ((x1 + x2)^2 - x1 * x2 + a) / (y1 + y2), with a = 0 for secp256k1's curve equation.
     *    x3 = lambda^2 - (x1 + x2)
     *    2*y3 = lambda * (x1 + x2 - 2 * x3) - (y1 + y2).
     *
     *  Substituting x_i = Xi / Zi^2 and yi = Yi / Zi^3, for i=1,2,3, gives:
     *    U1 = X1*Z2^2, U2 = X2*Z1^2
     *    S1 = Y1*Z2^3, S2 = Y2*Z1^3
     *    Z = Z1*Z2
     *    T = U1+U2
     *    M = S1+S2
     *    Q = T*M^2
     *    R = T^2-U1*U2
     *    X3 = 4*(R^2-Q)
     *    Y3 = 4*(R*(3*Q-2*R^2)-M^4)
     *    Z3 = 2*M*Z
     *  (Note that the paper uses xi = Xi / Zi and yi = Yi / Zi instead.)
     */

    secp256k1_fe_t zz; secp256k1_fe_sqr(&zz, &a->z);                /* z = Z1^2 */
    secp256k1_fe_t u1 = a->x; secp256k1_fe_normalize(&u1);          /* u1 = U1 = X1*Z2^2 (1) */
    secp256k1_fe_t u2; secp256k1_fe_mul(&u2, &b->x, &zz);           /* u2 = U2 = X2*Z1^2 (1) */
    secp256k1_fe_t s1 = a->y; secp256k1_fe_normalize(&s1);          /* s1 = S1 = Y1*Z2^3 (1) */
    secp256k1_fe_t s2; secp256k1_fe_mul(&s2, &b->y, &zz);           /* s2 = Y2*Z2^2 (1) */
    secp256k1_fe_mul(&s2, &s2, &a->z);                              /* s2 = S2 = Y2*Z1^3 (1) */
    secp256k1_fe_t z = a->z;                                        /* z = Z = Z1*Z2 (8) */
    secp256k1_fe_t t = u1; secp256k1_fe_add(&t, &u2);               /* t = T = U1+U2 (2) */
    secp256k1_fe_t m = s1; secp256k1_fe_add(&m, &s2);               /* m = M = S1+S2 (2) */
    secp256k1_fe_t n; secp256k1_fe_sqr(&n, &m);                     /* n = M^2 (1) */
    secp256k1_fe_t q; secp256k1_fe_mul(&q, &n, &t);                 /* q = Q = T*M^2 (1) */
    secp256k1_fe_sqr(&n, &n);                                       /* n = M^4 (1) */
    secp256k1_fe_t rr; secp256k1_fe_sqr(&rr, &t);                   /* rr = T^2 (1) */
    secp256k1_fe_mul(&t, &u1, &u2); secp256k1_fe_negate(&t, &t, 1); /* t = -U1*U2 (2) */
    secp256k1_fe_add(&rr, &t);                                      /* rr = R = T^2-U1*U2 (3) */
    secp256k1_fe_sqr(&t, &rr);                                      /* t = R^2 (1) */
    secp256k1_fe_mul(&r->z, &m, &z);                                /* r->z = M*Z (1) */
    secp256k1_fe_normalize(&r->z);
    int infinity = secp256k1_fe_is_zero(&r->z) * (1 - a->infinity);
    secp256k1_fe_mul_int(&r->z, 2 * (1 - a->infinity)); /* r->z = Z3 = 2*M*Z (2) */
    r->x = t;                                           /* r->x = R^2 (1) */
    secp256k1_fe_negate(&q, &q, 1);                     /* q = -Q (2) */
    secp256k1_fe_add(&r->x, &q);                        /* r->x = R^2-Q (3) */
    secp256k1_fe_normalize(&r->x);
    secp256k1_fe_mul_int(&q, 3);                        /* q = -3*Q (6) */
    secp256k1_fe_mul_int(&t, 2);                        /* t = 2*R^2 (2) */
    secp256k1_fe_add(&t, &q);                           /* t = 2*R^2-3*Q (8) */
    secp256k1_fe_mul(&t, &t, &rr);                      /* t = R*(2*R^2-3*Q) (1) */
    secp256k1_fe_add(&t, &n);                           /* t = R*(2*R^2-3*Q)+M^4 (2) */
    secp256k1_fe_negate(&r->y, &t, 2);                  /* r->y = R*(3*Q-2*R^2)-M^4 (3) */
    secp256k1_fe_normalize(&r->y);
    secp256k1_fe_mul_int(&r->x, 4 * (1 - a->infinity)); /* r->x = X3 = 4*(R^2-Q) */
    secp256k1_fe_mul_int(&r->y, 4 * (1 - a->infinity)); /* r->y = Y3 = 4*R*(3*Q-2*R^2)-4*M^4 (4) */

    /** In case a->infinity == 1, the above code results in r->x, r->y, and r->z all equal to 0.
     *  Add b->x to x, b->y to y, and 1 to z in that case.
     */
    t = b->x; secp256k1_fe_mul_int(&t, a->infinity);
    secp256k1_fe_add(&r->x, &t);
    t = b->y; secp256k1_fe_mul_int(&t, a->infinity);
    secp256k1_fe_add(&r->y, &t);
    secp256k1_fe_set_int(&t, a->infinity);
    secp256k1_fe_add(&r->z, &t);
    r->infinity = infinity;
}



static void secp256k1_gej_get_hex(char *r, int *rlen, const secp256k1_gej_t *a) {
    secp256k1_gej_t c = *a;
    secp256k1_ge_t t; secp256k1_ge_set_gej(&t, &c);
    secp256k1_ge_get_hex(r, rlen, &t);
}

#ifdef USE_ENDOMORPHISM
static void secp256k1_gej_mul_lambda(secp256k1_gej_t *r, const secp256k1_gej_t *a) {
    const secp256k1_fe_t *beta = &secp256k1_ge_consts->beta;
    *r = *a;
    secp256k1_fe_mul(&r->x, &r->x, beta);
}
#endif

static void secp256k1_ge_start(void) {
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
#ifdef USE_ENDOMORPHISM
    /* properties of secp256k1's efficiently computable endomorphism */
    static const unsigned char secp256k1_ge_consts_beta[] = {
        0x7a,0xe9,0x6a,0x2b,0x65,0x7c,0x07,0x10,
        0x6e,0x64,0x47,0x9e,0xac,0x34,0x34,0xe9,
        0x9c,0xf0,0x49,0x75,0x12,0xf5,0x89,0x95,
        0xc1,0x39,0x6c,0x28,0x71,0x95,0x01,0xee
    };
#endif
    if (secp256k1_ge_consts == NULL) {
        secp256k1_ge_consts_t *ret = (secp256k1_ge_consts_t*)checked_malloc(sizeof(secp256k1_ge_consts_t));
#ifdef USE_ENDOMORPHISM
        VERIFY_CHECK(secp256k1_fe_set_b32(&ret->beta, secp256k1_ge_consts_beta));
#endif
        secp256k1_fe_t g_x, g_y;
        VERIFY_CHECK(secp256k1_fe_set_b32(&g_x, secp256k1_ge_consts_g_x));
        VERIFY_CHECK(secp256k1_fe_set_b32(&g_y, secp256k1_ge_consts_g_y));
        secp256k1_ge_set_xy(&ret->g, &g_x, &g_y);
        secp256k1_ge_consts = ret;
    }
}

static void secp256k1_ge_stop(void) {
    if (secp256k1_ge_consts != NULL) {
        secp256k1_ge_consts_t *c = (secp256k1_ge_consts_t*)secp256k1_ge_consts;
        free((void*)c);
        secp256k1_ge_consts = NULL;
    }
}

#endif
