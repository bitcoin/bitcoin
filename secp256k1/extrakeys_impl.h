/***********************************************************************
 * Copyright (c) 2020 Jonas Nick                                       *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_EXTRAKEYS_IMPL
#define SECP256K1_EXTRAKEYS_IMPL

#include "secp256k1.h"
#include "extrakeys.h"

static SECP256K1_INLINE int secp256k1_xonly_pubkey_load(secp256k1_ge *ge, const secp256k1_xonly_pubkey *pubkey) {
    return secp256k1_pubkey_load(ge, (const secp256k1_pubkey *) pubkey);
}

static SECP256K1_INLINE void secp256k1_xonly_pubkey_save(secp256k1_xonly_pubkey *pubkey, secp256k1_ge *ge) {
    secp256k1_pubkey_save((secp256k1_pubkey *) pubkey, ge);
}

static int secp256k1_xonly_pubkey_parse(secp256k1_xonly_pubkey *pubkey, const unsigned char *input32) {
    secp256k1_ge pk;
    secp256k1_fe x;

    ARG_CHECK(pubkey != NULL);
    memset(pubkey, 0, sizeof(*pubkey));
    ARG_CHECK(input32 != NULL);

    if (!secp256k1_fe_set_b32(&x, input32)) {
        return 0;
    }
    if (!secp256k1_ge_set_xo_var(&pk, &x, 0)) {
        return 0;
    }
    if (!secp256k1_ge_is_in_correct_subgroup(&pk)) {
        return 0;
    }
    secp256k1_xonly_pubkey_save(pubkey, &pk);
    return 1;
}

static int secp256k1_xonly_pubkey_serialize(unsigned char *output32, const secp256k1_xonly_pubkey *pubkey) {
    secp256k1_ge pk;

    ARG_CHECK(output32 != NULL);
    memset(output32, 0, 32);
    ARG_CHECK(pubkey != NULL);

    if (!secp256k1_xonly_pubkey_load(&pk, pubkey)) {
        return 0;
    }
    secp256k1_fe_get_b32(output32, &pk.x);
    return 1;
}

/** Keeps a group element as is if it has an even Y and otherwise negates it.
 *  y_parity is set to 0 in the former case and to 1 in the latter case.
 *  Requires that the coordinates of r are normalized. */
static int secp256k1_extrakeys_ge_even_y(secp256k1_ge *r) {
    int y_parity = 0;
    VERIFY_CHECK(!secp256k1_ge_is_infinity(r));

    if (secp256k1_fe_is_odd(&r->y)) {
        secp256k1_fe_negate(&r->y, &r->y, 1);
        y_parity = 1;
    }
    return y_parity;
}

static int secp256k1_xonly_pubkey_from_pubkey(secp256k1_xonly_pubkey *xonly_pubkey, int *pk_parity, const secp256k1_pubkey *pubkey) {
    secp256k1_ge pk;
    int tmp;

    ARG_CHECK(xonly_pubkey != NULL);
    ARG_CHECK(pubkey != NULL);

    if (!secp256k1_pubkey_load(&pk, pubkey)) {
        return 0;
    }
    tmp = secp256k1_extrakeys_ge_even_y(&pk);
    if (pk_parity != NULL) {
        *pk_parity = tmp;
    }
    secp256k1_xonly_pubkey_save(xonly_pubkey, &pk);
    return 1;
}

static int secp256k1_xonly_pubkey_tweak_add(secp256k1_pubkey *output_pubkey, const secp256k1_xonly_pubkey *internal_pubkey, const unsigned char *tweak32) {
    secp256k1_ge pk;

    ARG_CHECK(output_pubkey != NULL);
    memset(output_pubkey, 0, sizeof(*output_pubkey));
    ARG_CHECK(internal_pubkey != NULL);
    ARG_CHECK(tweak32 != NULL);

    if (!secp256k1_xonly_pubkey_load(&pk, internal_pubkey)
        || !secp256k1_ec_pubkey_tweak_add_helper(&pk, tweak32)) {
        return 0;
    }
    secp256k1_pubkey_save(output_pubkey, &pk);
    return 1;
}
#endif
