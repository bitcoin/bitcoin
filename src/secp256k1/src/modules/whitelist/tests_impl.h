/**********************************************************************
 * Copyright (c) 2014-2016 Pieter Wuille, Andrew Poelstra             *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef SECP256K1_MODULE_WHITELIST_TESTS
#define SECP256K1_MODULE_WHITELIST_TESTS

#include "include/secp256k1_whitelist.h"

void test_whitelist_end_to_end(const size_t n_keys) {
    unsigned char **online_seckey = (unsigned char **) malloc(n_keys * sizeof(*online_seckey));
    unsigned char **summed_seckey = (unsigned char **) malloc(n_keys * sizeof(*summed_seckey));
    secp256k1_pubkey *online_pubkeys = (secp256k1_pubkey *) malloc(n_keys * sizeof(*online_pubkeys));
    secp256k1_pubkey *offline_pubkeys = (secp256k1_pubkey *) malloc(n_keys * sizeof(*offline_pubkeys));

    secp256k1_scalar ssub;
    unsigned char csub[32];
    secp256k1_pubkey sub_pubkey;

    /* Generate random keys */
    size_t i;
    /* Start with subkey */
    random_scalar_order_test(&ssub);
    secp256k1_scalar_get_b32(csub, &ssub);
    CHECK(secp256k1_ec_seckey_verify(ctx, csub) == 1);
    CHECK(secp256k1_ec_pubkey_create(ctx, &sub_pubkey, csub) == 1);
    /* Then offline and online whitelist keys */
    for (i = 0; i < n_keys; i++) {
        secp256k1_scalar son, soff;

        online_seckey[i] = (unsigned char *) malloc(32);
        summed_seckey[i] = (unsigned char *) malloc(32);

        /* Create two keys */
        random_scalar_order_test(&son);
        secp256k1_scalar_get_b32(online_seckey[i], &son);
        CHECK(secp256k1_ec_seckey_verify(ctx, online_seckey[i]) == 1);
        CHECK(secp256k1_ec_pubkey_create(ctx, &online_pubkeys[i], online_seckey[i]) == 1);

        random_scalar_order_test(&soff);
        secp256k1_scalar_get_b32(summed_seckey[i], &soff);
        CHECK(secp256k1_ec_seckey_verify(ctx, summed_seckey[i]) == 1);
        CHECK(secp256k1_ec_pubkey_create(ctx, &offline_pubkeys[i], summed_seckey[i]) == 1);

        /* Make summed_seckey correspond to the sum of offline_pubkey and sub_pubkey */
        secp256k1_scalar_add(&soff, &soff, &ssub);
        secp256k1_scalar_get_b32(summed_seckey[i], &soff);
        CHECK(secp256k1_ec_seckey_verify(ctx, summed_seckey[i]) == 1);
    }

    /* Sign/verify with each one */
    for (i = 0; i < n_keys; i++) {
        unsigned char serialized[32 + 4 + 32 * SECP256K1_WHITELIST_MAX_N_KEYS] = {0};
        secp256k1_whitelist_signature sig;
        secp256k1_whitelist_signature sig1;

        CHECK(secp256k1_whitelist_sign(ctx, &sig, online_pubkeys, offline_pubkeys, n_keys, &sub_pubkey, online_seckey[i], summed_seckey[i], i, NULL, NULL));
        CHECK(secp256k1_whitelist_verify(ctx, &sig, online_pubkeys, offline_pubkeys, &sub_pubkey) == 1);
        /* Check that exchanging keys causes a failure */
        CHECK(secp256k1_whitelist_verify(ctx, &sig, offline_pubkeys, online_pubkeys, &sub_pubkey) != 1);
        /* Serialization round trip */
        CHECK(secp256k1_whitelist_signature_serialize(ctx, serialized, &sig) == 1);
        CHECK(secp256k1_whitelist_signature_parse(ctx, &sig1, serialized) == 1);
        CHECK(secp256k1_whitelist_verify(ctx, &sig1, online_pubkeys, offline_pubkeys, &sub_pubkey) == 1);
        CHECK(secp256k1_whitelist_verify(ctx, &sig1, offline_pubkeys, online_pubkeys, &sub_pubkey) != 1);
        /* Test n_keys */
        CHECK(secp256k1_whitelist_signature_n_keys(&sig) == n_keys);
        CHECK(secp256k1_whitelist_signature_n_keys(&sig1) == n_keys);
    }

    for (i = 0; i < n_keys; i++) {
        free(online_seckey[i]);
        free(summed_seckey[i]);
    }
    free(online_seckey);
    free(summed_seckey);
    free(online_pubkeys);
    free(offline_pubkeys);
}

void test_whitelist_bad_parse(void) {
    const unsigned char serialized[] = {
        /* Hash */
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        /* Length in excess of maximum */
        0x00, 0x00, 0x01, 0x00
        /* No room for s-values; parse should be rejected before reading past length */
    };
    secp256k1_whitelist_signature sig;

    CHECK(secp256k1_whitelist_signature_parse(ctx, &sig, serialized) == 0);
}

void run_whitelist_tests(void) {
    int i;
    for (i = 0; i < count; i++) {
        test_whitelist_end_to_end(1);
        test_whitelist_end_to_end(10);
        test_whitelist_end_to_end(50);
    }
}

#endif
