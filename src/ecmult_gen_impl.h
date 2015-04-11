/**********************************************************************
 * Copyright (c) 2013, 2014 Pieter Wuille                             *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef _SECP256K1_ECMULT_GEN_IMPL_H_
#define _SECP256K1_ECMULT_GEN_IMPL_H_

#include "scalar.h"
#include "group.h"
#include "ecmult_gen.h"

static void secp256k1_ecmult_gen_context_init(secp256k1_ecmult_gen_context_t *ctx) {
    ctx->prec = NULL;
}

static void secp256k1_ecmult_gen_context_build(secp256k1_ecmult_gen_context_t *ctx) {
    secp256k1_ge_t prec[1024];
    secp256k1_gej_t gj;
    secp256k1_gej_t nums_gej;
    int i, j;

    if (ctx->prec != NULL) {
        return;
    }

    ctx->prec = (secp256k1_ge_storage_t (*)[64][16])checked_malloc(sizeof(*ctx->prec));

    /* get the generator */
    secp256k1_gej_set_ge(&gj, &secp256k1_ge_const_g);

    /* Construct a group element with no known corresponding scalar (nothing up my sleeve). */
    {
        static const unsigned char nums_b32[33] = "The scalar for this x is unknown";
        secp256k1_fe_t nums_x;
        secp256k1_ge_t nums_ge;
        VERIFY_CHECK(secp256k1_fe_set_b32(&nums_x, nums_b32));
        VERIFY_CHECK(secp256k1_ge_set_xo_var(&nums_ge, &nums_x, 0));
        secp256k1_gej_set_ge(&nums_gej, &nums_ge);
        /* Add G to make the bits in x uniformly distributed. */
        secp256k1_gej_add_ge_var(&nums_gej, &nums_gej, &secp256k1_ge_const_g);
    }

    /* compute prec. */
    {
        secp256k1_gej_t precj[1024]; /* Jacobian versions of prec. */
        secp256k1_gej_t gbase;
        secp256k1_gej_t numsbase;
        gbase = gj; /* 16^j * G */
        numsbase = nums_gej; /* 2^j * nums. */
        for (j = 0; j < 64; j++) {
            /* Set precj[j*16 .. j*16+15] to (numsbase, numsbase + gbase, ..., numsbase + 15*gbase). */
            precj[j*16] = numsbase;
            for (i = 1; i < 16; i++) {
                secp256k1_gej_add_var(&precj[j*16 + i], &precj[j*16 + i - 1], &gbase);
            }
            /* Multiply gbase by 16. */
            for (i = 0; i < 4; i++) {
                secp256k1_gej_double_var(&gbase, &gbase);
            }
            /* Multiply numbase by 2. */
            secp256k1_gej_double_var(&numsbase, &numsbase);
            if (j == 62) {
                /* In the last iteration, numsbase is (1 - 2^j) * nums instead. */
                secp256k1_gej_neg(&numsbase, &numsbase);
                secp256k1_gej_add_var(&numsbase, &numsbase, &nums_gej);
            }
        }
        secp256k1_ge_set_all_gej_var(1024, prec, precj);
    }
    for (j = 0; j < 64; j++) {
        for (i = 0; i < 16; i++) {
            secp256k1_ge_to_storage(&(*ctx->prec)[j][i], &prec[j*16 + i]);
        }
    }
}

static int secp256k1_ecmult_gen_context_is_built(const secp256k1_ecmult_gen_context_t* ctx) {
    return ctx->prec != NULL;
}

static void secp256k1_ecmult_gen_context_clone(secp256k1_ecmult_gen_context_t *dst,
                                               const secp256k1_ecmult_gen_context_t *src) {
    if (src->prec == NULL) {
        dst->prec = NULL;
    } else {
        dst->prec = (secp256k1_ge_storage_t (*)[64][16])checked_malloc(sizeof(*dst->prec));
        memcpy(dst->prec, src->prec, sizeof(*dst->prec));
    }
}

static void secp256k1_ecmult_gen_context_clear(secp256k1_ecmult_gen_context_t *ctx) {
    free(ctx->prec);
    ctx->prec = NULL;
}

static void secp256k1_ecmult_gen(const secp256k1_ecmult_gen_context_t *ctx, secp256k1_gej_t *r, const secp256k1_scalar_t *gn) {
    secp256k1_ge_t add;
    secp256k1_ge_storage_t adds;
    int bits;
    int i, j;
    memset(&adds, 0, sizeof(adds));
    secp256k1_gej_set_infinity(r);
    add.infinity = 0;
    for (j = 0; j < 64; j++) {
        bits = secp256k1_scalar_get_bits(gn, j * 4, 4);
        for (i = 0; i < 16; i++) {
            /** This uses a conditional move to avoid any secret data in array indexes.
             *   _Any_ use of secret indexes has been demonstrated to result in timing
             *   sidechannels, even when the cache-line access patterns are uniform.
             *  See also:
             *   "A word of warning", CHES 2013 Rump Session, by Daniel J. Bernstein and Peter Schwabe
             *    (https://cryptojedi.org/peter/data/chesrump-20130822.pdf) and
             *   "Cache Attacks and Countermeasures: the Case of AES", RSA 2006,
             *    by Dag Arne Osvik, Adi Shamir, and Eran Tromer
             *    (http://www.tau.ac.il/~tromer/papers/cache.pdf)
             */
            secp256k1_ge_storage_cmov(&adds, &(*ctx->prec)[j][i], i == bits);
        }
        secp256k1_ge_from_storage(&add, &adds);
        secp256k1_gej_add_ge(r, r, &add);
    }
    bits = 0;
    secp256k1_ge_clear(&add);
}

#endif
