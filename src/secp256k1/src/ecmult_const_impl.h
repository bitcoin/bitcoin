/***********************************************************************
 * Copyright (c) 2015, 2022 Pieter Wuille, Andrew Poelstra             *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_ECMULT_CONST_IMPL_H
#define SECP256K1_ECMULT_CONST_IMPL_H

#include "scalar.h"
#include "group.h"
#include "ecmult_const.h"
#include "ecmult_impl.h"

#if defined(EXHAUSTIVE_TEST_ORDER)
/* We need 2^ECMULT_CONST_GROUP_SIZE - 1 to be less than EXHAUSTIVE_TEST_ORDER, because
 * the tables cannot have infinities in them (this breaks the effective-affine technique's
 * z-ratio tracking) */
#  if EXHAUSTIVE_TEST_ORDER == 199
#    define ECMULT_CONST_GROUP_SIZE 4
#  elif EXHAUSTIVE_TEST_ORDER == 13
#    define ECMULT_CONST_GROUP_SIZE 3
#  elif EXHAUSTIVE_TEST_ORDER == 7
#    define ECMULT_CONST_GROUP_SIZE 2
#  else
#    error "Unknown EXHAUSTIVE_TEST_ORDER"
#  endif
#else
/* Group size 4 or 5 appears optimal. */
#  define ECMULT_CONST_GROUP_SIZE 5
#endif

#define ECMULT_CONST_TABLE_SIZE (1L << (ECMULT_CONST_GROUP_SIZE - 1))
#define ECMULT_CONST_GROUPS ((129 + ECMULT_CONST_GROUP_SIZE - 1) / ECMULT_CONST_GROUP_SIZE)
#define ECMULT_CONST_BITS (ECMULT_CONST_GROUPS * ECMULT_CONST_GROUP_SIZE)

/** Fill a table 'pre' with precomputed odd multiples of a.
 *
 *  The resulting point set is brought to a single constant Z denominator, stores the X and Y
 *  coordinates as ge points in pre, and stores the global Z in globalz.
 *
 *  'pre' must be an array of size ECMULT_CONST_TABLE_SIZE.
 */
static void secp256k1_ecmult_const_odd_multiples_table_globalz(secp256k1_ge *pre, secp256k1_fe *globalz, const secp256k1_gej *a) {
    secp256k1_fe zr[ECMULT_CONST_TABLE_SIZE];

    secp256k1_ecmult_odd_multiples_table(ECMULT_CONST_TABLE_SIZE, pre, zr, globalz, a);
    secp256k1_ge_table_set_globalz(ECMULT_CONST_TABLE_SIZE, pre, zr);
}

/* Given a table 'pre' with odd multiples of a point, put in r the signed-bit multiplication of n with that point.
 *
 * For example, if ECMULT_CONST_GROUP_SIZE is 4, then pre is expected to contain 8 entries:
 * [1*P, 3*P, 5*P, 7*P, 9*P, 11*P, 13*P, 15*P]. n is then expected to be a 4-bit integer (range 0-15), and its
 * bits are interpreted as signs of powers of two to look up.
 *
 * For example, if n=4, which is 0100 in binary, which is interpreted as [- + - -], so the looked up value is
 * [ -(2^3) + (2^2) - (2^1) - (2^0) ]*P = -7*P. Every valid n translates to an odd number in range [-15,15],
 * which means we just need to look up one of the precomputed values, and optionally negate it.
 */
#define ECMULT_CONST_TABLE_GET_GE(r,pre,n) do { \
    unsigned int m = 0; \
    /* If the top bit of n is 0, we want the negation. */ \
    volatile unsigned int negative = ((n) >> (ECMULT_CONST_GROUP_SIZE - 1)) ^ 1; \
    /* Let n[i] be the i-th bit of n, then the index is
     *     sum(cnot(n[i]) * 2^i, i=0..l-2)
     * where cnot(b) = b if n[l-1] = 1 and 1 - b otherwise.
     * For example, if n = 4, in binary 0100, the index is 3, in binary 011.
     *
     * Proof:
     *     Let
     *         x = sum((2*n[i] - 1)*2^i, i=0..l-1)
     *           = 2*sum(n[i] * 2^i, i=0..l-1) - 2^l + 1
     *     be the value represented by n.
     *     The index is (x - 1)/2 if x > 0 and -(x + 1)/2 otherwise.
     *     Case x > 0:
     *         n[l-1] = 1
     *         index = sum(n[i] * 2^i, i=0..l-1) - 2^(l-1)
     *               = sum(n[i] * 2^i, i=0..l-2)
     *     Case x <= 0:
     *         n[l-1] = 0
     *          index = -(2*sum(n[i] * 2^i, i=0..l-1) - 2^l + 2)/2
     *                = 2^(l-1) - 1 - sum(n[i] * 2^i, i=0..l-1)
     *                = sum((1 - n[i]) * 2^i, i=0..l-2)
     */ \
    unsigned int index = ((unsigned int)(-negative) ^ n) & ((1U << (ECMULT_CONST_GROUP_SIZE - 1)) - 1U); \
    secp256k1_fe neg_y; \
    VERIFY_CHECK((n) < (1U << ECMULT_CONST_GROUP_SIZE)); \
    VERIFY_CHECK(index < (1U << (ECMULT_CONST_GROUP_SIZE - 1))); \
    /* Unconditionally set r->x = (pre)[m].x. r->y = (pre)[m].y. because it's either the correct one
     * or will get replaced in the later iterations, this is needed to make sure `r` is initialized. */ \
    (r)->x = (pre)[m].x; \
    (r)->y = (pre)[m].y; \
    for (m = 1; m < ECMULT_CONST_TABLE_SIZE; m++) { \
        /* This loop is used to avoid secret data in array indices. See
         * the comment in ecmult_gen_impl.h for rationale. */ \
        secp256k1_fe_cmov(&(r)->x, &(pre)[m].x, m == index); \
        secp256k1_fe_cmov(&(r)->y, &(pre)[m].y, m == index); \
    } \
    (r)->infinity = 0; \
    secp256k1_fe_negate(&neg_y, &(r)->y, 1); \
    secp256k1_fe_cmov(&(r)->y, &neg_y, negative); \
} while(0)

/* For K as defined in the comment of secp256k1_ecmult_const, we have several precomputed
 * formulas/constants.
 * - in exhaustive test mode, we give an explicit expression to compute it at compile time: */
#ifdef EXHAUSTIVE_TEST_ORDER
static const secp256k1_scalar secp256k1_ecmult_const_K = ((SECP256K1_SCALAR_CONST(0, 0, 0, (1U << (ECMULT_CONST_BITS - 128)) - 2U, 0, 0, 0, 0) + EXHAUSTIVE_TEST_ORDER - 1U) * (1U + EXHAUSTIVE_TEST_LAMBDA)) % EXHAUSTIVE_TEST_ORDER;
/* - for the real secp256k1 group we have constants for various ECMULT_CONST_BITS values. */
#elif ECMULT_CONST_BITS == 129
/* For GROUP_SIZE = 1,3. */
static const secp256k1_scalar secp256k1_ecmult_const_K = SECP256K1_SCALAR_CONST(0xac9c52b3ul, 0x3fa3cf1ful, 0x5ad9e3fdul, 0x77ed9ba4ul, 0xa880b9fcul, 0x8ec739c2ul, 0xe0cfc810ul, 0xb51283ceul);
#elif ECMULT_CONST_BITS == 130
/* For GROUP_SIZE = 2,5. */
static const secp256k1_scalar secp256k1_ecmult_const_K = SECP256K1_SCALAR_CONST(0xa4e88a7dul, 0xcb13034eul, 0xc2bdd6bful, 0x7c118d6bul, 0x589ae848ul, 0x26ba29e4ul, 0xb5c2c1dcul, 0xde9798d9ul);
#elif ECMULT_CONST_BITS == 132
/* For GROUP_SIZE = 4,6 */
static const secp256k1_scalar secp256k1_ecmult_const_K = SECP256K1_SCALAR_CONST(0x76b1d93dul, 0x0fae3c6bul, 0x3215874bul, 0x94e93813ul, 0x7937fe0dul, 0xb66bcaaful, 0xb3749ca5ul, 0xd7b6171bul);
#else
#  error "Unknown ECMULT_CONST_BITS"
#endif

static void secp256k1_ecmult_const(secp256k1_gej *r, const secp256k1_ge *a, const secp256k1_scalar *q) {
    /* The approach below combines the signed-digit logic from Mike Hamburg's
     * "Fast and compact elliptic-curve cryptography" (https://eprint.iacr.org/2012/309)
     * Section 3.3, with the GLV endomorphism.
     *
     * The idea there is to interpret the bits of a scalar as signs (1 = +, 0 = -), and compute a
     * point multiplication in that fashion. Let v be an n-bit non-negative integer (0 <= v < 2^n),
     * and v[i] its i'th bit (so v = sum(v[i] * 2^i, i=0..n-1)). Then define:
     *
     *   C_l(v, A) = sum((2*v[i] - 1) * 2^i*A, i=0..l-1)
     *
     * Then it holds that C_l(v, A) = sum((2*v[i] - 1) * 2^i*A, i=0..l-1)
     *                              = (2*sum(v[i] * 2^i, i=0..l-1) + 1 - 2^l) * A
     *                              = (2*v + 1 - 2^l) * A
     *
     * Thus, one can compute q*A as C_256((q + 2^256 - 1) / 2, A). This is the basis for the
     * paper's signed-digit multi-comb algorithm for multiplication using a precomputed table.
     *
     * It is appealing to try to combine this with the GLV optimization: the idea that a scalar
     * s can be written as s1 + lambda*s2, where lambda is a curve-specific constant such that
     * lambda*A is easy to compute, and where s1 and s2 are small. In particular we have the
     * secp256k1_scalar_split_lambda function which performs such a split with the resulting s1
     * and s2 in range (-2^128, 2^128) mod n. This does work, but is uninteresting:
     *
     *   To compute q*A:
     *   - Let s1, s2 = split_lambda(q)
     *   - Let R1 = C_256((s1 + 2^256 - 1) / 2, A)
     *   - Let R2 = C_256((s2 + 2^256 - 1) / 2, lambda*A)
     *   - Return R1 + R2
     *
     * The issue is that while s1 and s2 are small-range numbers, (s1 + 2^256 - 1) / 2 (mod n)
     * and (s2 + 2^256 - 1) / 2 (mod n) are not, undoing the benefit of the splitting.
     *
     * To make it work, we want to modify the input scalar q first, before splitting, and then only
     * add a 2^128 offset of the split results (so that they end up in the single 129-bit range
     * [0,2^129]). A slightly smaller offset would work due to the bounds on the split, but we pick
     * 2^128 for simplicity. Let s be the scalar fed to split_lambda, and f(q) the function to
     * compute it from q:
     *
     *   To compute q*A:
     *   - Compute s = f(q)
     *   - Let s1, s2 = split_lambda(s)
     *   - Let v1 = s1 + 2^128 (mod n)
     *   - Let v2 = s2 + 2^128 (mod n)
     *   - Let R1 = C_l(v1, A)
     *   - Let R2 = C_l(v2, lambda*A)
     *   - Return R1 + R2
     *
     * l will thus need to be at least 129, but we may overshoot by a few bits (see
     * further), so keep it as a variable.
     *
     * To solve for s, we reason:
     *     q*A  = R1 + R2
     * <=> q*A  = C_l(s1 + 2^128, A) + C_l(s2 + 2^128, lambda*A)
     * <=> q*A  = (2*(s1 + 2^128) + 1 - 2^l) * A + (2*(s2 + 2^128) + 1 - 2^l) * lambda*A
     * <=> q*A  = (2*(s1 + s2*lambda) + (2^129 + 1 - 2^l) * (1 + lambda)) * A
     * <=> q    = 2*(s1 + s2*lambda) + (2^129 + 1 - 2^l) * (1 + lambda) (mod n)
     * <=> q    = 2*s + (2^129 + 1 - 2^l) * (1 + lambda) (mod n)
     * <=> s    = (q + (2^l - 2^129 - 1) * (1 + lambda)) / 2 (mod n)
     * <=> f(q) = (q + K) / 2 (mod n)
     *            where K = (2^l - 2^129 - 1)*(1 + lambda) (mod n)
     *
     * We will process the computation of C_l(v1, A) and C_l(v2, lambda*A) in groups of
     * ECMULT_CONST_GROUP_SIZE, so we set l to the smallest multiple of ECMULT_CONST_GROUP_SIZE
     * that is not less than 129; this equals ECMULT_CONST_BITS.
     */

    /* The offset to add to s1 and s2 to make them non-negative. Equal to 2^128. */
    static const secp256k1_scalar S_OFFSET = SECP256K1_SCALAR_CONST(0, 0, 0, 1, 0, 0, 0, 0);
    secp256k1_scalar s, v1, v2;
    secp256k1_ge pre_a[ECMULT_CONST_TABLE_SIZE];
    secp256k1_ge pre_a_lam[ECMULT_CONST_TABLE_SIZE];
    secp256k1_fe global_z;
    int group, i;

    /* We're allowed to be non-constant time in the point, and the code below (in particular,
     * secp256k1_ecmult_const_odd_multiples_table_globalz) cannot deal with infinity in a
     * constant-time manner anyway. */
    if (secp256k1_ge_is_infinity(a)) {
        secp256k1_gej_set_infinity(r);
        return;
    }

    /* Compute v1 and v2. */
    secp256k1_scalar_add(&s, q, &secp256k1_ecmult_const_K);
    secp256k1_scalar_half(&s, &s);
    secp256k1_scalar_split_lambda(&v1, &v2, &s);
    secp256k1_scalar_add(&v1, &v1, &S_OFFSET);
    secp256k1_scalar_add(&v2, &v2, &S_OFFSET);

#ifdef VERIFY
    /* Verify that v1 and v2 are in range [0, 2^129-1]. */
    for (i = 129; i < 256; ++i) {
        VERIFY_CHECK(secp256k1_scalar_get_bits_limb32(&v1, i, 1) == 0);
        VERIFY_CHECK(secp256k1_scalar_get_bits_limb32(&v2, i, 1) == 0);
    }
#endif

    /* Calculate odd multiples of A and A*lambda.
     * All multiples are brought to the same Z 'denominator', which is stored
     * in global_z. Due to secp256k1' isomorphism we can do all operations pretending
     * that the Z coordinate was 1, use affine addition formulae, and correct
     * the Z coordinate of the result once at the end.
     */
    secp256k1_gej_set_ge(r, a);
    secp256k1_ecmult_const_odd_multiples_table_globalz(pre_a, &global_z, r);
    for (i = 0; i < ECMULT_CONST_TABLE_SIZE; i++) {
        secp256k1_ge_mul_lambda(&pre_a_lam[i], &pre_a[i]);
    }

    /* Next, we compute r = C_l(v1, A) + C_l(v2, lambda*A).
     *
     * We proceed in groups of ECMULT_CONST_GROUP_SIZE bits, operating on that many bits
     * at a time, from high in v1, v2 to low. Call these bits1 (from v1) and bits2 (from v2).
     *
     * Now note that ECMULT_CONST_TABLE_GET_GE(&t, pre_a, bits1) loads into t a point equal
     * to C_{ECMULT_CONST_GROUP_SIZE}(bits1, A), and analogously for pre_lam_a / bits2.
     * This means that all we need to do is add these looked up values together, multiplied
     * by 2^(ECMULT_GROUP_SIZE * group).
     */
    for (group = ECMULT_CONST_GROUPS - 1; group >= 0; --group) {
        /* Using the _var get_bits function is ok here, since it's only variable in offset and count, not in the scalar. */
        unsigned int bits1 = secp256k1_scalar_get_bits_var(&v1, group * ECMULT_CONST_GROUP_SIZE, ECMULT_CONST_GROUP_SIZE);
        unsigned int bits2 = secp256k1_scalar_get_bits_var(&v2, group * ECMULT_CONST_GROUP_SIZE, ECMULT_CONST_GROUP_SIZE);
        secp256k1_ge t;
        int j;

        ECMULT_CONST_TABLE_GET_GE(&t, pre_a, bits1);
        if (group == ECMULT_CONST_GROUPS - 1) {
            /* Directly set r in the first iteration. */
            secp256k1_gej_set_ge(r, &t);
        } else {
            /* Shift the result so far up. */
            for (j = 0; j < ECMULT_CONST_GROUP_SIZE; ++j) {
                secp256k1_gej_double(r, r);
            }
            secp256k1_gej_add_ge(r, r, &t);
        }
        ECMULT_CONST_TABLE_GET_GE(&t, pre_a_lam, bits2);
        secp256k1_gej_add_ge(r, r, &t);
    }

    /* Map the result back to the secp256k1 curve from the isomorphic curve. */
    secp256k1_fe_mul(&r->z, &r->z, &global_z);
}

static int secp256k1_ecmult_const_xonly(secp256k1_fe* r, const secp256k1_fe *n, const secp256k1_fe *d, const secp256k1_scalar *q, int known_on_curve) {

    /* This algorithm is a generalization of Peter Dettman's technique for
     * avoiding the square root in a random-basepoint x-only multiplication
     * on a Weierstrass curve:
     * https://mailarchive.ietf.org/arch/msg/cfrg/7DyYY6gg32wDgHAhgSb6XxMDlJA/
     *
     *
     * === Background: the effective affine technique ===
     *
     * Let phi_u be the isomorphism that maps (x, y) on secp256k1 curve y^2 = x^3 + 7 to
     * x' = u^2*x, y' = u^3*y on curve y'^2 = x'^3 + u^6*7. This new curve has the same order as
     * the original (it is isomorphic), but moreover, has the same addition/doubling formulas, as
     * the curve b=7 coefficient does not appear in those formulas (or at least does not appear in
     * the formulas implemented in this codebase, both affine and Jacobian). See also Example 9.5.2
     * in https://www.math.auckland.ac.nz/~sgal018/crypto-book/ch9.pdf.
     *
     * This means any linear combination of secp256k1 points can be computed by applying phi_u
     * (with non-zero u) on all input points (including the generator, if used), computing the
     * linear combination on the isomorphic curve (using the same group laws), and then applying
     * phi_u^{-1} to get back to secp256k1.
     *
     * Switching to Jacobian coordinates, note that phi_u applied to (X, Y, Z) is simply
     * (X, Y, Z/u). Thus, if we want to compute (X1, Y1, Z) + (X2, Y2, Z), with identical Z
     * coordinates, we can use phi_Z to transform it to (X1, Y1, 1) + (X2, Y2, 1) on an isomorphic
     * curve where the affine addition formula can be used instead.
     * If (X3, Y3, Z3) = (X1, Y1) + (X2, Y2) on that curve, then our answer on secp256k1 is
     * (X3, Y3, Z3*Z).
     *
     * This is the effective affine technique: if we have a linear combination of group elements
     * to compute, and all those group elements have the same Z coordinate, we can simply pretend
     * that all those Z coordinates are 1, perform the computation that way, and then multiply the
     * original Z coordinate back in.
     *
     * The technique works on any a=0 short Weierstrass curve. It is possible to generalize it to
     * other curves too, but there the isomorphic curves will have different 'a' coefficients,
     * which typically does affect the group laws.
     *
     *
     * === Avoiding the square root for x-only point multiplication ===
     *
     * In this function, we want to compute the X coordinate of q*(n/d, y), for
     * y = sqrt((n/d)^3 + 7). Its negation would also be a valid Y coordinate, but by convention
     * we pick whatever sqrt returns (which we assume to be a deterministic function).
     *
     * Let g = y^2*d^3 = n^3 + 7*d^3. This also means y = sqrt(g/d^3).
     * Further let v = sqrt(d*g), which must exist as d*g = y^2*d^4 = (y*d^2)^2.
     *
     * The input point (n/d, y) also has Jacobian coordinates:
     *
     *     (n/d, y, 1)
     *   = (n/d * v^2, y * v^3, v)
     *   = (n/d * d*g, y * sqrt(d^3*g^3), v)
     *   = (n/d * d*g, sqrt(y^2 * d^3*g^3), v)
     *   = (n*g, sqrt(g/d^3 * d^3*g^3), v)
     *   = (n*g, sqrt(g^4), v)
     *   = (n*g, g^2, v)
     *
     * It is easy to verify that both (n*g, g^2, v) and its negation (n*g, -g^2, v) have affine X
     * coordinate n/d, and this holds even when the square root function doesn't have a
     * deterministic sign. We choose the (n*g, g^2, v) version.
     *
     * Now switch to the effective affine curve using phi_v, where the input point has coordinates
     * (n*g, g^2). Compute (X, Y, Z) = q * (n*g, g^2) there.
     *
     * Back on secp256k1, that means q * (n*g, g^2, v) = (X, Y, v*Z). This last point has affine X
     * coordinate X / (v^2*Z^2) = X / (d*g*Z^2). Determining the affine Y coordinate would involve
     * a square root, but as long as we only care about the resulting X coordinate, no square root
     * is needed anywhere in this computation.
     */

    secp256k1_fe g, i;
    secp256k1_ge p;
    secp256k1_gej rj;

    /* Compute g = (n^3 + B*d^3). */
    secp256k1_fe_sqr(&g, n);
    secp256k1_fe_mul(&g, &g, n);
    if (d) {
        secp256k1_fe b;
        VERIFY_CHECK(!secp256k1_fe_normalizes_to_zero(d));
        secp256k1_fe_sqr(&b, d);
        VERIFY_CHECK(SECP256K1_B <= 8); /* magnitude of b will be <= 8 after the next call */
        secp256k1_fe_mul_int(&b, SECP256K1_B);
        secp256k1_fe_mul(&b, &b, d);
        secp256k1_fe_add(&g, &b);
        if (!known_on_curve) {
            /* We need to determine whether (n/d)^3 + 7 is square.
             *
             *     is_square((n/d)^3 + 7)
             * <=> is_square(((n/d)^3 + 7) * d^4)
             * <=> is_square((n^3 + 7*d^3) * d)
             * <=> is_square(g * d)
             */
            secp256k1_fe c;
            secp256k1_fe_mul(&c, &g, d);
            if (!secp256k1_fe_is_square_var(&c)) return 0;
        }
    } else {
        secp256k1_fe_add_int(&g, SECP256K1_B);
        if (!known_on_curve) {
            /* g at this point equals x^3 + 7. Test if it is square. */
            if (!secp256k1_fe_is_square_var(&g)) return 0;
        }
    }

    /* Compute base point P = (n*g, g^2), the effective affine version of (n*g, g^2, v), which has
     * corresponding affine X coordinate n/d. */
    secp256k1_fe_mul(&p.x, &g, n);
    secp256k1_fe_sqr(&p.y, &g);
    p.infinity = 0;

    /* Perform x-only EC multiplication of P with q. */
    VERIFY_CHECK(!secp256k1_scalar_is_zero(q));
    secp256k1_ecmult_const(&rj, &p, q);
    VERIFY_CHECK(!secp256k1_gej_is_infinity(&rj));

    /* The resulting (X, Y, Z) point on the effective-affine isomorphic curve corresponds to
     * (X, Y, Z*v) on the secp256k1 curve. The affine version of that has X coordinate
     * (X / (Z^2*d*g)). */
    secp256k1_fe_sqr(&i, &rj.z);
    secp256k1_fe_mul(&i, &i, &g);
    if (d) secp256k1_fe_mul(&i, &i, d);
    secp256k1_fe_inv(&i, &i);
    secp256k1_fe_mul(r, &rj.x, &i);

    return 1;
}

#endif /* SECP256K1_ECMULT_CONST_IMPL_H */
