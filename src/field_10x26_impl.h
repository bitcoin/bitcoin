// Copyright (c) 2013 Pieter Wuille
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef _SECP256K1_FIELD_REPR_IMPL_H_
#define _SECP256K1_FIELD_REPR_IMPL_H_

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "util.h"
#include "num.h"
#include "field.h"

void static secp256k1_fe_inner_start(void) {}
void static secp256k1_fe_inner_stop(void) {}

void static secp256k1_fe_normalize(secp256k1_fe_t *r) {
    uint32_t t0 = r->n[0], t1 = r->n[1], t2 = r->n[2], t3 = r->n[3], t4 = r->n[4],
             t5 = r->n[5], t6 = r->n[6], t7 = r->n[7], t8 = r->n[8], t9 = r->n[9];

    // Reduce t9 at the start so there will be at most a single carry from the first pass
    uint32_t x = t9 >> 22; t9 &= 0x03FFFFFUL;
    uint32_t m;

    // The first pass ensures the magnitude is 1, ...
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

    // ... except for a possible carry at bit 22 of t9 (i.e. bit 256 of the field element)
    VERIFY_CHECK(t9 >> 23 == 0);

    // At most a single final reduction is needed; check if the value is >= the field characteristic
    x = (t9 >> 22) | ((t9 == 0x03FFFFFULL) & (m == 0x3FFFFFFULL)
        & ((t1 + 0x40UL + ((t0 + 0x3D1UL) >> 26)) > 0x3FFFFFFULL));

    // Apply the final reduction (for constant-time behaviour, we do it always)
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

    // If t9 didn't carry to bit 22 already, then it should have after any final reduction
    VERIFY_CHECK(t9 >> 22 == x);

    // Mask off the possible multiple of 2^256 from the final reduction
    t9 &= 0x03FFFFFUL;

    r->n[0] = t0; r->n[1] = t1; r->n[2] = t2; r->n[3] = t3; r->n[4] = t4;
    r->n[5] = t5; r->n[6] = t6; r->n[7] = t7; r->n[8] = t8; r->n[9] = t9;

#ifdef VERIFY
    r->magnitude = 1;
    r->normalized = 1;
#endif
}

void static inline secp256k1_fe_set_int(secp256k1_fe_t *r, int a) {
    r->n[0] = a;
    r->n[1] = r->n[2] = r->n[3] = r->n[4] = r->n[5] = r->n[6] = r->n[7] = r->n[8] = r->n[9] = 0;
#ifdef VERIFY
    r->magnitude = 1;
    r->normalized = 1;
#endif
}

// TODO: not constant time!
int static inline secp256k1_fe_is_zero(const secp256k1_fe_t *a) {
#ifdef VERIFY
    VERIFY_CHECK(a->normalized);
#endif
    return (a->n[0] == 0 && a->n[1] == 0 && a->n[2] == 0 && a->n[3] == 0 && a->n[4] == 0 && a->n[5] == 0 && a->n[6] == 0 && a->n[7] == 0 && a->n[8] == 0 && a->n[9] == 0);
}

int static inline secp256k1_fe_is_odd(const secp256k1_fe_t *a) {
#ifdef VERIFY
    VERIFY_CHECK(a->normalized);
#endif
    return a->n[0] & 1;
}

void static inline secp256k1_fe_clear(secp256k1_fe_t *a) {
#ifdef VERIFY
    a->normalized = 0;
    a->magnitude = 0;
#endif
    for (int i=0; i<10; i++) {
        a->n[i] = 0;
    }
}

// TODO: not constant time!
int static inline secp256k1_fe_equal(const secp256k1_fe_t *a, const secp256k1_fe_t *b) {
#ifdef VERIFY
    VERIFY_CHECK(a->normalized);
    VERIFY_CHECK(b->normalized);
#endif
    return (a->n[0] == b->n[0] && a->n[1] == b->n[1] && a->n[2] == b->n[2] && a->n[3] == b->n[3] && a->n[4] == b->n[4] &&
            a->n[5] == b->n[5] && a->n[6] == b->n[6] && a->n[7] == b->n[7] && a->n[8] == b->n[8] && a->n[9] == b->n[9]);
}

void static secp256k1_fe_set_b32(secp256k1_fe_t *r, const unsigned char *a) {
    r->n[0] = r->n[1] = r->n[2] = r->n[3] = r->n[4] = 0;
    r->n[5] = r->n[6] = r->n[7] = r->n[8] = r->n[9] = 0;
    for (int i=0; i<32; i++) {
        for (int j=0; j<4; j++) {
            int limb = (8*i+2*j)/26;
            int shift = (8*i+2*j)%26;
            r->n[limb] |= (uint32_t)((a[31-i] >> (2*j)) & 0x3) << shift;
        }
    }
#ifdef VERIFY
    r->magnitude = 1;
    r->normalized = 1;
#endif
}

/** Convert a field element to a 32-byte big endian value. Requires the input to be normalized */
void static secp256k1_fe_get_b32(unsigned char *r, const secp256k1_fe_t *a) {
#ifdef VERIFY
    VERIFY_CHECK(a->normalized);
#endif
    for (int i=0; i<32; i++) {
        int c = 0;
        for (int j=0; j<4; j++) {
            int limb = (8*i+2*j)/26;
            int shift = (8*i+2*j)%26;
            c |= ((a->n[limb] >> shift) & 0x3) << (2 * j);
        }
        r[31-i] = c;
    }
}

void static inline secp256k1_fe_negate(secp256k1_fe_t *r, const secp256k1_fe_t *a, int m) {
#ifdef VERIFY
    VERIFY_CHECK(a->magnitude <= m);
    r->magnitude = m + 1;
    r->normalized = 0;
#endif
    r->n[0] = 0x3FFFC2FUL * (m + 1) - a->n[0];
    r->n[1] = 0x3FFFFBFUL * (m + 1) - a->n[1];
    r->n[2] = 0x3FFFFFFUL * (m + 1) - a->n[2];
    r->n[3] = 0x3FFFFFFUL * (m + 1) - a->n[3];
    r->n[4] = 0x3FFFFFFUL * (m + 1) - a->n[4];
    r->n[5] = 0x3FFFFFFUL * (m + 1) - a->n[5];
    r->n[6] = 0x3FFFFFFUL * (m + 1) - a->n[6];
    r->n[7] = 0x3FFFFFFUL * (m + 1) - a->n[7];
    r->n[8] = 0x3FFFFFFUL * (m + 1) - a->n[8];
    r->n[9] = 0x03FFFFFUL * (m + 1) - a->n[9];
}

void static inline secp256k1_fe_mul_int(secp256k1_fe_t *r, int a) {
#ifdef VERIFY
    r->magnitude *= a;
    r->normalized = 0;
#endif
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

void static inline secp256k1_fe_add(secp256k1_fe_t *r, const secp256k1_fe_t *a) {
#ifdef VERIFY
    r->magnitude += a->magnitude;
    r->normalized = 0;
#endif
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

void static inline secp256k1_fe_mul_inner(const uint32_t *a, const uint32_t *b, uint32_t *r) {
    uint64_t c = (uint64_t)a[0] * b[0];
    uint32_t t0 = c & 0x3FFFFFFUL; c = c >> 26;
    c = c + (uint64_t)a[0] * b[1] +
            (uint64_t)a[1] * b[0];
    uint32_t t1 = c & 0x3FFFFFFUL; c = c >> 26;
    c = c + (uint64_t)a[0] * b[2] +
            (uint64_t)a[1] * b[1] +
            (uint64_t)a[2] * b[0];
    uint32_t t2 = c & 0x3FFFFFFUL; c = c >> 26;
    c = c + (uint64_t)a[0] * b[3] +
            (uint64_t)a[1] * b[2] +
            (uint64_t)a[2] * b[1] +
            (uint64_t)a[3] * b[0];
    uint32_t t3 = c & 0x3FFFFFFUL; c = c >> 26;
    c = c + (uint64_t)a[0] * b[4] +
            (uint64_t)a[1] * b[3] +
            (uint64_t)a[2] * b[2] +
            (uint64_t)a[3] * b[1] +
            (uint64_t)a[4] * b[0];
    uint32_t t4 = c & 0x3FFFFFFUL; c = c >> 26;
    c = c + (uint64_t)a[0] * b[5] +
            (uint64_t)a[1] * b[4] +
            (uint64_t)a[2] * b[3] +
            (uint64_t)a[3] * b[2] +
            (uint64_t)a[4] * b[1] +
            (uint64_t)a[5] * b[0];
    uint32_t t5 = c & 0x3FFFFFFUL; c = c >> 26;
    c = c + (uint64_t)a[0] * b[6] +
            (uint64_t)a[1] * b[5] +
            (uint64_t)a[2] * b[4] +
            (uint64_t)a[3] * b[3] +
            (uint64_t)a[4] * b[2] +
            (uint64_t)a[5] * b[1] +
            (uint64_t)a[6] * b[0];
    uint32_t t6 = c & 0x3FFFFFFUL; c = c >> 26;
    c = c + (uint64_t)a[0] * b[7] +
            (uint64_t)a[1] * b[6] +
            (uint64_t)a[2] * b[5] +
            (uint64_t)a[3] * b[4] +
            (uint64_t)a[4] * b[3] +
            (uint64_t)a[5] * b[2] +
            (uint64_t)a[6] * b[1] +
            (uint64_t)a[7] * b[0];
    uint32_t t7 = c & 0x3FFFFFFUL; c = c >> 26;
    c = c + (uint64_t)a[0] * b[8] +
            (uint64_t)a[1] * b[7] +
            (uint64_t)a[2] * b[6] +
            (uint64_t)a[3] * b[5] +
            (uint64_t)a[4] * b[4] +
            (uint64_t)a[5] * b[3] +
            (uint64_t)a[6] * b[2] +
            (uint64_t)a[7] * b[1] +
            (uint64_t)a[8] * b[0];
    uint32_t t8 = c & 0x3FFFFFFUL; c = c >> 26;
    c = c + (uint64_t)a[0] * b[9] +
            (uint64_t)a[1] * b[8] +
            (uint64_t)a[2] * b[7] +
            (uint64_t)a[3] * b[6] +
            (uint64_t)a[4] * b[5] +
            (uint64_t)a[5] * b[4] +
            (uint64_t)a[6] * b[3] +
            (uint64_t)a[7] * b[2] +
            (uint64_t)a[8] * b[1] +
            (uint64_t)a[9] * b[0];
    uint32_t t9 = c & 0x3FFFFFFUL; c = c >> 26;
    c = c + (uint64_t)a[1] * b[9] +
            (uint64_t)a[2] * b[8] +
            (uint64_t)a[3] * b[7] +
            (uint64_t)a[4] * b[6] +
            (uint64_t)a[5] * b[5] +
            (uint64_t)a[6] * b[4] +
            (uint64_t)a[7] * b[3] +
            (uint64_t)a[8] * b[2] +
            (uint64_t)a[9] * b[1];
    uint32_t t10 = c & 0x3FFFFFFUL; c = c >> 26;
    c = c + (uint64_t)a[2] * b[9] +
            (uint64_t)a[3] * b[8] +
            (uint64_t)a[4] * b[7] +
            (uint64_t)a[5] * b[6] +
            (uint64_t)a[6] * b[5] +
            (uint64_t)a[7] * b[4] +
            (uint64_t)a[8] * b[3] +
            (uint64_t)a[9] * b[2];
    uint32_t t11 = c & 0x3FFFFFFUL; c = c >> 26;
    c = c + (uint64_t)a[3] * b[9] +
            (uint64_t)a[4] * b[8] +
            (uint64_t)a[5] * b[7] +
            (uint64_t)a[6] * b[6] +
            (uint64_t)a[7] * b[5] +
            (uint64_t)a[8] * b[4] +
            (uint64_t)a[9] * b[3];
    uint32_t t12 = c & 0x3FFFFFFUL; c = c >> 26;
    c = c + (uint64_t)a[4] * b[9] +
            (uint64_t)a[5] * b[8] +
            (uint64_t)a[6] * b[7] +
            (uint64_t)a[7] * b[6] +
            (uint64_t)a[8] * b[5] +
            (uint64_t)a[9] * b[4];
    uint32_t t13 = c & 0x3FFFFFFUL; c = c >> 26;
    c = c + (uint64_t)a[5] * b[9] +
            (uint64_t)a[6] * b[8] +
            (uint64_t)a[7] * b[7] +
            (uint64_t)a[8] * b[6] +
            (uint64_t)a[9] * b[5];
    uint32_t t14 = c & 0x3FFFFFFUL; c = c >> 26;
    c = c + (uint64_t)a[6] * b[9] +
            (uint64_t)a[7] * b[8] +
            (uint64_t)a[8] * b[7] +
            (uint64_t)a[9] * b[6];
    uint32_t t15 = c & 0x3FFFFFFUL; c = c >> 26;
    c = c + (uint64_t)a[7] * b[9] +
            (uint64_t)a[8] * b[8] +
            (uint64_t)a[9] * b[7];
    uint32_t t16 = c & 0x3FFFFFFUL; c = c >> 26;
    c = c + (uint64_t)a[8] * b[9] +
            (uint64_t)a[9] * b[8];
    uint32_t t17 = c & 0x3FFFFFFUL; c = c >> 26;
    c = c + (uint64_t)a[9] * b[9];
    uint32_t t18 = c & 0x3FFFFFFUL; c = c >> 26;
    uint32_t t19 = c;

    c = t0 + (uint64_t)t10 * 0x3D10UL;
    t0 = c & 0x3FFFFFFUL; c = c >> 26;
    c = c + t1 + (uint64_t)t10*0x400UL + (uint64_t)t11 * 0x3D10UL;
    t1 = c & 0x3FFFFFFUL; c = c >> 26;
    c = c + t2 + (uint64_t)t11*0x400UL + (uint64_t)t12 * 0x3D10UL;
    t2 = c & 0x3FFFFFFUL; c = c >> 26;
    c = c + t3 + (uint64_t)t12*0x400UL + (uint64_t)t13 * 0x3D10UL;
    r[3] = c & 0x3FFFFFFUL; c = c >> 26;
    c = c + t4 + (uint64_t)t13*0x400UL + (uint64_t)t14 * 0x3D10UL;
    r[4] = c & 0x3FFFFFFUL; c = c >> 26;
    c = c + t5 + (uint64_t)t14*0x400UL + (uint64_t)t15 * 0x3D10UL;
    r[5] = c & 0x3FFFFFFUL; c = c >> 26;
    c = c + t6 + (uint64_t)t15*0x400UL + (uint64_t)t16 * 0x3D10UL;
    r[6] = c & 0x3FFFFFFUL; c = c >> 26;
    c = c + t7 + (uint64_t)t16*0x400UL + (uint64_t)t17 * 0x3D10UL;
    r[7] = c & 0x3FFFFFFUL; c = c >> 26;
    c = c + t8 + (uint64_t)t17*0x400UL + (uint64_t)t18 * 0x3D10UL;
    r[8] = c & 0x3FFFFFFUL; c = c >> 26;
    c = c + t9 + (uint64_t)t18*0x400UL + (uint64_t)t19 * 0x1000003D10ULL;
    r[9] = c & 0x03FFFFFUL; c = c >> 22;
    uint64_t d = t0 + c * 0x3D1UL;
    r[0] = d & 0x3FFFFFFUL; d = d >> 26;
    d = d + t1 + c*0x40;
    r[1] = d & 0x3FFFFFFUL; d = d >> 26;
    r[2] = t2 + d;
}

void static inline secp256k1_fe_sqr_inner(const uint32_t *a, uint32_t *r) {
    uint64_t c = (uint64_t)a[0] * a[0];
    uint32_t t0 = c & 0x3FFFFFFUL; c = c >> 26;
    c = c + (uint64_t)(a[0]*2) * a[1];
    uint32_t t1 = c & 0x3FFFFFFUL; c = c >> 26;
    c = c + (uint64_t)(a[0]*2) * a[2] +
            (uint64_t)a[1] * a[1];
    uint32_t t2 = c & 0x3FFFFFFUL; c = c >> 26;
    c = c + (uint64_t)(a[0]*2) * a[3] +
            (uint64_t)(a[1]*2) * a[2];
    uint32_t t3 = c & 0x3FFFFFFUL; c = c >> 26;
    c = c + (uint64_t)(a[0]*2) * a[4] +
            (uint64_t)(a[1]*2) * a[3] +
            (uint64_t)a[2] * a[2];
    uint32_t t4 = c & 0x3FFFFFFUL; c = c >> 26;
    c = c + (uint64_t)(a[0]*2) * a[5] +
            (uint64_t)(a[1]*2) * a[4] +
            (uint64_t)(a[2]*2) * a[3];
    uint32_t t5 = c & 0x3FFFFFFUL; c = c >> 26;
    c = c + (uint64_t)(a[0]*2) * a[6] +
            (uint64_t)(a[1]*2) * a[5] +
            (uint64_t)(a[2]*2) * a[4] +
            (uint64_t)a[3] * a[3];
    uint32_t t6 = c & 0x3FFFFFFUL; c = c >> 26;
    c = c + (uint64_t)(a[0]*2) * a[7] +
            (uint64_t)(a[1]*2) * a[6] +
            (uint64_t)(a[2]*2) * a[5] +
            (uint64_t)(a[3]*2) * a[4];
    uint32_t t7 = c & 0x3FFFFFFUL; c = c >> 26;
    c = c + (uint64_t)(a[0]*2) * a[8] +
            (uint64_t)(a[1]*2) * a[7] +
            (uint64_t)(a[2]*2) * a[6] +
            (uint64_t)(a[3]*2) * a[5] +
            (uint64_t)a[4] * a[4];
    uint32_t t8 = c & 0x3FFFFFFUL; c = c >> 26;
    c = c + (uint64_t)(a[0]*2) * a[9] +
            (uint64_t)(a[1]*2) * a[8] +
            (uint64_t)(a[2]*2) * a[7] +
            (uint64_t)(a[3]*2) * a[6] +
            (uint64_t)(a[4]*2) * a[5];
    uint32_t t9 = c & 0x3FFFFFFUL; c = c >> 26;
    c = c + (uint64_t)(a[1]*2) * a[9] +
            (uint64_t)(a[2]*2) * a[8] +
            (uint64_t)(a[3]*2) * a[7] +
            (uint64_t)(a[4]*2) * a[6] +
            (uint64_t)a[5] * a[5];
    uint32_t t10 = c & 0x3FFFFFFUL; c = c >> 26;
    c = c + (uint64_t)(a[2]*2) * a[9] +
            (uint64_t)(a[3]*2) * a[8] +
            (uint64_t)(a[4]*2) * a[7] +
            (uint64_t)(a[5]*2) * a[6];
    uint32_t t11 = c & 0x3FFFFFFUL; c = c >> 26;
    c = c + (uint64_t)(a[3]*2) * a[9] +
            (uint64_t)(a[4]*2) * a[8] +
            (uint64_t)(a[5]*2) * a[7] +
            (uint64_t)a[6] * a[6];
    uint32_t t12 = c & 0x3FFFFFFUL; c = c >> 26;
    c = c + (uint64_t)(a[4]*2) * a[9] +
            (uint64_t)(a[5]*2) * a[8] +
            (uint64_t)(a[6]*2) * a[7];
    uint32_t t13 = c & 0x3FFFFFFUL; c = c >> 26;
    c = c + (uint64_t)(a[5]*2) * a[9] +
            (uint64_t)(a[6]*2) * a[8] +
            (uint64_t)a[7] * a[7];
    uint32_t t14 = c & 0x3FFFFFFUL; c = c >> 26;
    c = c + (uint64_t)(a[6]*2) * a[9] +
            (uint64_t)(a[7]*2) * a[8];
    uint32_t t15 = c & 0x3FFFFFFUL; c = c >> 26;
    c = c + (uint64_t)(a[7]*2) * a[9] +
            (uint64_t)a[8] * a[8];
    uint32_t t16 = c & 0x3FFFFFFUL; c = c >> 26;
    c = c + (uint64_t)(a[8]*2) * a[9];
    uint32_t t17 = c & 0x3FFFFFFUL; c = c >> 26;
    c = c + (uint64_t)a[9] * a[9];
    uint32_t t18 = c & 0x3FFFFFFUL; c = c >> 26;
    uint32_t t19 = c;

    c = t0 + (uint64_t)t10 * 0x3D10UL;
    t0 = c & 0x3FFFFFFUL; c = c >> 26;
    c = c + t1 + (uint64_t)t10*0x400UL + (uint64_t)t11 * 0x3D10UL;
    t1 = c & 0x3FFFFFFUL; c = c >> 26;
    c = c + t2 + (uint64_t)t11*0x400UL + (uint64_t)t12 * 0x3D10UL;
    t2 = c & 0x3FFFFFFUL; c = c >> 26;
    c = c + t3 + (uint64_t)t12*0x400UL + (uint64_t)t13 * 0x3D10UL;
    r[3] = c & 0x3FFFFFFUL; c = c >> 26;
    c = c + t4 + (uint64_t)t13*0x400UL + (uint64_t)t14 * 0x3D10UL;
    r[4] = c & 0x3FFFFFFUL; c = c >> 26;
    c = c + t5 + (uint64_t)t14*0x400UL + (uint64_t)t15 * 0x3D10UL;
    r[5] = c & 0x3FFFFFFUL; c = c >> 26;
    c = c + t6 + (uint64_t)t15*0x400UL + (uint64_t)t16 * 0x3D10UL;
    r[6] = c & 0x3FFFFFFUL; c = c >> 26;
    c = c + t7 + (uint64_t)t16*0x400UL + (uint64_t)t17 * 0x3D10UL;
    r[7] = c & 0x3FFFFFFUL; c = c >> 26;
    c = c + t8 + (uint64_t)t17*0x400UL + (uint64_t)t18 * 0x3D10UL;
    r[8] = c & 0x3FFFFFFUL; c = c >> 26;
    c = c + t9 + (uint64_t)t18*0x400UL + (uint64_t)t19 * 0x1000003D10ULL;
    r[9] = c & 0x03FFFFFUL; c = c >> 22;
    uint64_t d = t0 + c * 0x3D1UL;
    r[0] = d & 0x3FFFFFFUL; d = d >> 26;
    d = d + t1 + c*0x40;
    r[1] = d & 0x3FFFFFFUL; d = d >> 26;
    r[2] = t2 + d;
}


void static secp256k1_fe_mul(secp256k1_fe_t *r, const secp256k1_fe_t *a, const secp256k1_fe_t *b) {
#ifdef VERIFY
    VERIFY_CHECK(a->magnitude <= 8);
    VERIFY_CHECK(b->magnitude <= 8);
    r->magnitude = 1;
    r->normalized = 0;
#endif
    secp256k1_fe_mul_inner(a->n, b->n, r->n);
}

void static secp256k1_fe_sqr(secp256k1_fe_t *r, const secp256k1_fe_t *a) {
#ifdef VERIFY
    VERIFY_CHECK(a->magnitude <= 8);
    r->magnitude = 1;
    r->normalized = 0;
#endif
    secp256k1_fe_sqr_inner(a->n, r->n);
}

#endif
