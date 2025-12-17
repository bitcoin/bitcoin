/**********************************************************************
 * Copyright (c) 2016 Andrew Poelstra & Pieter Wuille                 *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef SECP256K1_GENERATOR_IMPL_H
#define SECP256K1_GENERATOR_IMPL_H

#include "field.h"
#include "group.h"
#include "generator.h"
#include "../sha256.h"
#include "scalar.h"

#if 0
#include "../generator/pedersen_impl.h"

/** Alternative generator for secp256k1.
 *  This is the sha256 of 'g' after standard encoding (without compression),
 *  which happens to be a point on the curve. More precisely, the generator is
 *  derived by running the following script with the sage mathematics software.

    import hashlib
    F = FiniteField (0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFC2F)
    G = '0479be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798483ada7726a3c4655da4fbfc0e1108a8fd17b448a68554199c47d08ffb10d4b8'
    H = EllipticCurve ([F (0), F (7)]).lift_x(F(int(hashlib.sha256(G.decode('hex')).hexdigest(),16)))
    print('%x %x' % H.xy())
 */
static const secp256k1_generator secp256k1_generator_h_internal = {{
    0x50, 0x92, 0x9b, 0x74, 0xc1, 0xa0, 0x49, 0x54, 0xb7, 0x8b, 0x4b, 0x60, 0x35, 0xe9, 0x7a, 0x5e,
    0x07, 0x8a, 0x5a, 0x0f, 0x28, 0xec, 0x96, 0xd5, 0x47, 0xbf, 0xee, 0x9a, 0xce, 0x80, 0x3a, 0xc0,
    0x31, 0xd3, 0xc6, 0x86, 0x39, 0x73, 0x92, 0x6e, 0x04, 0x9e, 0x63, 0x7c, 0xb1, 0xb5, 0xf4, 0x0a,
    0x36, 0xda, 0xc2, 0x8a, 0xf1, 0x76, 0x69, 0x68, 0xc3, 0x0c, 0x23, 0x13, 0xf3, 0xa3, 0x89, 0x04
}};

const secp256k1_generator *secp256k1_generator_h = &secp256k1_generator_h_internal;
#endif

static void secp256k1_generator_load(secp256k1_ge* ge, const secp256k1_generator* gen) {
    int succeed;
    succeed = secp256k1_fe_set_b32(&ge->x, &gen->data[0]);
    VERIFY_CHECK(succeed != 0);
    succeed = secp256k1_fe_set_b32(&ge->y, &gen->data[32]);
    VERIFY_CHECK(succeed != 0);
    ge->infinity = 0;
    (void) succeed;
}

static void secp256k1_generator_save(secp256k1_generator *gen, secp256k1_ge* ge) {
    VERIFY_CHECK(!secp256k1_ge_is_infinity(ge));
    secp256k1_fe_normalize_var(&ge->x);
    secp256k1_fe_normalize_var(&ge->y);
    secp256k1_fe_get_b32(&gen->data[0], &ge->x);
    secp256k1_fe_get_b32(&gen->data[32], &ge->y);
}

#if 0
int secp256k1_generator_parse(const secp256k1_context* ctx, secp256k1_generator* gen, const unsigned char *input) {
    secp256k1_fe x;
    secp256k1_ge ge;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(gen != NULL);
    ARG_CHECK(input != NULL);

    if ((input[0] & 0xFE) != 10 ||
        !secp256k1_fe_set_b32(&x, &input[1]) ||
        !secp256k1_ge_set_xquad(&ge, &x)) {
        return 0;
    }
    if (input[0] & 1) {
        secp256k1_ge_neg(&ge, &ge);
    }
    secp256k1_generator_save(gen, &ge);
    return 1;
}

int secp256k1_generator_serialize(const secp256k1_context* ctx, unsigned char *output, const secp256k1_generator* gen) {
    secp256k1_ge ge;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(output != NULL);
    ARG_CHECK(gen != NULL);

    secp256k1_generator_load(&ge, gen);

    output[0] = 11 ^ secp256k1_fe_is_quad_var(&ge.y);
    secp256k1_fe_normalize_var(&ge.x);
    secp256k1_fe_get_b32(&output[1], &ge.x);
    return 1;
}
#endif

static void shallue_van_de_woestijne(secp256k1_ge* ge, const secp256k1_fe* t) {
    /* Implements the algorithm from:
     *    Indifferentiable Hashing to Barreto-Naehrig Curves
     *    Pierre-Alain Fouque and Mehdi Tibouchi
     *    Latincrypt 2012
     */

    /* Basic algorithm:

       c = sqrt(-3)
       d = (c - 1)/2

       w = c * t / (1 + b + t^2)  [with b = 7]
       x1 = d - t*w
       x2 = -(x1 + 1)
       x3 = 1 + 1/w^2

       To avoid the 2 divisions, compute the joint denominator j = wd * x3d, where
       wd = 1 + b + t^2
       x3d = c^2 * t^2 = -3 * t^2

       so that if j != 0, then

       1 / wd = 1/j * x3d
       1 / x3d = 1/j * wd

       x1 = d - c * t^2 * x3d / j
       x3 = 1 + wd^3 / j

       If j = 0, the function outputs the point (d, f(d)). This point is equal
       to (x1, f(x1)) as defined above if division by 0 is defined to be 0. In
       below code this is not special-cased because secp256k1_fe_inv returns 0
       on input 0.

       j = 0 happens only when t = 0 (since wd != 0 as -8 is not a square).
    */

    static const secp256k1_fe negc = SECP256K1_FE_CONST(0xf5d2d456, 0xcaf80e20, 0xdcc88f3d, 0x586869d3, 0x39e092ea, 0x25eb132b, 0x8272d850, 0xe32a03dd);
    static const secp256k1_fe d = SECP256K1_FE_CONST(0x851695d4, 0x9a83f8ef, 0x919bb861, 0x53cbcb16, 0x630fb68a, 0xed0a766a, 0x3ec693d6, 0x8e6afa40);

    secp256k1_fe wd, x3d, jinv, tmp, x1, x2, x3, alphain, betain, gammain, y1, y2, y3;
    int alphaquad, betaquad;

    /* wd = t^2 */
    secp256k1_fe_sqr(&wd, t); /* mag 1 */
    /* x1 = -c * t^2 */
    secp256k1_fe_mul(&x1, &negc, &wd); /* mag 1 */
    /* x3d = t^2 */
    x3d = wd; /* mag 1 */
    /* x3d = 3 * t^2 */
    secp256k1_fe_mul_int(&x3d, 3); /* mag 3 */
    /* x3d = -3 * t^2 */
    secp256k1_fe_negate(&x3d, &x3d, 3); /* mag 4 */
    /* wd = 1 + b + t^2 */
    secp256k1_fe_add_int(&wd, SECP256K1_B + 1); /* mag 2 */
    /* jinv = wd * x3d */
    secp256k1_fe_mul(&jinv, &wd, &x3d); /* mag 1 */
    /* jinv = 1/(wd * x3d) */
    secp256k1_fe_inv_var(&jinv, &jinv); /* mag 1 */
    /* x1 = -c * t^2 * x3d */
    secp256k1_fe_mul(&x1, &x1, &x3d); /* mag 1 */
    /* x1 = -c * t^2 * x3d * 1/j */
    secp256k1_fe_mul(&x1, &x1, &jinv); /* mag 1 */
    /* x1 = d + -c * t^2 * x3d * 1/j */
    secp256k1_fe_add(&x1, &d); /* mag 2 */
    /* x2 = x1 */
    x2 = x1; /* mag 2 */
    /* x2 = x1 + 1 */
    secp256k1_fe_add_int(&x2, 1); /* mag 3 */
    /* x2 = - (x1 + 1) */
    secp256k1_fe_negate(&x2, &x2, 3); /* mag 4 */
    /* x3 = wd^2 */
    secp256k1_fe_sqr(&x3, &wd); /* mag 1 */
    /* x3 = wd^3 */
    secp256k1_fe_mul(&x3, &x3, &wd); /* mag 1 */
    /* x3 = wd^3 * 1/j */
    secp256k1_fe_mul(&x3, &x3, &jinv); /* mag 1 */
    /* x3 = 1 + (wd^3 * 1/j) */
    secp256k1_fe_add_int(&x3, 1); /* mag 2 */

    secp256k1_fe_sqr(&alphain, &x1); /* mag 1 */
    secp256k1_fe_mul(&alphain, &alphain, &x1); /* mag 1 */
    secp256k1_fe_add_int(&alphain, SECP256K1_B); /* mag 2 */
    secp256k1_fe_sqr(&betain, &x2); /* mag 1 */
    secp256k1_fe_mul(&betain, &betain, &x2); /* mag 1 */
    secp256k1_fe_add_int(&betain, SECP256K1_B); /* mag 2 */
    secp256k1_fe_sqr(&gammain, &x3); /* mag 1 */
    secp256k1_fe_mul(&gammain, &gammain, &x3); /* mag 1 */
    secp256k1_fe_add_int(&gammain, SECP256K1_B); /* mag 2 */

    alphaquad = secp256k1_fe_sqrt_var(&y1, &alphain);
    betaquad = secp256k1_fe_sqrt_var(&y2, &betain);
    secp256k1_fe_sqrt_var(&y3, &gammain);

    secp256k1_fe_cmov(&x1, &x2, (!alphaquad) & betaquad);
    secp256k1_fe_cmov(&y1, &y2, (!alphaquad) & betaquad);
    secp256k1_fe_cmov(&x1, &x3, (!alphaquad) & !betaquad);
    secp256k1_fe_cmov(&y1, &y3, (!alphaquad) & !betaquad);

    secp256k1_ge_set_xy(ge, &x1, &y1);

    /* The linked algorithm from the paper uses the Jacobi symbol of t to
     * determine the Jacobi symbol of the produced y coordinate. Since the
     * rest of the algorithm only uses t^2, we can safely use another criterion
     * as long as negation of t results in negation of the y coordinate. Here
     * we choose to use t's oddness, as it is faster to determine. */
    secp256k1_fe_negate(&tmp, &ge->y, 1);
    secp256k1_fe_cmov(&ge->y, &tmp, secp256k1_fe_is_odd(t));
}

static int secp256k1_generator_generate_internal(secp256k1_generator* gen, const unsigned char *key32, const unsigned char *blind32) {
    static const unsigned char prefix1[17] = "1st generation: ";
    static const unsigned char prefix2[17] = "2nd generation: ";
    secp256k1_fe t = SECP256K1_FE_CONST(0, 0, 0, 0, 0, 0, 0, 4);
    secp256k1_ge add;
    secp256k1_gej accum;
    int overflow;
    sha256_midstate sha256_buf;
    unsigned char b32[32];
    int ret = 1;

    if (blind32) {
        static const secp256k1_gej inf = SECP256K1_GEJ_CONST_INFINITY;
        static const secp256k1_scalar zero = SECP256K1_SCALAR_CONST(0, 0, 0, 0, 0, 0, 0, 0);
        secp256k1_scalar blind;
        secp256k1_scalar_set_b32(&blind, blind32, &overflow);
        ret = !overflow;
        secp256k1_ecmult(&accum, &inf, &zero, &blind);
    }

    {
        sha256_context sha256_ctx = sha256_init(sha256_buf.s);
        sha256_uchars(&sha256_ctx, prefix1, 16);
        sha256_uchars(&sha256_ctx, key32, 32);
        sha256_finalize(&sha256_ctx);
        sha256_fromMidstate(b32, sha256_buf.s);
    }
    ret &= secp256k1_fe_set_b32(&t, b32);
    shallue_van_de_woestijne(&add, &t);
    if (blind32) {
        secp256k1_gej_add_ge_var(&accum, &accum, &add, NULL);
    } else {
        secp256k1_gej_set_ge(&accum, &add);
    }

    {
        sha256_context sha256_ctx = sha256_init(sha256_buf.s);
        sha256_uchars(&sha256_ctx, prefix2, 16);
        sha256_uchars(&sha256_ctx, key32, 32);
        sha256_finalize(&sha256_ctx);
        sha256_fromMidstate(b32, sha256_buf.s);
    }
    ret &= secp256k1_fe_set_b32(&t, b32);
    shallue_van_de_woestijne(&add, &t);
    secp256k1_gej_add_ge_var(&accum, &accum, &add, NULL);

    secp256k1_ge_set_gej_var(&add, &accum);
    secp256k1_generator_save(gen, &add);
    return ret;
}

int secp256k1_generator_generate(secp256k1_generator* gen, const unsigned char *key32) {
    ARG_CHECK(gen != NULL);
    ARG_CHECK(key32 != NULL);
    return secp256k1_generator_generate_internal(gen, key32, NULL);
}

#if 0
int secp256k1_generator_generate_blinded(const secp256k1_context* ctx, secp256k1_generator* gen, const unsigned char *key32, const unsigned char *blind32) {
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(gen != NULL);
    ARG_CHECK(key32 != NULL);
    ARG_CHECK(blind32 != NULL);
    ARG_CHECK(secp256k1_ecmult_gen_context_is_built(&ctx->ecmult_gen_ctx));
    return secp256k1_generator_generate_internal(ctx, gen, key32, blind32);
}

static void secp256k1_pedersen_commitment_load(secp256k1_ge* ge, const secp256k1_pedersen_commitment* commit) {
    secp256k1_fe fe;
    secp256k1_fe_set_b32(&fe, &commit->data[1]);
    secp256k1_ge_set_xquad(ge, &fe);
    if (commit->data[0] & 1) {
        secp256k1_ge_neg(ge, ge);
    }
}

static void secp256k1_pedersen_commitment_save(secp256k1_pedersen_commitment* commit, secp256k1_ge* ge) {
    secp256k1_fe_normalize(&ge->x);
    secp256k1_fe_get_b32(&commit->data[1], &ge->x);
    commit->data[0] = 9 ^ secp256k1_fe_is_quad_var(&ge->y);
}

int secp256k1_pedersen_commitment_parse(const secp256k1_context* ctx, secp256k1_pedersen_commitment* commit, const unsigned char *input) {
    secp256k1_fe x;
    secp256k1_ge ge;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(commit != NULL);
    ARG_CHECK(input != NULL);
    (void) ctx;

    if ((input[0] & 0xFE) != 8 ||
        !secp256k1_fe_set_b32(&x, &input[1]) ||
        !secp256k1_ge_set_xquad(&ge, &x)) {
        return 0;
    }
    if (input[0] & 1) {
        secp256k1_ge_neg(&ge, &ge);
    }
    secp256k1_pedersen_commitment_save(commit, &ge);
    return 1;
}

int secp256k1_pedersen_commitment_serialize(const secp256k1_context* ctx, unsigned char *output, const secp256k1_pedersen_commitment* commit) {
    secp256k1_ge ge;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(output != NULL);
    ARG_CHECK(commit != NULL);

    secp256k1_pedersen_commitment_load(&ge, commit);

    output[0] = 9 ^ secp256k1_fe_is_quad_var(&ge.y);
    secp256k1_fe_normalize_var(&ge.x);
    secp256k1_fe_get_b32(&output[1], &ge.x);
    return 1;
}

/* Generates a pedersen commitment: *commit = blind * G + value * G2. The blinding factor is 32 bytes.*/
int secp256k1_pedersen_commit(const secp256k1_context* ctx, secp256k1_pedersen_commitment *commit, const unsigned char *blind, uint64_t value, const secp256k1_generator* gen) {
    secp256k1_ge genp;
    secp256k1_gej rj;
    secp256k1_ge r;
    secp256k1_scalar sec;
    int overflow;
    int ret = 0;
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(secp256k1_ecmult_gen_context_is_built(&ctx->ecmult_gen_ctx));
    ARG_CHECK(commit != NULL);
    ARG_CHECK(blind != NULL);
    ARG_CHECK(gen != NULL);
    secp256k1_generator_load(&genp, gen);
    secp256k1_scalar_set_b32(&sec, blind, &overflow);
    if (!overflow) {
        secp256k1_pedersen_ecmult(&ctx->ecmult_gen_ctx, &rj, &sec, value, &genp);
        if (!secp256k1_gej_is_infinity(&rj)) {
            secp256k1_ge_set_gej(&r, &rj);
            secp256k1_pedersen_commitment_save(commit, &r);
            ret = 1;
        }
        secp256k1_gej_clear(&rj);
        secp256k1_ge_clear(&r);
    }
    secp256k1_scalar_clear(&sec);
    return ret;
}

/** Takes a list of n pointers to 32 byte blinding values, the first negs of which are treated with positive sign and the rest
 *  negative, then calculates an additional blinding value that adds to zero.
 */
int secp256k1_pedersen_blind_sum(const secp256k1_context* ctx, unsigned char *blind_out, const unsigned char * const *blinds, size_t n, size_t npositive) {
    secp256k1_scalar acc;
    secp256k1_scalar x;
    size_t i;
    int overflow;
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(blind_out != NULL);
    ARG_CHECK(blinds != NULL);
    ARG_CHECK(npositive <= n);
    (void) ctx;
    secp256k1_scalar_set_int(&acc, 0);
    for (i = 0; i < n; i++) {
        secp256k1_scalar_set_b32(&x, blinds[i], &overflow);
        if (overflow) {
            return 0;
        }
        if (i >= npositive) {
            secp256k1_scalar_negate(&x, &x);
        }
        secp256k1_scalar_add(&acc, &acc, &x);
    }
    secp256k1_scalar_get_b32(blind_out, &acc);
    secp256k1_scalar_clear(&acc);
    secp256k1_scalar_clear(&x);
    return 1;
}

/* Takes two lists of commitments and sums the first set and subtracts the second and verifies that they sum to excess. */
int secp256k1_pedersen_verify_tally(const secp256k1_context* ctx, const secp256k1_pedersen_commitment * const* commits, size_t pcnt, const secp256k1_pedersen_commitment * const* ncommits, size_t ncnt) {
    secp256k1_gej accj;
    secp256k1_ge add;
    size_t i;
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(!pcnt || (commits != NULL));
    ARG_CHECK(!ncnt || (ncommits != NULL));
    (void) ctx;
    secp256k1_gej_set_infinity(&accj);
    for (i = 0; i < ncnt; i++) {
        secp256k1_pedersen_commitment_load(&add, ncommits[i]);
        secp256k1_gej_add_ge_var(&accj, &accj, &add, NULL);
    }
    secp256k1_gej_neg(&accj, &accj);
    for (i = 0; i < pcnt; i++) {
        secp256k1_pedersen_commitment_load(&add, commits[i]);
        secp256k1_gej_add_ge_var(&accj, &accj, &add, NULL);
    }
    return secp256k1_gej_is_infinity(&accj);
}

int secp256k1_pedersen_blind_generator_blind_sum(const secp256k1_context* ctx, const uint64_t *value, const unsigned char* const* generator_blind, unsigned char* const* blinding_factor, size_t n_total, size_t n_inputs) {
    secp256k1_scalar sum;
    secp256k1_scalar tmp;
    size_t i;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(n_total == 0 || value != NULL);
    ARG_CHECK(n_total == 0 || generator_blind != NULL);
    ARG_CHECK(n_total == 0 || blinding_factor != NULL);
    ARG_CHECK(n_total > n_inputs);
    (void) ctx;

    if (n_total == 0) {
        return 1;
    }

    secp256k1_scalar_set_int(&sum, 0);
    secp256k1_scalar_set_int(&tmp, 0);

    /* Here, n_total > 0. Thus the loop runs at least once.
       Thus we may use a do-while loop, which checks the loop
       condition only at the end.

       The do-while loop helps GCC prove that the loop runs at least
       once and suppresses a -Wmaybe-uninitialized warning. */
    i = 0;
    do {
        int overflow = 0;
        secp256k1_scalar addend;
        secp256k1_scalar_set_u64(&addend, value[i]);  /* s = v */

        secp256k1_scalar_set_b32(&tmp, generator_blind[i], &overflow);
        if (overflow == 1) {
            secp256k1_scalar_clear(&tmp);
            secp256k1_scalar_clear(&addend);
            secp256k1_scalar_clear(&sum);
            return 0;
        }
        secp256k1_scalar_mul(&addend, &addend, &tmp); /* s = vr */

        secp256k1_scalar_set_b32(&tmp, blinding_factor[i], &overflow);
        if (overflow == 1) {
            secp256k1_scalar_clear(&tmp);
            secp256k1_scalar_clear(&addend);
            secp256k1_scalar_clear(&sum);
            return 0;
        }
        secp256k1_scalar_add(&addend, &addend, &tmp); /* s = vr + r' */
        secp256k1_scalar_cond_negate(&addend, i < n_inputs);  /* s is negated if it's an input */
        secp256k1_scalar_add(&sum, &sum, &addend);    /* sum += s */
        secp256k1_scalar_clear(&addend);

        i++;
    } while (i < n_total);

    /* Right now tmp has the last pedersen blinding factor. Subtract the sum from it. */
    secp256k1_scalar_negate(&sum, &sum);
    secp256k1_scalar_add(&tmp, &tmp, &sum);
    secp256k1_scalar_get_b32(blinding_factor[n_total - 1], &tmp);

    secp256k1_scalar_clear(&tmp);
    secp256k1_scalar_clear(&sum);
    return 1;
}

#endif
#endif
