/**********************************************************************
 * Copyright (c) 2016 Andrew Poelstra & Pieter Wuille                 *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef SECP256K1_MODULE_GENERATOR_MAIN
#define SECP256K1_MODULE_GENERATOR_MAIN

#include <stdio.h>

#include "field.h"
#include "group.h"
#include "hash.h"
#include "scalar.h"

/** Standard secp256k1 generator */
const secp256k1_generator secp256k1_generator_const_g = {{
    0x79, 0xbe, 0x66, 0x7e, 0xf9, 0xdc, 0xbb, 0xac, 0x55, 0xa0, 0x62, 0x95, 0xce, 0x87, 0x0b, 0x07,
    0x02, 0x9b, 0xfc, 0xdb, 0x2d, 0xce, 0x28, 0xd9, 0x59, 0xf2, 0x81, 0x5b, 0x16, 0xf8, 0x17, 0x98,
    0x48, 0x3a, 0xda, 0x77, 0x26, 0xa3, 0xc4, 0x65, 0x5d, 0xa4, 0xfb, 0xfc, 0x0e, 0x11, 0x08, 0xa8,
    0xfd, 0x17, 0xb4, 0x48, 0xa6, 0x85, 0x54, 0x19, 0x9c, 0x47, 0xd0, 0x8f, 0xfb, 0x10, 0xd4, 0xb8
}};

/** Alternate secp256k1 generator, used in Elements Alpha.
 *  Computed as the hash of the above G, DER-encoded with 0x04 (uncompressed pubkey) as its flag byte.
 *  import hashlib
 *  C = EllipticCurve ([F (0), F (7)])
 *  G_bytes = '0479be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798483ada7726a3c4655da4fbfc0e1108a8fd17b448a68554199c47d08ffb10d4b8'.decode('hex')
 *  H = C.lift_x(int(hashlib.sha256(G_bytes).hexdigest(),16))
 */
const secp256k1_generator secp256k1_generator_const_h = {{
    0x50, 0x92, 0x9b, 0x74, 0xc1, 0xa0, 0x49, 0x54, 0xb7, 0x8b, 0x4b, 0x60, 0x35, 0xe9, 0x7a, 0x5e,
    0x07, 0x8a, 0x5a, 0x0f, 0x28, 0xec, 0x96, 0xd5, 0x47, 0xbf, 0xee, 0x9a, 0xce, 0x80, 0x3a, 0xc0,
    0x31, 0xd3, 0xc6, 0x86, 0x39, 0x73, 0x92, 0x6e, 0x04, 0x9e, 0x63, 0x7c, 0xb1, 0xb5, 0xf4, 0x0a,
    0x36, 0xda, 0xc2, 0x8a, 0xf1, 0x76, 0x69, 0x68, 0xc3, 0x0c, 0x23, 0x13, 0xf3, 0xa3, 0x89, 0x04
}};

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

       To avoid the 2 divisions, compute the above in numerator/denominator form:
       wn = c * t
       wd = 1 + 7 + t^2
       x1n = d*wd - t*wn
       x1d = wd
       x2n = -(x1n + wd)
       x2d = wd
       x3n = wd^2 + c^2 + t^2
       x3d = (c * t)^2

       The joint denominator j = wd * c^2 * t^2, and
       1 / x1d = 1/j * c^2 * t^2
       1 / x2d = x3d = 1/j * wd
    */

    static const secp256k1_fe c = SECP256K1_FE_CONST(0x0a2d2ba9, 0x3507f1df, 0x233770c2, 0xa797962c, 0xc61f6d15, 0xda14ecd4, 0x7d8d27ae, 0x1cd5f852);
    static const secp256k1_fe d = SECP256K1_FE_CONST(0x851695d4, 0x9a83f8ef, 0x919bb861, 0x53cbcb16, 0x630fb68a, 0xed0a766a, 0x3ec693d6, 0x8e6afa40);
    static const secp256k1_fe b = SECP256K1_FE_CONST(0, 0, 0, 0, 0, 0, 0, 7);
    static const secp256k1_fe b_plus_one = SECP256K1_FE_CONST(0, 0, 0, 0, 0, 0, 0, 8);

    secp256k1_fe wn, wd, x1n, x2n, x3n, x3d, jinv, tmp, x1, x2, x3, alphain, betain, gammain, y1, y2, y3;
    int alphaquad, betaquad;

    secp256k1_fe_mul(&wn, &c, t); /* mag 1 */
    secp256k1_fe_sqr(&wd, t); /* mag 1 */
    secp256k1_fe_add(&wd, &b_plus_one); /* mag 2 */
    secp256k1_fe_mul(&tmp, t, &wn); /* mag 1 */
    secp256k1_fe_negate(&tmp, &tmp, 1); /* mag 2 */
    secp256k1_fe_mul(&x1n, &d, &wd); /* mag 1 */
    secp256k1_fe_add(&x1n, &tmp); /* mag 3 */
    x2n = x1n; /* mag 3 */
    secp256k1_fe_add(&x2n, &wd); /* mag 5 */
    secp256k1_fe_negate(&x2n, &x2n, 5); /* mag 6 */
    secp256k1_fe_mul(&x3d, &c, t); /* mag 1 */
    secp256k1_fe_sqr(&x3d, &x3d); /* mag 1 */
    secp256k1_fe_sqr(&x3n, &wd); /* mag 1 */
    secp256k1_fe_add(&x3n, &x3d); /* mag 2 */
    secp256k1_fe_mul(&jinv, &x3d, &wd); /* mag 1 */
    secp256k1_fe_inv(&jinv, &jinv); /* mag 1 */
    secp256k1_fe_mul(&x1, &x1n, &x3d); /* mag 1 */
    secp256k1_fe_mul(&x1, &x1, &jinv); /* mag 1 */
    secp256k1_fe_mul(&x2, &x2n, &x3d); /* mag 1 */
    secp256k1_fe_mul(&x2, &x2, &jinv); /* mag 1 */
    secp256k1_fe_mul(&x3, &x3n, &wd); /* mag 1 */
    secp256k1_fe_mul(&x3, &x3, &jinv); /* mag 1 */

    secp256k1_fe_sqr(&alphain, &x1); /* mag 1 */
    secp256k1_fe_mul(&alphain, &alphain, &x1); /* mag 1 */
    secp256k1_fe_add(&alphain, &b); /* mag 2 */
    secp256k1_fe_sqr(&betain, &x2); /* mag 1 */
    secp256k1_fe_mul(&betain, &betain, &x2); /* mag 1 */
    secp256k1_fe_add(&betain, &b); /* mag 2 */
    secp256k1_fe_sqr(&gammain, &x3); /* mag 1 */
    secp256k1_fe_mul(&gammain, &gammain, &x3); /* mag 1 */
    secp256k1_fe_add(&gammain, &b); /* mag 2 */

    alphaquad = secp256k1_fe_sqrt(&y1, &alphain);
    betaquad = secp256k1_fe_sqrt(&y2, &betain);
    secp256k1_fe_sqrt(&y3, &gammain);

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

static int secp256k1_generator_generate_internal(const secp256k1_context* ctx, secp256k1_generator* gen, const unsigned char *key32, const unsigned char *blind32) {
    static const unsigned char prefix1[17] = "1st generation: ";
    static const unsigned char prefix2[17] = "2nd generation: ";
    secp256k1_fe t = SECP256K1_FE_CONST(0, 0, 0, 0, 0, 0, 0, 4);
    secp256k1_ge add;
    secp256k1_gej accum;
    int overflow;
    secp256k1_sha256 sha256;
    unsigned char b32[32];
    int ret = 1;

    if (blind32) {
        secp256k1_scalar blind;
        secp256k1_scalar_set_b32(&blind, blind32, &overflow);
        ret = !overflow;
        CHECK(ret);
        secp256k1_ecmult_gen(&ctx->ecmult_gen_ctx, &accum, &blind);
    }

    secp256k1_sha256_initialize(&sha256);
    secp256k1_sha256_write(&sha256, prefix1, 16);
    secp256k1_sha256_write(&sha256, key32, 32);
    secp256k1_sha256_finalize(&sha256, b32);
    ret &= secp256k1_fe_set_b32(&t, b32);
    CHECK(ret);
    shallue_van_de_woestijne(&add, &t);
    if (blind32) {
        secp256k1_gej_add_ge(&accum, &accum, &add);
    } else {
        secp256k1_gej_set_ge(&accum, &add);
    }

    secp256k1_sha256_initialize(&sha256);
    secp256k1_sha256_write(&sha256, prefix2, 16);
    secp256k1_sha256_write(&sha256, key32, 32);
    secp256k1_sha256_finalize(&sha256, b32);
    ret &= secp256k1_fe_set_b32(&t, b32);
    CHECK(ret);
    shallue_van_de_woestijne(&add, &t);
    secp256k1_gej_add_ge(&accum, &accum, &add);

    secp256k1_ge_set_gej(&add, &accum);
    secp256k1_generator_save(gen, &add);
    return ret;
}

int secp256k1_generator_generate(const secp256k1_context* ctx, secp256k1_generator* gen, const unsigned char *key32) {
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(gen != NULL);
    ARG_CHECK(key32 != NULL);
    return secp256k1_generator_generate_internal(ctx, gen, key32, NULL);
}

int secp256k1_generator_generate_blinded(const secp256k1_context* ctx, secp256k1_generator* gen, const unsigned char *key32, const unsigned char *blind32) {
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(gen != NULL);
    ARG_CHECK(key32 != NULL);
    ARG_CHECK(blind32 != NULL);
    ARG_CHECK(secp256k1_ecmult_gen_context_is_built(&ctx->ecmult_gen_ctx));
    return secp256k1_generator_generate_internal(ctx, gen, key32, blind32);
}

#endif
