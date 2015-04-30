/**********************************************************************
 * Copyright (c) 2013, 2014 Pieter Wuille                             *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef _SECP256K1_ECMULT_GEN_
#define _SECP256K1_ECMULT_GEN_

#include "scalar.h"
#include "group.h"

static void secp256k1_ecmult_gen_start(void);
static void secp256k1_ecmult_gen_stop(void);

/** Multiply with the generator: R = a*G */
static void secp256k1_ecmult_gen(secp256k1_gej_t *r, const secp256k1_scalar_t *a);

#endif
