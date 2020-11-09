/**********************************************************************
 * Copyright (c) 2013, 2014 Pieter Wuille                             *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef SECP256K1_ECKEY_IMPL_H
#define SECP256K1_ECKEY_IMPL_H

#include "eckey.h"

#include "scalar.h"
#include "field.h"
#include "group.h"
#include "ecmult_gen.h"

static int secp256k1_eckey_pubkey_parse(secp256k1_ge *elem, const unsigned char *pub, size_t size) {
    if (size == 33 && (pub[0] == SECP256K1_TAG_PUBKEY_EVEN || pub[0] == SECP256K1_TAG_PUBKEY_ODD)) {
        secp256k1_fe x;
        return secp256k1_fe_set_b32(&x, pub+1) && secp256k1_ge_set_xo_var(elem, &x, pub[0] == SECP256K1_TAG_PUBKEY_ODD);
    } else if (size == 65 && (pub[0] == SECP256K1_TAG_PUBKEY_UNCOMPRESSED || pub[0] == SECP256K1_TAG_PUBKEY_HYBRID_EVEN || pub[0] == SECP256K1_TAG_PUBKEY_HYBRID_ODD)) {
        secp256k1_fe x, y;
        if (!secp256k1_fe_set_b32(&x, pub+1) || !secp256k1_fe_set_b32(&y, pub+33)) {
            return 0;
        }
        secp256k1_ge_set_xy(elem, &x, &y);
        if ((pub[0] == SECP256K1_TAG_PUBKEY_HYBRID_EVEN || pub[0] == SECP256K1_TAG_PUBKEY_HYBRID_ODD) &&
            secp256k1_fe_is_odd(&y) != (pub[0] == SECP256K1_TAG_PUBKEY_HYBRID_ODD)) {
            return 0;
        }
        return secp256k1_ge_is_valid_var(elem);
    } else {
        return 0;
    }
}

static int secp256k1_eckey_pubkey_serialize(secp256k1_ge *elem, unsigned char *pub, size_t *size, int compressed) {
    if (secp256k1_ge_is_infinity(elem)) {
        return 0;
    }
    secp256k1_fe_normalize_var(&elem->x);
    secp256k1_fe_normalize_var(&elem->y);
    secp256k1_fe_get_b32(&pub[1], &elem->x);
    if (compressed) {
        *size = 33;
        pub[0] = secp256k1_fe_is_odd(&elem->y) ? SECP256K1_TAG_PUBKEY_ODD : SECP256K1_TAG_PUBKEY_EVEN;
    } else {
        *size = 65;
        pub[0] = SECP256K1_TAG_PUBKEY_UNCOMPRESSED;
        secp256k1_fe_get_b32(&pub[33], &elem->y);
    }
    return 1;
}

static int secp256k1_eckey_privkey_tweak_add(secp256k1_scalar *key, const secp256k1_scalar *tweak) {
    secp256k1_scalar_add(key, key, tweak);
    return !secp256k1_scalar_is_zero(key);
}

static int secp256k1_eckey_pubkey_tweak_add(const secp256k1_ecmult_context *ctx, secp256k1_ge *key, const secp256k1_scalar *tweak) {
    secp256k1_gej pt;
    secp256k1_scalar one;
    secp256k1_gej_set_ge(&pt, key);
    secp256k1_scalar_set_int(&one, 1);
    secp256k1_ecmult(ctx, &pt, &pt, &one, tweak);

    if (secp256k1_gej_is_infinity(&pt)) {
        return 0;
    }
    secp256k1_ge_set_gej(key, &pt);
    return 1;
}

static int secp256k1_eckey_privkey_tweak_mul(secp256k1_scalar *key, const secp256k1_scalar *tweak) {
    int ret;
    ret = !secp256k1_scalar_is_zero(tweak);

    secp256k1_scalar_mul(key, key, tweak);
    return ret;
}

static int secp256k1_eckey_pubkey_tweak_mul(const secp256k1_ecmult_context *ctx, secp256k1_ge *key, const secp256k1_scalar *tweak) {
    secp256k1_scalar zero;
    secp256k1_gej pt;
    if (secp256k1_scalar_is_zero(tweak)) {
        return 0;
    }

    secp256k1_scalar_set_int(&zero, 0);
    secp256k1_gej_set_ge(&pt, key);
    secp256k1_ecmult(ctx, &pt, &pt, tweak, &zero);
    secp256k1_ge_set_gej(key, &pt);
    return 1;
}

#endif /* SECP256K1_ECKEY_IMPL_H */
