/*************************************************************************
 * To the extent possible under law, the author(s) have dedicated all    *
 * copyright and related and neighboring rights to the software in this  *
 * file to the public domain worldwide. This software is distributed     *
 * without any warranty. For the CC0 Public Domain Dedication, see       *
 * EXAMPLES_COPYING or https://creativecommons.org/publicdomain/zero/1.0 *
 *************************************************************************/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <secp256k1_extrakeys.h>
#include <secp256k1_silentpayments.h>

#include "examples_util.h"

#define N_INPUTS  2
#define N_OUTPUTS 3

/* Static data for Bob and Carol's Silent Payments addresses */
static unsigned char smallest_outpoint[36] = {
    0x16, 0x9e, 0x1e, 0x83, 0xe9, 0x30, 0x85, 0x33, 0x91,
    0xbc, 0x6f, 0x35, 0xf6, 0x05, 0xc6, 0x75, 0x4c, 0xfe,
    0xad, 0x57, 0xcf, 0x83, 0x87, 0x63, 0x9d, 0x3b, 0x40,
    0x96, 0xc5, 0x4f, 0x18, 0xf4, 0x00, 0x00, 0x00, 0x00
};
static unsigned char bob_scan_key[32] = {
    0xa8, 0x90, 0x54, 0xc9, 0x5b, 0xe3, 0xc3, 0x01,
    0x56, 0x65, 0x74, 0xf2, 0xaa, 0x93, 0xad, 0xe0,
    0x51, 0x85, 0x09, 0x03, 0xa6, 0x9c, 0xbd, 0xd1,
    0xd4, 0x7e, 0xae, 0x26, 0x3d, 0x7b, 0xc0, 0x31
};
static unsigned char bob_spend_key[32] = {
    0x9d, 0x6a, 0xd8, 0x55, 0xce, 0x34, 0x17, 0xef,
    0x84, 0xe8, 0x36, 0x89, 0x2e, 0x5a, 0x56, 0x39,
    0x2b, 0xfb, 0xa0, 0x5f, 0xa5, 0xd9, 0x7c, 0xce,
    0xa3, 0x0e, 0x26, 0x6f, 0x54, 0x0e, 0x08, 0xb3
};
static unsigned char bob_scan_and_spend_pubkeys[2][33] = {
    {
        0x02, 0x15, 0x40, 0xae, 0xa8, 0x97, 0x54, 0x7a,
        0xd4, 0x39, 0xb4, 0xe0, 0xf6, 0x09, 0xe5, 0xf0,
        0xfa, 0x63, 0xde, 0x89, 0xab, 0x11, 0xed, 0xe3,
        0x1e, 0x8c, 0xde, 0x4b, 0xe2, 0x19, 0x42, 0x5f, 0x23
    },
    {
        0x02, 0x5c, 0xc9, 0x85, 0x6d, 0x6f, 0x83, 0x75,
        0x35, 0x0e, 0x12, 0x39, 0x78, 0xda, 0xac, 0x20,
        0x0c, 0x26, 0x0c, 0xb5, 0xb5, 0xae, 0x83, 0x10,
        0x6c, 0xab, 0x90, 0x48, 0x4d, 0xcd, 0x8f, 0xcf, 0x36
    }
};
static unsigned char carol_scan_key[32] = {
    0x04, 0xb2, 0xa4, 0x11, 0x63, 0x5c, 0x09, 0x77,
    0x59, 0xaa, 0xcd, 0x0f, 0x00, 0x5a, 0x4c, 0x82,
    0xc8, 0xc9, 0x28, 0x62, 0xc6, 0xfc, 0x28, 0x4b,
    0x80, 0xb8, 0xef, 0xeb, 0xc2, 0x0c, 0x3d, 0x17
};
static unsigned char carol_address[2][33] = {
    {
        0x03, 0xbb, 0xc6, 0x3f, 0x12, 0x74, 0x5d, 0x3b,
        0x9e, 0x9d, 0x24, 0xc6, 0xcd, 0x7a, 0x1e, 0xfe,
        0xba, 0xd0, 0xa7, 0xf4, 0x69, 0x23, 0x2f, 0xbe,
        0xcf, 0x31, 0xfb, 0xa7, 0xb4, 0xf7, 0xdd, 0xed, 0xa8
    },
    {
        0x03, 0x81, 0xeb, 0x9a, 0x9a, 0x9e, 0xc7, 0x39,
        0xd5, 0x27, 0xc1, 0x63, 0x1b, 0x31, 0xb4, 0x21,
        0x56, 0x6f, 0x5c, 0x2a, 0x47, 0xb4, 0xab, 0x5b,
        0x1f, 0x6a, 0x68, 0x6d, 0xfb, 0x68, 0xea, 0xb7, 0x16
    }
};

/** Labels
 *
 *  The structs and callback function are implemented here as a demonstration
 *  of how the label lookup callback is meant to query a label cache and return
 *  the label tweak when a match is found. This is for demonstration purposes
 *  only and not optimized. In production, it is expected that the
 *  caller will be using a much more performant method for storing and querying
 *  labels.
 *
 *  Recipients not using labels can ignore these steps and simply pass `NULL`
 *  for the label_lookup and label_context arguments:
 *
 *      secp256k1_silentpayments_recipient_scan_outputs(..., NULL, NULL);
 */

struct label_cache_entry {
    unsigned char label[33];
    unsigned char label_tweak[32];
};

struct labels_cache {
    size_t entries_used;
    struct label_cache_entry entries[5];
};

const unsigned char* label_lookup(
    const unsigned char* label33,
    const void* cache_ptr
) {
    const struct labels_cache* cache = (const struct labels_cache*)cache_ptr;
    size_t i;
    for (i = 0; i < cache->entries_used; i++) {
        if (memcmp(cache->entries[i].label, label33, 33) == 0) {
            return cache->entries[i].label_tweak;
        }
    }
    return NULL;
}

int main(void) {
    unsigned char randomize[32];
    unsigned char serialized_xonly[32];
    secp256k1_xonly_pubkey tx_inputs[N_INPUTS];
    const secp256k1_xonly_pubkey *tx_input_ptrs[N_INPUTS];
    secp256k1_xonly_pubkey tx_outputs[N_OUTPUTS];
    secp256k1_xonly_pubkey *tx_output_ptrs[N_OUTPUTS];
    secp256k1_silentpayments_found_output found_outputs[N_OUTPUTS];
    secp256k1_silentpayments_found_output *found_output_ptrs[N_OUTPUTS];
    secp256k1_silentpayments_prevouts_summary prevouts_summary;
    secp256k1_pubkey unlabeled_spend_pubkey;
    struct labels_cache bob_labels_cache;
    unsigned char bob_address[2][33];
    int ret;
    size_t i, n_found_outputs;

    /* Before we can call actual API functions, we need to create a "context" */
    secp256k1_context* ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
    if (!fill_random(randomize, sizeof(randomize))) {
        printf("Failed to generate randomness\n");
        return EXIT_FAILURE;
    }
    /* Randomizing the context is recommended to protect against side-channel
     * leakage. See `secp256k1_context_randomize` in secp256k1.h for more
     * information about it. This should never fail. */
    ret = secp256k1_context_randomize(ctx, randomize);
    assert(ret);

    /* Set up the pointer arrays. These will be used for sending and scanning. */
    for (i = 0; i < N_INPUTS; i++) {
        tx_input_ptrs[i] = &tx_inputs[i];
    }
    for (i = 0; i < N_OUTPUTS; i++) {
        tx_output_ptrs[i] = &tx_outputs[i];
        found_output_ptrs[i] = &found_outputs[i];
    }

    /*** Create a labeled Silent Payments address (Bob) ***/
    {
        size_t len = 33;
        secp256k1_pubkey label, labeled_spend_pubkey;
        unsigned int m = 1;

        /* Load Bob's spend public key */
        ret = secp256k1_ec_pubkey_parse(ctx,
            &unlabeled_spend_pubkey,
            bob_scan_and_spend_pubkeys[1],
            33
        );
        assert(ret);
        /* Create a (label_tweak, label) pair and add them to the labels cache.
         *
         * Bob MUST keep track of all of the labels he has used by adding them
         * to the labels cache. Otherwise, he will not find outputs sent to his
         * labeled addresses when scanning. If Bob ever loses access to his cache
         * or forgets the total number of labels used he can create a large cache
         * with m = 0 ... 100_000, following the recommendation from BIP0352.
         */
        ret = secp256k1_silentpayments_recipient_create_label(ctx,
            &label,
            bob_labels_cache.entries[0].label_tweak,
            bob_scan_key,
            m
        );
        if (!ret) {
            printf("Something went wrong, event with negligible probability happened.\n");
            return EXIT_FAILURE;
        }
        ret = secp256k1_ec_pubkey_serialize(ctx,
            bob_labels_cache.entries[0].label,
            &len,
            &label,
            SECP256K1_EC_COMPRESSED
        );
        assert(ret);
        bob_labels_cache.entries_used = 1;
        /* Now that the labels cache has been updated, Bob creates his labeled
         * Silent Payments address and publishes it.
         */
        ret = secp256k1_silentpayments_recipient_create_labeled_spend_pubkey(ctx, &labeled_spend_pubkey, &unlabeled_spend_pubkey, &label);
        if (!ret) {
            printf("Something went wrong, event with negligible probability happened.\n");
            return EXIT_FAILURE;
        }
        memcpy(bob_address[0], bob_scan_and_spend_pubkeys[0], 33);
        ret = secp256k1_ec_pubkey_serialize(ctx,
            bob_address[1],
            &len,
            &labeled_spend_pubkey,
            SECP256K1_EC_COMPRESSED
        );
        assert(ret);
    }
    /*** Sending (Alice) ***/
    {
        secp256k1_keypair sender_keypairs[N_INPUTS];
        const secp256k1_keypair *sender_keypair_ptrs[N_INPUTS];
        secp256k1_silentpayments_recipient recipients[N_OUTPUTS];
        const secp256k1_silentpayments_recipient *recipient_ptrs[N_OUTPUTS];
        /* 2D array for holding multiple public key pairs. The second index, i.e., [2],
         * is to represent the spend and scan public keys. */
        unsigned char (*sp_addresses[N_OUTPUTS])[2][33];
        unsigned char seckey[32];

        /*** Generate secret keys for the sender ***
         *
         * In this example, only taproot inputs are used but the function can be
         * called with a mix of taproot seckeys and plain seckeys. Taproot
         * seckeys are passed as keypairs to allow the sending function to check
         * if the secret keys need to be negated without needing to do an
         * expensive pubkey generation. This is not needed for plain seckeys
         * since there is no need for negation.
         *
         * The public key from each input keypair is saved in the `tx_inputs`
         * array. This array will be used later in the example to represent the
         * public keys the recipient will extract from the transaction inputs.
         *
         * If the secret key is zero or out of range (bigger than secp256k1's
         * order), fail. Note that the probability of this happening is
         * negligible. */
        for (i = 0; i < N_INPUTS; i++) {
            if (!fill_random(seckey, sizeof(seckey))) {
                printf("Failed to generate randomness\n");
                return EXIT_FAILURE;
            }
            /* Try to create a keypair with a valid context, it should only fail
             * if the secret key is zero or out of range. */
            if (secp256k1_keypair_create(ctx, &sender_keypairs[i], seckey)) {
                sender_keypair_ptrs[i] = &sender_keypairs[i];
                ret = secp256k1_keypair_xonly_pub(
                    ctx,
                    &tx_inputs[i],
                    NULL,
                    &sender_keypairs[i]
                );
                assert(ret);
            } else {
                printf("Failed to create keypair\n");
                return EXIT_FAILURE;
            }
        }
        /*** Create the recipient objects ***/

        /* Alice is sending to Bob and Carol in this transaction:
         *
         *     1. One output to Bob's labeled address
         *     2. Two outputs for Carol
         *
         * To create multiple outputs for Carol, Alice simply passes Carol's
         * Silent Payments address multiple times.
         */
        sp_addresses[0] = &carol_address;
        sp_addresses[1] = &bob_address;
        sp_addresses[2] = &carol_address;
        for (i = 0; i < N_OUTPUTS; i++) {
            ret = secp256k1_ec_pubkey_parse(ctx,
                &recipients[i].scan_pubkey,
                (*(sp_addresses[i]))[0],
                33
            );
            ret &= secp256k1_ec_pubkey_parse(ctx,
                &recipients[i].spend_pubkey,
                (*(sp_addresses[i]))[1],
                33
            );
            if (!ret) {
                printf("Something went wrong, this is not a valid Silent Payments address.\n");
                return EXIT_FAILURE;
            }

            /* Alice creates the recipient objects and adds the index of the
             * original ordering (the ordering of the `sp_addresses` array) to
             * each object. This index is used to return the generated outputs
             * in the original ordering so that Alice can match up the generated
             * outputs with the correct amounts.
             */
            recipients[i].index = i;
            recipient_ptrs[i] = &recipients[i];
        }
        ret = secp256k1_silentpayments_sender_create_outputs(ctx,
            tx_output_ptrs,
            recipient_ptrs, N_OUTPUTS,
            smallest_outpoint,
            sender_keypair_ptrs, N_INPUTS,
            NULL, 0
        );
        if (!ret) {
            printf("Something went wrong, a recipient provided an invalid address.\n");
            return EXIT_FAILURE;
        }
        printf("Alice created the following outputs for Bob and Carol:\n");
        for (i = 0; i < N_OUTPUTS; i++) {
            printf("    ");
            ret = secp256k1_xonly_pubkey_serialize(ctx,
                serialized_xonly,
                &tx_outputs[i]
            );
            assert(ret);
            print_hex(serialized_xonly, sizeof(serialized_xonly));
        }
        /* It's best practice to try to clear secrets from memory after using
         * them. This is done because some bugs can allow an attacker to leak
         * memory, for example through "out of bounds" array access (see
         * Heartbleed), or the OS swapping them to disk. Hence, we overwrite the
         * secret key buffer with zeros.
         *
         * Here we are preventing these writes from being optimized out, as any
         * good compiler will remove any writes that aren't used. */
        secure_erase(seckey, sizeof(seckey));
        for (i = 0; i < N_INPUTS; i++) {
            secure_erase(&sender_keypairs[i], sizeof(sender_keypairs[i]));
        }
    }

    /*** Receiving ***/
    {
        {
            /*** Scanning as a full node (Bob) ***
             *
             * Since Bob has access to the full transaction, scanning is simple:
             *
             *     1. Collect the relevant prevouts from the transaction and call
             *        `secp256k1_silentpayments_recipient_prevouts_summary_create`
             *     2. Call `secp256k1_silentpayments_recipient_scan_outputs`
             */
            ret = secp256k1_silentpayments_recipient_prevouts_summary_create(ctx,
                &prevouts_summary,
                smallest_outpoint,
                tx_input_ptrs, N_INPUTS,
                NULL, 0
            );
            if (!ret) {
                /* We need to always check that the prevouts data object is valid
                 * before proceeding.
                 */
                printf("This transaction is not valid for Silent Payments, skipping.\n");
                return EXIT_SUCCESS;
            }

            /* Scan the transaction */
            n_found_outputs = 0;
            ret = secp256k1_silentpayments_recipient_scan_outputs(ctx,
                found_output_ptrs, &n_found_outputs,
                (const secp256k1_xonly_pubkey**)tx_output_ptrs, N_OUTPUTS,
                bob_scan_key,
                &prevouts_summary,
                &unlabeled_spend_pubkey,
                label_lookup, &bob_labels_cache /* NULL, NULL for no labels */
            );
            if (!ret) {
                printf("This transaction is not valid for Silent Payments, skipping.\n");
                return EXIT_SUCCESS;
            }
            if (n_found_outputs > 0) {
                secp256k1_keypair kp;
                secp256k1_xonly_pubkey xonly_output;
                unsigned char full_seckey[32];

                printf("\n");
                printf("Bob found the following outputs: \n");
                for (i = 0; i < n_found_outputs; i++) {
                    printf("    ");
                    ret = secp256k1_xonly_pubkey_serialize(ctx,
                        serialized_xonly,
                        &found_outputs[i].output
                    );
                    assert(ret);
                    print_hex(serialized_xonly, sizeof(serialized_xonly));

                    /* Verify that this output is spendable by Bob by reconstructing the full
                     * secret key for the xonly output.
                     *
                     * This is done by adding the tweak from the transaction to Bob's spend key.
                     * If the output was sent to a labeled address, the label tweak has already
                     * been added to the tweak in `secp256k1_silentpayments_found_output`.
                     *
                     * To verify that we are able to sign for this output, it is sufficient to
                     * check that the public key generated from `full_seckey` matches the output
                     * in the transaction. For a full example on signing for a taproot ouput,
                     * see `examples/schnorr.c`.
                     */
                    memcpy(&full_seckey, &bob_spend_key, 32);
                    ret = secp256k1_ec_seckey_tweak_add(ctx, full_seckey, found_outputs[i].tweak);
                    ret &= secp256k1_keypair_create(ctx, &kp, full_seckey);
                    ret &= secp256k1_keypair_xonly_pub(
                        ctx,
                        &xonly_output,
                        NULL,
                        &kp
                    );
                    /* We assert here because the only way the seckey_tweak_add operation can fail
                     * is if the tweak is the negation of Bob's spend key.
                     *
                     * We also assert that the generated public key matches the transaction output,
                     * as it should be impossible for a mismatch at this point considering the
                     * scanning function completed without errors and indicated found outputs.
                     */
                    assert(ret);
                    assert(secp256k1_xonly_pubkey_cmp(ctx, &xonly_output, &found_outputs[i].output) == 0);
                    secure_erase(full_seckey, sizeof(full_seckey));
                }
            } else {
                printf("Bob did not find any outputs in this transaction.\n");
            }
        }
        {
            /*** Scanning as a full node (Carol) ***/
            /* TODO: switch this part to light client scanning once it is supported */

            /* Load Carol's spend public key. */
            ret = secp256k1_ec_pubkey_parse(ctx,
                &unlabeled_spend_pubkey,
                carol_address[1],
                33
            );
            assert(ret);

            n_found_outputs = 0;
            ret = secp256k1_silentpayments_recipient_scan_outputs(ctx,
                found_output_ptrs, &n_found_outputs,
                (const secp256k1_xonly_pubkey**)tx_output_ptrs, N_OUTPUTS,
                carol_scan_key,
                &prevouts_summary,
                &unlabeled_spend_pubkey,
                NULL, NULL /* NULL, NULL for no labels */
            );
            if (!ret) {
                printf("This transaction is not valid for Silent Payments, skipping.\n");
                return EXIT_SUCCESS;
            }
            if (n_found_outputs > 0) {
                /* Carol would spend these outputs the same as Bob, by tweaking her
                 * spend key with the tweak corresponding to the found output. See above
                 * for an example for Bob's outputs. */
                printf("\n");
                printf("Carol found the following outputs: \n");
                for (i = 0; i < n_found_outputs; i++) {
                    printf("    ");
                    ret = secp256k1_xonly_pubkey_serialize(ctx,
                        serialized_xonly,
                        &found_outputs[i].output
                    );
                    assert(ret);
                    print_hex(serialized_xonly, sizeof(serialized_xonly));
                }
            } else {
                printf("Carol did not find any outputs in this transaction.\n");
            }
        }
    }

    /* This will clear everything from the context and free the memory */
    secp256k1_context_destroy(ctx);
    return EXIT_SUCCESS;
}
