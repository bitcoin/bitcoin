/***********************************************************************
 * Copyright (c) Pieter Wuille, Gregory Maxwell                        *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_ECMULT_GEN_COMPUTE_TABLE_H
#define SECP256K1_ECMULT_GEN_COMPUTE_TABLE_H

#include "ecmult_gen.h"

static void secp256k1_ecmult_gen_compute_table(secp256k1_ge_storage* table, const secp256k1_ge* gen, int blocks, int teeth, int spacing);

#endif /* SECP256K1_ECMULT_GEN_COMPUTE_TABLE_H */
