/***********************************************************************
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_MODULE_SILENTPAYMENTS_TESTS_H
#define SECP256K1_MODULE_SILENTPAYMENTS_TESTS_H

#include "../../../include/secp256k1_silentpayments.h"
#include "../../../src/modules/silentpayments/vectors.h"

void run_silentpayments_test_vector_send(const struct bip352_test_vector *test, const unsigned char *a_sum, const unsigned char *input_hash) {
    secp256k1_pubkey last_scan_pubkey; /* needed for grouping outputs */
    unsigned char s_one[32] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1};
    size_t i;
    unsigned int k = 0;

    /* Check that sender creates expected outputs */
    CHECK(secp256k1_ec_pubkey_create(CTX, &last_scan_pubkey, s_one)); /* dirty hack: set last_scan_pubkey to generator point initially */
    for (i = 0; i < test->num_recipient_outputs; i++) {
        secp256k1_pubkey receiver_scan_pubkey;
        secp256k1_pubkey receiver_spend_pubkey;
        unsigned char shared_secret[33];
        secp256k1_xonly_pubkey created_output_xonly;
        unsigned char created_output[32];

        CHECK(secp256k1_ec_pubkey_parse(CTX, &receiver_scan_pubkey, test->receiver_pubkeys[i].scan_pubkey, 33));
        CHECK(secp256k1_ec_pubkey_parse(CTX, &receiver_spend_pubkey, test->receiver_pubkeys[i].spend_pubkey, 33));
        if (secp256k1_ec_pubkey_cmp(CTX, &receiver_scan_pubkey, &last_scan_pubkey) != 0) {
            k = 0;
        } else {
            k++;
        }
        CHECK(secp256k1_silentpayments_create_shared_secret(CTX, shared_secret, &receiver_scan_pubkey, a_sum, input_hash));
        CHECK(secp256k1_silentpayments_sender_create_output_pubkey(CTX, &created_output_xonly, shared_secret,
            &receiver_spend_pubkey, k));
        CHECK(secp256k1_xonly_pubkey_serialize(CTX, created_output, &created_output_xonly));
        CHECK(secp256k1_memcmp_var(created_output, test->recipient_outputs[i], 32) == 0);

        last_scan_pubkey = receiver_scan_pubkey;
    }
}

void run_silentpayments_test_vector_receive(const struct bip352_test_vector *test, const secp256k1_pubkey *A_sum, const unsigned char *input_hash) {
    struct label_cache_entry {
        secp256k1_pubkey label;
        unsigned char label_tweak[32];
    };
    struct labels_cache {
        size_t entries_used;
        struct label_cache_entry entries[10];
    };

    struct labels_cache labels_cache;
    secp256k1_pubkey receiver_scan_pubkey;
    secp256k1_pubkey receiver_spend_pubkey;
    unsigned char shared_secret[33];
    size_t i;
    unsigned int k = 0;
    int outputs_to_check[10] = {0};
    size_t num_found_outputs = 0;
    unsigned char found_output_pubkeys[10][32];
    unsigned char found_seckey_tweaks[10][32];
    unsigned char found_signatures[10][64];

    /* scan / spend pubkeys are not in the given data of the receiver part, so let's compute them */
    CHECK(secp256k1_ec_pubkey_create(CTX, &receiver_scan_pubkey, test->scan_seckey));
    CHECK(secp256k1_ec_pubkey_create(CTX, &receiver_spend_pubkey, test->spend_seckey));

    /* create shared secret */
    {
        unsigned char shared_secret_fullnode[33];
        unsigned char shared_secret_lightclient[33];
        secp256k1_pubkey A_tweaked;

        CHECK(secp256k1_silentpayments_create_shared_secret(CTX, shared_secret_fullnode, A_sum, test->scan_seckey, input_hash));
        /* check that creating shared secret in light client / index mode (with intermediate A_tweaked) leads to the same result */
        CHECK(secp256k1_silentpayments_create_tweaked_pubkey(CTX, &A_tweaked, A_sum, input_hash));
        CHECK(secp256k1_silentpayments_create_shared_secret(CTX, shared_secret_lightclient, &A_tweaked, test->scan_seckey, NULL));
        CHECK(secp256k1_memcmp_var(shared_secret_fullnode, shared_secret_lightclient, 33) == 0);

        memcpy(shared_secret, shared_secret_fullnode, 33);
    }

    /* create labels cache */
    labels_cache.entries_used = 0;
    for (i = 0; i < test->num_labels; i++) {
        unsigned int m = test->label_integers[i];
        struct label_cache_entry *cache_entry = &labels_cache.entries[labels_cache.entries_used];
        CHECK(secp256k1_silentpayments_create_label_tweak(CTX, &cache_entry->label, cache_entry->label_tweak, test->scan_seckey, m));
        labels_cache.entries_used++;
    }

    /* scan through outputs, check that expected found outputs match */
    for (i = 0; i < test->num_to_scan_outputs; i++) {
        outputs_to_check[i] = 1;
    }

    while(1) {
        int found_something = 0;

        for (i = 0; i < test->num_to_scan_outputs; i++) {
            int direct_match, output_detected = 0;
            unsigned char t_k[32];
            secp256k1_silentpayments_label_data label_candidates;
            secp256k1_xonly_pubkey to_scan_output;

            if (outputs_to_check[i] == 0)
                continue;

            CHECK(secp256k1_xonly_pubkey_parse(CTX, &to_scan_output, test->to_scan_outputs[i]));
            CHECK(secp256k1_silentpayments_receiver_scan_output(CTX, &direct_match, t_k, &label_candidates,
                shared_secret, &receiver_spend_pubkey, k, &to_scan_output));
            if (direct_match) {
                output_detected = 1;
            } else {
                /* no direct match, check labels cache */
                secp256k1_pubkey *label_found = NULL;
                unsigned char *label_tweak = NULL;

                size_t l;
                for (l = 0; l < labels_cache.entries_used; l++) {
                    if (secp256k1_ec_pubkey_cmp(CTX, &label_candidates.label, &labels_cache.entries[l].label) == 0 ||
                        secp256k1_ec_pubkey_cmp(CTX, &label_candidates.label_negated, &labels_cache.entries[l].label) == 0) {
                        label_found = &labels_cache.entries[l].label;
                        label_tweak = labels_cache.entries[l].label_tweak;
                        break;
                    }
                }
                if (label_found != NULL) {
                    CHECK(secp256k1_ec_seckey_tweak_add(CTX, t_k, label_tweak));
                    output_detected = 1;
                }
            }
            if (output_detected) {
                unsigned char full_seckey[32];
                secp256k1_keypair keypair;
                unsigned char signature[64];
                const unsigned char msg32[32] = /* sha256("message") */
                    {0xab,0x53,0x0a,0x13,0xe4,0x59,0x14,0x98,0x2b,0x79,0xf9,0xb7,0xe3,0xfb,0xa9,0x94,
                     0xcf,0xd1,0xf3,0xfb,0x22,0xf7,0x1c,0xea,0x1a,0xfb,0xf0,0x2b,0x46,0x0c,0x6d,0x1d};
                const unsigned char aux32[32] = /* sha256("random auxiliary data") */
                    {0x0b,0x3f,0xdd,0xfd,0x67,0xbf,0x76,0xae,0x76,0x39,0xee,0x73,0x5b,0x70,0xff,0x15,
                     0x83,0xfd,0x92,0x48,0xc0,0x57,0xd2,0x86,0x07,0xa2,0x15,0xf4,0x0b,0x0a,0x3e,0xcc};
                outputs_to_check[i] = 0;
                memcpy(found_output_pubkeys[num_found_outputs], test->to_scan_outputs[i], 32);
                memcpy(found_seckey_tweaks[num_found_outputs], t_k, 32);
                CHECK(secp256k1_silentpayments_create_output_seckey(CTX, full_seckey, test->spend_seckey, t_k, NULL));
                CHECK(secp256k1_keypair_create(CTX, &keypair, full_seckey));
                CHECK(secp256k1_schnorrsig_sign32(CTX, signature, msg32, &keypair, aux32));
                memcpy(found_signatures[num_found_outputs], signature, 64);
                num_found_outputs++;
                k++;
                found_something = 1;
            }
        }
        if (!found_something)
            break;
    }

    /* compare expected and scanned outputs (including calculated seckey tweaks and signatures) */
    CHECK(num_found_outputs == test->num_found_outputs);
    for (i = 0; i < num_found_outputs; i++) {
        CHECK(secp256k1_memcmp_var(found_output_pubkeys[i], test->found_output_pubkeys[i], 32) == 0);
        CHECK(secp256k1_memcmp_var(found_seckey_tweaks[i], test->found_seckey_tweaks[i], 32) == 0);
        CHECK(secp256k1_memcmp_var(found_signatures[i], test->found_signatures[i], 64) == 0);
    }
}

void run_silentpayments_test_vectors(void) {
    size_t i, j;

    for (i = 0; i < sizeof(bip352_test_vectors) / sizeof(bip352_test_vectors[0]); i++) {
        const struct bip352_test_vector *test = &bip352_test_vectors[i];
        unsigned char a_sum[32];
        secp256k1_pubkey A_sum;
        unsigned char input_hash_sender[32];
        unsigned char input_hash_receiver[32];

        /* Calculate private tweak data (sender perspective) */
        {
            unsigned char const *plain_seckeys[MAX_INPUTS_PER_TEST_CASE];
            unsigned char const *taproot_seckeys[MAX_INPUTS_PER_TEST_CASE];
            for (j = 0; j < test->num_plain_inputs; j++) {
                plain_seckeys[j] = test->plain_seckeys[j];
            }
            for (j = 0; j < test->num_taproot_inputs; j++) {
                taproot_seckeys[j] = test->taproot_seckeys[j];
            }
            CHECK(secp256k1_silentpayments_create_private_tweak_data(CTX, a_sum, input_hash_sender,
                test->num_plain_inputs > 0 ? plain_seckeys : NULL, test->num_plain_inputs,
                test->num_taproot_inputs > 0 ? taproot_seckeys : NULL, test->num_taproot_inputs,
                test->outpoint_smallest));
        }

        /* Calculate public tweak data (receiver perspective) */
        {
            secp256k1_pubkey plain_pubkeys_objs[MAX_INPUTS_PER_TEST_CASE];
            secp256k1_xonly_pubkey xonly_pubkeys_objs[MAX_INPUTS_PER_TEST_CASE];
            secp256k1_pubkey const *plain_pubkeys[MAX_INPUTS_PER_TEST_CASE];
            secp256k1_xonly_pubkey const *xonly_pubkeys[MAX_INPUTS_PER_TEST_CASE];
            for (j = 0; j < test->num_plain_inputs; j++) {
                CHECK(secp256k1_ec_pubkey_parse(CTX, &plain_pubkeys_objs[j], test->plain_pubkeys[j], 33));
                plain_pubkeys[j] = &plain_pubkeys_objs[j];
            }
            for (j = 0; j < test->num_taproot_inputs; j++) {
                CHECK(secp256k1_xonly_pubkey_parse(CTX, &xonly_pubkeys_objs[j], test->xonly_pubkeys[j]));
                xonly_pubkeys[j] = &xonly_pubkeys_objs[j];
            }
            CHECK(secp256k1_silentpayments_create_public_tweak_data(CTX, &A_sum, input_hash_receiver,
                test->num_plain_inputs > 0 ? plain_pubkeys : NULL, test->num_plain_inputs,
                test->num_taproot_inputs > 0 ? xonly_pubkeys : NULL, test->num_taproot_inputs,
                test->outpoint_smallest));
        }

        /* First sanity check: verify that a_sum * G == A_sum, and that input hashes match */
        {
            secp256k1_pubkey A_sum_calculated;
            CHECK(secp256k1_ec_pubkey_create(CTX, &A_sum_calculated, a_sum));
            CHECK(secp256k1_ec_pubkey_cmp(CTX, &A_sum, &A_sum_calculated) == 0);
            CHECK(secp256k1_memcmp_var(input_hash_sender, input_hash_receiver, 32) == 0);
        }

        run_silentpayments_test_vector_send(test, a_sum, input_hash_sender);
        run_silentpayments_test_vector_receive(test, &A_sum, input_hash_receiver);
    }
}

void run_silentpayments_tests(void) {
    run_silentpayments_test_vectors();

    /* TODO: add a few manual tests here, that target the ECC-related parts of silent payments */
}

#endif
