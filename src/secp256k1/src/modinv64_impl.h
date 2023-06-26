/***********************************************************************
 * Copyright (c) 2020 Peter Dettman                                    *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef SECP256K1_MODINV64_IMPL_H
#define SECP256K1_MODINV64_IMPL_H

#include "int128.h"
#include "modinv64.h"

/* This file implements modular inversion based on the paper "Fast constant-time gcd computation and
 * modular inversion" by Daniel J. Bernstein and Bo-Yin Yang.
 *
 * For an explanation of the algorithm, see doc/safegcd_implementation.md. This file contains an
 * implementation for N=62, using 62-bit signed limbs represented as int64_t.
 */

/* Data type for transition matrices (see section 3 of explanation).
 *
 * t = [ u  v ]
 *     [ q  r ]
 */
typedef struct {
    int64_t u, v, q, r;
} secp256k1_modinv64_trans2x2;

#ifdef VERIFY
/* Helper function to compute the absolute value of an int64_t.
 * (we don't use abs/labs/llabs as it depends on the int sizes). */
static int64_t secp256k1_modinv64_abs(int64_t v) {
    VERIFY_CHECK(v > INT64_MIN);
    if (v < 0) return -v;
    return v;
}

static const secp256k1_modinv64_signed62 SECP256K1_SIGNED62_ONE = {{1}};

/* Compute a*factor and put it in r. All but the top limb in r will be in range [0,2^62). */
static void secp256k1_modinv64_mul_62(secp256k1_modinv64_signed62 *r, const secp256k1_modinv64_signed62 *a, int alen, int64_t factor) {
    const uint64_t M62 = UINT64_MAX >> 2;
    secp256k1_int128 c, d;
    int i;
    secp256k1_i128_from_i64(&c, 0);
    for (i = 0; i < 4; ++i) {
        if (i < alen) secp256k1_i128_accum_mul(&c, a->v[i], factor);
        r->v[i] = secp256k1_i128_to_u64(&c) & M62; secp256k1_i128_rshift(&c, 62);
    }
    if (4 < alen) secp256k1_i128_accum_mul(&c, a->v[4], factor);
    secp256k1_i128_from_i64(&d, secp256k1_i128_to_i64(&c));
    VERIFY_CHECK(secp256k1_i128_eq_var(&c, &d));
    r->v[4] = secp256k1_i128_to_i64(&c);
}

/* Return -1 for a<b*factor, 0 for a==b*factor, 1 for a>b*factor. A has alen limbs; b has 5. */
static int secp256k1_modinv64_mul_cmp_62(const secp256k1_modinv64_signed62 *a, int alen, const secp256k1_modinv64_signed62 *b, int64_t factor) {
    int i;
    secp256k1_modinv64_signed62 am, bm;
    secp256k1_modinv64_mul_62(&am, a, alen, 1); /* Normalize all but the top limb of a. */
    secp256k1_modinv64_mul_62(&bm, b, 5, factor);
    for (i = 0; i < 4; ++i) {
        /* Verify that all but the top limb of a and b are normalized. */
        VERIFY_CHECK(am.v[i] >> 62 == 0);
        VERIFY_CHECK(bm.v[i] >> 62 == 0);
    }
    for (i = 4; i >= 0; --i) {
        if (am.v[i] < bm.v[i]) return -1;
        if (am.v[i] > bm.v[i]) return 1;
    }
    return 0;
}

/* Check if the determinant of t is equal to 1 << n. If abs, check if |det t| == 1 << n. */
static int secp256k1_modinv64_det_check_pow2(const secp256k1_modinv64_trans2x2 *t, unsigned int n, int abs) {
    secp256k1_int128 a;
    secp256k1_i128_det(&a, t->u, t->v, t->q, t->r);
    if (secp256k1_i128_check_pow2(&a, n, 1)) return 1;
    if (abs && secp256k1_i128_check_pow2(&a, n, -1)) return 1;
    return 0;
}
#endif

/* Take as input a signed62 number in range (-2*modulus,modulus), and add a multiple of the modulus
 * to it to bring it to range [0,modulus). If sign < 0, the input will also be negated in the
 * process. The input must have limbs in range (-2^62,2^62). The output will have limbs in range
 * [0,2^62). */
static void secp256k1_modinv64_normalize_62(secp256k1_modinv64_signed62 *r, int64_t sign, const secp256k1_modinv64_modinfo *modinfo) {
    const int64_t M62 = (int64_t)(UINT64_MAX >> 2);
    int64_t r0 = r->v[0], r1 = r->v[1], r2 = r->v[2], r3 = r->v[3], r4 = r->v[4];
    volatile int64_t cond_add, cond_negate;

#ifdef VERIFY
    /* Verify that all limbs are in range (-2^62,2^62). */
    int i;
    for (i = 0; i < 5; ++i) {
        VERIFY_CHECK(r->v[i] >= -M62);
        VERIFY_CHECK(r->v[i] <= M62);
    }
    VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(r, 5, &modinfo->modulus, -2) > 0); /* r > -2*modulus */
    VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(r, 5, &modinfo->modulus, 1) < 0); /* r < modulus */
#endif

    /* In a first step, add the modulus if the input is negative, and then negate if requested.
     * This brings r from range (-2*modulus,modulus) to range (-modulus,modulus). As all input
     * limbs are in range (-2^62,2^62), this cannot overflow an int64_t. Note that the right
     * shifts below are signed sign-extending shifts (see assumptions.h for tests that that is
     * indeed the behavior of the right shift operator). */
    cond_add = r4 >> 63;
    r0 += modinfo->modulus.v[0] & cond_add;
    r1 += modinfo->modulus.v[1] & cond_add;
    r2 += modinfo->modulus.v[2] & cond_add;
    r3 += modinfo->modulus.v[3] & cond_add;
    r4 += modinfo->modulus.v[4] & cond_add;
    cond_negate = sign >> 63;
    r0 = (r0 ^ cond_negate) - cond_negate;
    r1 = (r1 ^ cond_negate) - cond_negate;
    r2 = (r2 ^ cond_negate) - cond_negate;
    r3 = (r3 ^ cond_negate) - cond_negate;
    r4 = (r4 ^ cond_negate) - cond_negate;
    /* Propagate the top bits, to bring limbs back to range (-2^62,2^62). */
    r1 += r0 >> 62; r0 &= M62;
    r2 += r1 >> 62; r1 &= M62;
    r3 += r2 >> 62; r2 &= M62;
    r4 += r3 >> 62; r3 &= M62;

    /* In a second step add the modulus again if the result is still negative, bringing
     * r to range [0,modulus). */
    cond_add = r4 >> 63;
    r0 += modinfo->modulus.v[0] & cond_add;
    r1 += modinfo->modulus.v[1] & cond_add;
    r2 += modinfo->modulus.v[2] & cond_add;
    r3 += modinfo->modulus.v[3] & cond_add;
    r4 += modinfo->modulus.v[4] & cond_add;
    /* And propagate again. */
    r1 += r0 >> 62; r0 &= M62;
    r2 += r1 >> 62; r1 &= M62;
    r3 += r2 >> 62; r2 &= M62;
    r4 += r3 >> 62; r3 &= M62;

    r->v[0] = r0;
    r->v[1] = r1;
    r->v[2] = r2;
    r->v[3] = r3;
    r->v[4] = r4;

#ifdef VERIFY
    VERIFY_CHECK(r0 >> 62 == 0);
    VERIFY_CHECK(r1 >> 62 == 0);
    VERIFY_CHECK(r2 >> 62 == 0);
    VERIFY_CHECK(r3 >> 62 == 0);
    VERIFY_CHECK(r4 >> 62 == 0);
    VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(r, 5, &modinfo->modulus, 0) >= 0); /* r >= 0 */
    VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(r, 5, &modinfo->modulus, 1) < 0); /* r < modulus */
#endif
}

/* Compute the transition matrix and eta for 59 divsteps (where zeta=-(delta+1/2)).
 * Note that the transformation matrix is scaled by 2^62 and not 2^59.
 *
 * Input:  zeta: initial zeta
 *         f0:   bottom limb of initial f
 *         g0:   bottom limb of initial g
 * Output: t: transition matrix
 * Return: final zeta
 *
 * Implements the divsteps_n_matrix function from the explanation.
 */
static int64_t secp256k1_modinv64_divsteps_59(int64_t zeta, uint64_t f0, uint64_t g0, secp256k1_modinv64_trans2x2 *t) {
    /* u,v,q,r are the elements of the transformation matrix being built up,
     * starting with the identity matrix times 8 (because the caller expects
     * a result scaled by 2^62). Semantically they are signed integers
     * in range [-2^62,2^62], but here represented as unsigned mod 2^64. This
     * permits left shifting (which is UB for negative numbers). The range
     * being inside [-2^63,2^63) means that casting to signed works correctly.
     */
    uint64_t u = 8, v = 0, q = 0, r = 8;
    volatile uint64_t c1, c2;
    uint64_t mask1, mask2, f = f0, g = g0, x, y, z;
    int i;

    for (i = 3; i < 62; ++i) {
        VERIFY_CHECK((f & 1) == 1); /* f must always be odd */
        VERIFY_CHECK((u * f0 + v * g0) == f << i);
        VERIFY_CHECK((q * f0 + r * g0) == g << i);
        /* Compute conditional masks for (zeta < 0) and for (g & 1). */
        c1 = zeta >> 63;
        mask1 = c1;
        c2 = g & 1;
        mask2 = -c2;
        /* Compute x,y,z, conditionally negated versions of f,u,v. */
        x = (f ^ mask1) - mask1;
        y = (u ^ mask1) - mask1;
        z = (v ^ mask1) - mask1;
        /* Conditionally add x,y,z to g,q,r. */
        g += x & mask2;
        q += y & mask2;
        r += z & mask2;
        /* In what follows, c1 is a condition mask for (zeta < 0) and (g & 1). */
        mask1 &= mask2;
        /* Conditionally change zeta into -zeta-2 or zeta-1. */
        zeta = (zeta ^ mask1) - 1;
        /* Conditionally add g,q,r to f,u,v. */
        f += g & mask1;
        u += q & mask1;
        v += r & mask1;
        /* Shifts */
        g >>= 1;
        u <<= 1;
        v <<= 1;
        /* Bounds on zeta that follow from the bounds on iteration count (max 10*59 divsteps). */
        VERIFY_CHECK(zeta >= -591 && zeta <= 591);
    }
    /* Return data in t and return value. */
    t->u = (int64_t)u;
    t->v = (int64_t)v;
    t->q = (int64_t)q;
    t->r = (int64_t)r;
#ifdef VERIFY
    /* The determinant of t must be a power of two. This guarantees that multiplication with t
     * does not change the gcd of f and g, apart from adding a power-of-2 factor to it (which
     * will be divided out again). As each divstep's individual matrix has determinant 2, the
     * aggregate of 59 of them will have determinant 2^59. Multiplying with the initial
     * 8*identity (which has determinant 2^6) means the overall outputs has determinant
     * 2^65. */
    VERIFY_CHECK(secp256k1_modinv64_det_check_pow2(t, 65, 0));
#endif
    return zeta;
}

/* Compute the transition matrix and eta for 62 divsteps (variable time, eta=-delta).
 *
 * Input:  eta: initial eta
 *         f0:  bottom limb of initial f
 *         g0:  bottom limb of initial g
 * Output: t: transition matrix
 * Return: final eta
 *
 * Implements the divsteps_n_matrix_var function from the explanation.
 */
static int64_t secp256k1_modinv64_divsteps_62_var(int64_t eta, uint64_t f0, uint64_t g0, secp256k1_modinv64_trans2x2 *t) {
    /* Transformation matrix; see comments in secp256k1_modinv64_divsteps_62. */
    uint64_t u = 1, v = 0, q = 0, r = 1;
    uint64_t f = f0, g = g0, m;
    uint32_t w;
    int i = 62, limit, zeros;

    for (;;) {
        /* Use a sentinel bit to count zeros only up to i. */
        zeros = secp256k1_ctz64_var(g | (UINT64_MAX << i));
        /* Perform zeros divsteps at once; they all just divide g by two. */
        g >>= zeros;
        u <<= zeros;
        v <<= zeros;
        eta -= zeros;
        i -= zeros;
        /* We're done once we've done 62 divsteps. */
        if (i == 0) break;
        VERIFY_CHECK((f & 1) == 1);
        VERIFY_CHECK((g & 1) == 1);
        VERIFY_CHECK((u * f0 + v * g0) == f << (62 - i));
        VERIFY_CHECK((q * f0 + r * g0) == g << (62 - i));
        /* Bounds on eta that follow from the bounds on iteration count (max 12*62 divsteps). */
        VERIFY_CHECK(eta >= -745 && eta <= 745);
        /* If eta is negative, negate it and replace f,g with g,-f. */
        if (eta < 0) {
            uint64_t tmp;
            eta = -eta;
            tmp = f; f = g; g = -tmp;
            tmp = u; u = q; q = -tmp;
            tmp = v; v = r; r = -tmp;
            /* Use a formula to cancel out up to 6 bits of g. Also, no more than i can be cancelled
             * out (as we'd be done before that point), and no more than eta+1 can be done as its
             * sign will flip again once that happens. */
            limit = ((int)eta + 1) > i ? i : ((int)eta + 1);
            VERIFY_CHECK(limit > 0 && limit <= 62);
            /* m is a mask for the bottom min(limit, 6) bits. */
            m = (UINT64_MAX >> (64 - limit)) & 63U;
            /* Find what multiple of f must be added to g to cancel its bottom min(limit, 6)
             * bits. */
            w = (f * g * (f * f - 2)) & m;
        } else {
            /* In this branch, use a simpler formula that only lets us cancel up to 4 bits of g, as
             * eta tends to be smaller here. */
            limit = ((int)eta + 1) > i ? i : ((int)eta + 1);
            VERIFY_CHECK(limit > 0 && limit <= 62);
            /* m is a mask for the bottom min(limit, 4) bits. */
            m = (UINT64_MAX >> (64 - limit)) & 15U;
            /* Find what multiple of f must be added to g to cancel its bottom min(limit, 4)
             * bits. */
            w = f + (((f + 1) & 4) << 1);
            w = (-w * g) & m;
        }
        g += f * w;
        q += u * w;
        r += v * w;
        VERIFY_CHECK((g & m) == 0);
    }
    /* Return data in t and return value. */
    t->u = (int64_t)u;
    t->v = (int64_t)v;
    t->q = (int64_t)q;
    t->r = (int64_t)r;
#ifdef VERIFY
    /* The determinant of t must be a power of two. This guarantees that multiplication with t
     * does not change the gcd of f and g, apart from adding a power-of-2 factor to it (which
     * will be divided out again). As each divstep's individual matrix has determinant 2, the
     * aggregate of 62 of them will have determinant 2^62. */
    VERIFY_CHECK(secp256k1_modinv64_det_check_pow2(t, 62, 0));
#endif
    return eta;
}

/* Compute the transition matrix and eta for 62 posdivsteps (variable time, eta=-delta), and keeps track
 * of the Jacobi symbol along the way. f0 and g0 must be f and g mod 2^64 rather than 2^62, because
 * Jacobi tracking requires knowing (f mod 8) rather than just (f mod 2).
 *
 * Input:        eta: initial eta
 *               f0:  bottom limb of initial f
 *               g0:  bottom limb of initial g
 * Output:       t: transition matrix
 * Input/Output: (*jacp & 1) is bitflipped if and only if the Jacobi symbol of (f | g) changes sign
 *               by applying the returned transformation matrix to it. The other bits of *jacp may
 *               change, but are meaningless.
 * Return:       final eta
 */
static int64_t secp256k1_modinv64_posdivsteps_62_var(int64_t eta, uint64_t f0, uint64_t g0, secp256k1_modinv64_trans2x2 *t, int *jacp) {
    /* Transformation matrix; see comments in secp256k1_modinv64_divsteps_62. */
    uint64_t u = 1, v = 0, q = 0, r = 1;
    uint64_t f = f0, g = g0, m;
    uint32_t w;
    int i = 62, limit, zeros;
    int jac = *jacp;

    for (;;) {
        /* Use a sentinel bit to count zeros only up to i. */
        zeros = secp256k1_ctz64_var(g | (UINT64_MAX << i));
        /* Perform zeros divsteps at once; they all just divide g by two. */
        g >>= zeros;
        u <<= zeros;
        v <<= zeros;
        eta -= zeros;
        i -= zeros;
        /* Update the bottom bit of jac: when dividing g by an odd power of 2,
         * if (f mod 8) is 3 or 5, the Jacobi symbol changes sign. */
        jac ^= (zeros & ((f >> 1) ^ (f >> 2)));
        /* We're done once we've done 62 posdivsteps. */
        if (i == 0) break;
        VERIFY_CHECK((f & 1) == 1);
        VERIFY_CHECK((g & 1) == 1);
        VERIFY_CHECK((u * f0 + v * g0) == f << (62 - i));
        VERIFY_CHECK((q * f0 + r * g0) == g << (62 - i));
        /* If eta is negative, negate it and replace f,g with g,f. */
        if (eta < 0) {
            uint64_t tmp;
            eta = -eta;
            tmp = f; f = g; g = tmp;
            tmp = u; u = q; q = tmp;
            tmp = v; v = r; r = tmp;
            /* Update bottom bit of jac: when swapping f and g, the Jacobi symbol changes sign
             * if both f and g are 3 mod 4. */
            jac ^= ((f & g) >> 1);
            /* Use a formula to cancel out up to 6 bits of g. Also, no more than i can be cancelled
             * out (as we'd be done before that point), and no more than eta+1 can be done as its
             * sign will flip again once that happens. */
            limit = ((int)eta + 1) > i ? i : ((int)eta + 1);
            VERIFY_CHECK(limit > 0 && limit <= 62);
            /* m is a mask for the bottom min(limit, 6) bits. */
            m = (UINT64_MAX >> (64 - limit)) & 63U;
            /* Find what multiple of f must be added to g to cancel its bottom min(limit, 6)
             * bits. */
            w = (f * g * (f * f - 2)) & m;
        } else {
            /* In this branch, use a simpler formula that only lets us cancel up to 4 bits of g, as
             * eta tends to be smaller here. */
            limit = ((int)eta + 1) > i ? i : ((int)eta + 1);
            VERIFY_CHECK(limit > 0 && limit <= 62);
            /* m is a mask for the bottom min(limit, 4) bits. */
            m = (UINT64_MAX >> (64 - limit)) & 15U;
            /* Find what multiple of f must be added to g to cancel its bottom min(limit, 4)
             * bits. */
            w = f + (((f + 1) & 4) << 1);
            w = (-w * g) & m;
        }
        g += f * w;
        q += u * w;
        r += v * w;
        VERIFY_CHECK((g & m) == 0);
    }
    /* Return data in t and return value. */
    t->u = (int64_t)u;
    t->v = (int64_t)v;
    t->q = (int64_t)q;
    t->r = (int64_t)r;
#ifdef VERIFY
    /* The determinant of t must be a power of two. This guarantees that multiplication with t
     * does not change the gcd of f and g, apart from adding a power-of-2 factor to it (which
     * will be divided out again). As each divstep's individual matrix has determinant 2 or -2,
     * the aggregate of 62 of them will have determinant 2^62 or -2^62. */
    VERIFY_CHECK(secp256k1_modinv64_det_check_pow2(t, 62, 1));
#endif
    *jacp = jac;
    return eta;
}

/* Compute (t/2^62) * [d, e] mod modulus, where t is a transition matrix scaled by 2^62.
 *
 * On input and output, d and e are in range (-2*modulus,modulus). All output limbs will be in range
 * (-2^62,2^62).
 *
 * This implements the update_de function from the explanation.
 */
static void secp256k1_modinv64_update_de_62(secp256k1_modinv64_signed62 *d, secp256k1_modinv64_signed62 *e, const secp256k1_modinv64_trans2x2 *t, const secp256k1_modinv64_modinfo* modinfo) {
    const uint64_t M62 = UINT64_MAX >> 2;
    const int64_t d0 = d->v[0], d1 = d->v[1], d2 = d->v[2], d3 = d->v[3], d4 = d->v[4];
    const int64_t e0 = e->v[0], e1 = e->v[1], e2 = e->v[2], e3 = e->v[3], e4 = e->v[4];
    const int64_t u = t->u, v = t->v, q = t->q, r = t->r;
    int64_t md, me, sd, se;
    secp256k1_int128 cd, ce;
#ifdef VERIFY
    VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(d, 5, &modinfo->modulus, -2) > 0); /* d > -2*modulus */
    VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(d, 5, &modinfo->modulus, 1) < 0);  /* d <    modulus */
    VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(e, 5, &modinfo->modulus, -2) > 0); /* e > -2*modulus */
    VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(e, 5, &modinfo->modulus, 1) < 0);  /* e <    modulus */
    VERIFY_CHECK(secp256k1_modinv64_abs(u) <= (((int64_t)1 << 62) - secp256k1_modinv64_abs(v))); /* |u|+|v| <= 2^62 */
    VERIFY_CHECK(secp256k1_modinv64_abs(q) <= (((int64_t)1 << 62) - secp256k1_modinv64_abs(r))); /* |q|+|r| <= 2^62 */
#endif
    /* [md,me] start as zero; plus [u,q] if d is negative; plus [v,r] if e is negative. */
    sd = d4 >> 63;
    se = e4 >> 63;
    md = (u & sd) + (v & se);
    me = (q & sd) + (r & se);
    /* Begin computing t*[d,e]. */
    secp256k1_i128_mul(&cd, u, d0);
    secp256k1_i128_accum_mul(&cd, v, e0);
    secp256k1_i128_mul(&ce, q, d0);
    secp256k1_i128_accum_mul(&ce, r, e0);
    /* Correct md,me so that t*[d,e]+modulus*[md,me] has 62 zero bottom bits. */
    md -= (modinfo->modulus_inv62 * secp256k1_i128_to_u64(&cd) + md) & M62;
    me -= (modinfo->modulus_inv62 * secp256k1_i128_to_u64(&ce) + me) & M62;
    /* Update the beginning of computation for t*[d,e]+modulus*[md,me] now md,me are known. */
    secp256k1_i128_accum_mul(&cd, modinfo->modulus.v[0], md);
    secp256k1_i128_accum_mul(&ce, modinfo->modulus.v[0], me);
    /* Verify that the low 62 bits of the computation are indeed zero, and then throw them away. */
    VERIFY_CHECK((secp256k1_i128_to_u64(&cd) & M62) == 0); secp256k1_i128_rshift(&cd, 62);
    VERIFY_CHECK((secp256k1_i128_to_u64(&ce) & M62) == 0); secp256k1_i128_rshift(&ce, 62);
    /* Compute limb 1 of t*[d,e]+modulus*[md,me], and store it as output limb 0 (= down shift). */
    secp256k1_i128_accum_mul(&cd, u, d1);
    secp256k1_i128_accum_mul(&cd, v, e1);
    secp256k1_i128_accum_mul(&ce, q, d1);
    secp256k1_i128_accum_mul(&ce, r, e1);
    if (modinfo->modulus.v[1]) { /* Optimize for the case where limb of modulus is zero. */
        secp256k1_i128_accum_mul(&cd, modinfo->modulus.v[1], md);
        secp256k1_i128_accum_mul(&ce, modinfo->modulus.v[1], me);
    }
    d->v[0] = secp256k1_i128_to_u64(&cd) & M62; secp256k1_i128_rshift(&cd, 62);
    e->v[0] = secp256k1_i128_to_u64(&ce) & M62; secp256k1_i128_rshift(&ce, 62);
    /* Compute limb 2 of t*[d,e]+modulus*[md,me], and store it as output limb 1. */
    secp256k1_i128_accum_mul(&cd, u, d2);
    secp256k1_i128_accum_mul(&cd, v, e2);
    secp256k1_i128_accum_mul(&ce, q, d2);
    secp256k1_i128_accum_mul(&ce, r, e2);
    if (modinfo->modulus.v[2]) { /* Optimize for the case where limb of modulus is zero. */
        secp256k1_i128_accum_mul(&cd, modinfo->modulus.v[2], md);
        secp256k1_i128_accum_mul(&ce, modinfo->modulus.v[2], me);
    }
    d->v[1] = secp256k1_i128_to_u64(&cd) & M62; secp256k1_i128_rshift(&cd, 62);
    e->v[1] = secp256k1_i128_to_u64(&ce) & M62; secp256k1_i128_rshift(&ce, 62);
    /* Compute limb 3 of t*[d,e]+modulus*[md,me], and store it as output limb 2. */
    secp256k1_i128_accum_mul(&cd, u, d3);
    secp256k1_i128_accum_mul(&cd, v, e3);
    secp256k1_i128_accum_mul(&ce, q, d3);
    secp256k1_i128_accum_mul(&ce, r, e3);
    if (modinfo->modulus.v[3]) { /* Optimize for the case where limb of modulus is zero. */
        secp256k1_i128_accum_mul(&cd, modinfo->modulus.v[3], md);
        secp256k1_i128_accum_mul(&ce, modinfo->modulus.v[3], me);
    }
    d->v[2] = secp256k1_i128_to_u64(&cd) & M62; secp256k1_i128_rshift(&cd, 62);
    e->v[2] = secp256k1_i128_to_u64(&ce) & M62; secp256k1_i128_rshift(&ce, 62);
    /* Compute limb 4 of t*[d,e]+modulus*[md,me], and store it as output limb 3. */
    secp256k1_i128_accum_mul(&cd, u, d4);
    secp256k1_i128_accum_mul(&cd, v, e4);
    secp256k1_i128_accum_mul(&ce, q, d4);
    secp256k1_i128_accum_mul(&ce, r, e4);
    secp256k1_i128_accum_mul(&cd, modinfo->modulus.v[4], md);
    secp256k1_i128_accum_mul(&ce, modinfo->modulus.v[4], me);
    d->v[3] = secp256k1_i128_to_u64(&cd) & M62; secp256k1_i128_rshift(&cd, 62);
    e->v[3] = secp256k1_i128_to_u64(&ce) & M62; secp256k1_i128_rshift(&ce, 62);
    /* What remains is limb 5 of t*[d,e]+modulus*[md,me]; store it as output limb 4. */
    d->v[4] = secp256k1_i128_to_i64(&cd);
    e->v[4] = secp256k1_i128_to_i64(&ce);
#ifdef VERIFY
    VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(d, 5, &modinfo->modulus, -2) > 0); /* d > -2*modulus */
    VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(d, 5, &modinfo->modulus, 1) < 0);  /* d <    modulus */
    VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(e, 5, &modinfo->modulus, -2) > 0); /* e > -2*modulus */
    VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(e, 5, &modinfo->modulus, 1) < 0);  /* e <    modulus */
#endif
}

/* Compute (t/2^62) * [f, g], where t is a transition matrix scaled by 2^62.
 *
 * This implements the update_fg function from the explanation.
 */
static void secp256k1_modinv64_update_fg_62(secp256k1_modinv64_signed62 *f, secp256k1_modinv64_signed62 *g, const secp256k1_modinv64_trans2x2 *t) {
    const uint64_t M62 = UINT64_MAX >> 2;
    const int64_t f0 = f->v[0], f1 = f->v[1], f2 = f->v[2], f3 = f->v[3], f4 = f->v[4];
    const int64_t g0 = g->v[0], g1 = g->v[1], g2 = g->v[2], g3 = g->v[3], g4 = g->v[4];
    const int64_t u = t->u, v = t->v, q = t->q, r = t->r;
    secp256k1_int128 cf, cg;
    /* Start computing t*[f,g]. */
    secp256k1_i128_mul(&cf, u, f0);
    secp256k1_i128_accum_mul(&cf, v, g0);
    secp256k1_i128_mul(&cg, q, f0);
    secp256k1_i128_accum_mul(&cg, r, g0);
    /* Verify that the bottom 62 bits of the result are zero, and then throw them away. */
    VERIFY_CHECK((secp256k1_i128_to_u64(&cf) & M62) == 0); secp256k1_i128_rshift(&cf, 62);
    VERIFY_CHECK((secp256k1_i128_to_u64(&cg) & M62) == 0); secp256k1_i128_rshift(&cg, 62);
    /* Compute limb 1 of t*[f,g], and store it as output limb 0 (= down shift). */
    secp256k1_i128_accum_mul(&cf, u, f1);
    secp256k1_i128_accum_mul(&cf, v, g1);
    secp256k1_i128_accum_mul(&cg, q, f1);
    secp256k1_i128_accum_mul(&cg, r, g1);
    f->v[0] = secp256k1_i128_to_u64(&cf) & M62; secp256k1_i128_rshift(&cf, 62);
    g->v[0] = secp256k1_i128_to_u64(&cg) & M62; secp256k1_i128_rshift(&cg, 62);
    /* Compute limb 2 of t*[f,g], and store it as output limb 1. */
    secp256k1_i128_accum_mul(&cf, u, f2);
    secp256k1_i128_accum_mul(&cf, v, g2);
    secp256k1_i128_accum_mul(&cg, q, f2);
    secp256k1_i128_accum_mul(&cg, r, g2);
    f->v[1] = secp256k1_i128_to_u64(&cf) & M62; secp256k1_i128_rshift(&cf, 62);
    g->v[1] = secp256k1_i128_to_u64(&cg) & M62; secp256k1_i128_rshift(&cg, 62);
    /* Compute limb 3 of t*[f,g], and store it as output limb 2. */
    secp256k1_i128_accum_mul(&cf, u, f3);
    secp256k1_i128_accum_mul(&cf, v, g3);
    secp256k1_i128_accum_mul(&cg, q, f3);
    secp256k1_i128_accum_mul(&cg, r, g3);
    f->v[2] = secp256k1_i128_to_u64(&cf) & M62; secp256k1_i128_rshift(&cf, 62);
    g->v[2] = secp256k1_i128_to_u64(&cg) & M62; secp256k1_i128_rshift(&cg, 62);
    /* Compute limb 4 of t*[f,g], and store it as output limb 3. */
    secp256k1_i128_accum_mul(&cf, u, f4);
    secp256k1_i128_accum_mul(&cf, v, g4);
    secp256k1_i128_accum_mul(&cg, q, f4);
    secp256k1_i128_accum_mul(&cg, r, g4);
    f->v[3] = secp256k1_i128_to_u64(&cf) & M62; secp256k1_i128_rshift(&cf, 62);
    g->v[3] = secp256k1_i128_to_u64(&cg) & M62; secp256k1_i128_rshift(&cg, 62);
    /* What remains is limb 5 of t*[f,g]; store it as output limb 4. */
    f->v[4] = secp256k1_i128_to_i64(&cf);
    g->v[4] = secp256k1_i128_to_i64(&cg);
}

/* Compute (t/2^62) * [f, g], where t is a transition matrix for 62 divsteps.
 *
 * Version that operates on a variable number of limbs in f and g.
 *
 * This implements the update_fg function from the explanation.
 */
static void secp256k1_modinv64_update_fg_62_var(int len, secp256k1_modinv64_signed62 *f, secp256k1_modinv64_signed62 *g, const secp256k1_modinv64_trans2x2 *t) {
    const uint64_t M62 = UINT64_MAX >> 2;
    const int64_t u = t->u, v = t->v, q = t->q, r = t->r;
    int64_t fi, gi;
    secp256k1_int128 cf, cg;
    int i;
    VERIFY_CHECK(len > 0);
    /* Start computing t*[f,g]. */
    fi = f->v[0];
    gi = g->v[0];
    secp256k1_i128_mul(&cf, u, fi);
    secp256k1_i128_accum_mul(&cf, v, gi);
    secp256k1_i128_mul(&cg, q, fi);
    secp256k1_i128_accum_mul(&cg, r, gi);
    /* Verify that the bottom 62 bits of the result are zero, and then throw them away. */
    VERIFY_CHECK((secp256k1_i128_to_u64(&cf) & M62) == 0); secp256k1_i128_rshift(&cf, 62);
    VERIFY_CHECK((secp256k1_i128_to_u64(&cg) & M62) == 0); secp256k1_i128_rshift(&cg, 62);
    /* Now iteratively compute limb i=1..len of t*[f,g], and store them in output limb i-1 (shifting
     * down by 62 bits). */
    for (i = 1; i < len; ++i) {
        fi = f->v[i];
        gi = g->v[i];
        secp256k1_i128_accum_mul(&cf, u, fi);
        secp256k1_i128_accum_mul(&cf, v, gi);
        secp256k1_i128_accum_mul(&cg, q, fi);
        secp256k1_i128_accum_mul(&cg, r, gi);
        f->v[i - 1] = secp256k1_i128_to_u64(&cf) & M62; secp256k1_i128_rshift(&cf, 62);
        g->v[i - 1] = secp256k1_i128_to_u64(&cg) & M62; secp256k1_i128_rshift(&cg, 62);
    }
    /* What remains is limb (len) of t*[f,g]; store it as output limb (len-1). */
    f->v[len - 1] = secp256k1_i128_to_i64(&cf);
    g->v[len - 1] = secp256k1_i128_to_i64(&cg);
}

/* Compute the inverse of x modulo modinfo->modulus, and replace x with it (constant time in x). */
static void secp256k1_modinv64(secp256k1_modinv64_signed62 *x, const secp256k1_modinv64_modinfo *modinfo) {
    /* Start with d=0, e=1, f=modulus, g=x, zeta=-1. */
    secp256k1_modinv64_signed62 d = {{0, 0, 0, 0, 0}};
    secp256k1_modinv64_signed62 e = {{1, 0, 0, 0, 0}};
    secp256k1_modinv64_signed62 f = modinfo->modulus;
    secp256k1_modinv64_signed62 g = *x;
    int i;
    int64_t zeta = -1; /* zeta = -(delta+1/2); delta starts at 1/2. */

    /* Do 10 iterations of 59 divsteps each = 590 divsteps. This suffices for 256-bit inputs. */
    for (i = 0; i < 10; ++i) {
        /* Compute transition matrix and new zeta after 59 divsteps. */
        secp256k1_modinv64_trans2x2 t;
        zeta = secp256k1_modinv64_divsteps_59(zeta, f.v[0], g.v[0], &t);
        /* Update d,e using that transition matrix. */
        secp256k1_modinv64_update_de_62(&d, &e, &t, modinfo);
        /* Update f,g using that transition matrix. */
#ifdef VERIFY
        VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(&f, 5, &modinfo->modulus, -1) > 0); /* f > -modulus */
        VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(&f, 5, &modinfo->modulus, 1) <= 0); /* f <= modulus */
        VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(&g, 5, &modinfo->modulus, -1) > 0); /* g > -modulus */
        VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(&g, 5, &modinfo->modulus, 1) < 0);  /* g <  modulus */
#endif
        secp256k1_modinv64_update_fg_62(&f, &g, &t);
#ifdef VERIFY
        VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(&f, 5, &modinfo->modulus, -1) > 0); /* f > -modulus */
        VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(&f, 5, &modinfo->modulus, 1) <= 0); /* f <= modulus */
        VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(&g, 5, &modinfo->modulus, -1) > 0); /* g > -modulus */
        VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(&g, 5, &modinfo->modulus, 1) < 0);  /* g <  modulus */
#endif
    }

    /* At this point sufficient iterations have been performed that g must have reached 0
     * and (if g was not originally 0) f must now equal +/- GCD of the initial f, g
     * values i.e. +/- 1, and d now contains +/- the modular inverse. */
#ifdef VERIFY
    /* g == 0 */
    VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(&g, 5, &SECP256K1_SIGNED62_ONE, 0) == 0);
    /* |f| == 1, or (x == 0 and d == 0 and |f|=modulus) */
    VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(&f, 5, &SECP256K1_SIGNED62_ONE, -1) == 0 ||
                 secp256k1_modinv64_mul_cmp_62(&f, 5, &SECP256K1_SIGNED62_ONE, 1) == 0 ||
                 (secp256k1_modinv64_mul_cmp_62(x, 5, &SECP256K1_SIGNED62_ONE, 0) == 0 &&
                  secp256k1_modinv64_mul_cmp_62(&d, 5, &SECP256K1_SIGNED62_ONE, 0) == 0 &&
                  (secp256k1_modinv64_mul_cmp_62(&f, 5, &modinfo->modulus, 1) == 0 ||
                   secp256k1_modinv64_mul_cmp_62(&f, 5, &modinfo->modulus, -1) == 0)));
#endif

    /* Optionally negate d, normalize to [0,modulus), and return it. */
    secp256k1_modinv64_normalize_62(&d, f.v[4], modinfo);
    *x = d;
}

/* Compute the inverse of x modulo modinfo->modulus, and replace x with it (variable time). */
static void secp256k1_modinv64_var(secp256k1_modinv64_signed62 *x, const secp256k1_modinv64_modinfo *modinfo) {
    /* Start with d=0, e=1, f=modulus, g=x, eta=-1. */
    secp256k1_modinv64_signed62 d = {{0, 0, 0, 0, 0}};
    secp256k1_modinv64_signed62 e = {{1, 0, 0, 0, 0}};
    secp256k1_modinv64_signed62 f = modinfo->modulus;
    secp256k1_modinv64_signed62 g = *x;
#ifdef VERIFY
    int i = 0;
#endif
    int j, len = 5;
    int64_t eta = -1; /* eta = -delta; delta is initially 1 */
    int64_t cond, fn, gn;

    /* Do iterations of 62 divsteps each until g=0. */
    while (1) {
        /* Compute transition matrix and new eta after 62 divsteps. */
        secp256k1_modinv64_trans2x2 t;
        eta = secp256k1_modinv64_divsteps_62_var(eta, f.v[0], g.v[0], &t);
        /* Update d,e using that transition matrix. */
        secp256k1_modinv64_update_de_62(&d, &e, &t, modinfo);
        /* Update f,g using that transition matrix. */
#ifdef VERIFY
        VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(&f, len, &modinfo->modulus, -1) > 0); /* f > -modulus */
        VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(&f, len, &modinfo->modulus, 1) <= 0); /* f <= modulus */
        VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(&g, len, &modinfo->modulus, -1) > 0); /* g > -modulus */
        VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(&g, len, &modinfo->modulus, 1) < 0);  /* g <  modulus */
#endif
        secp256k1_modinv64_update_fg_62_var(len, &f, &g, &t);
        /* If the bottom limb of g is zero, there is a chance that g=0. */
        if (g.v[0] == 0) {
            cond = 0;
            /* Check if the other limbs are also 0. */
            for (j = 1; j < len; ++j) {
                cond |= g.v[j];
            }
            /* If so, we're done. */
            if (cond == 0) break;
        }

        /* Determine if len>1 and limb (len-1) of both f and g is 0 or -1. */
        fn = f.v[len - 1];
        gn = g.v[len - 1];
        cond = ((int64_t)len - 2) >> 63;
        cond |= fn ^ (fn >> 63);
        cond |= gn ^ (gn >> 63);
        /* If so, reduce length, propagating the sign of f and g's top limb into the one below. */
        if (cond == 0) {
            f.v[len - 2] |= (uint64_t)fn << 62;
            g.v[len - 2] |= (uint64_t)gn << 62;
            --len;
        }
#ifdef VERIFY
        VERIFY_CHECK(++i < 12); /* We should never need more than 12*62 = 744 divsteps */
        VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(&f, len, &modinfo->modulus, -1) > 0); /* f > -modulus */
        VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(&f, len, &modinfo->modulus, 1) <= 0); /* f <= modulus */
        VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(&g, len, &modinfo->modulus, -1) > 0); /* g > -modulus */
        VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(&g, len, &modinfo->modulus, 1) < 0);  /* g <  modulus */
#endif
    }

    /* At this point g is 0 and (if g was not originally 0) f must now equal +/- GCD of
     * the initial f, g values i.e. +/- 1, and d now contains +/- the modular inverse. */
#ifdef VERIFY
    /* g == 0 */
    VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(&g, len, &SECP256K1_SIGNED62_ONE, 0) == 0);
    /* |f| == 1, or (x == 0 and d == 0 and |f|=modulus) */
    VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(&f, len, &SECP256K1_SIGNED62_ONE, -1) == 0 ||
                 secp256k1_modinv64_mul_cmp_62(&f, len, &SECP256K1_SIGNED62_ONE, 1) == 0 ||
                 (secp256k1_modinv64_mul_cmp_62(x, 5, &SECP256K1_SIGNED62_ONE, 0) == 0 &&
                  secp256k1_modinv64_mul_cmp_62(&d, 5, &SECP256K1_SIGNED62_ONE, 0) == 0 &&
                  (secp256k1_modinv64_mul_cmp_62(&f, len, &modinfo->modulus, 1) == 0 ||
                   secp256k1_modinv64_mul_cmp_62(&f, len, &modinfo->modulus, -1) == 0)));
#endif

    /* Optionally negate d, normalize to [0,modulus), and return it. */
    secp256k1_modinv64_normalize_62(&d, f.v[len - 1], modinfo);
    *x = d;
}

/* Do up to 25 iterations of 62 posdivsteps (up to 1550 steps; more is extremely rare) each until f=1.
 * In VERIFY mode use a lower number of iterations (744, close to the median 756), so failure actually occurs. */
#ifdef VERIFY
#define JACOBI64_ITERATIONS 12
#else
#define JACOBI64_ITERATIONS 25
#endif

/* Compute the Jacobi symbol of x modulo modinfo->modulus (variable time). gcd(x,modulus) must be 1. */
static int secp256k1_jacobi64_maybe_var(const secp256k1_modinv64_signed62 *x, const secp256k1_modinv64_modinfo *modinfo) {
    /* Start with f=modulus, g=x, eta=-1. */
    secp256k1_modinv64_signed62 f = modinfo->modulus;
    secp256k1_modinv64_signed62 g = *x;
    int j, len = 5;
    int64_t eta = -1; /* eta = -delta; delta is initially 1 */
    int64_t cond, fn, gn;
    int jac = 0;
    int count;

    /* The input limbs must all be non-negative. */
    VERIFY_CHECK(g.v[0] >= 0 && g.v[1] >= 0 && g.v[2] >= 0 && g.v[3] >= 0 && g.v[4] >= 0);

    /* If x > 0, then if the loop below converges, it converges to f=g=gcd(x,modulus). Since we
     * require that gcd(x,modulus)=1 and modulus>=3, x cannot be 0. Thus, we must reach f=1 (or
     * time out). */
    VERIFY_CHECK((g.v[0] | g.v[1] | g.v[2] | g.v[3] | g.v[4]) != 0);

    for (count = 0; count < JACOBI64_ITERATIONS; ++count) {
        /* Compute transition matrix and new eta after 62 posdivsteps. */
        secp256k1_modinv64_trans2x2 t;
        eta = secp256k1_modinv64_posdivsteps_62_var(eta, f.v[0] | ((uint64_t)f.v[1] << 62), g.v[0] | ((uint64_t)g.v[1] << 62), &t, &jac);
        /* Update f,g using that transition matrix. */
#ifdef VERIFY
        VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(&f, len, &modinfo->modulus, 0) > 0); /* f > 0 */
        VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(&f, len, &modinfo->modulus, 1) <= 0); /* f <= modulus */
        VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(&g, len, &modinfo->modulus, 0) > 0); /* g > 0 */
        VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(&g, len, &modinfo->modulus, 1) < 0);  /* g < modulus */
#endif
        secp256k1_modinv64_update_fg_62_var(len, &f, &g, &t);
        /* If the bottom limb of f is 1, there is a chance that f=1. */
        if (f.v[0] == 1) {
            cond = 0;
            /* Check if the other limbs are also 0. */
            for (j = 1; j < len; ++j) {
                cond |= f.v[j];
            }
            /* If so, we're done. When f=1, the Jacobi symbol (g | f)=1. */
            if (cond == 0) return 1 - 2*(jac & 1);
        }

        /* Determine if len>1 and limb (len-1) of both f and g is 0. */
        fn = f.v[len - 1];
        gn = g.v[len - 1];
        cond = ((int64_t)len - 2) >> 63;
        cond |= fn;
        cond |= gn;
        /* If so, reduce length. */
        if (cond == 0) --len;
#ifdef VERIFY
        VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(&f, len, &modinfo->modulus, 0) > 0); /* f > 0 */
        VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(&f, len, &modinfo->modulus, 1) <= 0); /* f <= modulus */
        VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(&g, len, &modinfo->modulus, 0) > 0); /* g > 0 */
        VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(&g, len, &modinfo->modulus, 1) < 0);  /* g < modulus */
#endif
    }

    /* The loop failed to converge to f=g after 1550 iterations. Return 0, indicating unknown result. */
    return 0;
}

#endif /* SECP256K1_MODINV64_IMPL_H */
