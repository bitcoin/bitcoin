/**********************************************************************
 * Copyright (c) 2013, 2014 Pieter Wuille                             *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef _SECP256K1_ECMULT_
#define _SECP256K1_ECMULT_

#include "num.h"
#include "group.h"

typedef struct {
    /* For accelerating the computation of a*P + b*G: */
    secp256k1_ge_storage_t (*pre_g)[];    /* odd multiples of the generator */
#ifdef USE_ENDOMORPHISM
    secp256k1_ge_storage_t (*pre_g_128)[]; /* odd multiples of 2^128*generator */
#endif
} secp256k1_ecmult_context_t;

static void secp256k1_ecmult_context_init(secp256k1_ecmult_context_t *ctx);
static void secp256k1_ecmult_context_build(secp256k1_ecmult_context_t *ctx, const callback_t *cb);
static void secp256k1_ecmult_context_clone(secp256k1_ecmult_context_t *dst,
                                           const secp256k1_ecmult_context_t *src, const callback_t *cb);
static void secp256k1_ecmult_context_clear(secp256k1_ecmult_context_t *ctx);
static int secp256k1_ecmult_context_is_built(const secp256k1_ecmult_context_t *ctx);

/** Double multiply: R = na*A + ng*G */
static void secp256k1_ecmult(const secp256k1_ecmult_context_t *ctx, secp256k1_gej_t *r, const secp256k1_gej_t *a, const secp256k1_scalar_t *na, const secp256k1_scalar_t *ng);

#endif
