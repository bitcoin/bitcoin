/***********************************************************************
 * Copyright (c) 2013, 2014 Pieter Wuille                              *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_FIELD_INNER5X52_IMPL_H
#define SECP256K1_FIELD_INNER5X52_IMPL_H

#include <stdint.h>

#include "int128.h"

#ifdef VERIFY
#define VERIFY_BITS(x, n) VERIFY_CHECK(((x) >> (n)) == 0)
#define VERIFY_BITS_128(x, n) VERIFY_CHECK(secp256k1_u128_check_bits((x), (n)))
#else
#define VERIFY_BITS(x, n) do { } while(0)
#define VERIFY_BITS_128(x, n) do { } while(0)
#endif

SECP256K1_INLINE static void secp256k1_fe_mul_inner(uint64_t *r, const uint64_t *a, const uint64_t * SECP256K1_RESTRICT b) {
    secp256k1_uint128 c, d;
    uint64_t t3, t4, tx, u0;
    uint64_t a0 = a[0], a1 = a[1], a2 = a[2], a3 = a[3], a4 = a[4];
    const uint64_t M = 0xFFFFFFFFFFFFFULL, R = 0x1000003D10ULL;

    VERIFY_BITS(a[0], 56);
    VERIFY_BITS(a[1], 56);
    VERIFY_BITS(a[2], 56);
    VERIFY_BITS(a[3], 56);
    VERIFY_BITS(a[4], 52);
    VERIFY_BITS(b[0], 56);
    VERIFY_BITS(b[1], 56);
    VERIFY_BITS(b[2], 56);
    VERIFY_BITS(b[3], 56);
    VERIFY_BITS(b[4], 52);
    VERIFY_CHECK(r != b);
    VERIFY_CHECK(a != b);

    /*  [... a b c] is a shorthand for ... + a<<104 + b<<52 + c<<0 mod n.
     *  for 0 <= x <= 4, px is a shorthand for sum(a[i]*b[x-i], i=0..x).
     *  for 4 <= x <= 8, px is a shorthand for sum(a[i]*b[x-i], i=(x-4)..4)
     *  Note that [x 0 0 0 0 0] = [x*R].
     */

    secp256k1_u128_mul(&d, a0, b[3]);
    secp256k1_u128_accum_mul(&d, a1, b[2]);
    secp256k1_u128_accum_mul(&d, a2, b[1]);
    secp256k1_u128_accum_mul(&d, a3, b[0]);
    VERIFY_BITS_128(&d, 114);
    /* [d 0 0 0] = [p3 0 0 0] */
    secp256k1_u128_mul(&c, a4, b[4]);
    VERIFY_BITS_128(&c, 112);
    /* [c 0 0 0 0 d 0 0 0] = [p8 0 0 0 0 p3 0 0 0] */
    secp256k1_u128_accum_mul(&d, R, secp256k1_u128_to_u64(&c)); secp256k1_u128_rshift(&c, 64);
    VERIFY_BITS_128(&d, 115);
    VERIFY_BITS_128(&c, 48);
    /* [(c<<12) 0 0 0 0 0 d 0 0 0] = [p8 0 0 0 0 p3 0 0 0] */
    t3 = secp256k1_u128_to_u64(&d) & M; secp256k1_u128_rshift(&d, 52);
    VERIFY_BITS(t3, 52);
    VERIFY_BITS_128(&d, 63);
    /* [(c<<12) 0 0 0 0 d t3 0 0 0] = [p8 0 0 0 0 p3 0 0 0] */

    secp256k1_u128_accum_mul(&d, a0, b[4]);
    secp256k1_u128_accum_mul(&d, a1, b[3]);
    secp256k1_u128_accum_mul(&d, a2, b[2]);
    secp256k1_u128_accum_mul(&d, a3, b[1]);
    secp256k1_u128_accum_mul(&d, a4, b[0]);
    VERIFY_BITS_128(&d, 115);
    /* [(c<<12) 0 0 0 0 d t3 0 0 0] = [p8 0 0 0 p4 p3 0 0 0] */
    secp256k1_u128_accum_mul(&d, R << 12, secp256k1_u128_to_u64(&c));
    VERIFY_BITS_128(&d, 116);
    /* [d t3 0 0 0] = [p8 0 0 0 p4 p3 0 0 0] */
    t4 = secp256k1_u128_to_u64(&d) & M; secp256k1_u128_rshift(&d, 52);
    VERIFY_BITS(t4, 52);
    VERIFY_BITS_128(&d, 64);
    /* [d t4 t3 0 0 0] = [p8 0 0 0 p4 p3 0 0 0] */
    tx = (t4 >> 48); t4 &= (M >> 4);
    VERIFY_BITS(tx, 4);
    VERIFY_BITS(t4, 48);
    /* [d t4+(tx<<48) t3 0 0 0] = [p8 0 0 0 p4 p3 0 0 0] */

    secp256k1_u128_mul(&c, a0, b[0]);
    VERIFY_BITS_128(&c, 112);
    /* [d t4+(tx<<48) t3 0 0 c] = [p8 0 0 0 p4 p3 0 0 p0] */
    secp256k1_u128_accum_mul(&d, a1, b[4]);
    secp256k1_u128_accum_mul(&d, a2, b[3]);
    secp256k1_u128_accum_mul(&d, a3, b[2]);
    secp256k1_u128_accum_mul(&d, a4, b[1]);
    VERIFY_BITS_128(&d, 115);
    /* [d t4+(tx<<48) t3 0 0 c] = [p8 0 0 p5 p4 p3 0 0 p0] */
    u0 = secp256k1_u128_to_u64(&d) & M; secp256k1_u128_rshift(&d, 52);
    VERIFY_BITS(u0, 52);
    VERIFY_BITS_128(&d, 63);
    /* [d u0 t4+(tx<<48) t3 0 0 c] = [p8 0 0 p5 p4 p3 0 0 p0] */
    /* [d 0 t4+(tx<<48)+(u0<<52) t3 0 0 c] = [p8 0 0 p5 p4 p3 0 0 p0] */
    u0 = (u0 << 4) | tx;
    VERIFY_BITS(u0, 56);
    /* [d 0 t4+(u0<<48) t3 0 0 c] = [p8 0 0 p5 p4 p3 0 0 p0] */
    secp256k1_u128_accum_mul(&c, u0, R >> 4);
    VERIFY_BITS_128(&c, 115);
    /* [d 0 t4 t3 0 0 c] = [p8 0 0 p5 p4 p3 0 0 p0] */
    r[0] = secp256k1_u128_to_u64(&c) & M; secp256k1_u128_rshift(&c, 52);
    VERIFY_BITS(r[0], 52);
    VERIFY_BITS_128(&c, 61);
    /* [d 0 t4 t3 0 c r0] = [p8 0 0 p5 p4 p3 0 0 p0] */

    secp256k1_u128_accum_mul(&c, a0, b[1]);
    secp256k1_u128_accum_mul(&c, a1, b[0]);
    VERIFY_BITS_128(&c, 114);
    /* [d 0 t4 t3 0 c r0] = [p8 0 0 p5 p4 p3 0 p1 p0] */
    secp256k1_u128_accum_mul(&d, a2, b[4]);
    secp256k1_u128_accum_mul(&d, a3, b[3]);
    secp256k1_u128_accum_mul(&d, a4, b[2]);
    VERIFY_BITS_128(&d, 114);
    /* [d 0 t4 t3 0 c r0] = [p8 0 p6 p5 p4 p3 0 p1 p0] */
    secp256k1_u128_accum_mul(&c, secp256k1_u128_to_u64(&d) & M, R); secp256k1_u128_rshift(&d, 52);
    VERIFY_BITS_128(&c, 115);
    VERIFY_BITS_128(&d, 62);
    /* [d 0 0 t4 t3 0 c r0] = [p8 0 p6 p5 p4 p3 0 p1 p0] */
    r[1] = secp256k1_u128_to_u64(&c) & M; secp256k1_u128_rshift(&c, 52);
    VERIFY_BITS(r[1], 52);
    VERIFY_BITS_128(&c, 63);
    /* [d 0 0 t4 t3 c r1 r0] = [p8 0 p6 p5 p4 p3 0 p1 p0] */

    secp256k1_u128_accum_mul(&c, a0, b[2]);
    secp256k1_u128_accum_mul(&c, a1, b[1]);
    secp256k1_u128_accum_mul(&c, a2, b[0]);
    VERIFY_BITS_128(&c, 114);
    /* [d 0 0 t4 t3 c r1 r0] = [p8 0 p6 p5 p4 p3 p2 p1 p0] */
    secp256k1_u128_accum_mul(&d, a3, b[4]);
    secp256k1_u128_accum_mul(&d, a4, b[3]);
    VERIFY_BITS_128(&d, 114);
    /* [d 0 0 t4 t3 c t1 r0] = [p8 p7 p6 p5 p4 p3 p2 p1 p0] */
    secp256k1_u128_accum_mul(&c, R, secp256k1_u128_to_u64(&d)); secp256k1_u128_rshift(&d, 64);
    VERIFY_BITS_128(&c, 115);
    VERIFY_BITS_128(&d, 50);
    /* [(d<<12) 0 0 0 t4 t3 c r1 r0] = [p8 p7 p6 p5 p4 p3 p2 p1 p0] */

    r[2] = secp256k1_u128_to_u64(&c) & M; secp256k1_u128_rshift(&c, 52);
    VERIFY_BITS(r[2], 52);
    VERIFY_BITS_128(&c, 63);
    /* [(d<<12) 0 0 0 t4 t3+c r2 r1 r0] = [p8 p7 p6 p5 p4 p3 p2 p1 p0] */
    secp256k1_u128_accum_mul(&c, R << 12, secp256k1_u128_to_u64(&d));
    secp256k1_u128_accum_u64(&c, t3);
    VERIFY_BITS_128(&c, 100);
    /* [t4 c r2 r1 r0] = [p8 p7 p6 p5 p4 p3 p2 p1 p0] */
    r[3] = secp256k1_u128_to_u64(&c) & M; secp256k1_u128_rshift(&c, 52);
    VERIFY_BITS(r[3], 52);
    VERIFY_BITS_128(&c, 48);
    /* [t4+c r3 r2 r1 r0] = [p8 p7 p6 p5 p4 p3 p2 p1 p0] */
    r[4] = secp256k1_u128_to_u64(&c) + t4;
    VERIFY_BITS(r[4], 49);
    /* [r4 r3 r2 r1 r0] = [p8 p7 p6 p5 p4 p3 p2 p1 p0] */
}

SECP256K1_INLINE static void secp256k1_fe_sqr_inner(uint64_t *r, const uint64_t *a) {
    secp256k1_uint128 c, d;
    uint64_t a0 = a[0], a1 = a[1], a2 = a[2], a3 = a[3], a4 = a[4];
    int64_t t3, t4, tx, u0;
    const uint64_t M = 0xFFFFFFFFFFFFFULL, R = 0x1000003D10ULL;

    VERIFY_BITS(a[0], 56);
    VERIFY_BITS(a[1], 56);
    VERIFY_BITS(a[2], 56);
    VERIFY_BITS(a[3], 56);
    VERIFY_BITS(a[4], 52);

    /**  [... a b c] is a shorthand for ... + a<<104 + b<<52 + c<<0 mod n.
     *  px is a shorthand for sum(a[i]*a[x-i], i=0..x).
     *  Note that [x 0 0 0 0 0] = [x*R].
     */

    secp256k1_u128_mul(&d, a0*2, a3);
    secp256k1_u128_accum_mul(&d, a1*2, a2);
    VERIFY_BITS_128(&d, 114);
    /* [d 0 0 0] = [p3 0 0 0] */
    secp256k1_u128_mul(&c, a4, a4);
    VERIFY_BITS_128(&c, 112);
    /* [c 0 0 0 0 d 0 0 0] = [p8 0 0 0 0 p3 0 0 0] */
    secp256k1_u128_accum_mul(&d, R, secp256k1_u128_to_u64(&c)); secp256k1_u128_rshift(&c, 64);
    VERIFY_BITS_128(&d, 115);
    VERIFY_BITS_128(&c, 48);
    /* [(c<<12) 0 0 0 0 0 d 0 0 0] = [p8 0 0 0 0 p3 0 0 0] */
    t3 = secp256k1_u128_to_u64(&d) & M; secp256k1_u128_rshift(&d, 52);
    VERIFY_BITS(t3, 52);
    VERIFY_BITS_128(&d, 63);
    /* [(c<<12) 0 0 0 0 d t3 0 0 0] = [p8 0 0 0 0 p3 0 0 0] */

    a4 *= 2;
    secp256k1_u128_accum_mul(&d, a0, a4);
    secp256k1_u128_accum_mul(&d, a1*2, a3);
    secp256k1_u128_accum_mul(&d, a2, a2);
    VERIFY_BITS_128(&d, 115);
    /* [(c<<12) 0 0 0 0 d t3 0 0 0] = [p8 0 0 0 p4 p3 0 0 0] */
    secp256k1_u128_accum_mul(&d, R << 12, secp256k1_u128_to_u64(&c));
    VERIFY_BITS_128(&d, 116);
    /* [d t3 0 0 0] = [p8 0 0 0 p4 p3 0 0 0] */
    t4 = secp256k1_u128_to_u64(&d) & M; secp256k1_u128_rshift(&d, 52);
    VERIFY_BITS(t4, 52);
    VERIFY_BITS_128(&d, 64);
    /* [d t4 t3 0 0 0] = [p8 0 0 0 p4 p3 0 0 0] */
    tx = (t4 >> 48); t4 &= (M >> 4);
    VERIFY_BITS(tx, 4);
    VERIFY_BITS(t4, 48);
    /* [d t4+(tx<<48) t3 0 0 0] = [p8 0 0 0 p4 p3 0 0 0] */

    secp256k1_u128_mul(&c, a0, a0);
    VERIFY_BITS_128(&c, 112);
    /* [d t4+(tx<<48) t3 0 0 c] = [p8 0 0 0 p4 p3 0 0 p0] */
    secp256k1_u128_accum_mul(&d, a1, a4);
    secp256k1_u128_accum_mul(&d, a2*2, a3);
    VERIFY_BITS_128(&d, 114);
    /* [d t4+(tx<<48) t3 0 0 c] = [p8 0 0 p5 p4 p3 0 0 p0] */
    u0 = secp256k1_u128_to_u64(&d) & M; secp256k1_u128_rshift(&d, 52);
    VERIFY_BITS(u0, 52);
    VERIFY_BITS_128(&d, 62);
    /* [d u0 t4+(tx<<48) t3 0 0 c] = [p8 0 0 p5 p4 p3 0 0 p0] */
    /* [d 0 t4+(tx<<48)+(u0<<52) t3 0 0 c] = [p8 0 0 p5 p4 p3 0 0 p0] */
    u0 = (u0 << 4) | tx;
    VERIFY_BITS(u0, 56);
    /* [d 0 t4+(u0<<48) t3 0 0 c] = [p8 0 0 p5 p4 p3 0 0 p0] */
    secp256k1_u128_accum_mul(&c, u0, R >> 4);
    VERIFY_BITS_128(&c, 113);
    /* [d 0 t4 t3 0 0 c] = [p8 0 0 p5 p4 p3 0 0 p0] */
    r[0] = secp256k1_u128_to_u64(&c) & M; secp256k1_u128_rshift(&c, 52);
    VERIFY_BITS(r[0], 52);
    VERIFY_BITS_128(&c, 61);
    /* [d 0 t4 t3 0 c r0] = [p8 0 0 p5 p4 p3 0 0 p0] */

    a0 *= 2;
    secp256k1_u128_accum_mul(&c, a0, a1);
    VERIFY_BITS_128(&c, 114);
    /* [d 0 t4 t3 0 c r0] = [p8 0 0 p5 p4 p3 0 p1 p0] */
    secp256k1_u128_accum_mul(&d, a2, a4);
    secp256k1_u128_accum_mul(&d, a3, a3);
    VERIFY_BITS_128(&d, 114);
    /* [d 0 t4 t3 0 c r0] = [p8 0 p6 p5 p4 p3 0 p1 p0] */
    secp256k1_u128_accum_mul(&c, secp256k1_u128_to_u64(&d) & M, R); secp256k1_u128_rshift(&d, 52);
    VERIFY_BITS_128(&c, 115);
    VERIFY_BITS_128(&d, 62);
    /* [d 0 0 t4 t3 0 c r0] = [p8 0 p6 p5 p4 p3 0 p1 p0] */
    r[1] = secp256k1_u128_to_u64(&c) & M; secp256k1_u128_rshift(&c, 52);
    VERIFY_BITS(r[1], 52);
    VERIFY_BITS_128(&c, 63);
    /* [d 0 0 t4 t3 c r1 r0] = [p8 0 p6 p5 p4 p3 0 p1 p0] */

    secp256k1_u128_accum_mul(&c, a0, a2);
    secp256k1_u128_accum_mul(&c, a1, a1);
    VERIFY_BITS_128(&c, 114);
    /* [d 0 0 t4 t3 c r1 r0] = [p8 0 p6 p5 p4 p3 p2 p1 p0] */
    secp256k1_u128_accum_mul(&d, a3, a4);
    VERIFY_BITS_128(&d, 114);
    /* [d 0 0 t4 t3 c r1 r0] = [p8 p7 p6 p5 p4 p3 p2 p1 p0] */
    secp256k1_u128_accum_mul(&c, R, secp256k1_u128_to_u64(&d)); secp256k1_u128_rshift(&d, 64);
    VERIFY_BITS_128(&c, 115);
    VERIFY_BITS_128(&d, 50);
    /* [(d<<12) 0 0 0 t4 t3 c r1 r0] = [p8 p7 p6 p5 p4 p3 p2 p1 p0] */
    r[2] = secp256k1_u128_to_u64(&c) & M; secp256k1_u128_rshift(&c, 52);
    VERIFY_BITS(r[2], 52);
    VERIFY_BITS_128(&c, 63);
    /* [(d<<12) 0 0 0 t4 t3+c r2 r1 r0] = [p8 p7 p6 p5 p4 p3 p2 p1 p0] */

    secp256k1_u128_accum_mul(&c, R << 12, secp256k1_u128_to_u64(&d));
    secp256k1_u128_accum_u64(&c, t3);
    VERIFY_BITS_128(&c, 100);
    /* [t4 c r2 r1 r0] = [p8 p7 p6 p5 p4 p3 p2 p1 p0] */
    r[3] = secp256k1_u128_to_u64(&c) & M; secp256k1_u128_rshift(&c, 52);
    VERIFY_BITS(r[3], 52);
    VERIFY_BITS_128(&c, 48);
    /* [t4+c r3 r2 r1 r0] = [p8 p7 p6 p5 p4 p3 p2 p1 p0] */
    r[4] = secp256k1_u128_to_u64(&c) + t4;
    VERIFY_BITS(r[4], 49);
    /* [r4 r3 r2 r1 r0] = [p8 p7 p6 p5 p4 p3 p2 p1 p0] */
}

#endif /* SECP256K1_FIELD_INNER5X52_IMPL_H */
