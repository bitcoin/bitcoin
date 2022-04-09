/***********************************************************************
 * Copyright (c) 2015 Pieter Wuille, Andrew Poelstra                   *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_ECMULT_CONST_IMPL_H
#define SECP256K1_ECMULT_CONST_IMPL_H

#include "scalar.h"
#include "group.h"
#include "ecmult_const.h"
#include "ecmult_impl.h"

/** Fill a table 'pre' with precomputed odd multiples of a.
 *
 *  The resulting point set is brought to a single constant Z denominator, stores the X and Y
 *  coordinates as ge_storage points in pre, and stores the global Z in globalz.
 *  It only operates on tables sized for WINDOW_A wnaf multiples.
 */
static void secp256k1_ecmult_odd_multiples_table_globalz_windowa(secp256k1_ge *pre, secp256k1_fe *globalz, const secp256k1_gej *a) {
    secp256k1_fe zr[ECMULT_TABLE_SIZE(WINDOW_A)];

    secp256k1_ecmult_odd_multiples_table(ECMULT_TABLE_SIZE(WINDOW_A), pre, zr, globalz, a);
    secp256k1_ge_table_set_globalz(ECMULT_TABLE_SIZE(WINDOW_A), pre, zr);
}

/* This is like `ECMULT_TABLE_GET_GE` but is constant time */
#define ECMULT_CONST_TABLE_GET_GE(r,pre,n,w) do { \
    int m = 0; \
    /* Extract the sign-bit for a constant time absolute-value. */ \
    int mask = (n) >> (sizeof(n) * CHAR_BIT - 1); \
    int abs_n = ((n) + mask) ^ mask; \
    int idx_n = abs_n >> 1; \
    secp256k1_fe neg_y; \
    VERIFY_CHECK(((n) & 1) == 1); \
    VERIFY_CHECK((n) >= -((1 << ((w)-1)) - 1)); \
    VERIFY_CHECK((n) <=  ((1 << ((w)-1)) - 1)); \
    VERIFY_SETUP(secp256k1_fe_clear(&(r)->x)); \
    VERIFY_SETUP(secp256k1_fe_clear(&(r)->y)); \
    /* Unconditionally set r->x = (pre)[m].x. r->y = (pre)[m].y. because it's either the correct one \
     * or will get replaced in the later iterations, this is needed to make sure `r` is initialized. */ \
    (r)->x = (pre)[m].x; \
    (r)->y = (pre)[m].y; \
    for (m = 1; m < ECMULT_TABLE_SIZE(w); m++) { \
        /* This loop is used to avoid secret data in array indices. See
         * the comment in ecmult_gen_impl.h for rationale. */ \
        secp256k1_fe_cmov(&(r)->x, &(pre)[m].x, m == idx_n); \
        secp256k1_fe_cmov(&(r)->y, &(pre)[m].y, m == idx_n); \
    } \
    (r)->infinity = 0; \
    secp256k1_fe_negate(&neg_y, &(r)->y, 1); \
    secp256k1_fe_cmov(&(r)->y, &neg_y, (n) != abs_n); \
} while(0)

/** Convert a number to WNAF notation.
 *  The number becomes represented by sum(2^{wi} * wnaf[i], i=0..WNAF_SIZE(w)+1) - return_val.
 *  It has the following guarantees:
 *  - each wnaf[i] an odd integer between -(1 << w) and (1 << w)
 *  - each wnaf[i] is nonzero
 *  - the number of words set is always WNAF_SIZE(w) + 1
 *
 *  Adapted from `The Width-w NAF Method Provides Small Memory and Fast Elliptic Scalar
 *  Multiplications Secure against Side Channel Attacks`, Okeya and Tagaki. M. Joye (Ed.)
 *  CT-RSA 2003, LNCS 2612, pp. 328-443, 2003. Springer-Verlag Berlin Heidelberg 2003
 *
 *  Numbers reference steps of `Algorithm SPA-resistant Width-w NAF with Odd Scalar` on pp. 335
 */
static int secp256k1_wnaf_const(int *wnaf, const secp256k1_scalar *scalar, int w, int size) {
    int global_sign;
    int skew;
    int word = 0;

    /* 1 2 3 */
    int u_last;
    int u;

    int flip;
    secp256k1_scalar s = *scalar;

    VERIFY_CHECK(w > 0);
    VERIFY_CHECK(size > 0);

    /* Note that we cannot handle even numbers by negating them to be odd, as is
     * done in other implementations, since if our scalars were specified to have
     * width < 256 for performance reasons, their negations would have width 256
     * and we'd lose any performance benefit. Instead, we use a variation of a
     * technique from Section 4.2 of the Okeya/Tagaki paper, which is to add 1 to the
     * number we are encoding when it is even, returning a skew value indicating
     * this, and having the caller compensate after doing the multiplication.
     *
     * In fact, we _do_ want to negate numbers to minimize their bit-lengths (and in
     * particular, to ensure that the outputs from the endomorphism-split fit into
     * 128 bits). If we negate, the parity of our number flips, affecting whether
     * we want to add to the scalar to ensure that it's odd. */
    flip = secp256k1_scalar_is_high(&s);
    skew = flip ^ secp256k1_scalar_is_even(&s);
    secp256k1_scalar_cadd_bit(&s, 0, skew);
    global_sign = secp256k1_scalar_cond_negate(&s, flip);

    /* 4 */
    u_last = secp256k1_scalar_shr_int(&s, w);
    do {
        int even;

        /* 4.1 4.4 */
        u = secp256k1_scalar_shr_int(&s, w);
        /* 4.2 */
        even = ((u & 1) == 0);
        /* In contrast to the original algorithm, u_last is always > 0 and
         * therefore we do not need to check its sign. In particular, it's easy
         * to see that u_last is never < 0 because u is never < 0. Moreover,
         * u_last is never = 0 because u is never even after a loop
         * iteration. The same holds analogously for the initial value of
         * u_last (in the first loop iteration). */
        VERIFY_CHECK(u_last > 0);
        VERIFY_CHECK((u_last & 1) == 1);
        u += even;
        u_last -= even * (1 << w);

        /* 4.3, adapted for global sign change */
        wnaf[word++] = u_last * global_sign;

        u_last = u;
    } while (word * w < size);
    wnaf[word] = u * global_sign;

    VERIFY_CHECK(secp256k1_scalar_is_zero(&s));
    VERIFY_CHECK(word == WNAF_SIZE_BITS(size, w));
    return skew;
}

static void secp256k1_ecmult_const(secp256k1_gej *r, const secp256k1_ge *a, const secp256k1_scalar *scalar, int size) {
    secp256k1_ge pre_a[ECMULT_TABLE_SIZE(WINDOW_A)];
    secp256k1_ge tmpa;
    secp256k1_fe Z;

    int skew_1;
    secp256k1_ge pre_a_lam[ECMULT_TABLE_SIZE(WINDOW_A)];
    int wnaf_lam[1 + WNAF_SIZE(WINDOW_A - 1)];
    int skew_lam;
    secp256k1_scalar q_1, q_lam;
    int wnaf_1[1 + WNAF_SIZE(WINDOW_A - 1)];

    int i;

    /* build wnaf representation for q. */
    int rsize = size;
    if (size > 128) {
        rsize = 128;
        /* split q into q_1 and q_lam (where q = q_1 + q_lam*lambda, and q_1 and q_lam are ~128 bit) */
        secp256k1_scalar_split_lambda(&q_1, &q_lam, scalar);
        skew_1   = secp256k1_wnaf_const(wnaf_1,   &q_1,   WINDOW_A - 1, 128);
        skew_lam = secp256k1_wnaf_const(wnaf_lam, &q_lam, WINDOW_A - 1, 128);
    } else
    {
        skew_1   = secp256k1_wnaf_const(wnaf_1, scalar, WINDOW_A - 1, size);
        skew_lam = 0;
    }

    /* Calculate odd multiples of a.
     * All multiples are brought to the same Z 'denominator', which is stored
     * in Z. Due to secp256k1' isomorphism we can do all operations pretending
     * that the Z coordinate was 1, use affine addition formulae, and correct
     * the Z coordinate of the result once at the end.
     */
    VERIFY_CHECK(!a->infinity);
    secp256k1_gej_set_ge(r, a);
    secp256k1_ecmult_odd_multiples_table_globalz_windowa(pre_a, &Z, r);
    for (i = 0; i < ECMULT_TABLE_SIZE(WINDOW_A); i++) {
        secp256k1_fe_normalize_weak(&pre_a[i].y);
    }
    if (size > 128) {
        for (i = 0; i < ECMULT_TABLE_SIZE(WINDOW_A); i++) {
            secp256k1_ge_mul_lambda(&pre_a_lam[i], &pre_a[i]);
        }

    }

    /* first loop iteration (separated out so we can directly set r, rather
     * than having it start at infinity, get doubled several times, then have
     * its new value added to it) */
    i = wnaf_1[WNAF_SIZE_BITS(rsize, WINDOW_A - 1)];
    VERIFY_CHECK(i != 0);
    ECMULT_CONST_TABLE_GET_GE(&tmpa, pre_a, i, WINDOW_A);
    secp256k1_gej_set_ge(r, &tmpa);
    if (size > 128) {
        i = wnaf_lam[WNAF_SIZE_BITS(rsize, WINDOW_A - 1)];
        VERIFY_CHECK(i != 0);
        ECMULT_CONST_TABLE_GET_GE(&tmpa, pre_a_lam, i, WINDOW_A);
        secp256k1_gej_add_ge(r, r, &tmpa);
    }
    /* remaining loop iterations */
    for (i = WNAF_SIZE_BITS(rsize, WINDOW_A - 1) - 1; i >= 0; i--) {
        int n;
        int j;
        for (j = 0; j < WINDOW_A - 1; ++j) {
            secp256k1_gej_double(r, r);
        }

        n = wnaf_1[i];
        ECMULT_CONST_TABLE_GET_GE(&tmpa, pre_a, n, WINDOW_A);
        VERIFY_CHECK(n != 0);
        secp256k1_gej_add_ge(r, r, &tmpa);
        if (size > 128) {
            n = wnaf_lam[i];
            ECMULT_CONST_TABLE_GET_GE(&tmpa, pre_a_lam, n, WINDOW_A);
            VERIFY_CHECK(n != 0);
            secp256k1_gej_add_ge(r, r, &tmpa);
        }
    }

    {
        /* Correct for wNAF skew */
        secp256k1_gej tmpj;

        secp256k1_ge_neg(&tmpa, &pre_a[0]);
        secp256k1_gej_add_ge(&tmpj, r, &tmpa);
        secp256k1_gej_cmov(r, &tmpj, skew_1);

        if (size > 128) {
            secp256k1_ge_neg(&tmpa, &pre_a_lam[0]);
            secp256k1_gej_add_ge(&tmpj, r, &tmpa);
            secp256k1_gej_cmov(r, &tmpj, skew_lam);
        }
    }

    secp256k1_fe_mul(&r->z, &r->z, &Z);
}

#endif /* SECP256K1_ECMULT_CONST_IMPL_H */
