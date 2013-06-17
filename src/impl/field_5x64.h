// Copyright (c) 2013 Pieter Wuille
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef _SECP256K1_FIELD_REPR_IMPL_H_
#define _SECP256K1_FIELD_REPR_IMPL_H_

#include <assert.h>
#include <string.h>
#include "../num.h"
#include "../field.h"

#include <stdio.h>
#include "field_5x64_asm.h"

/** Implements arithmetic modulo FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFE FFFFFC2F,
 *  represented as 4 uint64_t's in base 2^64, and one overflow uint64_t.
 */

#define FULL_LIMB (0xFFFFFFFFFFFFFFFFULL)
#define LAST_LIMB (0xFFFFFFFEFFFFFC2FULL)
#define COMP_LIMB (0x00000001000003D1ULL)

void static secp256k1_fe_inner_start(void) {}
void static secp256k1_fe_inner_stop(void) {}

void static secp256k1_fe_reduce(secp256k1_fe_t *r) {
    unsigned __int128 c = (unsigned __int128)r->n[4] * COMP_LIMB + r->n[0];
    uint64_t n0 = c;
    c = (c >> 64) + r->n[1];
    uint64_t n1 = c;
    c = (c >> 64) + r->n[2];
    r->n[2] = c;
    c = (c >> 64) + r->n[3];
    r->n[3] = c;
    c = (c >> 64) * COMP_LIMB + n0;
    r->n[0] = c;
    r->n[1] = n1 + (c >> 64);
    assert(r->n[1] >= n1);
    r->n[4] = 0;
#ifdef VERIFY
    r->reduced = 1;
#endif
}

void static secp256k1_fe_normalize(secp256k1_fe_t *r) {
    secp256k1_fe_reduce(r);

    // Subtract p if result >= p
    uint64_t mask = -(int64_t)((r->n[0] < LAST_LIMB) | (r->n[1] != ~0ULL) | (r->n[2] != ~0ULL) | (r->n[3] != ~0ULL));
    r->n[0] -= (~mask & LAST_LIMB);
    r->n[1] &= mask;
    r->n[2] &= mask;
    r->n[3] &= mask;
    assert(r->n[4] == 0);

#ifdef VERIFY
    r->normalized = 1;
#endif
}

void static inline secp256k1_fe_set_int(secp256k1_fe_t *r, int a) {
    r->n[0] = a;
    r->n[1] = r->n[2] = r->n[3] = r->n[4] = 0;

#ifdef VERIFY
    r->reduced = 1;
    r->normalized = 1;
#endif
}

// TODO: not constant time!
int static inline secp256k1_fe_is_zero(const secp256k1_fe_t *a) {
#ifdef VERIFY
    assert(a->normalized);
#endif
    return (a->n[0] == 0 && a->n[1] == 0 && a->n[2] == 0 && a->n[3] == 0);
}

int static inline secp256k1_fe_is_odd(const secp256k1_fe_t *a) {
#ifdef VERIFY
    assert(a->normalized);
#endif
    return a->n[0] & 1;
}

// TODO: not constant time!
int static inline secp256k1_fe_equal(const secp256k1_fe_t *a, const secp256k1_fe_t *b) {
#ifdef VERIFY
    assert(a->normalized);
    assert(b->normalized);
#endif
    return (a->n[0] == b->n[0] && a->n[1] == b->n[1] && a->n[2] == b->n[2] && a->n[3] == b->n[3]);
}

void static secp256k1_fe_set_b32(secp256k1_fe_t *r, const unsigned char *a) {
    r->n[0] = r->n[1] = r->n[2] = r->n[3] = r->n[4] = 0;
    for (int i=0; i<32; i++) {
        r->n[i/8] |= (uint64_t)a[31-i] << (i&7)*8;
    }
#ifdef VERIFY
    r->reduced = 1;
    r->normalized = 0;
#endif
}

/** Convert a field element to a 32-byte big endian value. Requires the input to be normalized */
void static secp256k1_fe_get_b32(unsigned char *r, const secp256k1_fe_t *a) {
#ifdef VERIFY
    assert(a->normalized);
#endif
    for (int i=0; i<32; i++) {
        r[31-i] = a->n[i/8] >> ((i&7)*8);
    }
}

void static inline secp256k1_fe_negate(secp256k1_fe_t *r, const secp256k1_fe_t *ac, int m) {
    secp256k1_fe_t a = *ac;
    secp256k1_fe_reduce(&a);
    unsigned __int128 c = (unsigned __int128)(~a.n[0]) + LAST_LIMB + 1;
    r->n[0] = c;
    c = (c >> 64) + (~a.n[1]) + FULL_LIMB;
    r->n[1] = c;
    c = (c >> 64) + (~a.n[2]) + FULL_LIMB;
    r->n[2] = c;
    c = (c >> 64) + (~a.n[3]) + FULL_LIMB;
    r->n[3] = c;
    r->n[4] = 0;
#ifdef VERIFY
    r->reduced = 1;
    r->normalized = 0;
#endif
}

void static inline secp256k1_fe_mul_int(secp256k1_fe_t *r, int a) {
#ifdef VERIFY
    r->reduced = 0;
    r->normalized = 0;
#endif
    unsigned __int128 c = (unsigned __int128)r->n[0] * a;
    r->n[0] = c;
    c = (c >> 64) + (unsigned __int128)r->n[1] * a;
    r->n[1] = c;
    c = (c >> 64) + (unsigned __int128)r->n[2] * a;
    r->n[2] = c;
    c = (c >> 64) + (unsigned __int128)r->n[3] * a;
    r->n[3] = c;
    c = (c >> 64) + (unsigned __int128)r->n[4] * a;
    r->n[4] = c;
}

void static inline secp256k1_fe_add(secp256k1_fe_t *r, const secp256k1_fe_t *a) {
#ifdef VERIFY
    r->reduced = 0;
    r->normalized = 0;
#endif
    unsigned __int128 c = (unsigned __int128)r->n[0] + a->n[0];
    r->n[0] = c;
    c = (unsigned __int128)r->n[1] + a->n[1] + (c >> 64);
    r->n[1] = c;
    c = (unsigned __int128)r->n[2] + a->n[2] + (c >> 64);
    r->n[2] = c;
    c = (unsigned __int128)r->n[3] + a->n[3] + (c >> 64);
    r->n[3] = c;
    c = (unsigned __int128)r->n[4] + a->n[4] + (c >> 64);
    r->n[4] = c;
    assert((c >> 64) == 0);
}

#if 0
#define muladd_c3(a,b,c0,c1,c2) { \
    unsigned __int128 q1 = ((unsigned __int128)(a)) * (b) + (c0); \
    (c0) = q1; \
    unsigned __int128 q2 = (q1 >> 64) + (c1) + (((unsigned __int128)(c2)) << 64); \
    (c1) = q2; \
    (c2) = q2 >> 64; \
}

#define sqradd_c3(a,c0,c1,c2) muladd_c3(a,a,c0,c1,c2)

/*#define muladd_c3(a,b,c0,c1,c2) { \
    unsigned __int128 q = (unsigned __int128)(a) * (b) + (c0); \
    (c0) = q; \
    (c1) += (q >> 64); \
    (c2) += ((c1) < (q >> 64))?1:0; \
}*/

#define muladd2_c3(a,b,c0,c1,c2) { \
    unsigned __int128 q = (unsigned __int128)(a) * (b); \
    uint64_t t1 = (q >> 64); \
    uint64_t t0 = q; \
    uint64_t t2 = t1+t1; (c2) += (t2<t1)?1:0; \
    t1 = t0+t0; t2 += (t1<t0)?1:0; \
    (c0) += t1; t2 += ((c0)<t1)?1:0; \
    (c1) += t2; (c2) += ((c1)<t2)?1:0; \
}

/*#define muladd2_c3(a,b,c0,c1,c2) { \
    muladd_c3(a,b,c0,c1,c2); \
    muladd_c3(a,b,c0,c1,c2); \
}*/
#else

#define muladd_c3(a,b,c0,c1,c2) {       \
        register uint64_t t1, t2;       \
        asm ("mulq %3"                  \
                : "=a"(t1),"=d"(t2)     \
                : "a"(a),"m"(b)         \
                : "cc");                \
        asm ("addq %2,%0; adcq %3,%1"   \
                : "+r"(c0),"+d"(t2)     \
                : "a"(t1),"g"(0)        \
                : "cc");                \
        asm ("addq %2,%0; adcq %3,%1"   \
                : "+r"(c1),"+r"(c2)     \
                : "d"(t2),"g"(0)        \
                : "cc");                \
        }

#define sqradd_c3(a,c0,c1,c2) {         \
        register uint64_t t1, t2;       \
        asm ("mulq %2"                  \
                : "=a"(t1),"=d"(t2)     \
                : "a"(a)                \
                : "cc");                \
        asm ("addq %2,%0; adcq %3,%1"   \
                : "+r"(c0),"+d"(t2)     \
                : "a"(t1),"g"(0)        \
                : "cc");                \
        asm ("addq %2,%0; adcq %3,%1"   \
                : "+r"(c1),"+r"(c2)     \
                : "d"(t2),"g"(0)        \
                : "cc");                \
        }

#define muladd2_c3(a,b,c0,c1,c2) {      \
        register uint64_t t1, t2;       \
        asm ("mulq %3"                  \
                : "=a"(t1),"=d"(t2)     \
                : "a"(a),"m"(b)         \
                : "cc");                \
        asm ("addq %0,%0; adcq %2,%1"   \
                : "+d"(t2),"+r"(c2)     \
                : "g"(0)                \
                : "cc");                \
        asm ("addq %0,%0; adcq %2,%1"   \
                : "+a"(t1),"+d"(t2)     \
                : "g"(0)                \
                : "cc");                \
        asm ("addq %2,%0; adcq %3,%1"   \
                : "+r"(c0),"+d"(t2)     \
                : "a"(t1),"g"(0)        \
                : "cc");                \
        asm ("addq %2,%0; adcq %3,%1"   \
                : "+r"(c1),"+r"(c2)     \
                : "d"(t2),"g"(0)        \
                : "cc");                \
        }
#endif

#define mul_c2(a,b,c0,c1) { \
    unsigned __int128 q = (unsigned __int128)(a) * (b); \
    (c0) = q; \
    (c1) = (q >> 64); \
}

void static secp256k1_fe_mul(secp256k1_fe_t *r, const secp256k1_fe_t *ac, const secp256k1_fe_t *bc) {

    secp256k1_fe_t a = *ac, b = *bc;
    secp256k1_fe_reduce(&a);
    secp256k1_fe_reduce(&b);

#ifdef USE_FIELD_5X64_ASM
    secp256k1_fe_mul_inner((&a)->n,(&b)->n,r->n);
#else
    uint64_t c1,c2,c3;
    c3=0;
    mul_c2(a.n[0], b.n[0], c1, c2);
    uint64_t r0 = c1; c1 = 0;
    muladd_c3(a.n[0], b.n[1], c2, c3, c1);
    muladd_c3(a.n[1], b.n[0], c2, c3, c1);
    uint64_t r1 = c2; c2 = 0;
    muladd_c3(a.n[2], b.n[0], c3, c1, c2);
    muladd_c3(a.n[1], b.n[1], c3, c1, c2);
    muladd_c3(a.n[0], b.n[2], c3, c1, c2);
    uint64_t r2 = c3; c3 = 0;
    muladd_c3(a.n[0], b.n[3], c1, c2, c3);
    muladd_c3(a.n[1], b.n[2], c1, c2, c3);
    muladd_c3(a.n[2], b.n[1], c1, c2, c3);
    muladd_c3(a.n[3], b.n[0], c1, c2, c3);
    uint64_t r3 = c1; c1 = 0;
    muladd_c3(a.n[3], b.n[1], c2, c3, c1);
    muladd_c3(a.n[2], b.n[2], c2, c3, c1);
    muladd_c3(a.n[1], b.n[3], c2, c3, c1);
    uint64_t r4 = c2; c2 = 0;
    muladd_c3(a.n[2], b.n[3], c3, c1, c2);
    muladd_c3(a.n[3], b.n[2], c3, c1, c2);
    uint64_t r5 = c3; c3 = 0;
    muladd_c3(a.n[3], b.n[3], c1, c2, c3);
    uint64_t r6 = c1;
    uint64_t r7 = c2;
    assert(c3 == 0);
    unsigned __int128 c = (unsigned __int128)r4 * COMP_LIMB + r0;
    r->n[0] = c;
    c = (unsigned __int128)r5 * COMP_LIMB + r1 + (c >> 64);
    r->n[1] = c;
    c = (unsigned __int128)r6 * COMP_LIMB + r2 + (c >> 64);
    r->n[2] = c;
    c = (unsigned __int128)r7 * COMP_LIMB + r3 + (c >> 64);
    r->n[3] = c;
    r->n[4] = c >> 64;
#endif

#ifdef VERIFY
    r->normalized = 0;
    r->reduced = 0;
#endif
    secp256k1_fe_reduce(r);
}

/*void static secp256k1_fe_sqr(secp256k1_fe_t *r, const secp256k1_fe_t *a) {
    secp256k1_fe_mul(r, a, a);
}*/

void static secp256k1_fe_sqr(secp256k1_fe_t *r, const secp256k1_fe_t *ac) {
    secp256k1_fe_t a = *ac;
    secp256k1_fe_reduce(&a);

#ifdef USE_FIELD_5X64_ASM
    secp256k1_fe_sqr_inner((&a)->n,r->n);
#else
    uint64_t c1,c2,c3;
    c3=0;
    mul_c2(a.n[0], a.n[0], c1, c2);
    uint64_t r0 = c1; c1 = 0;
    muladd2_c3(a.n[0], a.n[1], c2, c3, c1);
    uint64_t r1 = c2; c2 = 0;
    muladd2_c3(a.n[2], a.n[0], c3, c1, c2);
    sqradd_c3(a.n[1], c3, c1, c2);
    uint64_t r2 = c3; c3 = 0;
    muladd2_c3(a.n[0], a.n[3], c1, c2, c3);
    muladd2_c3(a.n[1], a.n[2], c1, c2, c3);
    uint64_t r3 = c1; c1 = 0;
    muladd2_c3(a.n[3], a.n[1], c2, c3, c1);
    sqradd_c3(a.n[2], c2, c3, c1);
    uint64_t r4 = c2; c2 = 0;
    muladd2_c3(a.n[2], a.n[3], c3, c1, c2);
    uint64_t r5 = c3; c3 = 0;
    sqradd_c3(a.n[3], c1, c2, c3);
    uint64_t r6 = c1;
    uint64_t r7 = c2;
    assert(c3 == 0);
    unsigned __int128 c = (unsigned __int128)r4 * COMP_LIMB + r0;
    r->n[0] = c;
    c = (unsigned __int128)r5 * COMP_LIMB + r1 + (c >> 64);
    r->n[1] = c;
    c = (unsigned __int128)r6 * COMP_LIMB + r2 + (c >> 64);
    r->n[2] = c;
    c = (unsigned __int128)r7 * COMP_LIMB + r3 + (c >> 64);
    r->n[3] = c;
    r->n[4] = c >> 64;
#endif

#ifdef VERIFY
    r->normalized = 0;
    r->reduced = 0;
#endif
    secp256k1_fe_reduce(r);
}

#endif
