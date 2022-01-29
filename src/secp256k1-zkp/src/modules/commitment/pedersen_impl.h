/***********************************************************************
 * Copyright (c) 2015 Gregory Maxwell                                  *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php. *
 ***********************************************************************/

#ifndef SECP256K1_MODULE_COMMITMENT_PEDERSEN
#define SECP256K1_MODULE_COMMITMENT_PEDERSEN

#include <string.h>

#include "ecmult_const.h"
#include "group.h"
#include "scalar.h"

/* sec * G + value * G2. */
SECP256K1_INLINE static void secp256k1_pedersen_ecmult(secp256k1_gej *rj, const secp256k1_scalar *sec, uint64_t value, const secp256k1_ge* value_gen, const secp256k1_ge* blind_gen) {
    secp256k1_scalar vs;
    secp256k1_gej bj;
    secp256k1_ge bp;

    secp256k1_scalar_set_u64(&vs, value);
    secp256k1_ecmult_const(rj, value_gen, &vs, 64);
    secp256k1_ecmult_const(&bj, blind_gen, sec, 256);

    /* zero blinding factor indicates that we are not trying to be zero-knowledge,
     * so not being constant-time in this case is OK. */
    if (!secp256k1_gej_is_infinity(&bj)) {
        secp256k1_ge_set_gej(&bp, &bj);
        secp256k1_gej_add_ge(rj, rj, &bp);
    }

    secp256k1_gej_clear(&bj);
    secp256k1_ge_clear(&bp);
    secp256k1_scalar_clear(&vs);
}

SECP256K1_INLINE static void secp256k1_pedersen_blind_ecmult(secp256k1_gej *rj, const secp256k1_scalar *sec, const secp256k1_scalar *value, const secp256k1_ge* value_gen, const secp256k1_ge* blind_gen) {
    secp256k1_gej bj;
    secp256k1_ge bp;

    secp256k1_ecmult_const(rj, value_gen, value, 256);
    secp256k1_ecmult_const(&bj, blind_gen, sec, 256);

    /* zero blinding factor indicates that we are not trying to be zero-knowledge,
     * so not being constant-time in this case is OK. */
    if (!secp256k1_gej_is_infinity(&bj)) {
        secp256k1_ge_set_gej(&bp, &bj);
        secp256k1_gej_add_ge(rj, rj, &bp);
    }

    secp256k1_gej_clear(&bj);
    secp256k1_ge_clear(&bp);
}

#endif
