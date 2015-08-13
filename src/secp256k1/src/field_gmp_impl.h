/**********************************************************************
 * Copyright (c) 2013, 2014 Pieter Wuille                             *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef _SECP256K1_FIELD_REPR_IMPL_H_
#define _SECP256K1_FIELD_REPR_IMPL_H_

#include <stdio.h>
#include <string.h>
#include "num.h"
#include "field.h"

static mp_limb_t secp256k1_field_p[FIELD_LIMBS];
static mp_limb_t secp256k1_field_pc[(33+GMP_NUMB_BITS-1)/GMP_NUMB_BITS];

static void secp256k1_fe_inner_start(void) {
    for (int i=0; i<(33+GMP_NUMB_BITS-1)/GMP_NUMB_BITS; i++)
        secp256k1_field_pc[i] = 0;
    secp256k1_field_pc[0] += 0x3D1UL;
    secp256k1_field_pc[32/GMP_NUMB_BITS] += (((mp_limb_t)1) << (32 % GMP_NUMB_BITS));
    for (int i=0; i<FIELD_LIMBS; i++) {
        secp256k1_field_p[i] = 0;
    }
    mpn_sub(secp256k1_field_p, secp256k1_field_p, FIELD_LIMBS, secp256k1_field_pc, (33+GMP_NUMB_BITS-1)/GMP_NUMB_BITS);
}

static void secp256k1_fe_inner_stop(void) {
}

static void secp256k1_fe_normalize(secp256k1_fe_t *r) {
    if (r->n[FIELD_LIMBS] != 0) {
#if (GMP_NUMB_BITS >= 40)
        mp_limb_t carry = mpn_add_1(r->n, r->n, FIELD_LIMBS, 0x1000003D1ULL * r->n[FIELD_LIMBS]);
        mpn_add_1(r->n, r->n, FIELD_LIMBS, 0x1000003D1ULL * carry);
#else
        mp_limb_t carry = mpn_add_1(r->n, r->n, FIELD_LIMBS, 0x3D1UL * r->n[FIELD_LIMBS]) +
                          mpn_add_1(r->n+(32/GMP_NUMB_BITS), r->n+(32/GMP_NUMB_BITS), FIELD_LIMBS-(32/GMP_NUMB_BITS), r->n[FIELD_LIMBS] << (32 % GMP_NUMB_BITS));
        mpn_add_1(r->n, r->n, FIELD_LIMBS, 0x3D1UL * carry);
        mpn_add_1(r->n+(32/GMP_NUMB_BITS), r->n+(32/GMP_NUMB_BITS), FIELD_LIMBS-(32/GMP_NUMB_BITS), carry << (32%GMP_NUMB_BITS));
#endif
        r->n[FIELD_LIMBS] = 0;
    }
    if (mpn_cmp(r->n, secp256k1_field_p, FIELD_LIMBS) >= 0)
        mpn_sub(r->n, r->n, FIELD_LIMBS, secp256k1_field_p, FIELD_LIMBS);
}

SECP256K1_INLINE static void secp256k1_fe_set_int(secp256k1_fe_t *r, int a) {
    r->n[0] = a;
    for (int i=1; i<FIELD_LIMBS+1; i++)
        r->n[i] = 0;
}

SECP256K1_INLINE static void secp256k1_fe_clear(secp256k1_fe_t *r) {
    for (int i=0; i<FIELD_LIMBS+1; i++)
        r->n[i] = 0;
}

SECP256K1_INLINE static int secp256k1_fe_is_zero(const secp256k1_fe_t *a) {
    int ret = 1;
    for (int i=0; i<FIELD_LIMBS+1; i++)
        ret &= (a->n[i] == 0);
    return ret;
}

SECP256K1_INLINE static int secp256k1_fe_is_odd(const secp256k1_fe_t *a) {
    return a->n[0] & 1;
}

SECP256K1_INLINE static int secp256k1_fe_equal(const secp256k1_fe_t *a, const secp256k1_fe_t *b) {
    int ret = 1;
    for (int i=0; i<FIELD_LIMBS+1; i++)
        ret &= (a->n[i] == b->n[i]);
    return ret;
}

SECP256K1_INLINE static int secp256k1_fe_cmp_var(const secp256k1_fe_t *a, const secp256k1_fe_t *b) {
    for (int i=FIELD_LIMBS; i>=0; i--) {
        if (a->n[i] > b->n[i]) return 1;
        if (a->n[i] < b->n[i]) return -1;
    }
    return 0;
}

static int secp256k1_fe_set_b32(secp256k1_fe_t *r, const unsigned char *a) {
    for (int i=0; i<FIELD_LIMBS+1; i++)
        r->n[i] = 0;
    for (int i=0; i<256; i++) {
        int limb = i/GMP_NUMB_BITS;
        int shift = i%GMP_NUMB_BITS;
        r->n[limb] |= (mp_limb_t)((a[31-i/8] >> (i%8)) & 0x1) << shift;
    }
    return (mpn_cmp(r->n, secp256k1_field_p, FIELD_LIMBS) < 0);
}

/** Convert a field element to a 32-byte big endian value. Requires the input to be normalized */
static void secp256k1_fe_get_b32(unsigned char *r, const secp256k1_fe_t *a) {
    for (int i=0; i<32; i++) {
        int c = 0;
        for (int j=0; j<8; j++) {
            int limb = (8*i+j)/GMP_NUMB_BITS;
            int shift = (8*i+j)%GMP_NUMB_BITS;
            c |= ((a->n[limb] >> shift) & 0x1) << j;
        }
        r[31-i] = c;
    }
}

SECP256K1_INLINE static void secp256k1_fe_negate(secp256k1_fe_t *r, const secp256k1_fe_t *a, int m) {
    (void)m;
    *r = *a;
    secp256k1_fe_normalize(r);
    for (int i=0; i<FIELD_LIMBS; i++)
        r->n[i] = ~(r->n[i]);
#if (GMP_NUMB_BITS >= 33)
    mpn_sub_1(r->n, r->n, FIELD_LIMBS, 0x1000003D0ULL);
#else
    mpn_sub_1(r->n, r->n, FIELD_LIMBS, 0x3D0UL);
    mpn_sub_1(r->n+(32/GMP_NUMB_BITS), r->n+(32/GMP_NUMB_BITS), FIELD_LIMBS-(32/GMP_NUMB_BITS), 0x1UL << (32%GMP_NUMB_BITS));
#endif
}

SECP256K1_INLINE static void secp256k1_fe_mul_int(secp256k1_fe_t *r, int a) {
    mpn_mul_1(r->n, r->n, FIELD_LIMBS+1, a);
}

SECP256K1_INLINE static void secp256k1_fe_add(secp256k1_fe_t *r, const secp256k1_fe_t *a) {
    mpn_add(r->n, r->n, FIELD_LIMBS+1, a->n, FIELD_LIMBS+1);
}

static void secp256k1_fe_reduce(secp256k1_fe_t *r, mp_limb_t *tmp) {
    /** <A1 A2 A3 A4> <B1 B2 B3 B4>
     *        B1 B2 B3 B4
     *  + C * A1 A2 A3 A4
     *  +  A1 A2 A3 A4
     */

#if (GMP_NUMB_BITS >= 33)
    mp_limb_t o = mpn_addmul_1(tmp, tmp+FIELD_LIMBS, FIELD_LIMBS, 0x1000003D1ULL);
#else
    mp_limb_t o = mpn_addmul_1(tmp, tmp+FIELD_LIMBS, FIELD_LIMBS, 0x3D1UL) +
                  mpn_addmul_1(tmp+(32/GMP_NUMB_BITS), tmp+FIELD_LIMBS, FIELD_LIMBS-(32/GMP_NUMB_BITS), 0x1UL << (32%GMP_NUMB_BITS));
#endif
    mp_limb_t q[1+(33+GMP_NUMB_BITS-1)/GMP_NUMB_BITS];
    q[(33+GMP_NUMB_BITS-1)/GMP_NUMB_BITS] = mpn_mul_1(q, secp256k1_field_pc, (33+GMP_NUMB_BITS-1)/GMP_NUMB_BITS, o);
#if (GMP_NUMB_BITS <= 32)
    mp_limb_t o2 = tmp[2*FIELD_LIMBS-(32/GMP_NUMB_BITS)] << (32%GMP_NUMB_BITS);
    q[(33+GMP_NUMB_BITS-1)/GMP_NUMB_BITS] += mpn_addmul_1(q, secp256k1_field_pc, (33+GMP_NUMB_BITS-1)/GMP_NUMB_BITS, o2);
#endif
    r->n[FIELD_LIMBS] = mpn_add(r->n, tmp, FIELD_LIMBS, q, 1+(33+GMP_NUMB_BITS-1)/GMP_NUMB_BITS);
}

static void secp256k1_fe_mul(secp256k1_fe_t *r, const secp256k1_fe_t *a, const secp256k1_fe_t * SECP256K1_RESTRICT b) {
    VERIFY_CHECK(r != b);
    secp256k1_fe_t ac = *a;
    secp256k1_fe_t bc = *b;
    secp256k1_fe_normalize(&ac);
    secp256k1_fe_normalize(&bc);
    mp_limb_t tmp[2*FIELD_LIMBS];
    mpn_mul_n(tmp, ac.n, bc.n, FIELD_LIMBS);
    secp256k1_fe_reduce(r, tmp);
}

static void secp256k1_fe_sqr(secp256k1_fe_t *r, const secp256k1_fe_t *a) {
    secp256k1_fe_t ac = *a;
    secp256k1_fe_normalize(&ac);
    mp_limb_t tmp[2*FIELD_LIMBS];
    mpn_sqr(tmp, ac.n, FIELD_LIMBS);
    secp256k1_fe_reduce(r, tmp);
}

static void secp256k1_fe_cmov(secp256k1_fe_t *r, const secp256k1_fe_t *a, int flag) {
    mp_limb_t mask0 = flag + ~((mp_limb_t)0), mask1 = ~mask0;
    for (int i = 0; i <= FIELD_LIMBS; i++) {
        r->n[i] = (r->n[i] & mask0) | (a->n[i] & mask1);
    }
}

#endif
