/**********************************************************************
 * Copyright (c) 2014, 2015 Gregory Maxwell                          *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef _SECP256K1_PEDERSEN_H_
#define _SECP256K1_PEDERSEN_H_

#include "ecmult_gen.h"
#include "group.h"
#include "scalar.h"

#include <stdint.h>

/** Multiply a small number with the generator: r = gn*G2 */
static void secp256k1_pedersen_ecmult_small(secp256k1_gej *r, uint64_t gn, const secp256k1_ge* genp);

/* sec * G + value * G2. */
static void secp256k1_pedersen_ecmult(const secp256k1_ecmult_gen_context *ecmult_gen_ctx, secp256k1_gej *rj, const secp256k1_scalar *sec, uint64_t value, const secp256k1_ge* genp);

#endif
