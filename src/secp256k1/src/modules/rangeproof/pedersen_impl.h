/***********************************************************************
 * Copyright (c) 2015 Gregory Maxwell                                  *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php. *
 ***********************************************************************/

#ifndef _SECP256K1_PEDERSEN_IMPL_H_
#define _SECP256K1_PEDERSEN_IMPL_H_

#include <string.h>

#include "eckey.h"
#include "ecmult_const.h"
#include "ecmult_gen.h"
#include "group.h"
#include "field.h"
#include "scalar.h"
#include "util.h"

static void secp256k1_pedersen_scalar_set_u64(secp256k1_scalar *sec, uint64_t value) {
    unsigned char data[32];
    int i;
    for (i = 0; i < 24; i++) {
        data[i] = 0;
    }
    for (; i < 32; i++) {
        data[i] = value >> 56;
        value <<= 8;
    }
    secp256k1_scalar_set_b32(sec, data, NULL);
    memset(data, 0, 32);
}

static void secp256k1_pedersen_ecmult_small(secp256k1_gej *r, uint64_t gn, const secp256k1_ge* genp) {
    secp256k1_scalar s;
    secp256k1_pedersen_scalar_set_u64(&s, gn);
    secp256k1_ecmult_const(r, genp, &s, 64);
    secp256k1_scalar_clear(&s);
}

/* sec * G + value * G2. */
SECP256K1_INLINE static void secp256k1_pedersen_ecmult(const secp256k1_ecmult_gen_context *ecmult_gen_ctx, secp256k1_gej *rj, const secp256k1_scalar *sec, uint64_t value, const secp256k1_ge* genp) {
    secp256k1_gej vj;
    secp256k1_ecmult_gen(ecmult_gen_ctx, rj, sec);
    secp256k1_pedersen_ecmult_small(&vj, value, genp);
    /* FIXME: constant time. */
    secp256k1_gej_add_var(rj, rj, &vj, NULL);
    secp256k1_gej_clear(&vj);
}

#endif
