/***********************************************************************
 * Copyright (c) 2020 Pieter Wuille                                    *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_MODULE_EXTRAKEYS_TESTS_EXHAUSTIVE_H
#define SECP256K1_MODULE_EXTRAKEYS_TESTS_EXHAUSTIVE_H

#include "src/modules/extrakeys/main_impl.h"
#include "../../../include/secp256k1_extrakeys.h"

static void test_exhaustive_extrakeys(const secp256k1_context *ctx, const secp256k1_ge* group) {
    secp256k1_keypair keypair[EXHAUSTIVE_TEST_ORDER - 1];
    secp256k1_pubkey pubkey[EXHAUSTIVE_TEST_ORDER - 1];
    secp256k1_xonly_pubkey xonly_pubkey[EXHAUSTIVE_TEST_ORDER - 1];
    int parities[EXHAUSTIVE_TEST_ORDER - 1];
    unsigned char xonly_pubkey_bytes[EXHAUSTIVE_TEST_ORDER - 1][32];
    int i;

    for (i = 1; i < EXHAUSTIVE_TEST_ORDER; i++) {
        secp256k1_fe fe;
        secp256k1_scalar scalar_i;
        unsigned char buf[33];
        int parity;

        secp256k1_scalar_set_int(&scalar_i, i);
        secp256k1_scalar_get_b32(buf, &scalar_i);

        /* Construct pubkey and keypair. */
        CHECK(secp256k1_keypair_create(ctx, &keypair[i - 1], buf));
        CHECK(secp256k1_ec_pubkey_create(ctx, &pubkey[i - 1], buf));

        /* Construct serialized xonly_pubkey from keypair. */
        CHECK(secp256k1_keypair_xonly_pub(ctx, &xonly_pubkey[i - 1], &parities[i - 1], &keypair[i - 1]));
        CHECK(secp256k1_xonly_pubkey_serialize(ctx, xonly_pubkey_bytes[i - 1], &xonly_pubkey[i - 1]));

        /* Parse the xonly_pubkey back and verify it matches the previously serialized value. */
        CHECK(secp256k1_xonly_pubkey_parse(ctx, &xonly_pubkey[i - 1], xonly_pubkey_bytes[i - 1]));
        CHECK(secp256k1_xonly_pubkey_serialize(ctx, buf, &xonly_pubkey[i - 1]));
        CHECK(secp256k1_memcmp_var(xonly_pubkey_bytes[i - 1], buf, 32) == 0);

        /* Construct the xonly_pubkey from the pubkey, and verify it matches the same. */
        CHECK(secp256k1_xonly_pubkey_from_pubkey(ctx, &xonly_pubkey[i - 1], &parity, &pubkey[i - 1]));
        CHECK(parity == parities[i - 1]);
        CHECK(secp256k1_xonly_pubkey_serialize(ctx, buf, &xonly_pubkey[i - 1]));
        CHECK(secp256k1_memcmp_var(xonly_pubkey_bytes[i - 1], buf, 32) == 0);

        /* Compare the xonly_pubkey bytes against the precomputed group. */
        secp256k1_fe_set_b32(&fe, xonly_pubkey_bytes[i - 1]);
        CHECK(secp256k1_fe_equal_var(&fe, &group[i].x));

        /* Check the parity against the precomputed group. */
        fe = group[i].y;
        secp256k1_fe_normalize_var(&fe);
        CHECK(secp256k1_fe_is_odd(&fe) == parities[i - 1]);

        /* Verify that the higher half is identical to the lower half mirrored. */
        if (i > EXHAUSTIVE_TEST_ORDER / 2) {
            CHECK(secp256k1_memcmp_var(xonly_pubkey_bytes[i - 1], xonly_pubkey_bytes[EXHAUSTIVE_TEST_ORDER - i - 1], 32) == 0);
            CHECK(parities[i - 1] == 1 - parities[EXHAUSTIVE_TEST_ORDER - i - 1]);
        }
    }

    /* TODO: keypair/xonly_pubkey tweak tests */
}

#endif
