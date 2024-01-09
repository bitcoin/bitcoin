/***********************************************************************
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_MODULE_ELLSWIFT_MAIN_H
#define SECP256K1_MODULE_ELLSWIFT_MAIN_H

#include "../../../include/secp256k1.h"
#include "../../../include/secp256k1_ellswift.h"
#include "../../eckey.h"
#include "../../hash.h"

/** c1 = (sqrt(-3)-1)/2 */
static const secp256k1_fe secp256k1_ellswift_c1 = SECP256K1_FE_CONST(0x851695d4, 0x9a83f8ef, 0x919bb861, 0x53cbcb16, 0x630fb68a, 0xed0a766a, 0x3ec693d6, 0x8e6afa40);
/** c2 = (-sqrt(-3)-1)/2 = -(c1+1) */
static const secp256k1_fe secp256k1_ellswift_c2 = SECP256K1_FE_CONST(0x7ae96a2b, 0x657c0710, 0x6e64479e, 0xac3434e9, 0x9cf04975, 0x12f58995, 0xc1396c28, 0x719501ee);
/** c3 = (-sqrt(-3)+1)/2 = -c1 = c2+1 */
static const secp256k1_fe secp256k1_ellswift_c3 = SECP256K1_FE_CONST(0x7ae96a2b, 0x657c0710, 0x6e64479e, 0xac3434e9, 0x9cf04975, 0x12f58995, 0xc1396c28, 0x719501ef);
/** c4 = (sqrt(-3)+1)/2 = -c2 = c1+1 */
static const secp256k1_fe secp256k1_ellswift_c4 = SECP256K1_FE_CONST(0x851695d4, 0x9a83f8ef, 0x919bb861, 0x53cbcb16, 0x630fb68a, 0xed0a766a, 0x3ec693d6, 0x8e6afa41);

/** Decode ElligatorSwift encoding (u, t) to a fraction xn/xd representing a curve X coordinate. */
static void secp256k1_ellswift_xswiftec_frac_var(secp256k1_fe *xn, secp256k1_fe *xd, const secp256k1_fe *u, const secp256k1_fe *t) {
    /* The implemented algorithm is the following (all operations in GF(p)):
     *
     * - Let c0 = sqrt(-3) = 0xa2d2ba93507f1df233770c2a797962cc61f6d15da14ecd47d8d27ae1cd5f852.
     * - If u = 0, set u = 1.
     * - If t = 0, set t = 1.
     * - If u^3+7+t^2 = 0, set t = 2*t.
     * - Let X = (u^3+7-t^2)/(2*t).
     * - Let Y = (X+t)/(c0*u).
     * - If x3 = u+4*Y^2 is a valid x coordinate, return it.
     * - If x2 = (-X/Y-u)/2 is a valid x coordinate, return it.
     * - Return x1 = (X/Y-u)/2 (which is now guaranteed to be a valid x coordinate).
     *
     * Introducing s=t^2, g=u^3+7, and simplifying x1=-(x2+u) we get:
     *
     * - Let c0 = ...
     * - If u = 0, set u = 1.
     * - If t = 0, set t = 1.
     * - Let s = t^2
     * - Let g = u^3+7
     * - If g+s = 0, set t = 2*t, s = 4*s
     * - Let X = (g-s)/(2*t).
     * - Let Y = (X+t)/(c0*u) = (g+s)/(2*c0*t*u).
     * - If x3 = u+4*Y^2 is a valid x coordinate, return it.
     * - If x2 = (-X/Y-u)/2 is a valid x coordinate, return it.
     * - Return x1 = -(x2+u).
     *
     * Now substitute Y^2 = -(g+s)^2/(12*s*u^2) and X/Y = c0*u*(g-s)/(g+s). This
     * means X and Y do not need to be evaluated explicitly anymore.
     *
     * - ...
     * - If g+s = 0, set s = 4*s.
     * - If x3 = u-(g+s)^2/(3*s*u^2) is a valid x coordinate, return it.
     * - If x2 = (-c0*u*(g-s)/(g+s)-u)/2 is a valid x coordinate, return it.
     * - Return x1 = -(x2+u).
     *
     * Simplifying x2 using 2 additional constants:
     *
     * - Let c1 = (c0-1)/2 = 0x851695d49a83f8ef919bb86153cbcb16630fb68aed0a766a3ec693d68e6afa40.
     * - Let c2 = (-c0-1)/2 = 0x7ae96a2b657c07106e64479eac3434e99cf0497512f58995c1396c28719501ee.
     * - ...
     * - If x2 = u*(c1*s+c2*g)/(g+s) is a valid x coordinate, return it.
     * - ...
     *
     * Writing x3 as a fraction:
     *
     * - ...
     * - If x3 = (3*s*u^3-(g+s)^2)/(3*s*u^2) ...
     * - ...

     * Overall, we get:
     *
     * - Let c1 = 0x851695d49a83f8ef919bb86153cbcb16630fb68aed0a766a3ec693d68e6afa40.
     * - Let c2 = 0x7ae96a2b657c07106e64479eac3434e99cf0497512f58995c1396c28719501ee.
     * - If u = 0, set u = 1.
     * - If t = 0, set s = 1, else set s = t^2.
     * - Let g = u^3+7.
     * - If g+s = 0, set s = 4*s.
     * - If x3 = (3*s*u^3-(g+s)^2)/(3*s*u^2) is a valid x coordinate, return it.
     * - If x2 = u*(c1*s+c2*g)/(g+s) is a valid x coordinate, return it.
     * - Return x1 = -(x2+u).
     */
    secp256k1_fe u1, s, g, p, d, n, l;
    u1 = *u;
    if (EXPECT(secp256k1_fe_normalizes_to_zero_var(&u1), 0)) u1 = secp256k1_fe_one;
    secp256k1_fe_sqr(&s, t);
    if (EXPECT(secp256k1_fe_normalizes_to_zero_var(t), 0)) s = secp256k1_fe_one;
    secp256k1_fe_sqr(&l, &u1);                                   /* l = u^2 */
    secp256k1_fe_mul(&g, &l, &u1);                               /* g = u^3 */
    secp256k1_fe_add_int(&g, SECP256K1_B);                       /* g = u^3 + 7 */
    p = g;                                                       /* p = g */
    secp256k1_fe_add(&p, &s);                                    /* p = g+s */
    if (EXPECT(secp256k1_fe_normalizes_to_zero_var(&p), 0)) {
        secp256k1_fe_mul_int(&s, 4);
        /* Recompute p = g+s */
        p = g;                                                   /* p = g */
        secp256k1_fe_add(&p, &s);                                /* p = g+s */
    }
    secp256k1_fe_mul(&d, &s, &l);                                /* d = s*u^2 */
    secp256k1_fe_mul_int(&d, 3);                                 /* d = 3*s*u^2 */
    secp256k1_fe_sqr(&l, &p);                                    /* l = (g+s)^2 */
    secp256k1_fe_negate(&l, &l, 1);                              /* l = -(g+s)^2 */
    secp256k1_fe_mul(&n, &d, &u1);                               /* n = 3*s*u^3 */
    secp256k1_fe_add(&n, &l);                                    /* n = 3*s*u^3-(g+s)^2 */
    if (secp256k1_ge_x_frac_on_curve_var(&n, &d)) {
        /* Return x3 = n/d = (3*s*u^3-(g+s)^2)/(3*s*u^2) */
        *xn = n;
        *xd = d;
        return;
    }
    *xd = p;
    secp256k1_fe_mul(&l, &secp256k1_ellswift_c1, &s);            /* l = c1*s */
    secp256k1_fe_mul(&n, &secp256k1_ellswift_c2, &g);            /* n = c2*g */
    secp256k1_fe_add(&n, &l);                                    /* n = c1*s+c2*g */
    secp256k1_fe_mul(&n, &n, &u1);                               /* n = u*(c1*s+c2*g) */
    /* Possible optimization: in the invocation below, p^2 = (g+s)^2 is computed,
     * which we already have computed above. This could be deduplicated. */
    if (secp256k1_ge_x_frac_on_curve_var(&n, &p)) {
        /* Return x2 = n/p = u*(c1*s+c2*g)/(g+s) */
        *xn = n;
        return;
    }
    secp256k1_fe_mul(&l, &p, &u1);                               /* l = u*(g+s) */
    secp256k1_fe_add(&n, &l);                                    /* n = u*(c1*s+c2*g)+u*(g+s) */
    secp256k1_fe_negate(xn, &n, 2);                              /* n = -u*(c1*s+c2*g)-u*(g+s) */

    VERIFY_CHECK(secp256k1_ge_x_frac_on_curve_var(xn, &p));
    /* Return x3 = n/p = -(u*(c1*s+c2*g)/(g+s)+u) */
}

/** Decode ElligatorSwift encoding (u, t) to X coordinate. */
static void secp256k1_ellswift_xswiftec_var(secp256k1_fe *x, const secp256k1_fe *u, const secp256k1_fe *t) {
    secp256k1_fe xn, xd;
    secp256k1_ellswift_xswiftec_frac_var(&xn, &xd, u, t);
    secp256k1_fe_inv_var(&xd, &xd);
    secp256k1_fe_mul(x, &xn, &xd);
}

/** Decode ElligatorSwift encoding (u, t) to point P. */
static void secp256k1_ellswift_swiftec_var(secp256k1_ge *p, const secp256k1_fe *u, const secp256k1_fe *t) {
    secp256k1_fe x;
    secp256k1_ellswift_xswiftec_var(&x, u, t);
    secp256k1_ge_set_xo_var(p, &x, secp256k1_fe_is_odd(t));
}

/* Try to complete an ElligatorSwift encoding (u, t) for X coordinate x, given u and x.
 *
 * There may be up to 8 distinct t values such that (u, t) decodes back to x, but also
 * fewer, or none at all. Each such partial inverse can be accessed individually using a
 * distinct input argument c (in range 0-7), and some or all of these may return failure.
 * The following guarantees exist:
 * - Given (x, u), no two distinct c values give the same successful result t.
 * - Every successful result maps back to x through secp256k1_ellswift_xswiftec_var.
 * - Given (x, u), all t values that map back to x can be reached by combining the
 *   successful results from this function over all c values, with the exception of:
 *   - this function cannot be called with u=0
 *   - no result with t=0 will be returned
 *   - no result for which u^3 + t^2 + 7 = 0 will be returned.
 *
 * The rather unusual encoding of bits in c (a large "if" based on the middle bit, and then
 * using the low and high bits to pick signs of square roots) is to match the paper's
 * encoding more closely: c=0 through c=3 match branches 1..4 in the paper, while c=4 through
 * c=7 are copies of those with an additional negation of sqrt(w).
 */
static int secp256k1_ellswift_xswiftec_inv_var(secp256k1_fe *t, const secp256k1_fe *x_in, const secp256k1_fe *u_in, int c) {
    /* The implemented algorithm is this (all arithmetic, except involving c, is mod p):
     *
     * - If (c & 2) = 0:
     *   - If (-x-u) is a valid X coordinate, fail.
     *   - Let s=-(u^3+7)/(u^2+u*x+x^2).
     *   - If s is not square, fail.
     *   - Let v=x.
     * - If (c & 2) = 2:
     *   - Let s=x-u.
     *   - If s is not square, fail.
     *   - Let r=sqrt(-s*(4*(u^3+7)+3*u^2*s)); fail if it doesn't exist.
     *   - If (c & 1) = 1 and r = 0, fail.
     *   - If s=0, fail.
     *   - Let v=(r/s-u)/2.
     * - Let w=sqrt(s).
     * - If (c & 5) = 0: return -w*(c3*u + v).
     * - If (c & 5) = 1: return  w*(c4*u + v).
     * - If (c & 5) = 4: return  w*(c3*u + v).
     * - If (c & 5) = 5: return -w*(c4*u + v).
     */
    secp256k1_fe x = *x_in, u = *u_in, g, v, s, m, r, q;
    int ret;

    secp256k1_fe_normalize_weak(&x);
    secp256k1_fe_normalize_weak(&u);

    VERIFY_CHECK(c >= 0 && c < 8);
    VERIFY_CHECK(secp256k1_ge_x_on_curve_var(&x));

    if (!(c & 2)) {
        /* c is in {0, 1, 4, 5}. In this case we look for an inverse under the x1 (if c=0 or
         * c=4) formula, or x2 (if c=1 or c=5) formula. */

        /* If -u-x is a valid X coordinate, fail. This would yield an encoding that roundtrips
         * back under the x3 formula instead (which has priority over x1 and x2, so the decoding
         * would not match x). */
        m = x;                                          /* m = x */
        secp256k1_fe_add(&m, &u);                       /* m = u+x */
        secp256k1_fe_negate(&m, &m, 2);                 /* m = -u-x */
        /* Test if (-u-x) is a valid X coordinate. If so, fail. */
        if (secp256k1_ge_x_on_curve_var(&m)) return 0;

        /* Let s = -(u^3 + 7)/(u^2 + u*x + x^2) [first part] */
        secp256k1_fe_sqr(&s, &m);                       /* s = (u+x)^2 */
        secp256k1_fe_negate(&s, &s, 1);                 /* s = -(u+x)^2 */
        secp256k1_fe_mul(&m, &u, &x);                   /* m = u*x */
        secp256k1_fe_add(&s, &m);                       /* s = -(u^2 + u*x + x^2) */

        /* Note that at this point, s = 0 is impossible. If it were the case:
         *             s = -(u^2 + u*x + x^2) = 0
         * =>                 u^2 + u*x + x^2 = 0
         * =>   (u + 2*x) * (u^2 + u*x + x^2) = 0
         * => 2*x^3 + 3*x^2*u + 3*x*u^2 + u^3 = 0
         * =>                 (x + u)^3 + x^3 = 0
         * =>                             x^3 = -(x + u)^3
         * =>                         x^3 + B = (-u - x)^3 + B
         *
         * However, we know x^3 + B is square (because x is on the curve) and
         * that (-u-x)^3 + B is not square (the secp256k1_ge_x_on_curve_var(&m)
         * test above would have failed). This is a contradiction, and thus the
         * assumption s=0 is false. */
        VERIFY_CHECK(!secp256k1_fe_normalizes_to_zero_var(&s));

        /* If s is not square, fail. We have not fully computed s yet, but s is square iff
         * -(u^3+7)*(u^2+u*x+x^2) is square (because a/b is square iff a*b is square and b is
         * nonzero). */
        secp256k1_fe_sqr(&g, &u);                       /* g = u^2 */
        secp256k1_fe_mul(&g, &g, &u);                   /* g = u^3 */
        secp256k1_fe_add_int(&g, SECP256K1_B);          /* g = u^3+7 */
        secp256k1_fe_mul(&m, &s, &g);                   /* m = -(u^3 + 7)*(u^2 + u*x + x^2) */
        if (!secp256k1_fe_is_square_var(&m)) return 0;

        /* Let s = -(u^3 + 7)/(u^2 + u*x + x^2) [second part] */
        secp256k1_fe_inv_var(&s, &s);                   /* s = -1/(u^2 + u*x + x^2) [no div by 0] */
        secp256k1_fe_mul(&s, &s, &g);                   /* s = -(u^3 + 7)/(u^2 + u*x + x^2) */

        /* Let v = x. */
        v = x;
    } else {
        /* c is in {2, 3, 6, 7}. In this case we look for an inverse under the x3 formula. */

        /* Let s = x-u. */
        secp256k1_fe_negate(&m, &u, 1);                 /* m = -u */
        s = m;                                          /* s = -u */
        secp256k1_fe_add(&s, &x);                       /* s = x-u */

        /* If s is not square, fail. */
        if (!secp256k1_fe_is_square_var(&s)) return 0;

        /* Let r = sqrt(-s*(4*(u^3+7)+3*u^2*s)); fail if it doesn't exist. */
        secp256k1_fe_sqr(&g, &u);                       /* g = u^2 */
        secp256k1_fe_mul(&q, &s, &g);                   /* q = s*u^2 */
        secp256k1_fe_mul_int(&q, 3);                    /* q = 3*s*u^2 */
        secp256k1_fe_mul(&g, &g, &u);                   /* g = u^3 */
        secp256k1_fe_mul_int(&g, 4);                    /* g = 4*u^3 */
        secp256k1_fe_add_int(&g, 4 * SECP256K1_B);      /* g = 4*(u^3+7) */
        secp256k1_fe_add(&q, &g);                       /* q = 4*(u^3+7)+3*s*u^2 */
        secp256k1_fe_mul(&q, &q, &s);                   /* q = s*(4*(u^3+7)+3*u^2*s) */
        secp256k1_fe_negate(&q, &q, 1);                 /* q = -s*(4*(u^3+7)+3*u^2*s) */
        if (!secp256k1_fe_is_square_var(&q)) return 0;
        ret = secp256k1_fe_sqrt(&r, &q);                /* r = sqrt(-s*(4*(u^3+7)+3*u^2*s)) */
#ifdef VERIFY
        VERIFY_CHECK(ret);
#else
        (void)ret;
#endif

        /* If (c & 1) = 1 and r = 0, fail. */
        if (EXPECT((c & 1) && secp256k1_fe_normalizes_to_zero_var(&r), 0)) return 0;

        /* If s = 0, fail. */
        if (EXPECT(secp256k1_fe_normalizes_to_zero_var(&s), 0)) return 0;

        /* Let v = (r/s-u)/2. */
        secp256k1_fe_inv_var(&v, &s);                   /* v = 1/s [no div by 0] */
        secp256k1_fe_mul(&v, &v, &r);                   /* v = r/s */
        secp256k1_fe_add(&v, &m);                       /* v = r/s-u */
        secp256k1_fe_half(&v);                          /* v = (r/s-u)/2 */
    }

    /* Let w = sqrt(s). */
    ret = secp256k1_fe_sqrt(&m, &s);                    /* m = sqrt(s) = w */
    VERIFY_CHECK(ret);

    /* Return logic. */
    if ((c & 5) == 0 || (c & 5) == 5) {
        secp256k1_fe_negate(&m, &m, 1);                 /* m = -w */
    }
    /* Now m = {-w if c&5=0 or c&5=5; w otherwise}. */
    secp256k1_fe_mul(&u, &u, c&1 ? &secp256k1_ellswift_c4 : &secp256k1_ellswift_c3);
    /* u = {c4 if c&1=1; c3 otherwise}*u */
    secp256k1_fe_add(&u, &v);                           /* u = {c4 if c&1=1; c3 otherwise}*u + v */
    secp256k1_fe_mul(t, &m, &u);
    return 1;
}

/** Use SHA256 as a PRNG, returning SHA256(hasher || cnt).
 *
 * hasher is a SHA256 object to which an incrementing 4-byte counter is written to generate randomness.
 * Writing 13 bytes (4 bytes for counter, plus 9 bytes for the SHA256 padding) cannot cross a
 * 64-byte block size boundary (to make sure it only triggers a single SHA256 compression). */
static void secp256k1_ellswift_prng(unsigned char* out32, const secp256k1_sha256 *hasher, uint32_t cnt) {
    secp256k1_sha256 hash = *hasher;
    unsigned char buf4[4];
#ifdef VERIFY
    size_t blocks = hash.bytes >> 6;
#endif
    buf4[0] = cnt;
    buf4[1] = cnt >> 8;
    buf4[2] = cnt >> 16;
    buf4[3] = cnt >> 24;
    secp256k1_sha256_write(&hash, buf4, 4);
    secp256k1_sha256_finalize(&hash, out32);

    /* Writing and finalizing together should trigger exactly one SHA256 compression. */
    VERIFY_CHECK(((hash.bytes) >> 6) == (blocks + 1));
}

/** Find an ElligatorSwift encoding (u, t) for X coordinate x, and random Y coordinate.
 *
 * u32 is the 32-byte big endian encoding of u; t is the output field element t that still
 * needs encoding.
 *
 * hasher is a hasher in the secp256k1_ellswift_prng sense, with the same restrictions. */
static void secp256k1_ellswift_xelligatorswift_var(unsigned char *u32, secp256k1_fe *t, const secp256k1_fe *x, const secp256k1_sha256 *hasher) {
    /* Pool of 3-bit branch values. */
    unsigned char branch_hash[32];
    /* Number of 3-bit values in branch_hash left. */
    int branches_left = 0;
    /* Field elements u and branch values are extracted from RNG based on hasher for consecutive
     * values of cnt. cnt==0 is first used to populate a pool of 64 4-bit branch values. The 64
     * cnt values that follow are used to generate field elements u. cnt==65 (and multiples
     * thereof) are used to repopulate the pool and start over, if that were ever necessary.
     * On average, 4 iterations are needed. */
    uint32_t cnt = 0;
    while (1) {
        int branch;
        secp256k1_fe u;
        /* If the pool of branch values is empty, populate it. */
        if (branches_left == 0) {
            secp256k1_ellswift_prng(branch_hash, hasher, cnt++);
            branches_left = 64;
        }
        /* Take a 3-bit branch value from the branch pool (top bit is discarded). */
        --branches_left;
        branch = (branch_hash[branches_left >> 1] >> ((branches_left & 1) << 2)) & 7;
        /* Compute a new u value by hashing. */
        secp256k1_ellswift_prng(u32, hasher, cnt++);
        /* overflow is not a problem (we prefer uniform u32 over uniform u). */
        secp256k1_fe_set_b32_mod(&u, u32);
        /* Since u is the output of a hash, it should practically never be 0. We could apply the
         * u=0 to u=1 correction here too to deal with that case still, but it's such a low
         * probability event that we do not bother. */
        VERIFY_CHECK(!secp256k1_fe_normalizes_to_zero_var(&u));

        /* Find a remainder t, and return it if found. */
        if (EXPECT(secp256k1_ellswift_xswiftec_inv_var(t, x, &u, branch), 0)) break;
    }
}

/** Find an ElligatorSwift encoding (u, t) for point P.
 *
 * This is similar secp256k1_ellswift_xelligatorswift_var, except it takes a full group element p
 * as input, and returns an encoding that matches the provided Y coordinate rather than a random
 * one.
 */
static void secp256k1_ellswift_elligatorswift_var(unsigned char *u32, secp256k1_fe *t, const secp256k1_ge *p, const secp256k1_sha256 *hasher) {
    secp256k1_ellswift_xelligatorswift_var(u32, t, &p->x, hasher);
    secp256k1_fe_normalize_var(t);
    if (secp256k1_fe_is_odd(t) != secp256k1_fe_is_odd(&p->y)) {
        secp256k1_fe_negate(t, t, 1);
        secp256k1_fe_normalize_var(t);
    }
}

/** Set hash state to the BIP340 tagged hash midstate for "secp256k1_ellswift_encode". */
static void secp256k1_ellswift_sha256_init_encode(secp256k1_sha256* hash) {
    secp256k1_sha256_initialize(hash);
    hash->s[0] = 0xd1a6524bul;
    hash->s[1] = 0x028594b3ul;
    hash->s[2] = 0x96e42f4eul;
    hash->s[3] = 0x1037a177ul;
    hash->s[4] = 0x1b8fcb8bul;
    hash->s[5] = 0x56023885ul;
    hash->s[6] = 0x2560ede1ul;
    hash->s[7] = 0xd626b715ul;

    hash->bytes = 64;
}

int secp256k1_ellswift_encode(const secp256k1_context *ctx, unsigned char *ell64, const secp256k1_pubkey *pubkey, const unsigned char *rnd32) {
    secp256k1_ge p;
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(ell64 != NULL);
    ARG_CHECK(pubkey != NULL);
    ARG_CHECK(rnd32 != NULL);

    if (secp256k1_pubkey_load(ctx, &p, pubkey)) {
        secp256k1_fe t;
        unsigned char p64[64] = {0};
        size_t ser_size;
        int ser_ret;
        secp256k1_sha256 hash;

        /* Set up hasher state; the used RNG is H(pubkey || "\x00"*31 || rnd32 || cnt++), using
         * BIP340 tagged hash with tag "secp256k1_ellswift_encode". */
        secp256k1_ellswift_sha256_init_encode(&hash);
        ser_ret = secp256k1_eckey_pubkey_serialize(&p, p64, &ser_size, 1);
#ifdef VERIFY
        VERIFY_CHECK(ser_ret && ser_size == 33);
#else
        (void)ser_ret;
#endif
        secp256k1_sha256_write(&hash, p64, sizeof(p64));
        secp256k1_sha256_write(&hash, rnd32, 32);

        /* Compute ElligatorSwift encoding and construct output. */
        secp256k1_ellswift_elligatorswift_var(ell64, &t, &p, &hash); /* puts u in ell64[0..32] */
        secp256k1_fe_get_b32(ell64 + 32, &t); /* puts t in ell64[32..64] */
        return 1;
    }
    /* Only reached in case the provided pubkey is invalid. */
    memset(ell64, 0, 64);
    return 0;
}

/** Set hash state to the BIP340 tagged hash midstate for "secp256k1_ellswift_create". */
static void secp256k1_ellswift_sha256_init_create(secp256k1_sha256* hash) {
    secp256k1_sha256_initialize(hash);
    hash->s[0] = 0xd29e1bf5ul;
    hash->s[1] = 0xf7025f42ul;
    hash->s[2] = 0x9b024773ul;
    hash->s[3] = 0x094cb7d5ul;
    hash->s[4] = 0xe59ed789ul;
    hash->s[5] = 0x03bc9786ul;
    hash->s[6] = 0x68335b35ul;
    hash->s[7] = 0x4e363b53ul;

    hash->bytes = 64;
}

int secp256k1_ellswift_create(const secp256k1_context *ctx, unsigned char *ell64, const unsigned char *seckey32, const unsigned char *auxrnd32) {
    secp256k1_ge p;
    secp256k1_fe t;
    secp256k1_sha256 hash;
    secp256k1_scalar seckey_scalar;
    int ret;
    static const unsigned char zero32[32] = {0};

    /* Sanity check inputs. */
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(ell64 != NULL);
    memset(ell64, 0, 64);
    ARG_CHECK(secp256k1_ecmult_gen_context_is_built(&ctx->ecmult_gen_ctx));
    ARG_CHECK(seckey32 != NULL);

    /* Compute (affine) public key */
    ret = secp256k1_ec_pubkey_create_helper(&ctx->ecmult_gen_ctx, &seckey_scalar, &p, seckey32);
    secp256k1_declassify(ctx, &p, sizeof(p)); /* not constant time in produced pubkey */
    secp256k1_fe_normalize_var(&p.x);
    secp256k1_fe_normalize_var(&p.y);

    /* Set up hasher state. The used RNG is H(privkey || "\x00"*32 [|| auxrnd32] || cnt++),
     * using BIP340 tagged hash with tag "secp256k1_ellswift_create". */
    secp256k1_ellswift_sha256_init_create(&hash);
    secp256k1_sha256_write(&hash, seckey32, 32);
    secp256k1_sha256_write(&hash, zero32, sizeof(zero32));
    secp256k1_declassify(ctx, &hash, sizeof(hash)); /* private key is hashed now */
    if (auxrnd32) secp256k1_sha256_write(&hash, auxrnd32, 32);

    /* Compute ElligatorSwift encoding and construct output. */
    secp256k1_ellswift_elligatorswift_var(ell64, &t, &p, &hash); /* puts u in ell64[0..32] */
    secp256k1_fe_get_b32(ell64 + 32, &t); /* puts t in ell64[32..64] */

    secp256k1_memczero(ell64, 64, !ret);
    secp256k1_scalar_clear(&seckey_scalar);

    return ret;
}

int secp256k1_ellswift_decode(const secp256k1_context *ctx, secp256k1_pubkey *pubkey, const unsigned char *ell64) {
    secp256k1_fe u, t;
    secp256k1_ge p;
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(pubkey != NULL);
    ARG_CHECK(ell64 != NULL);

    secp256k1_fe_set_b32_mod(&u, ell64);
    secp256k1_fe_set_b32_mod(&t, ell64 + 32);
    secp256k1_fe_normalize_var(&t);
    secp256k1_ellswift_swiftec_var(&p, &u, &t);
    secp256k1_pubkey_save(pubkey, &p);
    return 1;
}

static int ellswift_xdh_hash_function_prefix(unsigned char *output, const unsigned char *x32, const unsigned char *ell_a64, const unsigned char *ell_b64, void *data) {
    secp256k1_sha256 sha;

    secp256k1_sha256_initialize(&sha);
    secp256k1_sha256_write(&sha, data, 64);
    secp256k1_sha256_write(&sha, ell_a64, 64);
    secp256k1_sha256_write(&sha, ell_b64, 64);
    secp256k1_sha256_write(&sha, x32, 32);
    secp256k1_sha256_finalize(&sha, output);

    return 1;
}

/** Set hash state to the BIP340 tagged hash midstate for "bip324_ellswift_xonly_ecdh". */
static void secp256k1_ellswift_sha256_init_bip324(secp256k1_sha256* hash) {
    secp256k1_sha256_initialize(hash);
    hash->s[0] = 0x8c12d730ul;
    hash->s[1] = 0x827bd392ul;
    hash->s[2] = 0x9e4fb2eeul;
    hash->s[3] = 0x207b373eul;
    hash->s[4] = 0x2292bd7aul;
    hash->s[5] = 0xaa5441bcul;
    hash->s[6] = 0x15c3779ful;
    hash->s[7] = 0xcfb52549ul;

    hash->bytes = 64;
}

static int ellswift_xdh_hash_function_bip324(unsigned char* output, const unsigned char *x32, const unsigned char *ell_a64, const unsigned char *ell_b64, void *data) {
    secp256k1_sha256 sha;

    (void)data;

    secp256k1_ellswift_sha256_init_bip324(&sha);
    secp256k1_sha256_write(&sha, ell_a64, 64);
    secp256k1_sha256_write(&sha, ell_b64, 64);
    secp256k1_sha256_write(&sha, x32, 32);
    secp256k1_sha256_finalize(&sha, output);

    return 1;
}

const secp256k1_ellswift_xdh_hash_function secp256k1_ellswift_xdh_hash_function_prefix = ellswift_xdh_hash_function_prefix;
const secp256k1_ellswift_xdh_hash_function secp256k1_ellswift_xdh_hash_function_bip324 = ellswift_xdh_hash_function_bip324;

int secp256k1_ellswift_xdh(const secp256k1_context *ctx, unsigned char *output, const unsigned char *ell_a64, const unsigned char *ell_b64, const unsigned char *seckey32, int party, secp256k1_ellswift_xdh_hash_function hashfp, void *data) {
    int ret = 0;
    int overflow;
    secp256k1_scalar s;
    secp256k1_fe xn, xd, px, u, t;
    unsigned char sx[32];
    const unsigned char* theirs64;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(output != NULL);
    ARG_CHECK(ell_a64 != NULL);
    ARG_CHECK(ell_b64 != NULL);
    ARG_CHECK(seckey32 != NULL);
    ARG_CHECK(hashfp != NULL);

    /* Load remote public key (as fraction). */
    theirs64 = party ? ell_a64 : ell_b64;
    secp256k1_fe_set_b32_mod(&u, theirs64);
    secp256k1_fe_set_b32_mod(&t, theirs64 + 32);
    secp256k1_ellswift_xswiftec_frac_var(&xn, &xd, &u, &t);

    /* Load private key (using one if invalid). */
    secp256k1_scalar_set_b32(&s, seckey32, &overflow);
    overflow = secp256k1_scalar_is_zero(&s);
    secp256k1_scalar_cmov(&s, &secp256k1_scalar_one, overflow);

    /* Compute shared X coordinate. */
    secp256k1_ecmult_const_xonly(&px, &xn, &xd, &s, 1);
    secp256k1_fe_normalize(&px);
    secp256k1_fe_get_b32(sx, &px);

    /* Invoke hasher */
    ret = hashfp(output, sx, ell_a64, ell_b64, data);

    memset(sx, 0, 32);
    secp256k1_fe_clear(&px);
    secp256k1_scalar_clear(&s);

    return !!ret & !overflow;
}

#endif
