/***********************************************************************
 * Copyright (c) 2013, 2014 Pieter Wuille                              *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_FIELD_REPR_IMPL_H
#define SECP256K1_FIELD_REPR_IMPL_H

#include "checkmem.h"
#include "util.h"
#include "field.h"
#include "modinv32_impl.h"

#ifdef VERIFY
static void secp256k1_fe_impl_verify(const secp256k1_fe *a) {
    const uint32_t *d = a->n;
    int m = a->normalized ? 1 : 2 * a->magnitude;
    VERIFY_CHECK(d[0] <= 0x3FFFFFFUL * m);
    VERIFY_CHECK(d[1] <= 0x3FFFFFFUL * m);
    VERIFY_CHECK(d[2] <= 0x3FFFFFFUL * m);
    VERIFY_CHECK(d[3] <= 0x3FFFFFFUL * m);
    VERIFY_CHECK(d[4] <= 0x3FFFFFFUL * m);
    VERIFY_CHECK(d[5] <= 0x3FFFFFFUL * m);
    VERIFY_CHECK(d[6] <= 0x3FFFFFFUL * m);
    VERIFY_CHECK(d[7] <= 0x3FFFFFFUL * m);
    VERIFY_CHECK(d[8] <= 0x3FFFFFFUL * m);
    VERIFY_CHECK(d[9] <= 0x03FFFFFUL * m);
    if (a->normalized) {
        if (d[9] == 0x03FFFFFUL) {
            uint32_t mid = d[8] & d[7] & d[6] & d[5] & d[4] & d[3] & d[2];
            if (mid == 0x3FFFFFFUL) {
                VERIFY_CHECK((d[1] + 0x40UL + ((d[0] + 0x3D1UL) >> 26)) <= 0x3FFFFFFUL);
            }
        }
    }
}
#endif

static void secp256k1_fe_impl_get_bounds(secp256k1_fe *r, int m) {
    r->n[0] = 0x3FFFFFFUL * 2 * m;
    r->n[1] = 0x3FFFFFFUL * 2 * m;
    r->n[2] = 0x3FFFFFFUL * 2 * m;
    r->n[3] = 0x3FFFFFFUL * 2 * m;
    r->n[4] = 0x3FFFFFFUL * 2 * m;
    r->n[5] = 0x3FFFFFFUL * 2 * m;
    r->n[6] = 0x3FFFFFFUL * 2 * m;
    r->n[7] = 0x3FFFFFFUL * 2 * m;
    r->n[8] = 0x3FFFFFFUL * 2 * m;
    r->n[9] = 0x03FFFFFUL * 2 * m;
}

static void secp256k1_fe_impl_normalize(secp256k1_fe *r) {
    uint32_t t0 = r->n[0], t1 = r->n[1], t2 = r->n[2], t3 = r->n[3], t4 = r->n[4],
             t5 = r->n[5], t6 = r->n[6], t7 = r->n[7], t8 = r->n[8], t9 = r->n[9];

    /* Reduce t9 at the start so there will be at most a single carry from the first pass */
    uint32_t m;
    uint32_t x = t9 >> 22; t9 &= 0x03FFFFFUL;

    /* The first pass ensures the magnitude is 1, ... */
    t0 += x * 0x3D1UL; t1 += (x << 6);
    t1 += (t0 >> 26); t0 &= 0x3FFFFFFUL;
    t2 += (t1 >> 26); t1 &= 0x3FFFFFFUL;
    t3 += (t2 >> 26); t2 &= 0x3FFFFFFUL; m = t2;
    t4 += (t3 >> 26); t3 &= 0x3FFFFFFUL; m &= t3;
    t5 += (t4 >> 26); t4 &= 0x3FFFFFFUL; m &= t4;
    t6 += (t5 >> 26); t5 &= 0x3FFFFFFUL; m &= t5;
    t7 += (t6 >> 26); t6 &= 0x3FFFFFFUL; m &= t6;
    t8 += (t7 >> 26); t7 &= 0x3FFFFFFUL; m &= t7;
    t9 += (t8 >> 26); t8 &= 0x3FFFFFFUL; m &= t8;

    /* ... except for a possible carry at bit 22 of t9 (i.e. bit 256 of the field element) */
    VERIFY_CHECK(t9 >> 23 == 0);

    /* At most a single final reduction is needed; check if the value is >= the field characteristic */
    x = (t9 >> 22) | ((t9 == 0x03FFFFFUL) & (m == 0x3FFFFFFUL)
        & ((t1 + 0x40UL + ((t0 + 0x3D1UL) >> 26)) > 0x3FFFFFFUL));

    /* Apply the final reduction (for constant-time behaviour, we do it always) */
    t0 += x * 0x3D1UL; t1 += (x << 6);
    t1 += (t0 >> 26); t0 &= 0x3FFFFFFUL;
    t2 += (t1 >> 26); t1 &= 0x3FFFFFFUL;
    t3 += (t2 >> 26); t2 &= 0x3FFFFFFUL;
    t4 += (t3 >> 26); t3 &= 0x3FFFFFFUL;
    t5 += (t4 >> 26); t4 &= 0x3FFFFFFUL;
    t6 += (t5 >> 26); t5 &= 0x3FFFFFFUL;
    t7 += (t6 >> 26); t6 &= 0x3FFFFFFUL;
    t8 += (t7 >> 26); t7 &= 0x3FFFFFFUL;
    t9 += (t8 >> 26); t8 &= 0x3FFFFFFUL;

    /* If t9 didn't carry to bit 22 already, then it should have after any final reduction */
    VERIFY_CHECK(t9 >> 22 == x);

    /* Mask off the possible multiple of 2^256 from the final reduction */
    t9 &= 0x03FFFFFUL;

    r->n[0] = t0; r->n[1] = t1; r->n[2] = t2; r->n[3] = t3; r->n[4] = t4;
    r->n[5] = t5; r->n[6] = t6; r->n[7] = t7; r->n[8] = t8; r->n[9] = t9;
}

static void secp256k1_fe_impl_normalize_weak(secp256k1_fe *r) {
    uint32_t t0 = r->n[0], t1 = r->n[1], t2 = r->n[2], t3 = r->n[3], t4 = r->n[4],
             t5 = r->n[5], t6 = r->n[6], t7 = r->n[7], t8 = r->n[8], t9 = r->n[9];

    /* Reduce t9 at the start so there will be at most a single carry from the first pass */
    uint32_t x = t9 >> 22; t9 &= 0x03FFFFFUL;

    /* The first pass ensures the magnitude is 1, ... */
    t0 += x * 0x3D1UL; t1 += (x << 6);
    t1 += (t0 >> 26); t0 &= 0x3FFFFFFUL;
    t2 += (t1 >> 26); t1 &= 0x3FFFFFFUL;
    t3 += (t2 >> 26); t2 &= 0x3FFFFFFUL;
    t4 += (t3 >> 26); t3 &= 0x3FFFFFFUL;
    t5 += (t4 >> 26); t4 &= 0x3FFFFFFUL;
    t6 += (t5 >> 26); t5 &= 0x3FFFFFFUL;
    t7 += (t6 >> 26); t6 &= 0x3FFFFFFUL;
    t8 += (t7 >> 26); t7 &= 0x3FFFFFFUL;
    t9 += (t8 >> 26); t8 &= 0x3FFFFFFUL;

    /* ... except for a possible carry at bit 22 of t9 (i.e. bit 256 of the field element) */
    VERIFY_CHECK(t9 >> 23 == 0);

    r->n[0] = t0; r->n[1] = t1; r->n[2] = t2; r->n[3] = t3; r->n[4] = t4;
    r->n[5] = t5; r->n[6] = t6; r->n[7] = t7; r->n[8] = t8; r->n[9] = t9;
}

static void secp256k1_fe_impl_normalize_var(secp256k1_fe *r) {
    uint32_t t0 = r->n[0], t1 = r->n[1], t2 = r->n[2], t3 = r->n[3], t4 = r->n[4],
             t5 = r->n[5], t6 = r->n[6], t7 = r->n[7], t8 = r->n[8], t9 = r->n[9];

    /* Reduce t9 at the start so there will be at most a single carry from the first pass */
    uint32_t m;
    uint32_t x = t9 >> 22; t9 &= 0x03FFFFFUL;

    /* The first pass ensures the magnitude is 1, ... */
    t0 += x * 0x3D1UL; t1 += (x << 6);
    t1 += (t0 >> 26); t0 &= 0x3FFFFFFUL;
    t2 += (t1 >> 26); t1 &= 0x3FFFFFFUL;
    t3 += (t2 >> 26); t2 &= 0x3FFFFFFUL; m = t2;
    t4 += (t3 >> 26); t3 &= 0x3FFFFFFUL; m &= t3;
    t5 += (t4 >> 26); t4 &= 0x3FFFFFFUL; m &= t4;
    t6 += (t5 >> 26); t5 &= 0x3FFFFFFUL; m &= t5;
    t7 += (t6 >> 26); t6 &= 0x3FFFFFFUL; m &= t6;
    t8 += (t7 >> 26); t7 &= 0x3FFFFFFUL; m &= t7;
    t9 += (t8 >> 26); t8 &= 0x3FFFFFFUL; m &= t8;

    /* ... except for a possible carry at bit 22 of t9 (i.e. bit 256 of the field element) */
    VERIFY_CHECK(t9 >> 23 == 0);

    /* At most a single final reduction is needed; check if the value is >= the field characteristic */
    x = (t9 >> 22) | ((t9 == 0x03FFFFFUL) & (m == 0x3FFFFFFUL)
        & ((t1 + 0x40UL + ((t0 + 0x3D1UL) >> 26)) > 0x3FFFFFFUL));

    if (x) {
        t0 += 0x3D1UL; t1 += (x << 6);
        t1 += (t0 >> 26); t0 &= 0x3FFFFFFUL;
        t2 += (t1 >> 26); t1 &= 0x3FFFFFFUL;
        t3 += (t2 >> 26); t2 &= 0x3FFFFFFUL;
        t4 += (t3 >> 26); t3 &= 0x3FFFFFFUL;
        t5 += (t4 >> 26); t4 &= 0x3FFFFFFUL;
        t6 += (t5 >> 26); t5 &= 0x3FFFFFFUL;
        t7 += (t6 >> 26); t6 &= 0x3FFFFFFUL;
        t8 += (t7 >> 26); t7 &= 0x3FFFFFFUL;
        t9 += (t8 >> 26); t8 &= 0x3FFFFFFUL;

        /* If t9 didn't carry to bit 22 already, then it should have after any final reduction */
        VERIFY_CHECK(t9 >> 22 == x);

        /* Mask off the possible multiple of 2^256 from the final reduction */
        t9 &= 0x03FFFFFUL;
    }

    r->n[0] = t0; r->n[1] = t1; r->n[2] = t2; r->n[3] = t3; r->n[4] = t4;
    r->n[5] = t5; r->n[6] = t6; r->n[7] = t7; r->n[8] = t8; r->n[9] = t9;
}

static int secp256k1_fe_impl_normalizes_to_zero(const secp256k1_fe *r) {
    uint32_t t0 = r->n[0], t1 = r->n[1], t2 = r->n[2], t3 = r->n[3], t4 = r->n[4],
             t5 = r->n[5], t6 = r->n[6], t7 = r->n[7], t8 = r->n[8], t9 = r->n[9];

    /* z0 tracks a possible raw value of 0, z1 tracks a possible raw value of P */
    uint32_t z0, z1;

    /* Reduce t9 at the start so there will be at most a single carry from the first pass */
    uint32_t x = t9 >> 22; t9 &= 0x03FFFFFUL;

    /* The first pass ensures the magnitude is 1, ... */
    t0 += x * 0x3D1UL; t1 += (x << 6);
    t1 += (t0 >> 26); t0 &= 0x3FFFFFFUL; z0  = t0; z1  = t0 ^ 0x3D0UL;
    t2 += (t1 >> 26); t1 &= 0x3FFFFFFUL; z0 |= t1; z1 &= t1 ^ 0x40UL;
    t3 += (t2 >> 26); t2 &= 0x3FFFFFFUL; z0 |= t2; z1 &= t2;
    t4 += (t3 >> 26); t3 &= 0x3FFFFFFUL; z0 |= t3; z1 &= t3;
    t5 += (t4 >> 26); t4 &= 0x3FFFFFFUL; z0 |= t4; z1 &= t4;
    t6 += (t5 >> 26); t5 &= 0x3FFFFFFUL; z0 |= t5; z1 &= t5;
    t7 += (t6 >> 26); t6 &= 0x3FFFFFFUL; z0 |= t6; z1 &= t6;
    t8 += (t7 >> 26); t7 &= 0x3FFFFFFUL; z0 |= t7; z1 &= t7;
    t9 += (t8 >> 26); t8 &= 0x3FFFFFFUL; z0 |= t8; z1 &= t8;
                                         z0 |= t9; z1 &= t9 ^ 0x3C00000UL;

    /* ... except for a possible carry at bit 22 of t9 (i.e. bit 256 of the field element) */
    VERIFY_CHECK(t9 >> 23 == 0);

    return (z0 == 0) | (z1 == 0x3FFFFFFUL);
}

static int secp256k1_fe_impl_normalizes_to_zero_var(const secp256k1_fe *r) {
    uint32_t t0, t1, t2, t3, t4, t5, t6, t7, t8, t9;
    uint32_t z0, z1;
    uint32_t x;

    t0 = r->n[0];
    t9 = r->n[9];

    /* Reduce t9 at the start so there will be at most a single carry from the first pass */
    x = t9 >> 22;

    /* The first pass ensures the magnitude is 1, ... */
    t0 += x * 0x3D1UL;

    /* z0 tracks a possible raw value of 0, z1 tracks a possible raw value of P */
    z0 = t0 & 0x3FFFFFFUL;
    z1 = z0 ^ 0x3D0UL;

    /* Fast return path should catch the majority of cases */
    if ((z0 != 0UL) & (z1 != 0x3FFFFFFUL)) {
        return 0;
    }

    t1 = r->n[1];
    t2 = r->n[2];
    t3 = r->n[3];
    t4 = r->n[4];
    t5 = r->n[5];
    t6 = r->n[6];
    t7 = r->n[7];
    t8 = r->n[8];

    t9 &= 0x03FFFFFUL;
    t1 += (x << 6);

    t1 += (t0 >> 26);
    t2 += (t1 >> 26); t1 &= 0x3FFFFFFUL; z0 |= t1; z1 &= t1 ^ 0x40UL;
    t3 += (t2 >> 26); t2 &= 0x3FFFFFFUL; z0 |= t2; z1 &= t2;
    t4 += (t3 >> 26); t3 &= 0x3FFFFFFUL; z0 |= t3; z1 &= t3;
    t5 += (t4 >> 26); t4 &= 0x3FFFFFFUL; z0 |= t4; z1 &= t4;
    t6 += (t5 >> 26); t5 &= 0x3FFFFFFUL; z0 |= t5; z1 &= t5;
    t7 += (t6 >> 26); t6 &= 0x3FFFFFFUL; z0 |= t6; z1 &= t6;
    t8 += (t7 >> 26); t7 &= 0x3FFFFFFUL; z0 |= t7; z1 &= t7;
    t9 += (t8 >> 26); t8 &= 0x3FFFFFFUL; z0 |= t8; z1 &= t8;
                                         z0 |= t9; z1 &= t9 ^ 0x3C00000UL;

    /* ... except for a possible carry at bit 22 of t9 (i.e. bit 256 of the field element) */
    VERIFY_CHECK(t9 >> 23 == 0);

    return (z0 == 0) | (z1 == 0x3FFFFFFUL);
}

SECP256K1_INLINE static void secp256k1_fe_impl_set_int(secp256k1_fe *r, int a) {
    r->n[0] = a;
    r->n[1] = r->n[2] = r->n[3] = r->n[4] = r->n[5] = r->n[6] = r->n[7] = r->n[8] = r->n[9] = 0;
}

SECP256K1_INLINE static int secp256k1_fe_impl_is_zero(const secp256k1_fe *a) {
    const uint32_t *t = a->n;
    return (t[0] | t[1] | t[2] | t[3] | t[4] | t[5] | t[6] | t[7] | t[8] | t[9]) == 0;
}

SECP256K1_INLINE static int secp256k1_fe_impl_is_odd(const secp256k1_fe *a) {
    return a->n[0] & 1;
}

static int secp256k1_fe_impl_cmp_var(const secp256k1_fe *a, const secp256k1_fe *b) {
    int i;
    for (i = 9; i >= 0; i--) {
        if (a->n[i] > b->n[i]) {
            return 1;
        }
        if (a->n[i] < b->n[i]) {
            return -1;
        }
    }
    return 0;
}

static void secp256k1_fe_impl_set_b32_mod(secp256k1_fe *r, const unsigned char *a) {
    r->n[0] = (uint32_t)a[31] | ((uint32_t)a[30] << 8) | ((uint32_t)a[29] << 16) | ((uint32_t)(a[28] & 0x3) << 24);
    r->n[1] = (uint32_t)((a[28] >> 2) & 0x3f) | ((uint32_t)a[27] << 6) | ((uint32_t)a[26] << 14) | ((uint32_t)(a[25] & 0xf) << 22);
    r->n[2] = (uint32_t)((a[25] >> 4) & 0xf) | ((uint32_t)a[24] << 4) | ((uint32_t)a[23] << 12) | ((uint32_t)(a[22] & 0x3f) << 20);
    r->n[3] = (uint32_t)((a[22] >> 6) & 0x3) | ((uint32_t)a[21] << 2) | ((uint32_t)a[20] << 10) | ((uint32_t)a[19] << 18);
    r->n[4] = (uint32_t)a[18] | ((uint32_t)a[17] << 8) | ((uint32_t)a[16] << 16) | ((uint32_t)(a[15] & 0x3) << 24);
    r->n[5] = (uint32_t)((a[15] >> 2) & 0x3f) | ((uint32_t)a[14] << 6) | ((uint32_t)a[13] << 14) | ((uint32_t)(a[12] & 0xf) << 22);
    r->n[6] = (uint32_t)((a[12] >> 4) & 0xf) | ((uint32_t)a[11] << 4) | ((uint32_t)a[10] << 12) | ((uint32_t)(a[9] & 0x3f) << 20);
    r->n[7] = (uint32_t)((a[9] >> 6) & 0x3) | ((uint32_t)a[8] << 2) | ((uint32_t)a[7] << 10) | ((uint32_t)a[6] << 18);
    r->n[8] = (uint32_t)a[5] | ((uint32_t)a[4] << 8) | ((uint32_t)a[3] << 16) | ((uint32_t)(a[2] & 0x3) << 24);
    r->n[9] = (uint32_t)((a[2] >> 2) & 0x3f) | ((uint32_t)a[1] << 6) | ((uint32_t)a[0] << 14);
}

static int secp256k1_fe_impl_set_b32_limit(secp256k1_fe *r, const unsigned char *a) {
    secp256k1_fe_impl_set_b32_mod(r, a);
    return !((r->n[9] == 0x3FFFFFUL) & ((r->n[8] & r->n[7] & r->n[6] & r->n[5] & r->n[4] & r->n[3] & r->n[2]) == 0x3FFFFFFUL) & ((r->n[1] + 0x40UL + ((r->n[0] + 0x3D1UL) >> 26)) > 0x3FFFFFFUL));
}

/** Convert a field element to a 32-byte big endian value. Requires the input to be normalized */
static void secp256k1_fe_impl_get_b32(unsigned char *r, const secp256k1_fe *a) {
    r[0] = (a->n[9] >> 14) & 0xff;
    r[1] = (a->n[9] >> 6) & 0xff;
    r[2] = ((a->n[9] & 0x3F) << 2) | ((a->n[8] >> 24) & 0x3);
    r[3] = (a->n[8] >> 16) & 0xff;
    r[4] = (a->n[8] >> 8) & 0xff;
    r[5] = a->n[8] & 0xff;
    r[6] = (a->n[7] >> 18) & 0xff;
    r[7] = (a->n[7] >> 10) & 0xff;
    r[8] = (a->n[7] >> 2) & 0xff;
    r[9] = ((a->n[7] & 0x3) << 6) | ((a->n[6] >> 20) & 0x3f);
    r[10] = (a->n[6] >> 12) & 0xff;
    r[11] = (a->n[6] >> 4) & 0xff;
    r[12] = ((a->n[6] & 0xf) << 4) | ((a->n[5] >> 22) & 0xf);
    r[13] = (a->n[5] >> 14) & 0xff;
    r[14] = (a->n[5] >> 6) & 0xff;
    r[15] = ((a->n[5] & 0x3f) << 2) | ((a->n[4] >> 24) & 0x3);
    r[16] = (a->n[4] >> 16) & 0xff;
    r[17] = (a->n[4] >> 8) & 0xff;
    r[18] = a->n[4] & 0xff;
    r[19] = (a->n[3] >> 18) & 0xff;
    r[20] = (a->n[3] >> 10) & 0xff;
    r[21] = (a->n[3] >> 2) & 0xff;
    r[22] = ((a->n[3] & 0x3) << 6) | ((a->n[2] >> 20) & 0x3f);
    r[23] = (a->n[2] >> 12) & 0xff;
    r[24] = (a->n[2] >> 4) & 0xff;
    r[25] = ((a->n[2] & 0xf) << 4) | ((a->n[1] >> 22) & 0xf);
    r[26] = (a->n[1] >> 14) & 0xff;
    r[27] = (a->n[1] >> 6) & 0xff;
    r[28] = ((a->n[1] & 0x3f) << 2) | ((a->n[0] >> 24) & 0x3);
    r[29] = (a->n[0] >> 16) & 0xff;
    r[30] = (a->n[0] >> 8) & 0xff;
    r[31] = a->n[0] & 0xff;
}

SECP256K1_INLINE static void secp256k1_fe_impl_negate_unchecked(secp256k1_fe *r, const secp256k1_fe *a, int m) {
    /* For all legal values of m (0..31), the following properties hold: */
    VERIFY_CHECK(0x3FFFC2FUL * 2 * (m + 1) >= 0x3FFFFFFUL * 2 * m);
    VERIFY_CHECK(0x3FFFFBFUL * 2 * (m + 1) >= 0x3FFFFFFUL * 2 * m);
    VERIFY_CHECK(0x3FFFFFFUL * 2 * (m + 1) >= 0x3FFFFFFUL * 2 * m);
    VERIFY_CHECK(0x03FFFFFUL * 2 * (m + 1) >= 0x03FFFFFUL * 2 * m);

    /* Due to the properties above, the left hand in the subtractions below is never less than
     * the right hand. */
    r->n[0] = 0x3FFFC2FUL * 2 * (m + 1) - a->n[0];
    r->n[1] = 0x3FFFFBFUL * 2 * (m + 1) - a->n[1];
    r->n[2] = 0x3FFFFFFUL * 2 * (m + 1) - a->n[2];
    r->n[3] = 0x3FFFFFFUL * 2 * (m + 1) - a->n[3];
    r->n[4] = 0x3FFFFFFUL * 2 * (m + 1) - a->n[4];
    r->n[5] = 0x3FFFFFFUL * 2 * (m + 1) - a->n[5];
    r->n[6] = 0x3FFFFFFUL * 2 * (m + 1) - a->n[6];
    r->n[7] = 0x3FFFFFFUL * 2 * (m + 1) - a->n[7];
    r->n[8] = 0x3FFFFFFUL * 2 * (m + 1) - a->n[8];
    r->n[9] = 0x03FFFFFUL * 2 * (m + 1) - a->n[9];
}

SECP256K1_INLINE static void secp256k1_fe_impl_mul_int_unchecked(secp256k1_fe *r, int a) {
    r->n[0] *= a;
    r->n[1] *= a;
    r->n[2] *= a;
    r->n[3] *= a;
    r->n[4] *= a;
    r->n[5] *= a;
    r->n[6] *= a;
    r->n[7] *= a;
    r->n[8] *= a;
    r->n[9] *= a;
}

SECP256K1_INLINE static void secp256k1_fe_impl_add(secp256k1_fe *r, const secp256k1_fe *a) {
    r->n[0] += a->n[0];
    r->n[1] += a->n[1];
    r->n[2] += a->n[2];
    r->n[3] += a->n[3];
    r->n[4] += a->n[4];
    r->n[5] += a->n[5];
    r->n[6] += a->n[6];
    r->n[7] += a->n[7];
    r->n[8] += a->n[8];
    r->n[9] += a->n[9];
}

SECP256K1_INLINE static void secp256k1_fe_impl_add_int(secp256k1_fe *r, int a) {
    r->n[0] += a;
}

#if defined(USE_EXTERNAL_ASM)

/* External assembler implementation */
void secp256k1_fe_mul_inner(uint32_t *r, const uint32_t *a, const uint32_t * SECP256K1_RESTRICT b);
void secp256k1_fe_sqr_inner(uint32_t *r, const uint32_t *a);

#else

#define VERIFY_BITS(x, n) VERIFY_CHECK(((x) >> (n)) == 0)

SECP256K1_INLINE static void secp256k1_fe_mul_inner(uint32_t *r, const uint32_t *a, const uint32_t * SECP256K1_RESTRICT b) {
    uint64_t c, d;
    uint64_t u0, u1, u2, u3, u4, u5, u6, u7, u8;
    uint32_t t9, t1, t0, t2, t3, t4, t5, t6, t7;
    const uint32_t M = 0x3FFFFFFUL, R0 = 0x3D10UL, R1 = 0x400UL;

    VERIFY_BITS(a[0], 30);
    VERIFY_BITS(a[1], 30);
    VERIFY_BITS(a[2], 30);
    VERIFY_BITS(a[3], 30);
    VERIFY_BITS(a[4], 30);
    VERIFY_BITS(a[5], 30);
    VERIFY_BITS(a[6], 30);
    VERIFY_BITS(a[7], 30);
    VERIFY_BITS(a[8], 30);
    VERIFY_BITS(a[9], 26);
    VERIFY_BITS(b[0], 30);
    VERIFY_BITS(b[1], 30);
    VERIFY_BITS(b[2], 30);
    VERIFY_BITS(b[3], 30);
    VERIFY_BITS(b[4], 30);
    VERIFY_BITS(b[5], 30);
    VERIFY_BITS(b[6], 30);
    VERIFY_BITS(b[7], 30);
    VERIFY_BITS(b[8], 30);
    VERIFY_BITS(b[9], 26);

    /** [... a b c] is a shorthand for ... + a<<52 + b<<26 + c<<0 mod n.
     *  for 0 <= x <= 9, px is a shorthand for sum(a[i]*b[x-i], i=0..x).
     *  for 9 <= x <= 18, px is a shorthand for sum(a[i]*b[x-i], i=(x-9)..9)
     *  Note that [x 0 0 0 0 0 0 0 0 0 0] = [x*R1 x*R0].
     */

    d  = (uint64_t)a[0] * b[9]
       + (uint64_t)a[1] * b[8]
       + (uint64_t)a[2] * b[7]
       + (uint64_t)a[3] * b[6]
       + (uint64_t)a[4] * b[5]
       + (uint64_t)a[5] * b[4]
       + (uint64_t)a[6] * b[3]
       + (uint64_t)a[7] * b[2]
       + (uint64_t)a[8] * b[1]
       + (uint64_t)a[9] * b[0];
    /* VERIFY_BITS(d, 64); */
    /* [d 0 0 0 0 0 0 0 0 0] = [p9 0 0 0 0 0 0 0 0 0] */
    t9 = d & M; d >>= 26;
    VERIFY_BITS(t9, 26);
    VERIFY_BITS(d, 38);
    /* [d t9 0 0 0 0 0 0 0 0 0] = [p9 0 0 0 0 0 0 0 0 0] */

    c  = (uint64_t)a[0] * b[0];
    VERIFY_BITS(c, 60);
    /* [d t9 0 0 0 0 0 0 0 0 c] = [p9 0 0 0 0 0 0 0 0 p0] */
    d += (uint64_t)a[1] * b[9]
       + (uint64_t)a[2] * b[8]
       + (uint64_t)a[3] * b[7]
       + (uint64_t)a[4] * b[6]
       + (uint64_t)a[5] * b[5]
       + (uint64_t)a[6] * b[4]
       + (uint64_t)a[7] * b[3]
       + (uint64_t)a[8] * b[2]
       + (uint64_t)a[9] * b[1];
    VERIFY_BITS(d, 63);
    /* [d t9 0 0 0 0 0 0 0 0 c] = [p10 p9 0 0 0 0 0 0 0 0 p0] */
    u0 = d & M; d >>= 26; c += u0 * R0;
    VERIFY_BITS(u0, 26);
    VERIFY_BITS(d, 37);
    VERIFY_BITS(c, 61);
    /* [d u0 t9 0 0 0 0 0 0 0 0 c-u0*R0] = [p10 p9 0 0 0 0 0 0 0 0 p0] */
    t0 = c & M; c >>= 26; c += u0 * R1;
    VERIFY_BITS(t0, 26);
    VERIFY_BITS(c, 37);
    /* [d u0 t9 0 0 0 0 0 0 0 c-u0*R1 t0-u0*R0] = [p10 p9 0 0 0 0 0 0 0 0 p0] */
    /* [d 0 t9 0 0 0 0 0 0 0 c t0] = [p10 p9 0 0 0 0 0 0 0 0 p0] */

    c += (uint64_t)a[0] * b[1]
       + (uint64_t)a[1] * b[0];
    VERIFY_BITS(c, 62);
    /* [d 0 t9 0 0 0 0 0 0 0 c t0] = [p10 p9 0 0 0 0 0 0 0 p1 p0] */
    d += (uint64_t)a[2] * b[9]
       + (uint64_t)a[3] * b[8]
       + (uint64_t)a[4] * b[7]
       + (uint64_t)a[5] * b[6]
       + (uint64_t)a[6] * b[5]
       + (uint64_t)a[7] * b[4]
       + (uint64_t)a[8] * b[3]
       + (uint64_t)a[9] * b[2];
    VERIFY_BITS(d, 63);
    /* [d 0 t9 0 0 0 0 0 0 0 c t0] = [p11 p10 p9 0 0 0 0 0 0 0 p1 p0] */
    u1 = d & M; d >>= 26; c += u1 * R0;
    VERIFY_BITS(u1, 26);
    VERIFY_BITS(d, 37);
    VERIFY_BITS(c, 63);
    /* [d u1 0 t9 0 0 0 0 0 0 0 c-u1*R0 t0] = [p11 p10 p9 0 0 0 0 0 0 0 p1 p0] */
    t1 = c & M; c >>= 26; c += u1 * R1;
    VERIFY_BITS(t1, 26);
    VERIFY_BITS(c, 38);
    /* [d u1 0 t9 0 0 0 0 0 0 c-u1*R1 t1-u1*R0 t0] = [p11 p10 p9 0 0 0 0 0 0 0 p1 p0] */
    /* [d 0 0 t9 0 0 0 0 0 0 c t1 t0] = [p11 p10 p9 0 0 0 0 0 0 0 p1 p0] */

    c += (uint64_t)a[0] * b[2]
       + (uint64_t)a[1] * b[1]
       + (uint64_t)a[2] * b[0];
    VERIFY_BITS(c, 62);
    /* [d 0 0 t9 0 0 0 0 0 0 c t1 t0] = [p11 p10 p9 0 0 0 0 0 0 p2 p1 p0] */
    d += (uint64_t)a[3] * b[9]
       + (uint64_t)a[4] * b[8]
       + (uint64_t)a[5] * b[7]
       + (uint64_t)a[6] * b[6]
       + (uint64_t)a[7] * b[5]
       + (uint64_t)a[8] * b[4]
       + (uint64_t)a[9] * b[3];
    VERIFY_BITS(d, 63);
    /* [d 0 0 t9 0 0 0 0 0 0 c t1 t0] = [p12 p11 p10 p9 0 0 0 0 0 0 p2 p1 p0] */
    u2 = d & M; d >>= 26; c += u2 * R0;
    VERIFY_BITS(u2, 26);
    VERIFY_BITS(d, 37);
    VERIFY_BITS(c, 63);
    /* [d u2 0 0 t9 0 0 0 0 0 0 c-u2*R0 t1 t0] = [p12 p11 p10 p9 0 0 0 0 0 0 p2 p1 p0] */
    t2 = c & M; c >>= 26; c += u2 * R1;
    VERIFY_BITS(t2, 26);
    VERIFY_BITS(c, 38);
    /* [d u2 0 0 t9 0 0 0 0 0 c-u2*R1 t2-u2*R0 t1 t0] = [p12 p11 p10 p9 0 0 0 0 0 0 p2 p1 p0] */
    /* [d 0 0 0 t9 0 0 0 0 0 c t2 t1 t0] = [p12 p11 p10 p9 0 0 0 0 0 0 p2 p1 p0] */

    c += (uint64_t)a[0] * b[3]
       + (uint64_t)a[1] * b[2]
       + (uint64_t)a[2] * b[1]
       + (uint64_t)a[3] * b[0];
    VERIFY_BITS(c, 63);
    /* [d 0 0 0 t9 0 0 0 0 0 c t2 t1 t0] = [p12 p11 p10 p9 0 0 0 0 0 p3 p2 p1 p0] */
    d += (uint64_t)a[4] * b[9]
       + (uint64_t)a[5] * b[8]
       + (uint64_t)a[6] * b[7]
       + (uint64_t)a[7] * b[6]
       + (uint64_t)a[8] * b[5]
       + (uint64_t)a[9] * b[4];
    VERIFY_BITS(d, 63);
    /* [d 0 0 0 t9 0 0 0 0 0 c t2 t1 t0] = [p13 p12 p11 p10 p9 0 0 0 0 0 p3 p2 p1 p0] */
    u3 = d & M; d >>= 26; c += u3 * R0;
    VERIFY_BITS(u3, 26);
    VERIFY_BITS(d, 37);
    /* VERIFY_BITS(c, 64); */
    /* [d u3 0 0 0 t9 0 0 0 0 0 c-u3*R0 t2 t1 t0] = [p13 p12 p11 p10 p9 0 0 0 0 0 p3 p2 p1 p0] */
    t3 = c & M; c >>= 26; c += u3 * R1;
    VERIFY_BITS(t3, 26);
    VERIFY_BITS(c, 39);
    /* [d u3 0 0 0 t9 0 0 0 0 c-u3*R1 t3-u3*R0 t2 t1 t0] = [p13 p12 p11 p10 p9 0 0 0 0 0 p3 p2 p1 p0] */
    /* [d 0 0 0 0 t9 0 0 0 0 c t3 t2 t1 t0] = [p13 p12 p11 p10 p9 0 0 0 0 0 p3 p2 p1 p0] */

    c += (uint64_t)a[0] * b[4]
       + (uint64_t)a[1] * b[3]
       + (uint64_t)a[2] * b[2]
       + (uint64_t)a[3] * b[1]
       + (uint64_t)a[4] * b[0];
    VERIFY_BITS(c, 63);
    /* [d 0 0 0 0 t9 0 0 0 0 c t3 t2 t1 t0] = [p13 p12 p11 p10 p9 0 0 0 0 p4 p3 p2 p1 p0] */
    d += (uint64_t)a[5] * b[9]
       + (uint64_t)a[6] * b[8]
       + (uint64_t)a[7] * b[7]
       + (uint64_t)a[8] * b[6]
       + (uint64_t)a[9] * b[5];
    VERIFY_BITS(d, 62);
    /* [d 0 0 0 0 t9 0 0 0 0 c t3 t2 t1 t0] = [p14 p13 p12 p11 p10 p9 0 0 0 0 p4 p3 p2 p1 p0] */
    u4 = d & M; d >>= 26; c += u4 * R0;
    VERIFY_BITS(u4, 26);
    VERIFY_BITS(d, 36);
    /* VERIFY_BITS(c, 64); */
    /* [d u4 0 0 0 0 t9 0 0 0 0 c-u4*R0 t3 t2 t1 t0] = [p14 p13 p12 p11 p10 p9 0 0 0 0 p4 p3 p2 p1 p0] */
    t4 = c & M; c >>= 26; c += u4 * R1;
    VERIFY_BITS(t4, 26);
    VERIFY_BITS(c, 39);
    /* [d u4 0 0 0 0 t9 0 0 0 c-u4*R1 t4-u4*R0 t3 t2 t1 t0] = [p14 p13 p12 p11 p10 p9 0 0 0 0 p4 p3 p2 p1 p0] */
    /* [d 0 0 0 0 0 t9 0 0 0 c t4 t3 t2 t1 t0] = [p14 p13 p12 p11 p10 p9 0 0 0 0 p4 p3 p2 p1 p0] */

    c += (uint64_t)a[0] * b[5]
       + (uint64_t)a[1] * b[4]
       + (uint64_t)a[2] * b[3]
       + (uint64_t)a[3] * b[2]
       + (uint64_t)a[4] * b[1]
       + (uint64_t)a[5] * b[0];
    VERIFY_BITS(c, 63);
    /* [d 0 0 0 0 0 t9 0 0 0 c t4 t3 t2 t1 t0] = [p14 p13 p12 p11 p10 p9 0 0 0 p5 p4 p3 p2 p1 p0] */
    d += (uint64_t)a[6] * b[9]
       + (uint64_t)a[7] * b[8]
       + (uint64_t)a[8] * b[7]
       + (uint64_t)a[9] * b[6];
    VERIFY_BITS(d, 62);
    /* [d 0 0 0 0 0 t9 0 0 0 c t4 t3 t2 t1 t0] = [p15 p14 p13 p12 p11 p10 p9 0 0 0 p5 p4 p3 p2 p1 p0] */
    u5 = d & M; d >>= 26; c += u5 * R0;
    VERIFY_BITS(u5, 26);
    VERIFY_BITS(d, 36);
    /* VERIFY_BITS(c, 64); */
    /* [d u5 0 0 0 0 0 t9 0 0 0 c-u5*R0 t4 t3 t2 t1 t0] = [p15 p14 p13 p12 p11 p10 p9 0 0 0 p5 p4 p3 p2 p1 p0] */
    t5 = c & M; c >>= 26; c += u5 * R1;
    VERIFY_BITS(t5, 26);
    VERIFY_BITS(c, 39);
    /* [d u5 0 0 0 0 0 t9 0 0 c-u5*R1 t5-u5*R0 t4 t3 t2 t1 t0] = [p15 p14 p13 p12 p11 p10 p9 0 0 0 p5 p4 p3 p2 p1 p0] */
    /* [d 0 0 0 0 0 0 t9 0 0 c t5 t4 t3 t2 t1 t0] = [p15 p14 p13 p12 p11 p10 p9 0 0 0 p5 p4 p3 p2 p1 p0] */

    c += (uint64_t)a[0] * b[6]
       + (uint64_t)a[1] * b[5]
       + (uint64_t)a[2] * b[4]
       + (uint64_t)a[3] * b[3]
       + (uint64_t)a[4] * b[2]
       + (uint64_t)a[5] * b[1]
       + (uint64_t)a[6] * b[0];
    VERIFY_BITS(c, 63);
    /* [d 0 0 0 0 0 0 t9 0 0 c t5 t4 t3 t2 t1 t0] = [p15 p14 p13 p12 p11 p10 p9 0 0 p6 p5 p4 p3 p2 p1 p0] */
    d += (uint64_t)a[7] * b[9]
       + (uint64_t)a[8] * b[8]
       + (uint64_t)a[9] * b[7];
    VERIFY_BITS(d, 61);
    /* [d 0 0 0 0 0 0 t9 0 0 c t5 t4 t3 t2 t1 t0] = [p16 p15 p14 p13 p12 p11 p10 p9 0 0 p6 p5 p4 p3 p2 p1 p0] */
    u6 = d & M; d >>= 26; c += u6 * R0;
    VERIFY_BITS(u6, 26);
    VERIFY_BITS(d, 35);
    /* VERIFY_BITS(c, 64); */
    /* [d u6 0 0 0 0 0 0 t9 0 0 c-u6*R0 t5 t4 t3 t2 t1 t0] = [p16 p15 p14 p13 p12 p11 p10 p9 0 0 p6 p5 p4 p3 p2 p1 p0] */
    t6 = c & M; c >>= 26; c += u6 * R1;
    VERIFY_BITS(t6, 26);
    VERIFY_BITS(c, 39);
    /* [d u6 0 0 0 0 0 0 t9 0 c-u6*R1 t6-u6*R0 t5 t4 t3 t2 t1 t0] = [p16 p15 p14 p13 p12 p11 p10 p9 0 0 p6 p5 p4 p3 p2 p1 p0] */
    /* [d 0 0 0 0 0 0 0 t9 0 c t6 t5 t4 t3 t2 t1 t0] = [p16 p15 p14 p13 p12 p11 p10 p9 0 0 p6 p5 p4 p3 p2 p1 p0] */

    c += (uint64_t)a[0] * b[7]
       + (uint64_t)a[1] * b[6]
       + (uint64_t)a[2] * b[5]
       + (uint64_t)a[3] * b[4]
       + (uint64_t)a[4] * b[3]
       + (uint64_t)a[5] * b[2]
       + (uint64_t)a[6] * b[1]
       + (uint64_t)a[7] * b[0];
    /* VERIFY_BITS(c, 64); */
    VERIFY_CHECK(c <= 0x8000007C00000007ULL);
    /* [d 0 0 0 0 0 0 0 t9 0 c t6 t5 t4 t3 t2 t1 t0] = [p16 p15 p14 p13 p12 p11 p10 p9 0 p7 p6 p5 p4 p3 p2 p1 p0] */
    d += (uint64_t)a[8] * b[9]
       + (uint64_t)a[9] * b[8];
    VERIFY_BITS(d, 58);
    /* [d 0 0 0 0 0 0 0 t9 0 c t6 t5 t4 t3 t2 t1 t0] = [p17 p16 p15 p14 p13 p12 p11 p10 p9 0 p7 p6 p5 p4 p3 p2 p1 p0] */
    u7 = d & M; d >>= 26; c += u7 * R0;
    VERIFY_BITS(u7, 26);
    VERIFY_BITS(d, 32);
    /* VERIFY_BITS(c, 64); */
    VERIFY_CHECK(c <= 0x800001703FFFC2F7ULL);
    /* [d u7 0 0 0 0 0 0 0 t9 0 c-u7*R0 t6 t5 t4 t3 t2 t1 t0] = [p17 p16 p15 p14 p13 p12 p11 p10 p9 0 p7 p6 p5 p4 p3 p2 p1 p0] */
    t7 = c & M; c >>= 26; c += u7 * R1;
    VERIFY_BITS(t7, 26);
    VERIFY_BITS(c, 38);
    /* [d u7 0 0 0 0 0 0 0 t9 c-u7*R1 t7-u7*R0 t6 t5 t4 t3 t2 t1 t0] = [p17 p16 p15 p14 p13 p12 p11 p10 p9 0 p7 p6 p5 p4 p3 p2 p1 p0] */
    /* [d 0 0 0 0 0 0 0 0 t9 c t7 t6 t5 t4 t3 t2 t1 t0] = [p17 p16 p15 p14 p13 p12 p11 p10 p9 0 p7 p6 p5 p4 p3 p2 p1 p0] */

    c += (uint64_t)a[0] * b[8]
       + (uint64_t)a[1] * b[7]
       + (uint64_t)a[2] * b[6]
       + (uint64_t)a[3] * b[5]
       + (uint64_t)a[4] * b[4]
       + (uint64_t)a[5] * b[3]
       + (uint64_t)a[6] * b[2]
       + (uint64_t)a[7] * b[1]
       + (uint64_t)a[8] * b[0];
    /* VERIFY_BITS(c, 64); */
    VERIFY_CHECK(c <= 0x9000007B80000008ULL);
    /* [d 0 0 0 0 0 0 0 0 t9 c t7 t6 t5 t4 t3 t2 t1 t0] = [p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
    d += (uint64_t)a[9] * b[9];
    VERIFY_BITS(d, 57);
    /* [d 0 0 0 0 0 0 0 0 t9 c t7 t6 t5 t4 t3 t2 t1 t0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
    u8 = d & M; d >>= 26; c += u8 * R0;
    VERIFY_BITS(u8, 26);
    VERIFY_BITS(d, 31);
    /* VERIFY_BITS(c, 64); */
    VERIFY_CHECK(c <= 0x9000016FBFFFC2F8ULL);
    /* [d u8 0 0 0 0 0 0 0 0 t9 c-u8*R0 t7 t6 t5 t4 t3 t2 t1 t0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */

    r[3] = t3;
    VERIFY_BITS(r[3], 26);
    /* [d u8 0 0 0 0 0 0 0 0 t9 c-u8*R0 t7 t6 t5 t4 r3 t2 t1 t0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
    r[4] = t4;
    VERIFY_BITS(r[4], 26);
    /* [d u8 0 0 0 0 0 0 0 0 t9 c-u8*R0 t7 t6 t5 r4 r3 t2 t1 t0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
    r[5] = t5;
    VERIFY_BITS(r[5], 26);
    /* [d u8 0 0 0 0 0 0 0 0 t9 c-u8*R0 t7 t6 r5 r4 r3 t2 t1 t0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
    r[6] = t6;
    VERIFY_BITS(r[6], 26);
    /* [d u8 0 0 0 0 0 0 0 0 t9 c-u8*R0 t7 r6 r5 r4 r3 t2 t1 t0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
    r[7] = t7;
    VERIFY_BITS(r[7], 26);
    /* [d u8 0 0 0 0 0 0 0 0 t9 c-u8*R0 r7 r6 r5 r4 r3 t2 t1 t0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */

    r[8] = c & M; c >>= 26; c += u8 * R1;
    VERIFY_BITS(r[8], 26);
    VERIFY_BITS(c, 39);
    /* [d u8 0 0 0 0 0 0 0 0 t9+c-u8*R1 r8-u8*R0 r7 r6 r5 r4 r3 t2 t1 t0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
    /* [d 0 0 0 0 0 0 0 0 0 t9+c r8 r7 r6 r5 r4 r3 t2 t1 t0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
    c   += d * R0 + t9;
    VERIFY_BITS(c, 45);
    /* [d 0 0 0 0 0 0 0 0 0 c-d*R0 r8 r7 r6 r5 r4 r3 t2 t1 t0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
    r[9] = c & (M >> 4); c >>= 22; c += d * (R1 << 4);
    VERIFY_BITS(r[9], 22);
    VERIFY_BITS(c, 46);
    /* [d 0 0 0 0 0 0 0 0 r9+((c-d*R1<<4)<<22)-d*R0 r8 r7 r6 r5 r4 r3 t2 t1 t0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
    /* [d 0 0 0 0 0 0 0 -d*R1 r9+(c<<22)-d*R0 r8 r7 r6 r5 r4 r3 t2 t1 t0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
    /* [r9+(c<<22) r8 r7 r6 r5 r4 r3 t2 t1 t0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */

    d    = c * (R0 >> 4) + t0;
    VERIFY_BITS(d, 56);
    /* [r9+(c<<22) r8 r7 r6 r5 r4 r3 t2 t1 d-c*R0>>4] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
    r[0] = d & M; d >>= 26;
    VERIFY_BITS(r[0], 26);
    VERIFY_BITS(d, 30);
    /* [r9+(c<<22) r8 r7 r6 r5 r4 r3 t2 t1+d r0-c*R0>>4] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
    d   += c * (R1 >> 4) + t1;
    VERIFY_BITS(d, 53);
    VERIFY_CHECK(d <= 0x10000003FFFFBFULL);
    /* [r9+(c<<22) r8 r7 r6 r5 r4 r3 t2 d-c*R1>>4 r0-c*R0>>4] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
    /* [r9 r8 r7 r6 r5 r4 r3 t2 d r0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
    r[1] = d & M; d >>= 26;
    VERIFY_BITS(r[1], 26);
    VERIFY_BITS(d, 27);
    VERIFY_CHECK(d <= 0x4000000ULL);
    /* [r9 r8 r7 r6 r5 r4 r3 t2+d r1 r0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
    d   += t2;
    VERIFY_BITS(d, 27);
    /* [r9 r8 r7 r6 r5 r4 r3 d r1 r0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
    r[2] = d;
    VERIFY_BITS(r[2], 27);
    /* [r9 r8 r7 r6 r5 r4 r3 r2 r1 r0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
}

SECP256K1_INLINE static void secp256k1_fe_sqr_inner(uint32_t *r, const uint32_t *a) {
    uint64_t c, d;
    uint64_t u0, u1, u2, u3, u4, u5, u6, u7, u8;
    uint32_t t9, t0, t1, t2, t3, t4, t5, t6, t7;
    const uint32_t M = 0x3FFFFFFUL, R0 = 0x3D10UL, R1 = 0x400UL;

    VERIFY_BITS(a[0], 30);
    VERIFY_BITS(a[1], 30);
    VERIFY_BITS(a[2], 30);
    VERIFY_BITS(a[3], 30);
    VERIFY_BITS(a[4], 30);
    VERIFY_BITS(a[5], 30);
    VERIFY_BITS(a[6], 30);
    VERIFY_BITS(a[7], 30);
    VERIFY_BITS(a[8], 30);
    VERIFY_BITS(a[9], 26);

    /** [... a b c] is a shorthand for ... + a<<52 + b<<26 + c<<0 mod n.
     *  px is a shorthand for sum(a[i]*a[x-i], i=0..x).
     *  Note that [x 0 0 0 0 0 0 0 0 0 0] = [x*R1 x*R0].
     */

    d  = (uint64_t)(a[0]*2) * a[9]
       + (uint64_t)(a[1]*2) * a[8]
       + (uint64_t)(a[2]*2) * a[7]
       + (uint64_t)(a[3]*2) * a[6]
       + (uint64_t)(a[4]*2) * a[5];
    /* VERIFY_BITS(d, 64); */
    /* [d 0 0 0 0 0 0 0 0 0] = [p9 0 0 0 0 0 0 0 0 0] */
    t9 = d & M; d >>= 26;
    VERIFY_BITS(t9, 26);
    VERIFY_BITS(d, 38);
    /* [d t9 0 0 0 0 0 0 0 0 0] = [p9 0 0 0 0 0 0 0 0 0] */

    c  = (uint64_t)a[0] * a[0];
    VERIFY_BITS(c, 60);
    /* [d t9 0 0 0 0 0 0 0 0 c] = [p9 0 0 0 0 0 0 0 0 p0] */
    d += (uint64_t)(a[1]*2) * a[9]
       + (uint64_t)(a[2]*2) * a[8]
       + (uint64_t)(a[3]*2) * a[7]
       + (uint64_t)(a[4]*2) * a[6]
       + (uint64_t)a[5] * a[5];
    VERIFY_BITS(d, 63);
    /* [d t9 0 0 0 0 0 0 0 0 c] = [p10 p9 0 0 0 0 0 0 0 0 p0] */
    u0 = d & M; d >>= 26; c += u0 * R0;
    VERIFY_BITS(u0, 26);
    VERIFY_BITS(d, 37);
    VERIFY_BITS(c, 61);
    /* [d u0 t9 0 0 0 0 0 0 0 0 c-u0*R0] = [p10 p9 0 0 0 0 0 0 0 0 p0] */
    t0 = c & M; c >>= 26; c += u0 * R1;
    VERIFY_BITS(t0, 26);
    VERIFY_BITS(c, 37);
    /* [d u0 t9 0 0 0 0 0 0 0 c-u0*R1 t0-u0*R0] = [p10 p9 0 0 0 0 0 0 0 0 p0] */
    /* [d 0 t9 0 0 0 0 0 0 0 c t0] = [p10 p9 0 0 0 0 0 0 0 0 p0] */

    c += (uint64_t)(a[0]*2) * a[1];
    VERIFY_BITS(c, 62);
    /* [d 0 t9 0 0 0 0 0 0 0 c t0] = [p10 p9 0 0 0 0 0 0 0 p1 p0] */
    d += (uint64_t)(a[2]*2) * a[9]
       + (uint64_t)(a[3]*2) * a[8]
       + (uint64_t)(a[4]*2) * a[7]
       + (uint64_t)(a[5]*2) * a[6];
    VERIFY_BITS(d, 63);
    /* [d 0 t9 0 0 0 0 0 0 0 c t0] = [p11 p10 p9 0 0 0 0 0 0 0 p1 p0] */
    u1 = d & M; d >>= 26; c += u1 * R0;
    VERIFY_BITS(u1, 26);
    VERIFY_BITS(d, 37);
    VERIFY_BITS(c, 63);
    /* [d u1 0 t9 0 0 0 0 0 0 0 c-u1*R0 t0] = [p11 p10 p9 0 0 0 0 0 0 0 p1 p0] */
    t1 = c & M; c >>= 26; c += u1 * R1;
    VERIFY_BITS(t1, 26);
    VERIFY_BITS(c, 38);
    /* [d u1 0 t9 0 0 0 0 0 0 c-u1*R1 t1-u1*R0 t0] = [p11 p10 p9 0 0 0 0 0 0 0 p1 p0] */
    /* [d 0 0 t9 0 0 0 0 0 0 c t1 t0] = [p11 p10 p9 0 0 0 0 0 0 0 p1 p0] */

    c += (uint64_t)(a[0]*2) * a[2]
       + (uint64_t)a[1] * a[1];
    VERIFY_BITS(c, 62);
    /* [d 0 0 t9 0 0 0 0 0 0 c t1 t0] = [p11 p10 p9 0 0 0 0 0 0 p2 p1 p0] */
    d += (uint64_t)(a[3]*2) * a[9]
       + (uint64_t)(a[4]*2) * a[8]
       + (uint64_t)(a[5]*2) * a[7]
       + (uint64_t)a[6] * a[6];
    VERIFY_BITS(d, 63);
    /* [d 0 0 t9 0 0 0 0 0 0 c t1 t0] = [p12 p11 p10 p9 0 0 0 0 0 0 p2 p1 p0] */
    u2 = d & M; d >>= 26; c += u2 * R0;
    VERIFY_BITS(u2, 26);
    VERIFY_BITS(d, 37);
    VERIFY_BITS(c, 63);
    /* [d u2 0 0 t9 0 0 0 0 0 0 c-u2*R0 t1 t0] = [p12 p11 p10 p9 0 0 0 0 0 0 p2 p1 p0] */
    t2 = c & M; c >>= 26; c += u2 * R1;
    VERIFY_BITS(t2, 26);
    VERIFY_BITS(c, 38);
    /* [d u2 0 0 t9 0 0 0 0 0 c-u2*R1 t2-u2*R0 t1 t0] = [p12 p11 p10 p9 0 0 0 0 0 0 p2 p1 p0] */
    /* [d 0 0 0 t9 0 0 0 0 0 c t2 t1 t0] = [p12 p11 p10 p9 0 0 0 0 0 0 p2 p1 p0] */

    c += (uint64_t)(a[0]*2) * a[3]
       + (uint64_t)(a[1]*2) * a[2];
    VERIFY_BITS(c, 63);
    /* [d 0 0 0 t9 0 0 0 0 0 c t2 t1 t0] = [p12 p11 p10 p9 0 0 0 0 0 p3 p2 p1 p0] */
    d += (uint64_t)(a[4]*2) * a[9]
       + (uint64_t)(a[5]*2) * a[8]
       + (uint64_t)(a[6]*2) * a[7];
    VERIFY_BITS(d, 63);
    /* [d 0 0 0 t9 0 0 0 0 0 c t2 t1 t0] = [p13 p12 p11 p10 p9 0 0 0 0 0 p3 p2 p1 p0] */
    u3 = d & M; d >>= 26; c += u3 * R0;
    VERIFY_BITS(u3, 26);
    VERIFY_BITS(d, 37);
    /* VERIFY_BITS(c, 64); */
    /* [d u3 0 0 0 t9 0 0 0 0 0 c-u3*R0 t2 t1 t0] = [p13 p12 p11 p10 p9 0 0 0 0 0 p3 p2 p1 p0] */
    t3 = c & M; c >>= 26; c += u3 * R1;
    VERIFY_BITS(t3, 26);
    VERIFY_BITS(c, 39);
    /* [d u3 0 0 0 t9 0 0 0 0 c-u3*R1 t3-u3*R0 t2 t1 t0] = [p13 p12 p11 p10 p9 0 0 0 0 0 p3 p2 p1 p0] */
    /* [d 0 0 0 0 t9 0 0 0 0 c t3 t2 t1 t0] = [p13 p12 p11 p10 p9 0 0 0 0 0 p3 p2 p1 p0] */

    c += (uint64_t)(a[0]*2) * a[4]
       + (uint64_t)(a[1]*2) * a[3]
       + (uint64_t)a[2] * a[2];
    VERIFY_BITS(c, 63);
    /* [d 0 0 0 0 t9 0 0 0 0 c t3 t2 t1 t0] = [p13 p12 p11 p10 p9 0 0 0 0 p4 p3 p2 p1 p0] */
    d += (uint64_t)(a[5]*2) * a[9]
       + (uint64_t)(a[6]*2) * a[8]
       + (uint64_t)a[7] * a[7];
    VERIFY_BITS(d, 62);
    /* [d 0 0 0 0 t9 0 0 0 0 c t3 t2 t1 t0] = [p14 p13 p12 p11 p10 p9 0 0 0 0 p4 p3 p2 p1 p0] */
    u4 = d & M; d >>= 26; c += u4 * R0;
    VERIFY_BITS(u4, 26);
    VERIFY_BITS(d, 36);
    /* VERIFY_BITS(c, 64); */
    /* [d u4 0 0 0 0 t9 0 0 0 0 c-u4*R0 t3 t2 t1 t0] = [p14 p13 p12 p11 p10 p9 0 0 0 0 p4 p3 p2 p1 p0] */
    t4 = c & M; c >>= 26; c += u4 * R1;
    VERIFY_BITS(t4, 26);
    VERIFY_BITS(c, 39);
    /* [d u4 0 0 0 0 t9 0 0 0 c-u4*R1 t4-u4*R0 t3 t2 t1 t0] = [p14 p13 p12 p11 p10 p9 0 0 0 0 p4 p3 p2 p1 p0] */
    /* [d 0 0 0 0 0 t9 0 0 0 c t4 t3 t2 t1 t0] = [p14 p13 p12 p11 p10 p9 0 0 0 0 p4 p3 p2 p1 p0] */

    c += (uint64_t)(a[0]*2) * a[5]
       + (uint64_t)(a[1]*2) * a[4]
       + (uint64_t)(a[2]*2) * a[3];
    VERIFY_BITS(c, 63);
    /* [d 0 0 0 0 0 t9 0 0 0 c t4 t3 t2 t1 t0] = [p14 p13 p12 p11 p10 p9 0 0 0 p5 p4 p3 p2 p1 p0] */
    d += (uint64_t)(a[6]*2) * a[9]
       + (uint64_t)(a[7]*2) * a[8];
    VERIFY_BITS(d, 62);
    /* [d 0 0 0 0 0 t9 0 0 0 c t4 t3 t2 t1 t0] = [p15 p14 p13 p12 p11 p10 p9 0 0 0 p5 p4 p3 p2 p1 p0] */
    u5 = d & M; d >>= 26; c += u5 * R0;
    VERIFY_BITS(u5, 26);
    VERIFY_BITS(d, 36);
    /* VERIFY_BITS(c, 64); */
    /* [d u5 0 0 0 0 0 t9 0 0 0 c-u5*R0 t4 t3 t2 t1 t0] = [p15 p14 p13 p12 p11 p10 p9 0 0 0 p5 p4 p3 p2 p1 p0] */
    t5 = c & M; c >>= 26; c += u5 * R1;
    VERIFY_BITS(t5, 26);
    VERIFY_BITS(c, 39);
    /* [d u5 0 0 0 0 0 t9 0 0 c-u5*R1 t5-u5*R0 t4 t3 t2 t1 t0] = [p15 p14 p13 p12 p11 p10 p9 0 0 0 p5 p4 p3 p2 p1 p0] */
    /* [d 0 0 0 0 0 0 t9 0 0 c t5 t4 t3 t2 t1 t0] = [p15 p14 p13 p12 p11 p10 p9 0 0 0 p5 p4 p3 p2 p1 p0] */

    c += (uint64_t)(a[0]*2) * a[6]
       + (uint64_t)(a[1]*2) * a[5]
       + (uint64_t)(a[2]*2) * a[4]
       + (uint64_t)a[3] * a[3];
    VERIFY_BITS(c, 63);
    /* [d 0 0 0 0 0 0 t9 0 0 c t5 t4 t3 t2 t1 t0] = [p15 p14 p13 p12 p11 p10 p9 0 0 p6 p5 p4 p3 p2 p1 p0] */
    d += (uint64_t)(a[7]*2) * a[9]
       + (uint64_t)a[8] * a[8];
    VERIFY_BITS(d, 61);
    /* [d 0 0 0 0 0 0 t9 0 0 c t5 t4 t3 t2 t1 t0] = [p16 p15 p14 p13 p12 p11 p10 p9 0 0 p6 p5 p4 p3 p2 p1 p0] */
    u6 = d & M; d >>= 26; c += u6 * R0;
    VERIFY_BITS(u6, 26);
    VERIFY_BITS(d, 35);
    /* VERIFY_BITS(c, 64); */
    /* [d u6 0 0 0 0 0 0 t9 0 0 c-u6*R0 t5 t4 t3 t2 t1 t0] = [p16 p15 p14 p13 p12 p11 p10 p9 0 0 p6 p5 p4 p3 p2 p1 p0] */
    t6 = c & M; c >>= 26; c += u6 * R1;
    VERIFY_BITS(t6, 26);
    VERIFY_BITS(c, 39);
    /* [d u6 0 0 0 0 0 0 t9 0 c-u6*R1 t6-u6*R0 t5 t4 t3 t2 t1 t0] = [p16 p15 p14 p13 p12 p11 p10 p9 0 0 p6 p5 p4 p3 p2 p1 p0] */
    /* [d 0 0 0 0 0 0 0 t9 0 c t6 t5 t4 t3 t2 t1 t0] = [p16 p15 p14 p13 p12 p11 p10 p9 0 0 p6 p5 p4 p3 p2 p1 p0] */

    c += (uint64_t)(a[0]*2) * a[7]
       + (uint64_t)(a[1]*2) * a[6]
       + (uint64_t)(a[2]*2) * a[5]
       + (uint64_t)(a[3]*2) * a[4];
    /* VERIFY_BITS(c, 64); */
    VERIFY_CHECK(c <= 0x8000007C00000007ULL);
    /* [d 0 0 0 0 0 0 0 t9 0 c t6 t5 t4 t3 t2 t1 t0] = [p16 p15 p14 p13 p12 p11 p10 p9 0 p7 p6 p5 p4 p3 p2 p1 p0] */
    d += (uint64_t)(a[8]*2) * a[9];
    VERIFY_BITS(d, 58);
    /* [d 0 0 0 0 0 0 0 t9 0 c t6 t5 t4 t3 t2 t1 t0] = [p17 p16 p15 p14 p13 p12 p11 p10 p9 0 p7 p6 p5 p4 p3 p2 p1 p0] */
    u7 = d & M; d >>= 26; c += u7 * R0;
    VERIFY_BITS(u7, 26);
    VERIFY_BITS(d, 32);
    /* VERIFY_BITS(c, 64); */
    VERIFY_CHECK(c <= 0x800001703FFFC2F7ULL);
    /* [d u7 0 0 0 0 0 0 0 t9 0 c-u7*R0 t6 t5 t4 t3 t2 t1 t0] = [p17 p16 p15 p14 p13 p12 p11 p10 p9 0 p7 p6 p5 p4 p3 p2 p1 p0] */
    t7 = c & M; c >>= 26; c += u7 * R1;
    VERIFY_BITS(t7, 26);
    VERIFY_BITS(c, 38);
    /* [d u7 0 0 0 0 0 0 0 t9 c-u7*R1 t7-u7*R0 t6 t5 t4 t3 t2 t1 t0] = [p17 p16 p15 p14 p13 p12 p11 p10 p9 0 p7 p6 p5 p4 p3 p2 p1 p0] */
    /* [d 0 0 0 0 0 0 0 0 t9 c t7 t6 t5 t4 t3 t2 t1 t0] = [p17 p16 p15 p14 p13 p12 p11 p10 p9 0 p7 p6 p5 p4 p3 p2 p1 p0] */

    c += (uint64_t)(a[0]*2) * a[8]
       + (uint64_t)(a[1]*2) * a[7]
       + (uint64_t)(a[2]*2) * a[6]
       + (uint64_t)(a[3]*2) * a[5]
       + (uint64_t)a[4] * a[4];
    /* VERIFY_BITS(c, 64); */
    VERIFY_CHECK(c <= 0x9000007B80000008ULL);
    /* [d 0 0 0 0 0 0 0 0 t9 c t7 t6 t5 t4 t3 t2 t1 t0] = [p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
    d += (uint64_t)a[9] * a[9];
    VERIFY_BITS(d, 57);
    /* [d 0 0 0 0 0 0 0 0 t9 c t7 t6 t5 t4 t3 t2 t1 t0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
    u8 = d & M; d >>= 26; c += u8 * R0;
    VERIFY_BITS(u8, 26);
    VERIFY_BITS(d, 31);
    /* VERIFY_BITS(c, 64); */
    VERIFY_CHECK(c <= 0x9000016FBFFFC2F8ULL);
    /* [d u8 0 0 0 0 0 0 0 0 t9 c-u8*R0 t7 t6 t5 t4 t3 t2 t1 t0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */

    r[3] = t3;
    VERIFY_BITS(r[3], 26);
    /* [d u8 0 0 0 0 0 0 0 0 t9 c-u8*R0 t7 t6 t5 t4 r3 t2 t1 t0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
    r[4] = t4;
    VERIFY_BITS(r[4], 26);
    /* [d u8 0 0 0 0 0 0 0 0 t9 c-u8*R0 t7 t6 t5 r4 r3 t2 t1 t0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
    r[5] = t5;
    VERIFY_BITS(r[5], 26);
    /* [d u8 0 0 0 0 0 0 0 0 t9 c-u8*R0 t7 t6 r5 r4 r3 t2 t1 t0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
    r[6] = t6;
    VERIFY_BITS(r[6], 26);
    /* [d u8 0 0 0 0 0 0 0 0 t9 c-u8*R0 t7 r6 r5 r4 r3 t2 t1 t0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
    r[7] = t7;
    VERIFY_BITS(r[7], 26);
    /* [d u8 0 0 0 0 0 0 0 0 t9 c-u8*R0 r7 r6 r5 r4 r3 t2 t1 t0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */

    r[8] = c & M; c >>= 26; c += u8 * R1;
    VERIFY_BITS(r[8], 26);
    VERIFY_BITS(c, 39);
    /* [d u8 0 0 0 0 0 0 0 0 t9+c-u8*R1 r8-u8*R0 r7 r6 r5 r4 r3 t2 t1 t0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
    /* [d 0 0 0 0 0 0 0 0 0 t9+c r8 r7 r6 r5 r4 r3 t2 t1 t0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
    c   += d * R0 + t9;
    VERIFY_BITS(c, 45);
    /* [d 0 0 0 0 0 0 0 0 0 c-d*R0 r8 r7 r6 r5 r4 r3 t2 t1 t0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
    r[9] = c & (M >> 4); c >>= 22; c += d * (R1 << 4);
    VERIFY_BITS(r[9], 22);
    VERIFY_BITS(c, 46);
    /* [d 0 0 0 0 0 0 0 0 r9+((c-d*R1<<4)<<22)-d*R0 r8 r7 r6 r5 r4 r3 t2 t1 t0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
    /* [d 0 0 0 0 0 0 0 -d*R1 r9+(c<<22)-d*R0 r8 r7 r6 r5 r4 r3 t2 t1 t0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
    /* [r9+(c<<22) r8 r7 r6 r5 r4 r3 t2 t1 t0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */

    d    = c * (R0 >> 4) + t0;
    VERIFY_BITS(d, 56);
    /* [r9+(c<<22) r8 r7 r6 r5 r4 r3 t2 t1 d-c*R0>>4] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
    r[0] = d & M; d >>= 26;
    VERIFY_BITS(r[0], 26);
    VERIFY_BITS(d, 30);
    /* [r9+(c<<22) r8 r7 r6 r5 r4 r3 t2 t1+d r0-c*R0>>4] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
    d   += c * (R1 >> 4) + t1;
    VERIFY_BITS(d, 53);
    VERIFY_CHECK(d <= 0x10000003FFFFBFULL);
    /* [r9+(c<<22) r8 r7 r6 r5 r4 r3 t2 d-c*R1>>4 r0-c*R0>>4] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
    /* [r9 r8 r7 r6 r5 r4 r3 t2 d r0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
    r[1] = d & M; d >>= 26;
    VERIFY_BITS(r[1], 26);
    VERIFY_BITS(d, 27);
    VERIFY_CHECK(d <= 0x4000000ULL);
    /* [r9 r8 r7 r6 r5 r4 r3 t2+d r1 r0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
    d   += t2;
    VERIFY_BITS(d, 27);
    /* [r9 r8 r7 r6 r5 r4 r3 d r1 r0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
    r[2] = d;
    VERIFY_BITS(r[2], 27);
    /* [r9 r8 r7 r6 r5 r4 r3 r2 r1 r0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
}
#endif

SECP256K1_INLINE static void secp256k1_fe_impl_mul(secp256k1_fe *r, const secp256k1_fe *a, const secp256k1_fe * SECP256K1_RESTRICT b) {
    secp256k1_fe_mul_inner(r->n, a->n, b->n);
}

SECP256K1_INLINE static void secp256k1_fe_impl_sqr(secp256k1_fe *r, const secp256k1_fe *a) {
    secp256k1_fe_sqr_inner(r->n, a->n);
}

SECP256K1_INLINE static void secp256k1_fe_impl_cmov(secp256k1_fe *r, const secp256k1_fe *a, int flag) {
    uint32_t mask0, mask1;
    volatile int vflag = flag;
    SECP256K1_CHECKMEM_CHECK_VERIFY(r->n, sizeof(r->n));
    mask0 = vflag + ~((uint32_t)0);
    mask1 = ~mask0;
    r->n[0] = (r->n[0] & mask0) | (a->n[0] & mask1);
    r->n[1] = (r->n[1] & mask0) | (a->n[1] & mask1);
    r->n[2] = (r->n[2] & mask0) | (a->n[2] & mask1);
    r->n[3] = (r->n[3] & mask0) | (a->n[3] & mask1);
    r->n[4] = (r->n[4] & mask0) | (a->n[4] & mask1);
    r->n[5] = (r->n[5] & mask0) | (a->n[5] & mask1);
    r->n[6] = (r->n[6] & mask0) | (a->n[6] & mask1);
    r->n[7] = (r->n[7] & mask0) | (a->n[7] & mask1);
    r->n[8] = (r->n[8] & mask0) | (a->n[8] & mask1);
    r->n[9] = (r->n[9] & mask0) | (a->n[9] & mask1);
}

static SECP256K1_INLINE void secp256k1_fe_impl_half(secp256k1_fe *r) {
    uint32_t t0 = r->n[0], t1 = r->n[1], t2 = r->n[2], t3 = r->n[3], t4 = r->n[4],
             t5 = r->n[5], t6 = r->n[6], t7 = r->n[7], t8 = r->n[8], t9 = r->n[9];
    uint32_t one = (uint32_t)1;
    uint32_t mask = -(t0 & one) >> 6;

    /* Bounds analysis (over the rationals).
     *
     * Let m = r->magnitude
     *     C = 0x3FFFFFFUL * 2
     *     D = 0x03FFFFFUL * 2
     *
     * Initial bounds: t0..t8 <= C * m
     *                     t9 <= D * m
     */

    t0 += 0x3FFFC2FUL & mask;
    t1 += 0x3FFFFBFUL & mask;
    t2 += mask;
    t3 += mask;
    t4 += mask;
    t5 += mask;
    t6 += mask;
    t7 += mask;
    t8 += mask;
    t9 += mask >> 4;

    VERIFY_CHECK((t0 & one) == 0);

    /* t0..t8: added <= C/2
     *     t9: added <= D/2
     *
     * Current bounds: t0..t8 <= C * (m + 1/2)
     *                     t9 <= D * (m + 1/2)
     */

    r->n[0] = (t0 >> 1) + ((t1 & one) << 25);
    r->n[1] = (t1 >> 1) + ((t2 & one) << 25);
    r->n[2] = (t2 >> 1) + ((t3 & one) << 25);
    r->n[3] = (t3 >> 1) + ((t4 & one) << 25);
    r->n[4] = (t4 >> 1) + ((t5 & one) << 25);
    r->n[5] = (t5 >> 1) + ((t6 & one) << 25);
    r->n[6] = (t6 >> 1) + ((t7 & one) << 25);
    r->n[7] = (t7 >> 1) + ((t8 & one) << 25);
    r->n[8] = (t8 >> 1) + ((t9 & one) << 25);
    r->n[9] = (t9 >> 1);

    /* t0..t8: shifted right and added <= C/4 + 1/2
     *     t9: shifted right
     *
     * Current bounds: t0..t8 <= C * (m/2 + 1/2)
     *                     t9 <= D * (m/2 + 1/4)
     *
     * Therefore the output magnitude (M) has to be set such that:
     *     t0..t8: C * M >= C * (m/2 + 1/2)
     *         t9: D * M >= D * (m/2 + 1/4)
     *
     * It suffices for all limbs that, for any input magnitude m:
     *     M >= m/2 + 1/2
     *
     * and since we want the smallest such integer value for M:
     *     M == floor(m/2) + 1
     */
}

static SECP256K1_INLINE void secp256k1_fe_storage_cmov(secp256k1_fe_storage *r, const secp256k1_fe_storage *a, int flag) {
    uint32_t mask0, mask1;
    volatile int vflag = flag;
    SECP256K1_CHECKMEM_CHECK_VERIFY(r->n, sizeof(r->n));
    mask0 = vflag + ~((uint32_t)0);
    mask1 = ~mask0;
    r->n[0] = (r->n[0] & mask0) | (a->n[0] & mask1);
    r->n[1] = (r->n[1] & mask0) | (a->n[1] & mask1);
    r->n[2] = (r->n[2] & mask0) | (a->n[2] & mask1);
    r->n[3] = (r->n[3] & mask0) | (a->n[3] & mask1);
    r->n[4] = (r->n[4] & mask0) | (a->n[4] & mask1);
    r->n[5] = (r->n[5] & mask0) | (a->n[5] & mask1);
    r->n[6] = (r->n[6] & mask0) | (a->n[6] & mask1);
    r->n[7] = (r->n[7] & mask0) | (a->n[7] & mask1);
}

static void secp256k1_fe_impl_to_storage(secp256k1_fe_storage *r, const secp256k1_fe *a) {
    r->n[0] = a->n[0] | a->n[1] << 26;
    r->n[1] = a->n[1] >> 6 | a->n[2] << 20;
    r->n[2] = a->n[2] >> 12 | a->n[3] << 14;
    r->n[3] = a->n[3] >> 18 | a->n[4] << 8;
    r->n[4] = a->n[4] >> 24 | a->n[5] << 2 | a->n[6] << 28;
    r->n[5] = a->n[6] >> 4 | a->n[7] << 22;
    r->n[6] = a->n[7] >> 10 | a->n[8] << 16;
    r->n[7] = a->n[8] >> 16 | a->n[9] << 10;
}

static SECP256K1_INLINE void secp256k1_fe_impl_from_storage(secp256k1_fe *r, const secp256k1_fe_storage *a) {
    r->n[0] = a->n[0] & 0x3FFFFFFUL;
    r->n[1] = a->n[0] >> 26 | ((a->n[1] << 6) & 0x3FFFFFFUL);
    r->n[2] = a->n[1] >> 20 | ((a->n[2] << 12) & 0x3FFFFFFUL);
    r->n[3] = a->n[2] >> 14 | ((a->n[3] << 18) & 0x3FFFFFFUL);
    r->n[4] = a->n[3] >> 8 | ((a->n[4] << 24) & 0x3FFFFFFUL);
    r->n[5] = (a->n[4] >> 2) & 0x3FFFFFFUL;
    r->n[6] = a->n[4] >> 28 | ((a->n[5] << 4) & 0x3FFFFFFUL);
    r->n[7] = a->n[5] >> 22 | ((a->n[6] << 10) & 0x3FFFFFFUL);
    r->n[8] = a->n[6] >> 16 | ((a->n[7] << 16) & 0x3FFFFFFUL);
    r->n[9] = a->n[7] >> 10;
}

static void secp256k1_fe_from_signed30(secp256k1_fe *r, const secp256k1_modinv32_signed30 *a) {
    const uint32_t M26 = UINT32_MAX >> 6;
    const uint32_t a0 = a->v[0], a1 = a->v[1], a2 = a->v[2], a3 = a->v[3], a4 = a->v[4],
                   a5 = a->v[5], a6 = a->v[6], a7 = a->v[7], a8 = a->v[8];

    /* The output from secp256k1_modinv32{_var} should be normalized to range [0,modulus), and
     * have limbs in [0,2^30). The modulus is < 2^256, so the top limb must be below 2^(256-30*8).
     */
    VERIFY_CHECK(a0 >> 30 == 0);
    VERIFY_CHECK(a1 >> 30 == 0);
    VERIFY_CHECK(a2 >> 30 == 0);
    VERIFY_CHECK(a3 >> 30 == 0);
    VERIFY_CHECK(a4 >> 30 == 0);
    VERIFY_CHECK(a5 >> 30 == 0);
    VERIFY_CHECK(a6 >> 30 == 0);
    VERIFY_CHECK(a7 >> 30 == 0);
    VERIFY_CHECK(a8 >> 16 == 0);

    r->n[0] =  a0                   & M26;
    r->n[1] = (a0 >> 26 | a1 <<  4) & M26;
    r->n[2] = (a1 >> 22 | a2 <<  8) & M26;
    r->n[3] = (a2 >> 18 | a3 << 12) & M26;
    r->n[4] = (a3 >> 14 | a4 << 16) & M26;
    r->n[5] = (a4 >> 10 | a5 << 20) & M26;
    r->n[6] = (a5 >>  6 | a6 << 24) & M26;
    r->n[7] = (a6 >>  2           ) & M26;
    r->n[8] = (a6 >> 28 | a7 <<  2) & M26;
    r->n[9] = (a7 >> 24 | a8 <<  6);
}

static void secp256k1_fe_to_signed30(secp256k1_modinv32_signed30 *r, const secp256k1_fe *a) {
    const uint32_t M30 = UINT32_MAX >> 2;
    const uint64_t a0 = a->n[0], a1 = a->n[1], a2 = a->n[2], a3 = a->n[3], a4 = a->n[4],
                   a5 = a->n[5], a6 = a->n[6], a7 = a->n[7], a8 = a->n[8], a9 = a->n[9];

    r->v[0] = (a0       | a1 << 26) & M30;
    r->v[1] = (a1 >>  4 | a2 << 22) & M30;
    r->v[2] = (a2 >>  8 | a3 << 18) & M30;
    r->v[3] = (a3 >> 12 | a4 << 14) & M30;
    r->v[4] = (a4 >> 16 | a5 << 10) & M30;
    r->v[5] = (a5 >> 20 | a6 <<  6) & M30;
    r->v[6] = (a6 >> 24 | a7 <<  2
                        | a8 << 28) & M30;
    r->v[7] = (a8 >>  2 | a9 << 24) & M30;
    r->v[8] =  a9 >>  6;
}

static const secp256k1_modinv32_modinfo secp256k1_const_modinfo_fe = {
    {{-0x3D1, -4, 0, 0, 0, 0, 0, 0, 65536}},
    0x2DDACACFL
};

static void secp256k1_fe_impl_inv(secp256k1_fe *r, const secp256k1_fe *x) {
    secp256k1_fe tmp = *x;
    secp256k1_modinv32_signed30 s;

    secp256k1_fe_normalize(&tmp);
    secp256k1_fe_to_signed30(&s, &tmp);
    secp256k1_modinv32(&s, &secp256k1_const_modinfo_fe);
    secp256k1_fe_from_signed30(r, &s);
}

static void secp256k1_fe_impl_inv_var(secp256k1_fe *r, const secp256k1_fe *x) {
    secp256k1_fe tmp = *x;
    secp256k1_modinv32_signed30 s;

    secp256k1_fe_normalize_var(&tmp);
    secp256k1_fe_to_signed30(&s, &tmp);
    secp256k1_modinv32_var(&s, &secp256k1_const_modinfo_fe);
    secp256k1_fe_from_signed30(r, &s);
}

static int secp256k1_fe_impl_is_square_var(const secp256k1_fe *x) {
    secp256k1_fe tmp;
    secp256k1_modinv32_signed30 s;
    int jac, ret;

    tmp = *x;
    secp256k1_fe_normalize_var(&tmp);
    /* secp256k1_jacobi32_maybe_var cannot deal with input 0. */
    if (secp256k1_fe_is_zero(&tmp)) return 1;
    secp256k1_fe_to_signed30(&s, &tmp);
    jac = secp256k1_jacobi32_maybe_var(&s, &secp256k1_const_modinfo_fe);
    if (jac == 0) {
        /* secp256k1_jacobi32_maybe_var failed to compute the Jacobi symbol. Fall back
         * to computing a square root. This should be extremely rare with random
         * input (except in VERIFY mode, where a lower iteration count is used). */
        secp256k1_fe dummy;
        ret = secp256k1_fe_sqrt(&dummy, &tmp);
    } else {
        ret = jac >= 0;
    }
    return ret;
}

#endif /* SECP256K1_FIELD_REPR_IMPL_H */
