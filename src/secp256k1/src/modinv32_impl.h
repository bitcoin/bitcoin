/***********************************************************************
 * Copyright (c) 2020 Peter Dettman                                    *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef SECP256K1_MODINV32_IMPL_H
#define SECP256K1_MODINV32_IMPL_H

#include "modinv32.h"

#include "util.h"

#include <stdlib.h>

/* This file implements modular inversion based on the paper "Fast constant-time gcd computation and
 * modular inversion" by Daniel J. Bernstein and Bo-Yin Yang.
 *
 * For an explanation of the algorithm, see doc/safegcd_implementation.md. This file contains an
 * implementation for N=30, using 30-bit signed limbs represented as int32_t.
 */

#ifdef VERIFY
static const secp256k1_modinv32_signed30 SECP256K1_SIGNED30_ONE = {{1}};

/* Compute a*factor and put it in r. All but the top limb in r will be in range [0,2^30). */
static void secp256k1_modinv32_mul_30(secp256k1_modinv32_signed30 *r, const secp256k1_modinv32_signed30 *a, int alen, int32_t factor) {
    const int32_t M30 = (int32_t)(UINT32_MAX >> 2);
    int64_t c = 0;
    int i;
    for (i = 0; i < 8; ++i) {
        if (i < alen) c += (int64_t)a->v[i] * factor;
        r->v[i] = (int32_t)c & M30; c >>= 30;
    }
    if (8 < alen) c += (int64_t)a->v[8] * factor;
    VERIFY_CHECK(c == (int32_t)c);
    r->v[8] = (int32_t)c;
}

/* Return -1 for a<b*factor, 0 for a==b*factor, 1 for a>b*factor. A consists of alen limbs; b has 9. */
static int secp256k1_modinv32_mul_cmp_30(const secp256k1_modinv32_signed30 *a, int alen, const secp256k1_modinv32_signed30 *b, int32_t factor) {
    int i;
    secp256k1_modinv32_signed30 am, bm;
    secp256k1_modinv32_mul_30(&am, a, alen, 1); /* Normalize all but the top limb of a. */
    secp256k1_modinv32_mul_30(&bm, b, 9, factor);
    for (i = 0; i < 8; ++i) {
        /* Verify that all but the top limb of a and b are normalized. */
        VERIFY_CHECK(am.v[i] >> 30 == 0);
        VERIFY_CHECK(bm.v[i] >> 30 == 0);
    }
    for (i = 8; i >= 0; --i) {
        if (am.v[i] < bm.v[i]) return -1;
        if (am.v[i] > bm.v[i]) return 1;
    }
    return 0;
}
#endif

/* Take as input a signed30 number in range (-2*modulus,modulus), and add a multiple of the modulus
 * to it to bring it to range [0,modulus). If sign < 0, the input will also be negated in the
 * process. The input must have limbs in range (-2^30,2^30). The output will have limbs in range
 * [0,2^30). */
static void secp256k1_modinv32_normalize_30(secp256k1_modinv32_signed30 *r, int32_t sign, const secp256k1_modinv32_modinfo *modinfo) {
    const int32_t M30 = (int32_t)(UINT32_MAX >> 2);
    int32_t r0 = r->v[0], r1 = r->v[1], r2 = r->v[2], r3 = r->v[3], r4 = r->v[4],
            r5 = r->v[5], r6 = r->v[6], r7 = r->v[7], r8 = r->v[8];
    volatile int32_t cond_add, cond_negate;

#ifdef VERIFY
    /* Verify that all limbs are in range (-2^30,2^30). */
    int i;
    for (i = 0; i < 9; ++i) {
        VERIFY_CHECK(r->v[i] >= -M30);
        VERIFY_CHECK(r->v[i] <= M30);
    }
    VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(r, 9, &modinfo->modulus, -2) > 0); /* r > -2*modulus */
    VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(r, 9, &modinfo->modulus, 1) < 0); /* r < modulus */
#endif

    /* In a first step, add the modulus if the input is negative, and then negate if requested.
     * This brings r from range (-2*modulus,modulus) to range (-modulus,modulus). As all input
     * limbs are in range (-2^30,2^30), this cannot overflow an int32_t. Note that the right
     * shifts below are signed sign-extending shifts (see assumptions.h for tests that that is
     * indeed the behavior of the right shift operator). */
    cond_add = r8 >> 31;
    r0 += modinfo->modulus.v[0] & cond_add;
    r1 += modinfo->modulus.v[1] & cond_add;
    r2 += modinfo->modulus.v[2] & cond_add;
    r3 += modinfo->modulus.v[3] & cond_add;
    r4 += modinfo->modulus.v[4] & cond_add;
    r5 += modinfo->modulus.v[5] & cond_add;
    r6 += modinfo->modulus.v[6] & cond_add;
    r7 += modinfo->modulus.v[7] & cond_add;
    r8 += modinfo->modulus.v[8] & cond_add;
    cond_negate = sign >> 31;
    r0 = (r0 ^ cond_negate) - cond_negate;
    r1 = (r1 ^ cond_negate) - cond_negate;
    r2 = (r2 ^ cond_negate) - cond_negate;
    r3 = (r3 ^ cond_negate) - cond_negate;
    r4 = (r4 ^ cond_negate) - cond_negate;
    r5 = (r5 ^ cond_negate) - cond_negate;
    r6 = (r6 ^ cond_negate) - cond_negate;
    r7 = (r7 ^ cond_negate) - cond_negate;
    r8 = (r8 ^ cond_negate) - cond_negate;
    /* Propagate the top bits, to bring limbs back to range (-2^30,2^30). */
    r1 += r0 >> 30; r0 &= M30;
    r2 += r1 >> 30; r1 &= M30;
    r3 += r2 >> 30; r2 &= M30;
    r4 += r3 >> 30; r3 &= M30;
    r5 += r4 >> 30; r4 &= M30;
    r6 += r5 >> 30; r5 &= M30;
    r7 += r6 >> 30; r6 &= M30;
    r8 += r7 >> 30; r7 &= M30;

    /* In a second step add the modulus again if the result is still negative, bringing r to range
     * [0,modulus). */
    cond_add = r8 >> 31;
    r0 += modinfo->modulus.v[0] & cond_add;
    r1 += modinfo->modulus.v[1] & cond_add;
    r2 += modinfo->modulus.v[2] & cond_add;
    r3 += modinfo->modulus.v[3] & cond_add;
    r4 += modinfo->modulus.v[4] & cond_add;
    r5 += modinfo->modulus.v[5] & cond_add;
    r6 += modinfo->modulus.v[6] & cond_add;
    r7 += modinfo->modulus.v[7] & cond_add;
    r8 += modinfo->modulus.v[8] & cond_add;
    /* And propagate again. */
    r1 += r0 >> 30; r0 &= M30;
    r2 += r1 >> 30; r1 &= M30;
    r3 += r2 >> 30; r2 &= M30;
    r4 += r3 >> 30; r3 &= M30;
    r5 += r4 >> 30; r4 &= M30;
    r6 += r5 >> 30; r5 &= M30;
    r7 += r6 >> 30; r6 &= M30;
    r8 += r7 >> 30; r7 &= M30;

    r->v[0] = r0;
    r->v[1] = r1;
    r->v[2] = r2;
    r->v[3] = r3;
    r->v[4] = r4;
    r->v[5] = r5;
    r->v[6] = r6;
    r->v[7] = r7;
    r->v[8] = r8;

#ifdef VERIFY
    VERIFY_CHECK(r0 >> 30 == 0);
    VERIFY_CHECK(r1 >> 30 == 0);
    VERIFY_CHECK(r2 >> 30 == 0);
    VERIFY_CHECK(r3 >> 30 == 0);
    VERIFY_CHECK(r4 >> 30 == 0);
    VERIFY_CHECK(r5 >> 30 == 0);
    VERIFY_CHECK(r6 >> 30 == 0);
    VERIFY_CHECK(r7 >> 30 == 0);
    VERIFY_CHECK(r8 >> 30 == 0);
    VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(r, 9, &modinfo->modulus, 0) >= 0); /* r >= 0 */
    VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(r, 9, &modinfo->modulus, 1) < 0); /* r < modulus */
#endif
}

/* Data type for transition matrices (see section 3 of explanation).
 *
 * t = [ u  v ]
 *     [ q  r ]
 */
typedef struct {
    int32_t u, v, q, r;
} secp256k1_modinv32_trans2x2;

/* Compute the transition matrix and zeta for 30 divsteps.
 *
 * Input:  zeta: initial zeta
 *         f0:   bottom limb of initial f
 *         g0:   bottom limb of initial g
 * Output: t: transition matrix
 * Return: final zeta
 *
 * Implements the divsteps_n_matrix function from the explanation.
 */
static int32_t secp256k1_modinv32_divsteps_30(int32_t zeta, uint32_t f0, uint32_t g0, secp256k1_modinv32_trans2x2 *t) {
    /* u,v,q,r are the elements of the transformation matrix being built up,
     * starting with the identity matrix. Semantically they are signed integers
     * in range [-2^30,2^30], but here represented as unsigned mod 2^32. This
     * permits left shifting (which is UB for negative numbers). The range
     * being inside [-2^31,2^31) means that casting to signed works correctly.
     */
    uint32_t u = 1, v = 0, q = 0, r = 1;
    volatile uint32_t c1, c2;
    uint32_t mask1, mask2, f = f0, g = g0, x, y, z;
    int i;

    for (i = 0; i < 30; ++i) {
        VERIFY_CHECK((f & 1) == 1); /* f must always be odd */
        VERIFY_CHECK((u * f0 + v * g0) == f << i);
        VERIFY_CHECK((q * f0 + r * g0) == g << i);
        /* Compute conditional masks for (zeta < 0) and for (g & 1). */
        c1 = zeta >> 31;
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
        /* In what follows, mask1 is a condition mask for (zeta < 0) and (g & 1). */
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
        /* Bounds on zeta that follow from the bounds on iteration count (max 20*30 divsteps). */
        VERIFY_CHECK(zeta >= -601 && zeta <= 601);
    }
    /* Return data in t and return value. */
    t->u = (int32_t)u;
    t->v = (int32_t)v;
    t->q = (int32_t)q;
    t->r = (int32_t)r;
    /* The determinant of t must be a power of two. This guarantees that multiplication with t
     * does not change the gcd of f and g, apart from adding a power-of-2 factor to it (which
     * will be divided out again). As each divstep's individual matrix has determinant 2, the
     * aggregate of 30 of them will have determinant 2^30. */
    VERIFY_CHECK((int64_t)t->u * t->r - (int64_t)t->v * t->q == ((int64_t)1) << 30);
    return zeta;
}

/* secp256k1_modinv32_inv256[i] = -(2*i+1)^-1 (mod 256) */
static const uint8_t secp256k1_modinv32_inv256[128] = {
    0xFF, 0x55, 0x33, 0x49, 0xC7, 0x5D, 0x3B, 0x11, 0x0F, 0xE5, 0xC3, 0x59,
    0xD7, 0xED, 0xCB, 0x21, 0x1F, 0x75, 0x53, 0x69, 0xE7, 0x7D, 0x5B, 0x31,
    0x2F, 0x05, 0xE3, 0x79, 0xF7, 0x0D, 0xEB, 0x41, 0x3F, 0x95, 0x73, 0x89,
    0x07, 0x9D, 0x7B, 0x51, 0x4F, 0x25, 0x03, 0x99, 0x17, 0x2D, 0x0B, 0x61,
    0x5F, 0xB5, 0x93, 0xA9, 0x27, 0xBD, 0x9B, 0x71, 0x6F, 0x45, 0x23, 0xB9,
    0x37, 0x4D, 0x2B, 0x81, 0x7F, 0xD5, 0xB3, 0xC9, 0x47, 0xDD, 0xBB, 0x91,
    0x8F, 0x65, 0x43, 0xD9, 0x57, 0x6D, 0x4B, 0xA1, 0x9F, 0xF5, 0xD3, 0xE9,
    0x67, 0xFD, 0xDB, 0xB1, 0xAF, 0x85, 0x63, 0xF9, 0x77, 0x8D, 0x6B, 0xC1,
    0xBF, 0x15, 0xF3, 0x09, 0x87, 0x1D, 0xFB, 0xD1, 0xCF, 0xA5, 0x83, 0x19,
    0x97, 0xAD, 0x8B, 0xE1, 0xDF, 0x35, 0x13, 0x29, 0xA7, 0x3D, 0x1B, 0xF1,
    0xEF, 0xC5, 0xA3, 0x39, 0xB7, 0xCD, 0xAB, 0x01
};

/* Compute the transition matrix and eta for 30 divsteps (variable time).
 *
 * Input:  eta: initial eta
 *         f0:  bottom limb of initial f
 *         g0:  bottom limb of initial g
 * Output: t: transition matrix
 * Return: final eta
 *
 * Implements the divsteps_n_matrix_var function from the explanation.
 */
static int32_t secp256k1_modinv32_divsteps_30_var(int32_t eta, uint32_t f0, uint32_t g0, secp256k1_modinv32_trans2x2 *t) {
    /* Transformation matrix; see comments in secp256k1_modinv32_divsteps_30. */
    uint32_t u = 1, v = 0, q = 0, r = 1;
    uint32_t f = f0, g = g0, m;
    uint16_t w;
    int i = 30, limit, zeros;

    for (;;) {
        /* Use a sentinel bit to count zeros only up to i. */
        zeros = secp256k1_ctz32_var(g | (UINT32_MAX << i));
        /* Perform zeros divsteps at once; they all just divide g by two. */
        g >>= zeros;
        u <<= zeros;
        v <<= zeros;
        eta -= zeros;
        i -= zeros;
         /* We're done once we've done 30 divsteps. */
        if (i == 0) break;
        VERIFY_CHECK((f & 1) == 1);
        VERIFY_CHECK((g & 1) == 1);
        VERIFY_CHECK((u * f0 + v * g0) == f << (30 - i));
        VERIFY_CHECK((q * f0 + r * g0) == g << (30 - i));
        /* Bounds on eta that follow from the bounds on iteration count (max 25*30 divsteps). */
        VERIFY_CHECK(eta >= -751 && eta <= 751);
        /* If eta is negative, negate it and replace f,g with g,-f. */
        if (eta < 0) {
            uint32_t tmp;
            eta = -eta;
            tmp = f; f = g; g = -tmp;
            tmp = u; u = q; q = -tmp;
            tmp = v; v = r; r = -tmp;
        }
        /* eta is now >= 0. In what follows we're going to cancel out the bottom bits of g. No more
         * than i can be cancelled out (as we'd be done before that point), and no more than eta+1
         * can be done as its sign will flip once that happens. */
        limit = ((int)eta + 1) > i ? i : ((int)eta + 1);
        /* m is a mask for the bottom min(limit, 8) bits (our table only supports 8 bits). */
        VERIFY_CHECK(limit > 0 && limit <= 30);
        m = (UINT32_MAX >> (32 - limit)) & 255U;
        /* Find what multiple of f must be added to g to cancel its bottom min(limit, 8) bits. */
        w = (g * secp256k1_modinv32_inv256[(f >> 1) & 127]) & m;
        /* Do so. */
        g += f * w;
        q += u * w;
        r += v * w;
        VERIFY_CHECK((g & m) == 0);
    }
    /* Return data in t and return value. */
    t->u = (int32_t)u;
    t->v = (int32_t)v;
    t->q = (int32_t)q;
    t->r = (int32_t)r;
    /* The determinant of t must be a power of two. This guarantees that multiplication with t
     * does not change the gcd of f and g, apart from adding a power-of-2 factor to it (which
     * will be divided out again). As each divstep's individual matrix has determinant 2, the
     * aggregate of 30 of them will have determinant 2^30. */
    VERIFY_CHECK((int64_t)t->u * t->r - (int64_t)t->v * t->q == ((int64_t)1) << 30);
    return eta;
}

/* Compute the transition matrix and eta for 30 posdivsteps (variable time, eta=-delta), and keeps track
 * of the Jacobi symbol along the way. f0 and g0 must be f and g mod 2^32 rather than 2^30, because
 * Jacobi tracking requires knowing (f mod 8) rather than just (f mod 2).
 *
 * Input:        eta: initial eta
 *               f0:  bottom limb of initial f
 *               g0:  bottom limb of initial g
 * Output:       t: transition matrix
 * Input/Output: (*jacp & 1) is bitflipped if and only if the Jacobi symbol of (f | g) changes sign
 *               by applying the returned transformation matrix to it. The other bits of *jacp may
 *               change, but are meaningless.
 * Return: final eta
 */
static int32_t secp256k1_modinv32_posdivsteps_30_var(int32_t eta, uint32_t f0, uint32_t g0, secp256k1_modinv32_trans2x2 *t, int *jacp) {
    /* Transformation matrix. */
    uint32_t u = 1, v = 0, q = 0, r = 1;
    uint32_t f = f0, g = g0, m;
    uint16_t w;
    int i = 30, limit, zeros;
    int jac = *jacp;

    for (;;) {
        /* Use a sentinel bit to count zeros only up to i. */
        zeros = secp256k1_ctz32_var(g | (UINT32_MAX << i));
        /* Perform zeros divsteps at once; they all just divide g by two. */
        g >>= zeros;
        u <<= zeros;
        v <<= zeros;
        eta -= zeros;
        i -= zeros;
        /* Update the bottom bit of jac: when dividing g by an odd power of 2,
         * if (f mod 8) is 3 or 5, the Jacobi symbol changes sign. */
        jac ^= (zeros & ((f >> 1) ^ (f >> 2)));
        /* We're done once we've done 30 posdivsteps. */
        if (i == 0) break;
        VERIFY_CHECK((f & 1) == 1);
        VERIFY_CHECK((g & 1) == 1);
        VERIFY_CHECK((u * f0 + v * g0) == f << (30 - i));
        VERIFY_CHECK((q * f0 + r * g0) == g << (30 - i));
        /* If eta is negative, negate it and replace f,g with g,f. */
        if (eta < 0) {
            uint32_t tmp;
            eta = -eta;
            /* Update bottom bit of jac: when swapping f and g, the Jacobi symbol changes sign
             * if both f and g are 3 mod 4. */
            jac ^= ((f & g) >> 1);
            tmp = f; f = g; g = tmp;
            tmp = u; u = q; q = tmp;
            tmp = v; v = r; r = tmp;
        }
        /* eta is now >= 0. In what follows we're going to cancel out the bottom bits of g. No more
         * than i can be cancelled out (as we'd be done before that point), and no more than eta+1
         * can be done as its sign will flip once that happens. */
        limit = ((int)eta + 1) > i ? i : ((int)eta + 1);
        /* m is a mask for the bottom min(limit, 8) bits (our table only supports 8 bits). */
        VERIFY_CHECK(limit > 0 && limit <= 30);
        m = (UINT32_MAX >> (32 - limit)) & 255U;
        /* Find what multiple of f must be added to g to cancel its bottom min(limit, 8) bits. */
        w = (g * secp256k1_modinv32_inv256[(f >> 1) & 127]) & m;
        /* Do so. */
        g += f * w;
        q += u * w;
        r += v * w;
        VERIFY_CHECK((g & m) == 0);
    }
    /* Return data in t and return value. */
    t->u = (int32_t)u;
    t->v = (int32_t)v;
    t->q = (int32_t)q;
    t->r = (int32_t)r;
    /* The determinant of t must be a power of two. This guarantees that multiplication with t
     * does not change the gcd of f and g, apart from adding a power-of-2 factor to it (which
     * will be divided out again). As each divstep's individual matrix has determinant 2 or -2,
     * the aggregate of 30 of them will have determinant 2^30 or -2^30. */
    VERIFY_CHECK((int64_t)t->u * t->r - (int64_t)t->v * t->q == ((int64_t)1) << 30 ||
                 (int64_t)t->u * t->r - (int64_t)t->v * t->q == -(((int64_t)1) << 30));
    *jacp = jac;
    return eta;
}

/* Compute (t/2^30) * [d, e] mod modulus, where t is a transition matrix for 30 divsteps.
 *
 * On input and output, d and e are in range (-2*modulus,modulus). All output limbs will be in range
 * (-2^30,2^30).
 *
 * This implements the update_de function from the explanation.
 */
static void secp256k1_modinv32_update_de_30(secp256k1_modinv32_signed30 *d, secp256k1_modinv32_signed30 *e, const secp256k1_modinv32_trans2x2 *t, const secp256k1_modinv32_modinfo* modinfo) {
    const int32_t M30 = (int32_t)(UINT32_MAX >> 2);
    const int32_t u = t->u, v = t->v, q = t->q, r = t->r;
    int32_t di, ei, md, me, sd, se;
    int64_t cd, ce;
    int i;
#ifdef VERIFY
    VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(d, 9, &modinfo->modulus, -2) > 0); /* d > -2*modulus */
    VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(d, 9, &modinfo->modulus, 1) < 0);  /* d <    modulus */
    VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(e, 9, &modinfo->modulus, -2) > 0); /* e > -2*modulus */
    VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(e, 9, &modinfo->modulus, 1) < 0);  /* e <    modulus */
    VERIFY_CHECK(labs(u) <= (M30 + 1 - labs(v))); /* |u|+|v| <= 2^30 */
    VERIFY_CHECK(labs(q) <= (M30 + 1 - labs(r))); /* |q|+|r| <= 2^30 */
#endif
    /* [md,me] start as zero; plus [u,q] if d is negative; plus [v,r] if e is negative. */
    sd = d->v[8] >> 31;
    se = e->v[8] >> 31;
    md = (u & sd) + (v & se);
    me = (q & sd) + (r & se);
    /* Begin computing t*[d,e]. */
    di = d->v[0];
    ei = e->v[0];
    cd = (int64_t)u * di + (int64_t)v * ei;
    ce = (int64_t)q * di + (int64_t)r * ei;
    /* Correct md,me so that t*[d,e]+modulus*[md,me] has 30 zero bottom bits. */
    md -= (modinfo->modulus_inv30 * (uint32_t)cd + md) & M30;
    me -= (modinfo->modulus_inv30 * (uint32_t)ce + me) & M30;
    /* Update the beginning of computation for t*[d,e]+modulus*[md,me] now md,me are known. */
    cd += (int64_t)modinfo->modulus.v[0] * md;
    ce += (int64_t)modinfo->modulus.v[0] * me;
    /* Verify that the low 30 bits of the computation are indeed zero, and then throw them away. */
    VERIFY_CHECK(((int32_t)cd & M30) == 0); cd >>= 30;
    VERIFY_CHECK(((int32_t)ce & M30) == 0); ce >>= 30;
    /* Now iteratively compute limb i=1..8 of t*[d,e]+modulus*[md,me], and store them in output
     * limb i-1 (shifting down by 30 bits). */
    for (i = 1; i < 9; ++i) {
        di = d->v[i];
        ei = e->v[i];
        cd += (int64_t)u * di + (int64_t)v * ei;
        ce += (int64_t)q * di + (int64_t)r * ei;
        cd += (int64_t)modinfo->modulus.v[i] * md;
        ce += (int64_t)modinfo->modulus.v[i] * me;
        d->v[i - 1] = (int32_t)cd & M30; cd >>= 30;
        e->v[i - 1] = (int32_t)ce & M30; ce >>= 30;
    }
    /* What remains is limb 9 of t*[d,e]+modulus*[md,me]; store it as output limb 8. */
    d->v[8] = (int32_t)cd;
    e->v[8] = (int32_t)ce;
#ifdef VERIFY
    VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(d, 9, &modinfo->modulus, -2) > 0); /* d > -2*modulus */
    VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(d, 9, &modinfo->modulus, 1) < 0);  /* d <    modulus */
    VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(e, 9, &modinfo->modulus, -2) > 0); /* e > -2*modulus */
    VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(e, 9, &modinfo->modulus, 1) < 0);  /* e <    modulus */
#endif
}

/* Compute (t/2^30) * [f, g], where t is a transition matrix for 30 divsteps.
 *
 * This implements the update_fg function from the explanation.
 */
static void secp256k1_modinv32_update_fg_30(secp256k1_modinv32_signed30 *f, secp256k1_modinv32_signed30 *g, const secp256k1_modinv32_trans2x2 *t) {
    const int32_t M30 = (int32_t)(UINT32_MAX >> 2);
    const int32_t u = t->u, v = t->v, q = t->q, r = t->r;
    int32_t fi, gi;
    int64_t cf, cg;
    int i;
    /* Start computing t*[f,g]. */
    fi = f->v[0];
    gi = g->v[0];
    cf = (int64_t)u * fi + (int64_t)v * gi;
    cg = (int64_t)q * fi + (int64_t)r * gi;
    /* Verify that the bottom 30 bits of the result are zero, and then throw them away. */
    VERIFY_CHECK(((int32_t)cf & M30) == 0); cf >>= 30;
    VERIFY_CHECK(((int32_t)cg & M30) == 0); cg >>= 30;
    /* Now iteratively compute limb i=1..8 of t*[f,g], and store them in output limb i-1 (shifting
     * down by 30 bits). */
    for (i = 1; i < 9; ++i) {
        fi = f->v[i];
        gi = g->v[i];
        cf += (int64_t)u * fi + (int64_t)v * gi;
        cg += (int64_t)q * fi + (int64_t)r * gi;
        f->v[i - 1] = (int32_t)cf & M30; cf >>= 30;
        g->v[i - 1] = (int32_t)cg & M30; cg >>= 30;
    }
    /* What remains is limb 9 of t*[f,g]; store it as output limb 8. */
    f->v[8] = (int32_t)cf;
    g->v[8] = (int32_t)cg;
}

/* Compute (t/2^30) * [f, g], where t is a transition matrix for 30 divsteps.
 *
 * Version that operates on a variable number of limbs in f and g.
 *
 * This implements the update_fg function from the explanation in modinv64_impl.h.
 */
static void secp256k1_modinv32_update_fg_30_var(int len, secp256k1_modinv32_signed30 *f, secp256k1_modinv32_signed30 *g, const secp256k1_modinv32_trans2x2 *t) {
    const int32_t M30 = (int32_t)(UINT32_MAX >> 2);
    const int32_t u = t->u, v = t->v, q = t->q, r = t->r;
    int32_t fi, gi;
    int64_t cf, cg;
    int i;
    VERIFY_CHECK(len > 0);
    /* Start computing t*[f,g]. */
    fi = f->v[0];
    gi = g->v[0];
    cf = (int64_t)u * fi + (int64_t)v * gi;
    cg = (int64_t)q * fi + (int64_t)r * gi;
    /* Verify that the bottom 62 bits of the result are zero, and then throw them away. */
    VERIFY_CHECK(((int32_t)cf & M30) == 0); cf >>= 30;
    VERIFY_CHECK(((int32_t)cg & M30) == 0); cg >>= 30;
    /* Now iteratively compute limb i=1..len of t*[f,g], and store them in output limb i-1 (shifting
     * down by 30 bits). */
    for (i = 1; i < len; ++i) {
        fi = f->v[i];
        gi = g->v[i];
        cf += (int64_t)u * fi + (int64_t)v * gi;
        cg += (int64_t)q * fi + (int64_t)r * gi;
        f->v[i - 1] = (int32_t)cf & M30; cf >>= 30;
        g->v[i - 1] = (int32_t)cg & M30; cg >>= 30;
    }
    /* What remains is limb (len) of t*[f,g]; store it as output limb (len-1). */
    f->v[len - 1] = (int32_t)cf;
    g->v[len - 1] = (int32_t)cg;
}

/* Compute the inverse of x modulo modinfo->modulus, and replace x with it (constant time in x). */
static void secp256k1_modinv32(secp256k1_modinv32_signed30 *x, const secp256k1_modinv32_modinfo *modinfo) {
    /* Start with d=0, e=1, f=modulus, g=x, zeta=-1. */
    secp256k1_modinv32_signed30 d = {{0}};
    secp256k1_modinv32_signed30 e = {{1}};
    secp256k1_modinv32_signed30 f = modinfo->modulus;
    secp256k1_modinv32_signed30 g = *x;
    int i;
    int32_t zeta = -1; /* zeta = -(delta+1/2); delta is initially 1/2. */

    /* Do 20 iterations of 30 divsteps each = 600 divsteps. 590 suffices for 256-bit inputs. */
    for (i = 0; i < 20; ++i) {
        /* Compute transition matrix and new zeta after 30 divsteps. */
        secp256k1_modinv32_trans2x2 t;
        zeta = secp256k1_modinv32_divsteps_30(zeta, f.v[0], g.v[0], &t);
        /* Update d,e using that transition matrix. */
        secp256k1_modinv32_update_de_30(&d, &e, &t, modinfo);
        /* Update f,g using that transition matrix. */
#ifdef VERIFY
        VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(&f, 9, &modinfo->modulus, -1) > 0); /* f > -modulus */
        VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(&f, 9, &modinfo->modulus, 1) <= 0); /* f <= modulus */
        VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(&g, 9, &modinfo->modulus, -1) > 0); /* g > -modulus */
        VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(&g, 9, &modinfo->modulus, 1) < 0);  /* g <  modulus */
#endif
        secp256k1_modinv32_update_fg_30(&f, &g, &t);
#ifdef VERIFY
        VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(&f, 9, &modinfo->modulus, -1) > 0); /* f > -modulus */
        VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(&f, 9, &modinfo->modulus, 1) <= 0); /* f <= modulus */
        VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(&g, 9, &modinfo->modulus, -1) > 0); /* g > -modulus */
        VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(&g, 9, &modinfo->modulus, 1) < 0);  /* g <  modulus */
#endif
    }

    /* At this point sufficient iterations have been performed that g must have reached 0
     * and (if g was not originally 0) f must now equal +/- GCD of the initial f, g
     * values i.e. +/- 1, and d now contains +/- the modular inverse. */
#ifdef VERIFY
    /* g == 0 */
    VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(&g, 9, &SECP256K1_SIGNED30_ONE, 0) == 0);
    /* |f| == 1, or (x == 0 and d == 0 and |f|=modulus) */
    VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(&f, 9, &SECP256K1_SIGNED30_ONE, -1) == 0 ||
                 secp256k1_modinv32_mul_cmp_30(&f, 9, &SECP256K1_SIGNED30_ONE, 1) == 0 ||
                 (secp256k1_modinv32_mul_cmp_30(x, 9, &SECP256K1_SIGNED30_ONE, 0) == 0 &&
                  secp256k1_modinv32_mul_cmp_30(&d, 9, &SECP256K1_SIGNED30_ONE, 0) == 0 &&
                  (secp256k1_modinv32_mul_cmp_30(&f, 9, &modinfo->modulus, 1) == 0 ||
                   secp256k1_modinv32_mul_cmp_30(&f, 9, &modinfo->modulus, -1) == 0)));
#endif

    /* Optionally negate d, normalize to [0,modulus), and return it. */
    secp256k1_modinv32_normalize_30(&d, f.v[8], modinfo);
    *x = d;
}

/* Compute the inverse of x modulo modinfo->modulus, and replace x with it (variable time). */
static void secp256k1_modinv32_var(secp256k1_modinv32_signed30 *x, const secp256k1_modinv32_modinfo *modinfo) {
    /* Start with d=0, e=1, f=modulus, g=x, eta=-1. */
    secp256k1_modinv32_signed30 d = {{0, 0, 0, 0, 0, 0, 0, 0, 0}};
    secp256k1_modinv32_signed30 e = {{1, 0, 0, 0, 0, 0, 0, 0, 0}};
    secp256k1_modinv32_signed30 f = modinfo->modulus;
    secp256k1_modinv32_signed30 g = *x;
#ifdef VERIFY
    int i = 0;
#endif
    int j, len = 9;
    int32_t eta = -1; /* eta = -delta; delta is initially 1 (faster for the variable-time code) */
    int32_t cond, fn, gn;

    /* Do iterations of 30 divsteps each until g=0. */
    while (1) {
        /* Compute transition matrix and new eta after 30 divsteps. */
        secp256k1_modinv32_trans2x2 t;
        eta = secp256k1_modinv32_divsteps_30_var(eta, f.v[0], g.v[0], &t);
        /* Update d,e using that transition matrix. */
        secp256k1_modinv32_update_de_30(&d, &e, &t, modinfo);
        /* Update f,g using that transition matrix. */
#ifdef VERIFY
        VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(&f, len, &modinfo->modulus, -1) > 0); /* f > -modulus */
        VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(&f, len, &modinfo->modulus, 1) <= 0); /* f <= modulus */
        VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(&g, len, &modinfo->modulus, -1) > 0); /* g > -modulus */
        VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(&g, len, &modinfo->modulus, 1) < 0);  /* g <  modulus */
#endif
        secp256k1_modinv32_update_fg_30_var(len, &f, &g, &t);
        /* If the bottom limb of g is 0, there is a chance g=0. */
        if (g.v[0] == 0) {
            cond = 0;
            /* Check if all other limbs are also 0. */
            for (j = 1; j < len; ++j) {
                cond |= g.v[j];
            }
            /* If so, we're done. */
            if (cond == 0) break;
        }

        /* Determine if len>1 and limb (len-1) of both f and g is 0 or -1. */
        fn = f.v[len - 1];
        gn = g.v[len - 1];
        cond = ((int32_t)len - 2) >> 31;
        cond |= fn ^ (fn >> 31);
        cond |= gn ^ (gn >> 31);
        /* If so, reduce length, propagating the sign of f and g's top limb into the one below. */
        if (cond == 0) {
            f.v[len - 2] |= (uint32_t)fn << 30;
            g.v[len - 2] |= (uint32_t)gn << 30;
            --len;
        }
#ifdef VERIFY
        VERIFY_CHECK(++i < 25); /* We should never need more than 25*30 = 750 divsteps */
        VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(&f, len, &modinfo->modulus, -1) > 0); /* f > -modulus */
        VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(&f, len, &modinfo->modulus, 1) <= 0); /* f <= modulus */
        VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(&g, len, &modinfo->modulus, -1) > 0); /* g > -modulus */
        VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(&g, len, &modinfo->modulus, 1) < 0);  /* g <  modulus */
#endif
    }

    /* At this point g is 0 and (if g was not originally 0) f must now equal +/- GCD of
     * the initial f, g values i.e. +/- 1, and d now contains +/- the modular inverse. */
#ifdef VERIFY
    /* g == 0 */
    VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(&g, len, &SECP256K1_SIGNED30_ONE, 0) == 0);
    /* |f| == 1, or (x == 0 and d == 0 and |f|=modulus) */
    VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(&f, len, &SECP256K1_SIGNED30_ONE, -1) == 0 ||
                 secp256k1_modinv32_mul_cmp_30(&f, len, &SECP256K1_SIGNED30_ONE, 1) == 0 ||
                 (secp256k1_modinv32_mul_cmp_30(x, 9, &SECP256K1_SIGNED30_ONE, 0) == 0 &&
                  secp256k1_modinv32_mul_cmp_30(&d, 9, &SECP256K1_SIGNED30_ONE, 0) == 0 &&
                  (secp256k1_modinv32_mul_cmp_30(&f, len, &modinfo->modulus, 1) == 0 ||
                   secp256k1_modinv32_mul_cmp_30(&f, len, &modinfo->modulus, -1) == 0)));
#endif

    /* Optionally negate d, normalize to [0,modulus), and return it. */
    secp256k1_modinv32_normalize_30(&d, f.v[len - 1], modinfo);
    *x = d;
}

/* Do up to 50 iterations of 30 posdivsteps (up to 1500 steps; more is extremely rare) each until f=1.
 * In VERIFY mode use a lower number of iterations (750, close to the median 756), so failure actually occurs. */
#ifdef VERIFY
#define JACOBI32_ITERATIONS 25
#else
#define JACOBI32_ITERATIONS 50
#endif

/* Compute the Jacobi symbol of x modulo modinfo->modulus (variable time). gcd(x,modulus) must be 1. */
static int secp256k1_jacobi32_maybe_var(const secp256k1_modinv32_signed30 *x, const secp256k1_modinv32_modinfo *modinfo) {
    /* Start with f=modulus, g=x, eta=-1. */
    secp256k1_modinv32_signed30 f = modinfo->modulus;
    secp256k1_modinv32_signed30 g = *x;
    int j, len = 9;
    int32_t eta = -1; /* eta = -delta; delta is initially 1 */
    int32_t cond, fn, gn;
    int jac = 0;
    int count;

    /* The input limbs must all be non-negative. */
    VERIFY_CHECK(g.v[0] >= 0 && g.v[1] >= 0 && g.v[2] >= 0 && g.v[3] >= 0 && g.v[4] >= 0 && g.v[5] >= 0 && g.v[6] >= 0 && g.v[7] >= 0 && g.v[8] >= 0);

    /* If x > 0, then if the loop below converges, it converges to f=g=gcd(x,modulus). Since we
     * require that gcd(x,modulus)=1 and modulus>=3, x cannot be 0. Thus, we must reach f=1 (or
     * time out). */
    VERIFY_CHECK((g.v[0] | g.v[1] | g.v[2] | g.v[3] | g.v[4] | g.v[5] | g.v[6] | g.v[7] | g.v[8]) != 0);

    for (count = 0; count < JACOBI32_ITERATIONS; ++count) {
        /* Compute transition matrix and new eta after 30 posdivsteps. */
        secp256k1_modinv32_trans2x2 t;
        eta = secp256k1_modinv32_posdivsteps_30_var(eta, f.v[0] | ((uint32_t)f.v[1] << 30), g.v[0] | ((uint32_t)g.v[1] << 30), &t, &jac);
        /* Update f,g using that transition matrix. */
#ifdef VERIFY
        VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(&f, len, &modinfo->modulus, 0) > 0); /* f > 0 */
        VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(&f, len, &modinfo->modulus, 1) <= 0); /* f <= modulus */
        VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(&g, len, &modinfo->modulus, 0) > 0); /* g > 0 */
        VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(&g, len, &modinfo->modulus, 1) < 0);  /* g < modulus */
#endif
        secp256k1_modinv32_update_fg_30_var(len, &f, &g, &t);
        /* If the bottom limb of f is 1, there is a chance that f=1. */
        if (f.v[0] == 1) {
            cond = 0;
            /* Check if the other limbs are also 0. */
            for (j = 1; j < len; ++j) {
                cond |= f.v[j];
            }
            /* If so, we're done. If f=1, the Jacobi symbol (g | f)=1. */
            if (cond == 0) return 1 - 2*(jac & 1);
        }

        /* Determine if len>1 and limb (len-1) of both f and g is 0. */
        fn = f.v[len - 1];
        gn = g.v[len - 1];
        cond = ((int32_t)len - 2) >> 31;
        cond |= fn;
        cond |= gn;
        /* If so, reduce length. */
        if (cond == 0) --len;
#ifdef VERIFY
        VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(&f, len, &modinfo->modulus, 0) > 0); /* f > 0 */
        VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(&f, len, &modinfo->modulus, 1) <= 0); /* f <= modulus */
        VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(&g, len, &modinfo->modulus, 0) > 0); /* g > 0 */
        VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(&g, len, &modinfo->modulus, 1) < 0);  /* g < modulus */
#endif
    }

    /* The loop failed to converge to f=g after 1500 iterations. Return 0, indicating unknown result. */
    return 0;
}

#endif /* SECP256K1_MODINV32_IMPL_H */
