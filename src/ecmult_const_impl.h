/**********************************************************************
 * Copyright (c) 2015 Pieter Wuille, Andrew Poelstra                  *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef _SECP256K1_ECMULT_CONST_IMPL_
#define _SECP256K1_ECMULT_CONST_IMPL_

#include "scalar.h"
#include "group.h"
#include "ecmult_const.h"
#include "ecmult_impl.h"

#define WNAF_BITS 256
#define WNAF_SIZE(w) ((WNAF_BITS + (w) - 1) / (w))

/* This is like `ECMULT_TABLE_GET_GE` but is constant time */
#define ECMULT_CONST_TABLE_GET_GE(r,pre,n,w) do { \
    int m; \
    int abs_n = (n) * (((n) > 0) * 2 - 1); \
    secp256k1_fe_t neg_y; \
    VERIFY_CHECK(((n) & 1) == 1); \
    VERIFY_CHECK((n) >= -((1 << ((w)-1)) - 1)); \
    VERIFY_CHECK((n) <=  ((1 << ((w)-1)) - 1)); \
    for (m = 1; m < (1 << ((w) - 1)); m += 2) { \
        /* This loop is used to avoid secret data in array indices. See
         * the comment in ecmult_gen_impl.h for rationale. */ \
        secp256k1_fe_cmov(&(r)->x, &(pre)[(m - 1) / 2].x, m == abs_n); \
        secp256k1_fe_cmov(&(r)->y, &(pre)[(m - 1) / 2].y, m == abs_n); \
    } \
    (r)->infinity = 0; \
    secp256k1_fe_normalize_weak(&(r)->x); \
    secp256k1_fe_normalize_weak(&(r)->y); \
    secp256k1_fe_negate(&neg_y, &(r)->y, 1); \
    secp256k1_fe_cmov(&(r)->y, &neg_y, (n) != abs_n); \
} while(0)


/** Convert a number to WNAF notation. The number becomes represented by sum(2^{wi} * wnaf[i], i=0..return_val)
 *  with the following guarantees:
 *  - each wnaf[i] an odd integer between -(1 << w) and (1 << w)
 *  - each wnaf[i] is nonzero
 *  - the number of words set is returned; this is always (WNAF_BITS + w - 1) / w
 *
 *  Adapted from `The Width-w NAF Method Provides Small Memory and Fast Elliptic Scalar
 *  Multiplications Secure against Side Channel Attacks`, Okeya and Tagaki. M. Joye (Ed.)
 *  CT-RSA 2003, LNCS 2612, pp. 328-443, 2003. Springer-Verlagy Berlin Heidelberg 2003
 *
 *  Numbers reference steps of `Algorithm SPA-resistant Width-w NAF with Odd Scalar` on pp. 335
 */
static void secp256k1_wnaf_const(int *wnaf, const secp256k1_scalar_t *a, int w) {
    secp256k1_scalar_t s = *a;
    /* Negate to force oddness */
    int is_even = secp256k1_scalar_is_even(&s);
    int global_sign = secp256k1_scalar_cond_negate(&s, is_even);

    int word = 0;
    /* 1 2 3 */
    int u_last = secp256k1_scalar_shr_int(&s, w);
    int u;
    /* 4 */
    while (word * w < WNAF_BITS) {
        int sign;
        int even;

        /* 4.1 4.4 */
        u = secp256k1_scalar_shr_int(&s, w);
        /* 4.2 */
        even = ((u & 1) == 0);
        sign = 2 * (u_last > 0) - 1;
        u += sign * even;
        u_last -= sign * even * (1 << w);

        /* 4.3, adapted for global sign change */
        wnaf[word++] = u_last * global_sign;

        u_last = u;
    }
    wnaf[word] = u * global_sign;

    VERIFY_CHECK(secp256k1_scalar_is_zero(&s));
    VERIFY_CHECK(word == WNAF_SIZE(w));
}


static void secp256k1_ecmult_const(secp256k1_gej_t *r, const secp256k1_ge_t *a, const secp256k1_scalar_t *scalar) {
    secp256k1_ge_t pre_a[ECMULT_TABLE_SIZE(WINDOW_A)];
    secp256k1_ge_t tmpa;
    secp256k1_fe_t Z;

    int wnaf[1 + WNAF_SIZE(WINDOW_A - 1)];

    int i;
    int is_zero = secp256k1_scalar_is_zero(scalar);
    secp256k1_scalar_t sc = *scalar;
    /* the wNAF ladder cannot handle zero, so bump this to one .. we will
     * correct the result after the fact */
    sc.d[0] += is_zero;

    /* build wnaf representation for q. */
    secp256k1_wnaf_const(wnaf, &sc, WINDOW_A - 1);

    /* Calculate odd multiples of a.
     * All multiples are brought to the same Z 'denominator', which is stored
     * in Z. Due to secp256k1' isomorphism we can do all operations pretending
     * that the Z coordinate was 1, use affine addition formulae, and correct
     * the Z coordinate of the result once at the end.
     */
    secp256k1_gej_set_ge(r, a);
    secp256k1_ecmult_odd_multiples_table_globalz_windowa(pre_a, &Z, r);

    /* first loop iteration (separated out so we can directly set r, rather
     * than having it start at infinity, get doubled several times, then have
     * its new value added to it) */
    i = wnaf[WNAF_SIZE(WINDOW_A - 1)];
    VERIFY_CHECK(i != 0);
    ECMULT_CONST_TABLE_GET_GE(&tmpa, pre_a, i, WINDOW_A);
    secp256k1_gej_set_ge(r, &tmpa);
    /* remaining loop iterations */
    for (i = WNAF_SIZE(WINDOW_A - 1) - 1; i >= 0; i--) {
        int n;
        int j;
        for (j = 0; j < WINDOW_A - 1; ++j) {
            secp256k1_gej_double_nonzero(r, r, NULL);
        }
        n = wnaf[i];
        VERIFY_CHECK(n != 0);
        ECMULT_CONST_TABLE_GET_GE(&tmpa, pre_a, n, WINDOW_A);
        secp256k1_gej_add_ge(r, r, &tmpa);
    }

    secp256k1_fe_mul(&r->z, &r->z, &Z);

    /* correct for zero */
    r->infinity |= is_zero;
}

#endif
