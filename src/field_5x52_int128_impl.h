// Copyright (c) 2013 Pieter Wuille
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef _SECP256K1_FIELD_INNER5X52_IMPL_H_
#define _SECP256K1_FIELD_INNER5X52_IMPL_H_

#include <stdint.h>

SECP256K1_INLINE static void secp256k1_fe_mul_inner(const uint64_t *a, const uint64_t *b, uint64_t *r) {

    const uint64_t M = 0xFFFFFFFFFFFFFULL, R = 0x1000003D10ULL;
    // [... a b c] is a shorthand for ... + a<<104 + b<<52 + c<<0 mod n.
    // px is a shorthand for sum(a[i]*b[x-i], i=0..x).
    // Note that [x 0 0 0 0 0] = [x*R].

    __int128 c, d;

    d  = (__int128)a[0] * b[3]
       + (__int128)a[1] * b[2]
       + (__int128)a[2] * b[1]
       + (__int128)a[3] * b[0];
    // [d 0 0 0] = [p3 0 0 0]
    c  = (__int128)a[4] * b[4];
    // [c 0 0 0 0 d 0 0 0] = [p8 0 0 0 0 p3 0 0 0]
    d += (c & M) * R; c >>= 52;
    // [c 0 0 0 0 0 d 0 0 0] = [p8 0 0 0 0 p3 0 0 0]
    uint64_t t3 = d & M; d >>= 52;
    // [c 0 0 0 0 d t3 0 0 0] = [p8 0 0 0 0 p3 0 0 0]

    d += (__int128)a[0] * b[4]
       + (__int128)a[1] * b[3]
       + (__int128)a[2] * b[2]
       + (__int128)a[3] * b[1]
       + (__int128)a[4] * b[0];
    // [c 0 0 0 0 d t3 0 0 0] = [p8 0 0 0 p4 p3 0 0 0]
    d += c * R;
    // [d t3 0 0 0] = [p8 0 0 0 p4 p3 0 0 0]
    uint64_t t4 = d & M; d >>= 52;
    // [d t4 t3 0 0 0] = [p8 0 0 0 p4 p3 0 0 0]
    uint64_t tx = (t4 >> 48); t4 &= (M >> 4);
    // [d t4+(tx<<48) t3 0 0 0] = [p8 0 0 0 p4 p3 0 0 0]

    c  = (__int128)a[0] * b[0];
    // [d t4+(tx<<48) t3 0 0 c] = [p8 0 0 0 p4 p3 0 0 p0]
    d += (__int128)a[1] * b[4]
       + (__int128)a[2] * b[3]
       + (__int128)a[3] * b[2]
       + (__int128)a[4] * b[1];
    // [d t4+(tx<<48) t3 0 0 c] = [p8 0 0 p5 p4 p3 0 0 p0]
    uint64_t u0 = d & M; d >>= 52;
    // [d u0 t4+(tx<<48) t3 0 0 c] = [p8 0 0 p5 p4 p3 0 0 p0]
    // [d 0 t4+(tx<<48)+(u0<<52) t3 0 0 c] = [p8 0 0 p5 p4 p3 0 0 p0]
    u0 = (u0 << 4) | tx;
    // [d 0 t4+(u0<<48) t3 0 0 c] = [p8 0 0 p5 p4 p3 0 0 p0]
    c += (__int128)u0 * (R >> 4);
    // [d 0 t4 t3 0 0 c] = [p8 0 0 p5 p4 p3 0 0 p0]
    uint64_t t0 = c & M; c >>= 52;
    // [d 0 t4 t3 0 c t0] = [p8 0 0 p5 p4 p3 0 0 p0]

    c += (__int128)a[0] * b[1]
       + (__int128)a[1] * b[0];
    // [d 0 t4 t3 0 c t0] = [p8 0 0 p5 p4 p3 0 p1 p0]
    d += (__int128)a[2] * b[4]
       + (__int128)a[3] * b[3]
       + (__int128)a[4] * b[2];
    // [d 0 t4 t3 0 c t0] = [p8 0 p6 p5 p4 p3 0 p1 p0]
    c += (d & M) * R; d >>= 52;
    // [d 0 0 t4 t3 0 c t0] = [p8 0 p6 p5 p4 p3 0 p1 p0]
    uint64_t t1 = c & M; c >>= 52;
    // [d 0 0 t4 t3 c t1 t0] = [p8 0 p6 p5 p4 p3 0 p1 p0]

    c += (__int128)a[0] * b[2]
       + (__int128)a[1] * b[1]
       + (__int128)a[2] * b[0];
    // [d 0 0 t4 t3 c t1 t0] = [p8 0 p6 p5 p4 p3 p2 p1 p0]
    d += (__int128)a[3] * b[4]
       + (__int128)a[4] * b[3];
    // [d 0 0 t4 t3 c t1 t0] = [p8 p7 p6 p5 p4 p3 p2 p1 p0]
    c += (d & M) * R; d >>= 52;
    // [d 0 0 0 t4 t3 c t1 t0] = [p8 p7 p6 p5 p4 p3 p2 p1 p0]

    r[0] = t0;
    // [d 0 0 0 t4 t3 c t1 r0] = [p8 p7 p6 p5 p4 p3 p2 p1 p0]
    r[1] = t1;
    // [d 0 0 0 t4 t3 c r1 r0] = [p8 p7 p6 p5 p4 p3 p2 p1 p0]
    r[2] = c & M; c >>= 52;
    // [d 0 0 0 t4 t3+c r2 r1 r0] = [p8 p7 p6 p5 p4 p3 p2 p1 p0]
    c   += d * R + t3;;
    // [t4 c r2 r1 r0] = [p8 p7 p6 p5 p4 p3 p2 p1 p0]
    r[3] = c & M; c >>= 52;
    // [t4+c r3 r2 r1 r0] = [p8 p7 p6 p5 p4 p3 p2 p1 p0]
    c   += t4;
    // [c r3 r2 r1 r0] = [p8 p7 p6 p5 p4 p3 p2 p1 p0]
    r[4] = c; 
    // [r4 r3 r2 r1 r0] = [p8 p7 p6 p5 p4 p3 p2 p1 p0]
}

SECP256K1_INLINE static void secp256k1_fe_sqr_inner(const uint64_t *a, uint64_t *r) {

    const uint64_t M = 0xFFFFFFFFFFFFFULL, R = 0x1000003D10ULL;
    // [... a b c] is a shorthand for ... + a<<104 + b<<52 + c<<0 mod n.
    // px is a shorthand for sum(a[i]*a[x-i], i=0..x).
    // Note that [x 0 0 0 0 0] = [x*R].

    __int128 c, d;

    uint64_t a0 = a[0], a1 = a[1], a2 = a[2], a3 = a[3], a4 = a[4];

    d  = (__int128)(a0*2) * a3
       + (__int128)(a1*2) * a2;
    // [d 0 0 0] = [p3 0 0 0]
    c  = (__int128)a4 * a4;
    // [c 0 0 0 0 d 0 0 0] = [p8 0 0 0 0 p3 0 0 0]
    d += (c & M) * R; c >>= 52;
    // [c 0 0 0 0 0 d 0 0 0] = [p8 0 0 0 0 p3 0 0 0]
    uint64_t t3 = d & M; d >>= 52;
    // [c 0 0 0 0 d t3 0 0 0] = [p8 0 0 0 0 p3 0 0 0]

    a4 *= 2;
    d += (__int128)a0 * a4
       + (__int128)(a1*2) * a3
       + (__int128)a2 * a2;
    // [c 0 0 0 0 d t3 0 0 0] = [p8 0 0 0 p4 p3 0 0 0]
    d += c * R;
    // [d t3 0 0 0] = [p8 0 0 0 p4 p3 0 0 0]
    uint64_t t4 = d & M; d >>= 52;
    // [d t4 t3 0 0 0] = [p8 0 0 0 p4 p3 0 0 0]
    uint64_t tx = (t4 >> 48); t4 &= (M >> 4);
    // [d t4+(tx<<48) t3 0 0 0] = [p8 0 0 0 p4 p3 0 0 0]

    c  = (__int128)a0 * a0;
    // [d t4+(tx<<48) t3 0 0 c] = [p8 0 0 0 p4 p3 0 0 p0]
    d += (__int128)a1 * a4
       + (__int128)(a2*2) * a3;
    // [d t4+(tx<<48) t3 0 0 c] = [p8 0 0 p5 p4 p3 0 0 p0]
    uint64_t u0 = d & M; d >>= 52;
    // [d u0 t4+(tx<<48) t3 0 0 c] = [p8 0 0 p5 p4 p3 0 0 p0]
    // [d 0 t4+(tx<<48)+(u0<<52) t3 0 0 c] = [p8 0 0 p5 p4 p3 0 0 p0]
    u0 = (u0 << 4) | tx;
    // [d 0 t4+(u0<<48) t3 0 0 c] = [p8 0 0 p5 p4 p3 0 0 p0]
    c += (__int128)u0 * (R >> 4);
    // [d 0 t4 t3 0 0 c] = [p8 0 0 p5 p4 p3 0 0 p0]
    r[0] = c & M; c >>= 52;
    // [d 0 t4 t3 0 c r0] = [p8 0 0 p5 p4 p3 0 0 p0]

    a0 *= 2;
    c += (__int128)a0 * a1;
    // [d 0 t4 t3 0 c r0] = [p8 0 0 p5 p4 p3 0 p1 p0]
    d += (__int128)a2 * a4
       + (__int128)a3 * a3;
    // [d 0 t4 t3 0 c r0] = [p8 0 p6 p5 p4 p3 0 p1 p0]
    c += (d & M) * R; d >>= 52;
    // [d 0 0 t4 t3 0 c r0] = [p8 0 p6 p5 p4 p3 0 p1 p0]
    r[1] = c & M; c >>= 52;
    // [d 0 0 t4 t3 c r1 r0] = [p8 0 p6 p5 p4 p3 0 p1 p0]

    c += (__int128)a0 * a2
       + (__int128)a1 * a1;
    // [d 0 0 t4 t3 c r1 r0] = [p8 0 p6 p5 p4 p3 p2 p1 p0]
    d += (__int128)a3 * a4;
    // [d 0 0 t4 t3 c r1 r0] = [p8 p7 p6 p5 p4 p3 p2 p1 p0]
    c += (d & M) * R; d >>= 52;
    // [d 0 0 0 t4 t3 c r1 r0] = [p8 p7 p6 p5 p4 p3 p2 p1 p0]
    r[2] = c & M; c >>= 52;
    // [d 0 0 0 t4 t3+c r2 r1 r0] = [p8 p7 p6 p5 p4 p3 p2 p1 p0]

    c   += d * R + t3;;
    // [t4 c r2 r1 r0] = [p8 p7 p6 p5 p4 p3 p2 p1 p0]
    r[3] = c & M; c >>= 52;
    // [t4+c r3 r2 r1 r0] = [p8 p7 p6 p5 p4 p3 p2 p1 p0]
    c   += t4;
    // [c r3 r2 r1 r0] = [p8 p7 p6 p5 p4 p3 p2 p1 p0]
    r[4] = c; 
    // [r4 r3 r2 r1 r0] = [p8 p7 p6 p5 p4 p3 p2 p1 p0]
}

#endif
