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

static int secp256k1_ecmult_const_xonly(secp256k1_fe* r, const secp256k1_fe *n, const secp256k1_fe *d, const secp256k1_scalar *q, int bits, int known_on_curve) {

    /* This algorithm is a generalization of Peter Dettman's technique for
     * avoiding the square root in a random-basepoint x-only multiplication
     * on a Weierstrass curve:
     * https://mailarchive.ietf.org/arch/msg/cfrg/7DyYY6gg32wDgHAhgSb6XxMDlJA/
     *
     *
     * === Background: the effective affine technique ===
     *
     * Let phi_u be the isomorphism that maps (x, y) on secp256k1 curve y^2 = x^3 + 7 to
     * x' = u^2*x, y' = u^3*y on curve y'^2 = x'^3 + u^6*7. This new curve has the same order as
     * the original (it is isomorphic), but moreover, has the same addition/doubling formulas, as
     * the curve b=7 coefficient does not appear in those formulas (or at least does not appear in
     * the formulas implemented in this codebase, both affine and Jacobian). See also Example 9.5.2
     * in https://www.math.auckland.ac.nz/~sgal018/crypto-book/ch9.pdf.
     *
     * This means any linear combination of secp256k1 points can be computed by applying phi_u
     * (with non-zero u) on all input points (including the generator, if used), computing the
     * linear combination on the isomorphic curve (using the same group laws), and then applying
     * phi_u^{-1} to get back to secp256k1.
     *
     * Switching to Jacobian coordinates, note that phi_u applied to (X, Y, Z) is simply
     * (X, Y, Z/u). Thus, if we want to compute (X1, Y1, Z) + (X2, Y2, Z), with identical Z
     * coordinates, we can use phi_Z to transform it to (X1, Y1, 1) + (X2, Y2, 1) on an isomorphic
     * curve where the affine addition formula can be used instead.
     * If (X3, Y3, Z3) = (X1, Y1) + (X2, Y2) on that curve, then our answer on secp256k1 is
     * (X3, Y3, Z3*Z).
     *
     * This is the effective affine technique: if we have a linear combination of group elements
     * to compute, and all those group elements have the same Z coordinate, we can simply pretend
     * that all those Z coordinates are 1, perform the computation that way, and then multiply the
     * original Z coordinate back in.
     *
     * The technique works on any a=0 short Weierstrass curve. It is possible to generalize it to
     * other curves too, but there the isomorphic curves will have different 'a' coefficients,
     * which typically does affect the group laws.
     *
     *
     * === Avoiding the square root for x-only point multiplication ===
     *
     * In this function, we want to compute the X coordinate of q*(n/d, y), for
     * y = sqrt((n/d)^3 + 7). Its negation would also be a valid Y coordinate, but by convention
     * we pick whatever sqrt returns (which we assume to be a deterministic function).
     *
     * Let g = y^2*d^3 = n^3 + 7*d^3. This also means y = sqrt(g/d^3).
     * Further let v = sqrt(d*g), which must exist as d*g = y^2*d^4 = (y*d^2)^2.
     *
     * The input point (n/d, y) also has Jacobian coordinates:
     *
     *     (n/d, y, 1)
     *   = (n/d * v^2, y * v^3, v)
     *   = (n/d * d*g, y * sqrt(d^3*g^3), v)
     *   = (n/d * d*g, sqrt(y^2 * d^3*g^3), v)
     *   = (n*g, sqrt(g/d^3 * d^3*g^3), v)
     *   = (n*g, sqrt(g^4), v)
     *   = (n*g, g^2, v)
     *
     * It is easy to verify that both (n*g, g^2, v) and its negation (n*g, -g^2, v) have affine X
     * coordinate n/d, and this holds even when the square root function doesn't have a
     * determinstic sign. We choose the (n*g, g^2, v) version.
     *
     * Now switch to the effective affine curve using phi_v, where the input point has coordinates
     * (n*g, g^2). Compute (X, Y, Z) = q * (n*g, g^2) there.
     *
     * Back on secp256k1, that means q * (n*g, g^2, v) = (X, Y, v*Z). This last point has affine X
     * coordinate X / (v^2*Z^2) = X / (d*g*Z^2). Determining the affine Y coordinate would involve
     * a square root, but as long as we only care about the resulting X coordinate, no square root
     * is needed anywhere in this computation.
     */

    secp256k1_fe g, i;
    secp256k1_ge p;
    secp256k1_gej rj;

    /* Compute g = (n^3 + B*d^3). */
    secp256k1_fe_sqr(&g, n);
    secp256k1_fe_mul(&g, &g, n);
    if (d) {
        secp256k1_fe b;
#ifdef VERIFY
        VERIFY_CHECK(!secp256k1_fe_normalizes_to_zero(d));
#endif
        secp256k1_fe_sqr(&b, d);
        VERIFY_CHECK(SECP256K1_B <= 8); /* magnitude of b will be <= 8 after the next call */
        secp256k1_fe_mul_int(&b, SECP256K1_B);
        secp256k1_fe_mul(&b, &b, d);
        secp256k1_fe_add(&g, &b);
        if (!known_on_curve) {
            /* We need to determine whether (n/d)^3 + 7 is square.
             *
             *     is_square((n/d)^3 + 7)
             * <=> is_square(((n/d)^3 + 7) * d^4)
             * <=> is_square((n^3 + 7*d^3) * d)
             * <=> is_square(g * d)
             */
            secp256k1_fe c;
            secp256k1_fe_mul(&c, &g, d);
            if (!secp256k1_fe_is_square_var(&c)) return 0;
        }
    } else {
        secp256k1_fe_add_int(&g, SECP256K1_B);
        if (!known_on_curve) {
            /* g at this point equals x^3 + 7. Test if it is square. */
            if (!secp256k1_fe_is_square_var(&g)) return 0;
        }
    }

    /* Compute base point P = (n*g, g^2), the effective affine version of (n*g, g^2, v), which has
     * corresponding affine X coordinate n/d. */
    secp256k1_fe_mul(&p.x, &g, n);
    secp256k1_fe_sqr(&p.y, &g);
    p.infinity = 0;

    /* Perform x-only EC multiplication of P with q. */
#ifdef VERIFY
    VERIFY_CHECK(!secp256k1_scalar_is_zero(q));
#endif
    secp256k1_ecmult_const(&rj, &p, q, bits);
#ifdef VERIFY
    VERIFY_CHECK(!secp256k1_gej_is_infinity(&rj));
#endif

    /* The resulting (X, Y, Z) point on the effective-affine isomorphic curve corresponds to
     * (X, Y, Z*v) on the secp256k1 curve. The affine version of that has X coordinate
     * (X / (Z^2*d*g)). */
    secp256k1_fe_sqr(&i, &rj.z);
    secp256k1_fe_mul(&i, &i, &g);
    if (d) secp256k1_fe_mul(&i, &i, d);
    secp256k1_fe_inv(&i, &i);
    secp256k1_fe_mul(r, &rj.x, &i);

    return 1;
}

#endif /* SECP256K1_ECMULT_CONST_IMPL_H */
