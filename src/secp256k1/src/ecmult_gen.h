/**********************************************************************
 * Copyright (c) 2013, 2014 Pieter Wuille                             *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef _SECP256K1_ECMULT_GEN_
#define _SECP256K1_ECMULT_GEN_

#include "scalar.h"
#include "group.h"

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
    secp256k1_ge_storage_t (*prec)[64][16]; /* prec[j][i] = 16^j * i * G + U_i */
    secp256k1_scalar_t blind;
    secp256k1_gej_t initial;
} secp256k1_ecmult_gen_context_t;

static void secp256k1_ecmult_gen_context_init(secp256k1_ecmult_gen_context_t* ctx);
static void secp256k1_ecmult_gen_context_build(secp256k1_ecmult_gen_context_t* ctx);
static void secp256k1_ecmult_gen_context_clone(secp256k1_ecmult_gen_context_t *dst,
                                               const secp256k1_ecmult_gen_context_t* src);
static void secp256k1_ecmult_gen_context_clear(secp256k1_ecmult_gen_context_t* ctx);
static int secp256k1_ecmult_gen_context_is_built(const secp256k1_ecmult_gen_context_t* ctx);

/** Multiply with the generator: R = a*G */
static void secp256k1_ecmult_gen(const secp256k1_ecmult_gen_context_t* ctx, secp256k1_gej_t *r, const secp256k1_scalar_t *a);

static void secp256k1_ecmult_gen_blind(secp256k1_ecmult_gen_context_t *ctx, const unsigned char *seed32);

#endif
