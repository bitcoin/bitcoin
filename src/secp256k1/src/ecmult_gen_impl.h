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

typedef struct {
    /* For accelerating the computation of a*G:
     * To harden against timing attacks, use the following mechanism:
     * * Break up the multiplicand into groups of 4 bits, called n_0, n_1, n_2, ..., n_63.
     * * Compute sum(n_i * 16^i * G + U_i, i=0..63), where:
     *   * U_i = U * 2^i (for i=0..62)
     *   * U_i = U * (1-2^63) (for i=63)
     *   where U is a point with no known corresponding scalar. Note that sum(U_i, i=0..63) = 0.
     * For each i, and each of the 16 possible values of n_i, (n_i * 16^i * G + U_i) is
     * precomputed (call it prec(i, n_i)). The formula now becomes sum(prec(i, n_i), i=0..63).
     * None of the resulting prec group elements have a known scalar, and neither do any of
     * the intermediate sums while computing a*G.
     */
    secp256k1_fe_t prec[64][16][2]; /* prec[j][i] = (16^j * i * G + U_i).{x,y} */
} secp256k1_ecmult_gen_consts_t;

static const secp256k1_ecmult_gen_consts_t *secp256k1_ecmult_gen_consts = NULL;

static void secp256k1_ecmult_gen_start(void) {
    if (secp256k1_ecmult_gen_consts != NULL)
        return;

    /* Allocate the precomputation table. */
    secp256k1_ecmult_gen_consts_t *ret = (secp256k1_ecmult_gen_consts_t*)malloc(sizeof(secp256k1_ecmult_gen_consts_t));

    /* get the generator */
    const secp256k1_ge_t *g = &secp256k1_ge_consts->g;
    secp256k1_gej_t gj; secp256k1_gej_set_ge(&gj, g);

    /* Construct a group element with no known corresponding scalar (nothing up my sleeve). */
    secp256k1_gej_t nums_gej;
    {
        static const unsigned char nums_b32[32] = "The scalar for this x is unknown";
        secp256k1_fe_t nums_x;
        VERIFY_CHECK(secp256k1_fe_set_b32(&nums_x, nums_b32));
        secp256k1_ge_t nums_ge;
        VERIFY_CHECK(secp256k1_ge_set_xo(&nums_ge, &nums_x, 0));
        secp256k1_gej_set_ge(&nums_gej, &nums_ge);
        /* Add G to make the bits in x uniformly distributed. */
        secp256k1_gej_add_ge_var(&nums_gej, &nums_gej, g);
    }

    /* compute prec. */
    secp256k1_ge_t prec[1024];
    {
        secp256k1_gej_t precj[1024]; /* Jacobian versions of prec. */
        secp256k1_gej_t gbase; gbase = gj; /* 16^j * G */
        secp256k1_gej_t numsbase; numsbase = nums_gej; /* 2^j * nums. */
        for (int j=0; j<64; j++) {
            /* Set precj[j*16 .. j*16+15] to (numsbase, numsbase + gbase, ..., numsbase + 15*gbase). */
            precj[j*16] = numsbase;
            for (int i=1; i<16; i++) {
                secp256k1_gej_add_var(&precj[j*16 + i], &precj[j*16 + i - 1], &gbase);
            }
            /* Multiply gbase by 16. */
            for (int i=0; i<4; i++) {
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
    for (int j=0; j<64; j++) {
        for (int i=0; i<16; i++) {
            VERIFY_CHECK(!secp256k1_ge_is_infinity(&prec[j*16 + i]));
            ret->prec[j][i][0] = prec[j*16 + i].x;
            ret->prec[j][i][1] = prec[j*16 + i].y;
        }
    }

    /* Set the global pointer to the precomputation table. */
    secp256k1_ecmult_gen_consts = ret;
}

static void secp256k1_ecmult_gen_stop(void) {
    if (secp256k1_ecmult_gen_consts == NULL)
        return;

    secp256k1_ecmult_gen_consts_t *c = (secp256k1_ecmult_gen_consts_t*)secp256k1_ecmult_gen_consts;
    secp256k1_ecmult_gen_consts = NULL;
    free(c);
}

static void secp256k1_ecmult_gen(secp256k1_gej_t *r, const secp256k1_scalar_t *gn) {
    const secp256k1_ecmult_gen_consts_t *c = secp256k1_ecmult_gen_consts;
    secp256k1_gej_set_infinity(r);
    secp256k1_ge_t add;
    add.infinity = 0;
    int bits;
    for (int j=0; j<64; j++) {
        bits = secp256k1_scalar_get_bits(gn, j * 4, 4);
        for (int i=0; i<16; i++) {
            secp256k1_fe_cmov(&add.x, &c->prec[j][i][0], i == bits);
            secp256k1_fe_cmov(&add.y, &c->prec[j][i][1], i == bits);
        }
        secp256k1_gej_add_ge(r, r, &add);
    }
    bits = 0;
    secp256k1_ge_clear(&add);
}

#endif
