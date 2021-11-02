/***********************************************************************
 * Copyright (c) 2021 Pieter Wuille                                    *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_MODULE_ELLSQ_MAIN_H
#define SECP256K1_MODULE_ELLSQ_MAIN_H

#include "../../../include/secp256k1.h"
#include "../../../include/secp256k1_ellsq.h"
#include "../../hash.h"

/* c1 = the square root of -3 ((-3)**((p+1)/4)). */
static const secp256k1_fe secp256k1_ellsq_c1 = SECP256K1_FE_CONST(0x0a2d2ba9, 0x3507f1df, 0x233770c2, 0xa797962c, 0xc61f6d15, 0xda14ecd4, 0x7d8d27ae, 0x1cd5f852);
/* c2 = (c1-1)/2 (a cube root of 1). */
static const secp256k1_fe secp256k1_ellsq_c2 = SECP256K1_FE_CONST(0x851695d4, 0x9a83f8ef, 0x919bb861, 0x53cbcb16, 0x630fb68a, 0xed0a766a, 0x3ec693d6, 0x8e6afa40);
/* c3 = (-c1-1)/2 (another cube root of 1). */
static const secp256k1_fe secp256k1_ellsq_c3 = SECP256K1_FE_CONST(0x7ae96a2b, 0x657c0710, 0x6e64479e, 0xac3434e9, 0x9cf04975, 0x12f58995, 0xc1396c28, 0x719501ee);
/* c4 = 16*(c1-1) */
static const secp256k1_fe secp256k1_ellsq_c4 = SECP256K1_FE_CONST(0xa2d2ba93, 0x507f1df2, 0x33770c2a, 0x797962cc, 0x61f6d15d, 0xa14ecd47, 0xd8d27ae1, 0xcd5f8510);
/* c5 = 1/2 */
static const secp256k1_fe secp256k1_ellsq_c5 = SECP256K1_FE_CONST(0x7fffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x7ffffe18);

/* Given a field element u, compute a group element out. This functions results in a
 * non-infinity point on the curve for every possible input u. It implements the function
 * f defined in secp256k1_ellsq.h.
 */
static void secp256k1_ellsq_fe_to_ge_var(secp256k1_ge* out, const secp256k1_fe* u) {
    secp256k1_fe t0, t1, t2, t3, t4, x, y;
    int ret;
    secp256k1_fe_sqr(&t0, u);
    secp256k1_fe_set_int(&t1, 8);
    secp256k1_fe_add(&t1, &t0);
    secp256k1_fe_mul(&t3, &t0, &secp256k1_ellsq_c1);
    secp256k1_fe_negate(&t3, &t3, 1);
    secp256k1_fe_mul(&t2, &t1, &secp256k1_ellsq_c2);
    secp256k1_fe_add(&t2, &t3);
    secp256k1_fe_sqr(&t4, &t1);
    secp256k1_fe_sqr(&t4, &t4);
    secp256k1_fe_mul_int(&t4, 7);
    secp256k1_fe_sqr(&t3, &t2);
    secp256k1_fe_mul(&t3, &t3, &t2);
    secp256k1_fe_mul(&t3, &t3, &t1);
    secp256k1_fe_add(&t3, &t4);
    if (secp256k1_fe_jacobi_var(&t3) >= 0) {
        ret = secp256k1_fe_sqrt(&t4, &t3);
        VERIFY_CHECK(ret);
        secp256k1_fe_inv_var(&t1, &t1);
        secp256k1_fe_mul(&x, &t1, &t2);
        secp256k1_fe_sqr(&t1, &t1);
        secp256k1_fe_mul(&y, &t1, &t4);
    } else {
        secp256k1_fe_add(&t2, &t1);
        secp256k1_fe_negate(&t2, &t2, 5);
        secp256k1_fe_sqr(&t3, &t2);
        secp256k1_fe_mul(&t3, &t3, &t2);
        secp256k1_fe_mul(&t3, &t3, &t1);
        secp256k1_fe_add(&t3, &t4);
        if (secp256k1_fe_jacobi_var(&t3) >= 0) {
            ret = secp256k1_fe_sqrt(&t4, &t3);
            VERIFY_CHECK(ret);
            secp256k1_fe_inv_var(&t1, &t1);
            secp256k1_fe_mul(&x, &t1, &t2);
            secp256k1_fe_sqr(&t1, &t1);
            secp256k1_fe_mul(&y, &t1, &t4);
        } else {
            secp256k1_fe_mul_int(&t0, 3);
            secp256k1_fe_inv_var(&t0, &t0);
            secp256k1_fe_sqr(&t1, &t1);
            secp256k1_fe_mul(&t0, &t0, &t1);
            secp256k1_fe_negate(&t0, &t0, 1);
            secp256k1_fe_set_int(&x, 1);
            secp256k1_fe_add(&x, &t0);
            secp256k1_fe_sqr(&t0, &x);
            secp256k1_fe_mul(&t0, &t0, &x);
            secp256k1_fe_set_int(&t1, 7);
            secp256k1_fe_add(&t1, &t0);
            ret = secp256k1_fe_sqrt(&y, &t1);
            VERIFY_CHECK(ret);
        }
    }
    t0 = *u;
    secp256k1_fe_normalize_var(&y);
    secp256k1_fe_normalize_var(&t0);
    if (secp256k1_fe_is_odd(&y) != secp256k1_fe_is_odd(&t0)) secp256k1_fe_negate(&y, &y, 1);
    secp256k1_ge_set_xy(out, &x, &y);
}

/* Given a point on the curve p, and an integer branch value i in [0,4), compute a
 * field element out which secp256k1_ellsq_fe_to_ge_var would map back to p, or
 * fail. Combining all non-failing outs for a given p, over all values of i,
 * results in the set of all preimages of p under secp256k1_ellsq_fe_to_ge_var. No
 * two (p, i) inputs map to the same out, if successful.
 *
 * i=0 will compute a preimage that maps to p using the "x1" above.
 * i=1 will compute a preimage that maps to p using the "x2" above.
 * i=2 and i=3 will compute a preimage that maps to using the "x3" above.
 *
 * All of them will fail if no preimage under the respective x formula exists.
 * When i>0, the function will fail when the would-be preimage maps to the curve
 * using a lowered-numbered x (so i=1 fails when its x1 lands on the curve, and
 * i=2 and i=3 fail when its x1 or x2 land on the curve). In addition, failure is
 * returned when a lowered-value i would result in the same preimage.
 */
static int secp256k1_ellsq_ge_to_fe_var(secp256k1_fe* out, const secp256k1_ge* p, int i) {
    int ret;
    secp256k1_fe t0, t1, t2, t3, u, x = p->x, y = p->y;
    secp256k1_fe_normalize_var(&x);
    secp256k1_fe_normalize_var(&y);
    VERIFY_CHECK(i >= 0);
    VERIFY_CHECK(i < 4);
    if (i < 2) {
        t0 = x;
        secp256k1_fe_mul_int(&t0, 2);
        secp256k1_fe_set_int(&t1, 1);
        secp256k1_fe_add(&t0, &t1);
        secp256k1_fe_negate(&t1, &t0, 3);
        secp256k1_fe_add(&t1, &secp256k1_ellsq_c1);
        secp256k1_fe_add(&t0, &secp256k1_ellsq_c1);
        secp256k1_fe_mul(&t2, &t0, &t1);
        secp256k1_fe_mul_int(&t2, 8);
        if (secp256k1_fe_jacobi_var(&t2) < 0) return 0;
        if (i == 0) {
            if (secp256k1_fe_normalizes_to_zero_var(&t0)) return 0;
            if (secp256k1_fe_normalizes_to_zero_var(&t1) && secp256k1_fe_is_odd(&y)) return 0;
            ret = secp256k1_fe_sqrt(&t1, &t2);
            VERIFY_CHECK(ret);
            secp256k1_fe_inv_var(&t0, &t0);
            secp256k1_fe_mul(&u, &t0, &t1);
        } else { /* i == 1 */
            secp256k1_fe_set_int(&t0, 1);
            secp256k1_fe_add(&t0, &x);
            secp256k1_fe_negate(&t0, &t0, 2);
            secp256k1_fe_sqr(&t3, &t0);
            secp256k1_fe_mul(&t0, &t0, &t3);
            secp256k1_fe_set_int(&t3, 7);
            secp256k1_fe_add(&t0, &t3);
            if (secp256k1_fe_jacobi_var(&t0) >= 0) return 0;
            ret = secp256k1_fe_sqrt(&t0, &t2);
            VERIFY_CHECK(ret);
            secp256k1_fe_inv_var(&t1, &t1);
            secp256k1_fe_mul(&u, &t0, &t1);
        }
    } else {
        t0 = x;
        secp256k1_fe_mul_int(&t0, 6);
        secp256k1_fe_set_int(&t1, 26);
        secp256k1_fe_add(&t0, &t1);
        secp256k1_fe_sqr(&t1, &t0);
        secp256k1_fe_set_int(&t2, 1024);
        secp256k1_fe_negate(&t2, &t2, 1);
        secp256k1_fe_add(&t2, &t1);
        if (secp256k1_fe_jacobi_var(&t2) < 0) return 0;
        ret = secp256k1_fe_sqrt(&t1, &t2);
        VERIFY_CHECK(ret);
        if (i == 3) {
            if (secp256k1_fe_normalizes_to_zero_var(&t1)) return 0;
            secp256k1_fe_negate(&t1, &t1, 1);
        }
        secp256k1_fe_negate(&t0, &t0, 7);
        secp256k1_fe_add(&t0, &t1);
        if (secp256k1_fe_jacobi_var(&t0) < 0) return 0;
        secp256k1_fe_set_int(&t1, 32);
        secp256k1_fe_normalize_weak(&t0);
        secp256k1_fe_add(&t1, &t0);
        secp256k1_fe_mul(&t2, &t0, &secp256k1_ellsq_c3);
        secp256k1_fe_add(&t2, &secp256k1_ellsq_c4);
        secp256k1_fe_sqr(&t3, &t2);
        secp256k1_fe_mul(&t3, &t3, &t2);
        secp256k1_fe_mul(&t3, &t3, &t1);
        secp256k1_fe_sqr(&t1, &t1);
        secp256k1_fe_sqr(&t1, &t1);
        secp256k1_fe_mul_int(&t1, 7);
        secp256k1_fe_add(&t3, &t1);
        if (secp256k1_fe_jacobi_var(&t3) >= 0) return 0;
        ret = secp256k1_fe_sqrt(&u, &t0);
        VERIFY_CHECK(ret);
        secp256k1_fe_mul(&u, &u, &secp256k1_ellsq_c5);
    }
    secp256k1_fe_normalize_var(&u);
    if (secp256k1_fe_is_odd(&u) != secp256k1_fe_is_odd(&y)) {
        secp256k1_fe_negate(&u, &u, 1);
    }
    *out = u;
    return 1;
}

int secp256k1_ellsq_encode(const secp256k1_context* ctx, unsigned char *ell64, const unsigned char *rnd32, const secp256k1_pubkey *pubkey) {
    secp256k1_ge p;
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(ell64 != NULL);
    ARG_CHECK(rnd32 != NULL);
    ARG_CHECK(pubkey != NULL);

    if (secp256k1_pubkey_load(ctx, &p, pubkey)) {
        uint32_t cnt = 0;
        /* Field elements and branch values are extracted from
         * SHA256("secp256k1_ellsq_encode\x00" + uint32{cnt} + rnd32 + X + byte{Y & 1})
         * for consecutive values of cnt. cnt==0 is first used to populate a pool of
         * 128 2-bit branch values. The 128 cnt values that follow are used to
         * generate field elements u1. cnt==129 (and multiples thereof) are used to
         * repopulate the pool and start over, if that were ever necessary. */
        unsigned char hashdata[23 + 4 + 32 + 32 + 1] = "secp256k1_ellsq_encode";
        /* Pool of 2-bit branch values. */
        unsigned char branch_hash[32];
        /* Number of 2-bit values in branch_hash left. */
        int branches_left = 0;
        /* Fill up hashdata, excluding i. */
        memcpy(hashdata + 23 + 4, rnd32, 32);
        secp256k1_fe_get_b32(hashdata + 23 + 4 + 32, &p.x);
        hashdata[4 + 23 + 32 + 32] = secp256k1_fe_is_odd(&p.y);
        while (1) {
            int branch;
            secp256k1_fe u1, u2;
            secp256k1_ge q;
            secp256k1_gej qj;
            /* If the pool of branch values is empty, populate it. */
            if (branches_left == 0) {
                secp256k1_sha256 hash;
                hashdata[23 + 0] = cnt;
                hashdata[23 + 1] = cnt >> 8;
                hashdata[23 + 2] = cnt >> 16;
                hashdata[23 + 3] = cnt >> 24;
                secp256k1_sha256_initialize(&hash);
                secp256k1_sha256_write(&hash, hashdata, sizeof(hashdata));
                secp256k1_sha256_finalize(&hash, branch_hash);
                ++cnt;
                branches_left = 128;
            }
            /* Take a 2-bit branch value from the branch pool. */
            --branches_left;
            branch = (branch_hash[(127 - branches_left) >> 2] >> (((127 - branches_left) & 3) << 1)) & 3;
            /* Compute a new u1 value by hashing (a potential first 32 bytes of the output). */
            {
                secp256k1_sha256 hash;
                hashdata[23 + 0] = cnt;
                hashdata[23 + 1] = cnt >> 8;
                hashdata[23 + 2] = cnt >> 16;
                hashdata[23 + 3] = cnt >> 24;
                secp256k1_sha256_initialize(&hash);
                secp256k1_sha256_write(&hash, hashdata, sizeof(hashdata));
                secp256k1_sha256_finalize(&hash, ell64);
                ++cnt;
            }
            if (!secp256k1_fe_set_b32(&u1, ell64)) continue;
            /* Compute the remainder Q to encode in the last 32 bytes of the output. */
            secp256k1_ellsq_fe_to_ge_var(&q, &u1);
            secp256k1_ge_neg(&q, &q);
            secp256k1_gej_set_ge(&qj, &q);
            secp256k1_gej_add_ge_var(&qj, &qj, &p, NULL);
            if (!secp256k1_gej_is_infinity(&qj)) {
                 secp256k1_ge_set_gej_var(&q, &qj);
            } else {
                /* If Q=P-f(u1) is infinity, it means we're trying to encode P=f(u1).
                 * While no u2 exists such that f(u2)=Q in that case, it is still
                 * possible to encode P due to the special rule that if f(u1)+f(u2)
                 * is infinity, the result of decoding is f(u1).
                 * In other words, we're not trying to reach f(u2)=P-f(u1), but
                 * f(u2)=-f(u1) instead. -f(u1) is exactly what the variable q
                 * already holds at this point. Note that if u1 is generated using
                 * a secure hash function, the probability of reaching this branch
                 * is negligible. */
            }
            /* Try to find a u2 value which encodes Q. */
            if (secp256k1_ellsq_ge_to_fe_var(&u2, &q, branch)) {
                /* If that succeeds, store it in the output. */
                secp256k1_fe_normalize_var(&u2);
                secp256k1_fe_get_b32(ell64 + 32, &u2);
                break;
            }
        }
        memset(hashdata, 0, sizeof(hashdata));
        return 1;
    }
    /* Only returned in case the provided pubkey is invalid. */
    return 0;
}

int secp256k1_ellsq_decode(const secp256k1_context* ctx, secp256k1_pubkey *pubkey, const unsigned char *ell64) {
    secp256k1_fe f1, f2;
    secp256k1_ge p1, p2;
    secp256k1_gej acc;
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(pubkey != NULL);
    ARG_CHECK(ell64 != NULL);

    secp256k1_fe_set_b32(&f1, ell64);
    secp256k1_fe_set_b32(&f2, ell64 + 32);
    secp256k1_ellsq_fe_to_ge_var(&p1, &f1);
    secp256k1_ellsq_fe_to_ge_var(&p2, &f2);
    secp256k1_gej_set_ge(&acc, &p1);
    secp256k1_gej_add_ge_var(&acc, &acc, &p2, NULL);
    if (!secp256k1_gej_is_infinity(&acc)) {
        secp256k1_ge_set_gej_var(&p1, &acc);
    } else {
        /* f(u1)+f(u2) is infinity. In that case the decoding is defined to be
         * equal to f(u1) instead. f(u1) is already stored in the p1 variable
         * at this point. */
    }
    secp256k1_pubkey_save(pubkey, &p1);
    return 1;
}

#endif
