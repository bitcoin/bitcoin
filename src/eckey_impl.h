/***********************************************************************
 * Copyright (c) 2013, 2014 Pieter Wuille                              *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_ECKEY_IMPL_H
#define SECP256K1_ECKEY_IMPL_H

#include "eckey.h"

#include "util.h"
#include "scalar.h"
#include "field.h"
#include "group.h"
#include "ecmult_gen.h"

static int secp256k1_eckey_seckey_tweak_add(secp256k1_scalar *key, const secp256k1_scalar *tweak) {
    secp256k1_scalar_add(key, key, tweak);
    return !secp256k1_scalar_is_zero(key);
}

static int secp256k1_eckey_pubkey_tweak_add(secp256k1_ge *key, const secp256k1_scalar *tweak) {
    secp256k1_gej pt;
    secp256k1_gej_set_ge(&pt, key);
    secp256k1_ecmult(&pt, &pt, &secp256k1_scalar_one, tweak);

    if (secp256k1_gej_is_infinity(&pt)) {
        return 0;
    }
    secp256k1_ge_set_gej(key, &pt);
    return 1;
}

static int secp256k1_eckey_seckey_tweak_mul(secp256k1_scalar *key, const secp256k1_scalar *tweak) {
    int ret;
    ret = !secp256k1_scalar_is_zero(tweak);

    secp256k1_scalar_mul(key, key, tweak);
    return ret;
}

static int secp256k1_eckey_pubkey_tweak_mul(secp256k1_ge *key, const secp256k1_scalar *tweak) {
    secp256k1_gej pt;
    if (secp256k1_scalar_is_zero(tweak)) {
        return 0;
    }

    secp256k1_gej_set_ge(&pt, key);
    secp256k1_ecmult(&pt, &pt, tweak, NULL);
    secp256k1_ge_set_gej(key, &pt);
    return 1;
}

#endif /* SECP256K1_ECKEY_IMPL_H */
