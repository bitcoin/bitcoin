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

/* Static data for Bob and Carol's silent payment addresses */
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
static unsigned char bob_spend_pubkey[33] = {
    0x02, 0x5c, 0xc9, 0x85, 0x6d, 0x6f, 0x83, 0x75,
    0x35, 0x0e, 0x12, 0x39, 0x78, 0xda, 0xac, 0x20,
    0x0c, 0x26, 0x0c, 0xb5, 0xb5, 0xae, 0x83, 0x10,
    0x6c, 0xab, 0x90, 0x48, 0x4d, 0xcd, 0x8f, 0xcf, 0x36
};
static unsigned char bob_address[2][33] = {
    {
        0x02, 0x15, 0x40, 0xae, 0xa8, 0x97, 0x54, 0x7a,
        0xd4, 0x39, 0xb4, 0xe0, 0xf6, 0x09, 0xe5, 0xf0,
        0xfa, 0x63, 0xde, 0x89, 0xab, 0x11, 0xed, 0xe3,
        0x1e, 0x8c, 0xde, 0x4b, 0xe2, 0x19, 0x42, 0x5f, 0x23
    },
    {
        0x03, 0x0b, 0x9d, 0xd4, 0x9d, 0xf2, 0xc6, 0x85,
        0x23, 0xbb, 0x0c, 0x72, 0xd4, 0xfb, 0x59, 0xb6,
        0x4c, 0xe5, 0xc9, 0xa9, 0x33, 0x6d, 0x0b, 0xef,
        0x94, 0x9e, 0xe0, 0x77, 0x5b, 0xea, 0x61, 0xef, 0x05
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
 *  The structs and call back function are implemented here as a demonstration
 *  of how the label lookup callback is meant to query a label cache and return
 *  the label tweak when a match is found. This is for demonstration purposes
 *  only and not optimized. In a production usecase, it is expected that the
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
    enum { N_INPUTS = 2, N_OUTPUTS = 3 };
    unsigned char randomize[32];
    unsigned char serialized_xonly[32];
    secp256k1_xonly_pubkey tx_inputs[N_INPUTS];
    const secp256k1_xonly_pubkey *tx_input_ptrs[N_INPUTS];
    secp256k1_xonly_pubkey tx_outputs[N_OUTPUTS];
    secp256k1_xonly_pubkey *tx_output_ptrs[N_OUTPUTS];
    int ret;
    size_t i;

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

    /*** Sending ***/
    {
        secp256k1_keypair sender_keypairs[N_INPUTS];
        const secp256k1_keypair *sender_keypair_ptrs[N_INPUTS];
        secp256k1_silentpayments_recipient recipients[N_OUTPUTS];
        const secp256k1_silentpayments_recipient *recipient_ptrs[N_OUTPUTS];
        unsigned char (*sp_addresses[N_OUTPUTS])[2][33];
        unsigned char seckey[32];

        /*** Generate private keys for the sender ***
         *
         * In this example, only taproot inputs are used but the function can be
         * called with a mix of taproot seckeys and plain seckeys. Taproot
         * seckeys are passed as keypairs to allow the sending function to check
         * if the private keys need to be negated without needing to do an
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
         * silent payment address mutltiple times.
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
                printf("\n");
                printf("Something went wrong, this is not a valid silent payments address.");
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
        for (i = 0; i < N_OUTPUTS; i++) {
            tx_output_ptrs[i] = &tx_outputs[i];
        }
        ret = secp256k1_silentpayments_sender_create_outputs(ctx,
            tx_output_ptrs,
            recipient_ptrs, N_OUTPUTS,
            smallest_outpoint,
            sender_keypair_ptrs, N_INPUTS,
            NULL, 0
        );
        assert(ret);
        printf("Alice created the following outputs for Bob and Carol: \n");
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
        unsigned char light_client_data33[33];

        for (i = 0; i < N_INPUTS; i++) {
            tx_input_ptrs[i] = &tx_inputs[i];
        }
        for (i = 0; i < N_OUTPUTS; i++) {
            tx_output_ptrs[i] = &tx_outputs[i];
        }

        {
            /*** Scanning as a full node (Bob) ***
             *
             * Since Bob has access to the full transaction, scanning is simple:
             *
             *     1. Collect the relevant public data from the transaction
             *        and call `_silentpayments_recipient_prevouts_summary_create`
             *     2. Call `_silentpayments_recipient_scan_outputs`
             *
             */
            secp256k1_silentpayments_found_output found_outputs[N_OUTPUTS];
            secp256k1_silentpayments_found_output *found_output_ptrs[N_OUTPUTS];
            secp256k1_silentpayments_recipient_prevouts_summary prevouts_summary;
            secp256k1_pubkey spend_pubkey;
            secp256k1_pubkey labeled_spend_pubkey;
            secp256k1_pubkey address_labeled_spend_pubkey;
            size_t n_found_outputs;
            struct labels_cache labels_cache;

            for (i = 0; i < N_OUTPUTS; i++) {
                found_output_ptrs[i] = &found_outputs[i];
            }
            {
                /** Labels setup
                 *
                 *  These steps are only necessary if the recipient is using the
                 *  optional labels feature. Recipients not using labels can
                 *  ignore these steps and simply pass `NULL` for the
                 *  label_lookup and label_context arguments:
                 *
                 *      _silentpayments_recipient_scan_outputs(..., NULL, NULL);
                 *
                 *  In this case, since Bob has access to the full transaction
                 *  outputs when scanning, it's easy for him to scan with labels,
                 *  as demonstrated below. For efficient scanning, Bob keeps a
                 *  cache of every label he has previously used and uses a
                 *  callback to check if a potential label exists in his cache.
                 *  Since the labels are created using an incremental integer
                 *  `m`, if Bob ever forgets how many labels he has previously
                 *  used, he can pregenerate a large number of labels e.g.,
                 *  0..100_000.
                 */
                size_t len = 33;
                secp256k1_pubkey label;
                unsigned int m = 1;

                /* Load Bob's spend public key */
                ret = secp256k1_ec_pubkey_parse(ctx,
                    &spend_pubkey,
                    bob_spend_pubkey,
                    33
                );
                assert(ret);

                /* Add an entry to the cache. This implies Bob has previously
                 * called
                 * `secp256k1_silentpayments_recipient_create_labeled_spend_pubkey`
                 * and is using the resulting labeled spend pubkey to encode a
                 * labeled silent payments address.
                 */
                ret = secp256k1_silentpayments_recipient_create_label(ctx,
                    &label,
                    labels_cache.entries[0].label_tweak,
                    bob_scan_key,
                    m
                );
                assert(ret);
                ret = secp256k1_ec_pubkey_serialize(ctx,
                    labels_cache.entries[0].label,
                    &len,
                    &label,
                    SECP256K1_EC_COMPRESSED
                );
                assert(ret);
                labels_cache.entries_used = 1;

                /* Verify the label we just created and added to the cache is the
                 * same one used in Bob's silent payment address.
                 */
                ret = secp256k1_silentpayments_recipient_create_labeled_spend_pubkey(ctx,
                    &labeled_spend_pubkey,
                    &spend_pubkey,
                    &label
                );
                assert(ret);
                /* Load the labeled spend public key from Bob's address */
                ret = secp256k1_ec_pubkey_parse(ctx,
                    &address_labeled_spend_pubkey,
                    bob_address[1],
                    33
                );
                assert(ret);
                if (secp256k1_ec_pubkey_cmp(ctx, &labeled_spend_pubkey, &address_labeled_spend_pubkey) != 0) {
                    printf("\n");
                    printf("Something went wrong, the labeled spend public key does not match Bob's address.");
                };
            }

            /* Bob collects the public data from the transaction inputs and
             * creates a `secp256k1_silentpayments_recipient_prevouts_summary` object. He uses
             * this for his own scanning and also serializes the `prevouts_summary`
             * object to send to light clients. We will use this later for
             * Carol, who is scanning as a light client. Note, anyone can create
             * and provide these `prevouts_summary` objects, i.e. you don't need to be
             * a silent payments wallet, just someone interested in providing this
             * data to light clients, e.g. a wallet service provider. In our
             * example, Bob is scanning for himself but also sharing this data
             * with light clients.
             */
            ret = secp256k1_silentpayments_recipient_prevouts_summary_create(ctx,
                &prevouts_summary,
                smallest_outpoint,
                tx_input_ptrs, N_INPUTS,
                NULL, 0 /* NULL because no eligible plain pubkey inputs were found in the tx */
            );
            if (!ret) {
                /* We need to always check that the public data object is valid
                 * before proceeding, since a malicious actor could create a transaction
                 * such that the input public keys sum to the point at infinity, which
                 * could cause our node to crash if, e.g., we assume that prevouts_summary_create
                 * will always succeed."
                 */
                printf("\n");
                printf("This transaction is not valid for silent payments, skipping.");
                return EXIT_SUCCESS;
            }
            /* Serialize the public data object for later use. */
            ret = secp256k1_silentpayments_recipient_prevouts_summary_serialize(ctx,
                light_client_data33,
                &prevouts_summary
            );
            assert(ret);

            /* Scan the transaction */
            n_found_outputs = 0;
            ret = secp256k1_silentpayments_recipient_scan_outputs(ctx,
                found_output_ptrs, &n_found_outputs,
                (const secp256k1_xonly_pubkey * const *)tx_output_ptrs, N_OUTPUTS,
                bob_scan_key,
                &prevouts_summary,
                &spend_pubkey,
                label_lookup, &labels_cache /* NULL, NULL for no labels */
            );
            if (!ret) {
                /* Since we've already validated the public data, this shouldn't fail, but
                 * better to be careful here since we are scanning data that could have been
                 * maliciously created.
                 */
                printf("\n");
                printf("Something went wrong while scanning this transaction, skipping.");
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
                     * If the output was sent to a labeled address, the label tweak has
                     * already been added to the tweak returned in `_silentpayments_found_output`.
                     *
                     * To verify that we are able to sign for this output, it is sufficient to
                     * check that the public key generated from `full_seckey` matches the output
                     * in the transaction. For a full example on signing for a schnorr ouput,
                     * see `examples/schnorr.c` */
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
                     * scanning function completed without errors and indicated found outputs. */
                    assert(ret);
                    assert(secp256k1_xonly_pubkey_cmp(ctx, &xonly_output, &found_outputs[i].output) == 0);
                }
            } else {
                printf("\n");
                printf("Bob did not find any outputs in this transaction.");
            }
        }
        {
            /*** Scanning as a light client (Carol) ***
             *
             * Being a light client, Carol likely does not have access to the
             * transaction outputs. This means she will need to first generate
             * an output, check if it exists in the UTXO set (e.g. BIP158 or
             * some other means of querying) and only proceed to check the next
             * output (by incrementing `k`) if the first output exists. It's
             * also difficult for Carol to efficiently scan for labels without
             * the transaction outputs, but Carol can still use labels as a
             * light client by pregenerating all possible labels and adding them
             * to the generated output (i.e., `k = 0`). Once at least one output
             * is found, she can request the full block and scan the full
             * transaction. This assumes Carol will only use a small number of
             * of labels as a light client.
             *
             * Additionally, Carol likely does not have access to the
             * transaction inputs and prevout information, so she uses the
             * `prevouts_summary` object created by Bob's full node earlier. This
             * serialized `prevouts_summary` object contains everything she needs for
             * generating the shared secret, i.e., `input_hash * A_sum`.
             *
             * In practice, Carol wouldn't know the number of outputs ahead of
             * time but we are cheating here to keep the example simple.
             *
             */
            unsigned char ser_found_outputs[2][32];
            unsigned char shared_secret[33];
            secp256k1_pubkey spend_pubkey;
            secp256k1_silentpayments_recipient_prevouts_summary prevouts_summary;
            size_t n_found_outputs;

            /* Load Carol's spend public key */
            ret = secp256k1_ec_pubkey_parse(ctx,
                &spend_pubkey,
                carol_address[1],
                33
            );
            assert(ret);

            /* Scan one output at a time, using the serialized `prevouts_summary`
             * created by Bob's full node
             */
            ret = secp256k1_silentpayments_recipient_prevouts_summary_parse(ctx,
                &prevouts_summary,
                light_client_data33
            );
            if (!ret) {
                printf("\n");
                printf("This transaction is not valid for silent payments, skipping.");
                return EXIT_SUCCESS;
            }
            ret = secp256k1_silentpayments_recipient_create_shared_secret(ctx,
                shared_secret,
                carol_scan_key,
                &prevouts_summary
            );
            /* Since we've already validated the public data, the only reason this could fail
             * is if we input a bad scan key or bad spend public key, which should never happen
             * because this is data under our control.
             */
            assert(ret);
            n_found_outputs = 0;
            {
                int found = 0;
                size_t k = 0;
                secp256k1_xonly_pubkey potential_output;

                while (1) {
                    ret = secp256k1_silentpayments_recipient_create_output_pubkey(ctx,
                        &potential_output,
                        shared_secret,
                        &spend_pubkey,
                        k
                    );
                    assert(ret);
                    /* At this point, we check that the utxo exists with a light
                     * client protocol. For this example, we'll just iterate
                     * through the list of transaction outputs
                     */
                    found = 0;
                    for (i = 0; i < N_OUTPUTS; i++) {
                        if (secp256k1_xonly_pubkey_cmp(ctx, &potential_output, &tx_outputs[i]) == 0) {
                            ret = secp256k1_xonly_pubkey_serialize(ctx,
                                ser_found_outputs[n_found_outputs],
                                &potential_output
                            );
                            assert(ret);
                            /* If found, create a new output with k++ and check
                             * again */
                            found = 1;
                            n_found_outputs++;
                            k++;
                            break;
                        }
                    }
                    /* If we generate an output and it does not exist in the
                     * UTXO set, we are done scanning this transaction */
                    if (!found) {
                        break;
                    }
                }
            }

            if (n_found_outputs > 0) {
                /* Carol would spend these outputs the same as Bob, by tweaking her
                 * spend key with the tweak corresponding to the found output. See above
                 * for an example for Bob's outputs. */
                printf("\n");
                printf("Carol found the following outputs: \n");
                for (i = 0; i < n_found_outputs; i++) {
                    printf("    ");
                    print_hex(ser_found_outputs[i], 32);
                }
            } else {
                printf("\n");
                printf("Carol did not find any outputs in this transaction.");
            }
        }
    }

    /* This will clear everything from the context and free the memory */
    secp256k1_context_destroy(ctx);
    return EXIT_SUCCESS;
}
