/**********************************************************************
 * Copyright (c) 2013, 2014 Pieter Wuille                             *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef _SECP256K1_ECMULT_IMPL_H_
#define _SECP256K1_ECMULT_IMPL_H_

#include "group.h"
#include "scalar.h"
#include "ecmult.h"

/* optimal for 128-bit and 256-bit exponents. */
#define WINDOW_A 5

/** larger numbers may result in slightly better performance, at the cost of
    exponentially larger precomputed tables. WINDOW_G == 14 results in 640 KiB. */
#ifdef USE_ENDOMORPHISM
#define WINDOW_G 14
#else
#define WINDOW_G 15
#endif

/** Fill a table 'pre' with precomputed odd multiples of a. W determines the size of the table.
 *  pre will contains the values [1*a,3*a,5*a,...,(2^(w-1)-1)*a], so it needs place for
 *  2^(w-2) entries.
 *
 *  There are two versions of this function:
 *  - secp256k1_ecmult_precomp_wnaf_gej, which operates on group elements in jacobian notation,
 *    fast to precompute, but slower to use in later additions.
 *  - secp256k1_ecmult_precomp_wnaf_ge, which operates on group elements in affine notations,
 *    (much) slower to precompute, but a bit faster to use in later additions.
 *  To compute a*P + b*G, we use the jacobian version for P, and the affine version for G, as
 *  G is constant, so it only needs to be done once in advance.
 */
static void secp256k1_ecmult_table_precomp_gej_var(secp256k1_gej_t *pre, const secp256k1_gej_t *a, int w) {
    pre[0] = *a;
    secp256k1_gej_t d; secp256k1_gej_double_var(&d, &pre[0]);
    for (int i=1; i<(1 << (w-2)); i++)
        secp256k1_gej_add_var(&pre[i], &d, &pre[i-1]);
}

static void secp256k1_ecmult_table_precomp_ge_var(secp256k1_ge_t *pre, const secp256k1_gej_t *a, int w) {
    const int table_size = 1 << (w-2);
    secp256k1_gej_t prej[table_size];
    prej[0] = *a;
    secp256k1_gej_t d; secp256k1_gej_double_var(&d, a);
    for (int i=1; i<table_size; i++) {
        secp256k1_gej_add_var(&prej[i], &d, &prej[i-1]);
    }
    secp256k1_ge_set_all_gej_var(table_size, pre, prej);
}

/** The number of entries a table with precomputed multiples needs to have. */
#define ECMULT_TABLE_SIZE(w) (1 << ((w)-2))

/** The following two macro retrieves a particular odd multiple from a table
 *  of precomputed multiples. */
#define ECMULT_TABLE_GET(r,pre,n,w,neg) do { \
    VERIFY_CHECK(((n) & 1) == 1); \
    VERIFY_CHECK((n) >= -((1 << ((w)-1)) - 1)); \
    VERIFY_CHECK((n) <=  ((1 << ((w)-1)) - 1)); \
    if ((n) > 0) \
        *(r) = (pre)[((n)-1)/2]; \
    else \
        (neg)((r), &(pre)[(-(n)-1)/2]); \
} while(0)

#define ECMULT_TABLE_GET_GEJ(r,pre,n,w) ECMULT_TABLE_GET((r),(pre),(n),(w),secp256k1_gej_neg)
#define ECMULT_TABLE_GET_GE(r,pre,n,w)  ECMULT_TABLE_GET((r),(pre),(n),(w),secp256k1_ge_neg)

typedef struct {
    /* For accelerating the computation of a*P + b*G: */
    secp256k1_ge_t pre_g[ECMULT_TABLE_SIZE(WINDOW_G)];    /* odd multiples of the generator */
#ifdef USE_ENDOMORPHISM
    secp256k1_ge_t pre_g_128[ECMULT_TABLE_SIZE(WINDOW_G)]; /* odd multiples of 2^128*generator */
#endif
} secp256k1_ecmult_consts_t;

static const secp256k1_ecmult_consts_t *secp256k1_ecmult_consts = NULL;

static void secp256k1_ecmult_start(void) {
    if (secp256k1_ecmult_consts != NULL)
        return;

    /* Allocate the precomputation table. */
    secp256k1_ecmult_consts_t *ret = (secp256k1_ecmult_consts_t*)malloc(sizeof(secp256k1_ecmult_consts_t));

    /* get the generator */
    const secp256k1_ge_t *g = &secp256k1_ge_consts->g;
    secp256k1_gej_t gj; secp256k1_gej_set_ge(&gj, g);

#ifdef USE_ENDOMORPHISM
    /* calculate 2^128*generator */
    secp256k1_gej_t g_128j = gj;
    for (int i=0; i<128; i++)
        secp256k1_gej_double_var(&g_128j, &g_128j);
#endif

    /* precompute the tables with odd multiples */
    secp256k1_ecmult_table_precomp_ge_var(ret->pre_g, &gj, WINDOW_G);
#ifdef USE_ENDOMORPHISM
    secp256k1_ecmult_table_precomp_ge_var(ret->pre_g_128, &g_128j, WINDOW_G);
#endif

    /* Set the global pointer to the precomputation table. */
    secp256k1_ecmult_consts = ret;
}

static void secp256k1_ecmult_stop(void) {
    if (secp256k1_ecmult_consts == NULL)
        return;

    secp256k1_ecmult_consts_t *c = (secp256k1_ecmult_consts_t*)secp256k1_ecmult_consts;
    secp256k1_ecmult_consts = NULL;
    free(c);
}

/** Convert a number to WNAF notation. The number becomes represented by sum(2^i * wnaf[i], i=0..bits),
 *  with the following guarantees:
 *  - each wnaf[i] is either 0, or an odd integer between -(1<<(w-1) - 1) and (1<<(w-1) - 1)
 *  - two non-zero entries in wnaf are separated by at least w-1 zeroes.
 *  - the number of set values in wnaf is returned. This number is at most 256, and at most one more
 *  - than the number of bits in the (absolute value) of the input.
 */
static int secp256k1_ecmult_wnaf(int *wnaf, const secp256k1_scalar_t *a, int w) {
    secp256k1_scalar_t s = *a;

    int sign = 1;
    if (secp256k1_scalar_get_bits(&s, 255, 1)) {
        secp256k1_scalar_negate(&s, &s);
        sign = -1;
    }

    int set_bits = 0;
    int bit = 0;
    while (bit < 256) {
        if (secp256k1_scalar_get_bits(&s, bit, 1) == 0) {
            bit++;
            continue;
        }
        while (set_bits < bit) {
            wnaf[set_bits++] = 0;
        }
        int now = w;
        if (bit + now > 256) {
            now = 256 - bit;
        }
        int word = secp256k1_scalar_get_bits_var(&s, bit, now);
        if (word & (1 << (w-1))) {
            secp256k1_scalar_add_bit(&s, bit + w);
            wnaf[set_bits++] = sign * (word - (1 << w));
        } else {
            wnaf[set_bits++] = sign * word;
        }
        bit += now;
    }
    return set_bits;
}

static void secp256k1_ecmult(secp256k1_gej_t *r, const secp256k1_gej_t *a, const secp256k1_scalar_t *na, const secp256k1_scalar_t *ng) {
    const secp256k1_ecmult_consts_t *c = secp256k1_ecmult_consts;

#ifdef USE_ENDOMORPHISM
    secp256k1_scalar_t na_1, na_lam;
    /* split na into na_1 and na_lam (where na = na_1 + na_lam*lambda, and na_1 and na_lam are ~128 bit) */
    secp256k1_scalar_split_lambda_var(&na_1, &na_lam, na);

    /* build wnaf representation for na_1 and na_lam. */
    int wnaf_na_1[130];   int bits_na_1   = secp256k1_ecmult_wnaf(wnaf_na_1,   &na_1,   WINDOW_A);
    int wnaf_na_lam[130]; int bits_na_lam = secp256k1_ecmult_wnaf(wnaf_na_lam, &na_lam, WINDOW_A);
    VERIFY_CHECK(bits_na_1 <= 130);
    VERIFY_CHECK(bits_na_lam <= 130);
    int bits = bits_na_1;
    if (bits_na_lam > bits) bits = bits_na_lam;
#else
    /* build wnaf representation for na. */
    int wnaf_na[256];     int bits_na     = secp256k1_ecmult_wnaf(wnaf_na,     na,      WINDOW_A);
    int bits = bits_na;
#endif

    /* calculate odd multiples of a */
    secp256k1_gej_t pre_a[ECMULT_TABLE_SIZE(WINDOW_A)];
    secp256k1_ecmult_table_precomp_gej_var(pre_a, a, WINDOW_A);

#ifdef USE_ENDOMORPHISM
    secp256k1_gej_t pre_a_lam[ECMULT_TABLE_SIZE(WINDOW_A)];
    for (int i=0; i<ECMULT_TABLE_SIZE(WINDOW_A); i++)
        secp256k1_gej_mul_lambda(&pre_a_lam[i], &pre_a[i]);

    /* Splitted G factors. */
    secp256k1_scalar_t ng_1, ng_128;

    /* split ng into ng_1 and ng_128 (where gn = gn_1 + gn_128*2^128, and gn_1 and gn_128 are ~128 bit) */
    secp256k1_scalar_split_128(&ng_1, &ng_128, ng);

    /* Build wnaf representation for ng_1 and ng_128 */
    int wnaf_ng_1[129];   int bits_ng_1   = secp256k1_ecmult_wnaf(wnaf_ng_1,   &ng_1,   WINDOW_G);
    int wnaf_ng_128[129]; int bits_ng_128 = secp256k1_ecmult_wnaf(wnaf_ng_128, &ng_128, WINDOW_G);
    if (bits_ng_1 > bits) bits = bits_ng_1;
    if (bits_ng_128 > bits) bits = bits_ng_128;
#else
    int wnaf_ng[257];     int bits_ng     = secp256k1_ecmult_wnaf(wnaf_ng,     ng,      WINDOW_G);
    if (bits_ng > bits) bits = bits_ng;
#endif

    secp256k1_gej_set_infinity(r);
    secp256k1_gej_t tmpj;
    secp256k1_ge_t tmpa;

    for (int i=bits-1; i>=0; i--) {
        secp256k1_gej_double_var(r, r);
        int n;
#ifdef USE_ENDOMORPHISM
        if (i < bits_na_1 && (n = wnaf_na_1[i])) {
            ECMULT_TABLE_GET_GEJ(&tmpj, pre_a, n, WINDOW_A);
            secp256k1_gej_add_var(r, r, &tmpj);
        }
        if (i < bits_na_lam && (n = wnaf_na_lam[i])) {
            ECMULT_TABLE_GET_GEJ(&tmpj, pre_a_lam, n, WINDOW_A);
            secp256k1_gej_add_var(r, r, &tmpj);
        }
        if (i < bits_ng_1 && (n = wnaf_ng_1[i])) {
            ECMULT_TABLE_GET_GE(&tmpa, c->pre_g, n, WINDOW_G);
            secp256k1_gej_add_ge_var(r, r, &tmpa);
        }
        if (i < bits_ng_128 && (n = wnaf_ng_128[i])) {
            ECMULT_TABLE_GET_GE(&tmpa, c->pre_g_128, n, WINDOW_G);
            secp256k1_gej_add_ge_var(r, r, &tmpa);
        }
#else
        if (i < bits_na && (n = wnaf_na[i])) {
            ECMULT_TABLE_GET_GEJ(&tmpj, pre_a, n, WINDOW_A);
            secp256k1_gej_add_var(r, r, &tmpj);
        }
        if (i < bits_ng && (n = wnaf_ng[i])) {
            ECMULT_TABLE_GET_GE(&tmpa, c->pre_g, n, WINDOW_G);
            secp256k1_gej_add_ge_var(r, r, &tmpa);
        }
#endif
    }
}

#endif
