// Copyright (c) 2013 Pieter Wuille
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef _SECP256K1_FIELD_INNER5X52_IMPL_H_
#define _SECP256K1_FIELD_INNER5X52_IMPL_H_

#include <stdint.h>

SECP256K1_INLINE static void secp256k1_fe_mul_inner(const uint64_t *a, const uint64_t *b, uint64_t *r) {

    const uint64_t M = 0xFFFFFFFFFFFFFULL, R = 0x1000003D10ULL;

    __int128 c, d;

    d  = (__int128)a[0] * b[3]
       + (__int128)a[1] * b[2]
       + (__int128)a[2] * b[1]
       + (__int128)a[3] * b[0];
    c  = (__int128)a[4] * b[4];
    d += (c & M) * R; c >>= 52;
    uint64_t t3 = d & M; d >>= 52;

    d += (__int128)a[0] * b[4]
       + (__int128)a[1] * b[3]
       + (__int128)a[2] * b[2]
       + (__int128)a[3] * b[1]
       + (__int128)a[4] * b[0];
    d += c * R;
    uint64_t t4 = d & M; d >>= 52;
    uint64_t tx = (t4 >> 48); t4 &= (M >> 4);

    c  = (__int128)a[0] * b[0];
    d += (__int128)a[1] * b[4]
       + (__int128)a[2] * b[3]
       + (__int128)a[3] * b[2]
       + (__int128)a[4] * b[1];
    uint64_t u0 = d & M; d >>= 52;
    u0 = (u0 << 4) | tx;
    c += (__int128)u0 * (R >> 4);
    uint64_t t0 = c & M; c >>= 52;

    c += (__int128)a[0] * b[1]
       + (__int128)a[1] * b[0];
    d += (__int128)a[2] * b[4]
       + (__int128)a[3] * b[3]
       + (__int128)a[4] * b[2];
    c += (d & M) * R; d >>= 52;
    uint64_t t1 = c & M; c >>= 52;

    c += (__int128)a[0] * b[2]
       + (__int128)a[1] * b[1]
       + (__int128)a[2] * b[0];
    d += (__int128)a[3] * b[4]
       + (__int128)a[4] * b[3];
    c += (d & M) * R; d >>= 52;

    r[0] = t0;
    r[1] = t1;
    r[2] = c & M; c >>= 52;
    c   += d * R + t3;;
    r[3] = c & M; c >>= 52;
    c   += t4;
    r[4] = c; 
}

SECP256K1_INLINE static void secp256k1_fe_sqr_inner(const uint64_t *a, uint64_t *r) {

    const uint64_t M = 0xFFFFFFFFFFFFFULL, R = 0x1000003D10ULL;

    __int128 c, d;

    uint64_t a0 = a[0], a1 = a[1], a2 = a[2], a3 = a[3], a4 = a[4];

    d  = (__int128)(a0*2) * a3
       + (__int128)(a1*2) * a2;
    c  = (__int128)a4 * a4;
    d += (c & M) * R; c >>= 52;
    uint64_t t3 = d & M; d >>= 52;

    a4 *= 2;
    d += (__int128)a0 * a4
       + (__int128)(a1*2) * a3
       + (__int128)a2 * a2;
    d += c * R;
    uint64_t t4 = d & M; d >>= 52;
    uint64_t tx = (t4 >> 48); t4 &= (M >> 4);

    c  = (__int128)a0 * a0;
    d += (__int128)a1 * a4
       + (__int128)(a2*2) * a3;
    uint64_t u0 = d & M; d >>= 52;
    u0 = (u0 << 4) | tx;
    c += (__int128)u0 * (R >> 4);
    r[0] = c & M; c >>= 52;

    a0 *= 2;
    c += (__int128)a0 * a1;
    d += (__int128)a2 * a4
       + (__int128)a3 * a3;
    c += (d & M) * R; d >>= 52;
    r[1] = c & M; c >>= 52;

    c += (__int128)a0 * a2
       + (__int128)a1 * a1;
    d += (__int128)a3 * a4;
    c += (d & M) * R; d >>= 52;
    r[2] = c & M; c >>= 52;

    c   += d * R + t3;;
    r[3] = c & M; c >>= 52;
    c   += t4;
    r[4] = c; 
}

#endif
