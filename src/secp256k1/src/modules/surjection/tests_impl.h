/**********************************************************************
 * Copyright (c) 2016 Andrew Poelstra                                 *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef SECP256K1_MODULE_SURJECTIONPROOF_TESTS
#define SECP256K1_MODULE_SURJECTIONPROOF_TESTS

#include <assert.h>

#include "testrand.h"
#include "group.h"
#include "include/secp256k1_generator.h"
#include "include/secp256k1_rangeproof.h"
#include "include/secp256k1_surjectionproof.h"

static void run_input_selection_tests(size_t n_inputs) {
    unsigned char seed[32];
    size_t i;
    size_t result;
    size_t input_index;
    size_t try_count = n_inputs * 100;
    secp256k1_surjectionproof proof;
    secp256k1_fixed_asset_tag fixed_input_tags[1000];
    const size_t max_n_inputs = sizeof(fixed_input_tags) / sizeof(fixed_input_tags[0]) - 1;

    assert(n_inputs < max_n_inputs);
    secp256k1_rand256(seed);

    for (i = 0; i < n_inputs + 1; i++) {
        secp256k1_rand256(fixed_input_tags[i].data);
    }

    /* cannot match output when told to use zero keys */
    result = secp256k1_surjectionproof_initialize(ctx, &proof, &input_index, fixed_input_tags, n_inputs, 0, &fixed_input_tags[0], try_count, seed);
    assert(result == 0);
    assert(secp256k1_surjectionproof_n_used_inputs(ctx, &proof) == 0);
    assert(secp256k1_surjectionproof_n_total_inputs(ctx, &proof) == n_inputs);
    assert(secp256k1_surjectionproof_serialized_size(ctx, &proof) == 34 + (n_inputs + 7) / 8);
    if (n_inputs > 0) {
        /* succeed in 100*n_inputs tries (probability of failure e^-100) */
        result = secp256k1_surjectionproof_initialize(ctx, &proof, &input_index, fixed_input_tags, n_inputs, 1, &fixed_input_tags[0], try_count, seed);
        assert(result > 0);
        assert(result < n_inputs * 10);
        assert(secp256k1_surjectionproof_n_used_inputs(ctx, &proof) == 1);
        assert(secp256k1_surjectionproof_n_total_inputs(ctx, &proof) == n_inputs);
        assert(secp256k1_surjectionproof_serialized_size(ctx, &proof) == 66 + (n_inputs + 7) / 8);
        assert(input_index == 0);
    }

    if (n_inputs >= 3) {
        /* succeed in 10*n_inputs tries (probability of failure e^-10) */
        result = secp256k1_surjectionproof_initialize(ctx, &proof, &input_index, fixed_input_tags, n_inputs, 3, &fixed_input_tags[1], try_count, seed);
        assert(result > 0);
        assert(secp256k1_surjectionproof_n_used_inputs(ctx, &proof) == 3);
        assert(secp256k1_surjectionproof_n_total_inputs(ctx, &proof) == n_inputs);
        assert(secp256k1_surjectionproof_serialized_size(ctx, &proof) == 130 + (n_inputs + 7) / 8);
        assert(input_index == 1);

        /* fail, key not found */
        result = secp256k1_surjectionproof_initialize(ctx, &proof, &input_index, fixed_input_tags, n_inputs, 3, &fixed_input_tags[n_inputs], try_count, seed);
        assert(result == 0);

        /* succeed on first try when told to use all keys */
        result = secp256k1_surjectionproof_initialize(ctx, &proof, &input_index, fixed_input_tags, n_inputs, n_inputs, &fixed_input_tags[0], try_count, seed);
        assert(result == 1);
        assert(secp256k1_surjectionproof_n_used_inputs(ctx, &proof) == n_inputs);
        assert(secp256k1_surjectionproof_n_total_inputs(ctx, &proof) == n_inputs);
        assert(secp256k1_surjectionproof_serialized_size(ctx, &proof) == 2 + 32 * (n_inputs + 1) + (n_inputs + 7) / 8);
        assert(input_index == 0);

        /* succeed in less than 64 tries when told to use half keys. (probability of failure 2^-64) */
        result = secp256k1_surjectionproof_initialize(ctx, &proof, &input_index, fixed_input_tags, n_inputs, n_inputs / 2, &fixed_input_tags[0], 64, seed);
        assert(result > 0);
        assert(result < 64);
        assert(secp256k1_surjectionproof_n_used_inputs(ctx, &proof) == n_inputs / 2);
        assert(secp256k1_surjectionproof_n_total_inputs(ctx, &proof) == n_inputs);
        assert(secp256k1_surjectionproof_serialized_size(ctx, &proof) == 2 + 32 * (n_inputs / 2 + 1) + (n_inputs + 7) / 8);
        assert(input_index == 0);
    }
}

/** Runs surjectionproof_initilize multiple times and records the number of times each input was used.
 */
static void run_input_selection_distribution_tests_helper(const secp256k1_fixed_asset_tag* fixed_input_tags, const size_t n_input_tags, const size_t n_input_tags_to_use, size_t *used_inputs) {
    secp256k1_surjectionproof proof;
    size_t input_index;
    size_t i;
    size_t j;
    unsigned char seed[32];
    size_t result;
    for (i = 0; i < n_input_tags; i++) {
        used_inputs[i] = 0;
    }
    for(j = 0; j < 10000; j++) {
        secp256k1_rand256(seed);
        result = secp256k1_surjectionproof_initialize(ctx, &proof, &input_index, fixed_input_tags, n_input_tags, n_input_tags_to_use, &fixed_input_tags[0], 64, seed);
        assert(result == 1);

        for (i = 0; i < n_input_tags; i++) {
            if (proof.used_inputs[i / 8] & (1 << (i % 8))) {
                used_inputs[i] += 1;
            }
        }
    }
}

/** Probabilistic test of the distribution of used_inputs after surjectionproof_initialize.
 * Each confidence interval assertion fails incorrectly with a probability of 2^-128.
 */
static void run_input_selection_distribution_tests(void) {
    size_t i;
    size_t n_input_tags_to_use;
    const size_t n_inputs = 4;
    secp256k1_fixed_asset_tag fixed_input_tags[4];
    size_t used_inputs[4];

    for (i = 0; i < n_inputs; i++) {
        secp256k1_rand256(fixed_input_tags[i].data);
    }

    /* If there is one input tag to use, initialize must choose the one equal to fixed_output_tag. */
    n_input_tags_to_use = 1;
    run_input_selection_distribution_tests_helper(fixed_input_tags, n_inputs, n_input_tags_to_use, used_inputs);
    assert(used_inputs[0] == 10000);
    assert(used_inputs[1] == 0);
    assert(used_inputs[2] == 0);
    assert(used_inputs[3] == 0);

    n_input_tags_to_use = 2;
    /* The input equal to the fixed_output_tag must be included in all used_inputs sets.
     * For each fixed_input_tag != fixed_output_tag the probability that it's included
     * in the used_inputs set is P(used_input|not fixed_output_tag) = 1/3.
     */
    run_input_selection_distribution_tests_helper(fixed_input_tags, n_inputs, n_input_tags_to_use, used_inputs);
    assert(used_inputs[0] == 10000);
    assert(used_inputs[1] > 2725 && used_inputs[1] < 3961);
    assert(used_inputs[2] > 2725 && used_inputs[2] < 3961);
    assert(used_inputs[3] > 2725 && used_inputs[3] < 3961);

    n_input_tags_to_use = 3;
    /* P(used_input|not fixed_output_tag) = 2/3 */
    run_input_selection_distribution_tests_helper(fixed_input_tags, n_inputs, n_input_tags_to_use, used_inputs);
    assert(used_inputs[0] == 10000);
    assert(used_inputs[1] > 6039 && used_inputs[1] < 7275);
    assert(used_inputs[2] > 6039 && used_inputs[2] < 7275);
    assert(used_inputs[3] > 6039 && used_inputs[3] < 7275);


    n_input_tags_to_use = 1;
    /* Create second input tag that is equal to the output tag. Therefore, when using only
     * one input we have P(used_input|fixed_output_tag) = 1/2 and P(used_input|not fixed_output_tag) = 0
     */
    memcpy(fixed_input_tags[0].data, fixed_input_tags[1].data, 32);
    run_input_selection_distribution_tests_helper(fixed_input_tags, n_inputs, n_input_tags_to_use, used_inputs);
    assert(used_inputs[0] > 4345 && used_inputs[0] < 5655);
    assert(used_inputs[1] > 4345 && used_inputs[1] < 5655);
    assert(used_inputs[2] == 0);
    assert(used_inputs[3] == 0);

    n_input_tags_to_use = 2;
    /* When choosing 2 inputs in initialization there are 5 possible combinations of
     * input indexes {(0, 1), (1, 2), (0, 3), (1, 3), (0, 2)}. Therefore we have
     * P(used_input|fixed_output_tag) = 3/5 and P(used_input|not fixed_output_tag) = 2/5.
     */
    run_input_selection_distribution_tests_helper(fixed_input_tags, n_inputs, n_input_tags_to_use, used_inputs);
    assert(used_inputs[0] > 5352 && used_inputs[0] < 6637);
    assert(used_inputs[1] > 5352 && used_inputs[1] < 6637);
    assert(used_inputs[2] > 3363 && used_inputs[2] < 4648);
    assert(used_inputs[3] > 3363 && used_inputs[3] < 4648);

    n_input_tags_to_use = 3;
    /* There are 4 combinations, each with all inputs except one. Therefore we have
     * P(used_input|fixed_output_tag) = 3/4 and P(used_input|not fixed_output_tag) = 3/4.
     */
    run_input_selection_distribution_tests_helper(fixed_input_tags, n_inputs, n_input_tags_to_use, used_inputs);
    assert(used_inputs[0] > 6918 && used_inputs[0] < 8053);
    assert(used_inputs[1] > 6918 && used_inputs[1] < 8053);
    assert(used_inputs[2] > 6918 && used_inputs[2] < 8053);
    assert(used_inputs[3] > 6918 && used_inputs[3] < 8053);
}

static void run_gen_verify(size_t n_inputs, size_t n_used) {
    unsigned char seed[32];
    secp256k1_surjectionproof proof;
    unsigned char serialized_proof[SECP256K1_SURJECTIONPROOF_SERIALIZATION_BYTES_MAX];
    size_t serialized_len = SECP256K1_SURJECTIONPROOF_SERIALIZATION_BYTES_MAX;
    secp256k1_fixed_asset_tag fixed_input_tags[1000];
    secp256k1_generator ephemeral_input_tags[1000];
    unsigned char *input_blinding_key[1000];
    const size_t max_n_inputs = sizeof(fixed_input_tags) / sizeof(fixed_input_tags[0]) - 1;
    size_t try_count = n_inputs * 100;
    size_t key_index;
    size_t input_index;
    size_t i;
    int result;

    /* setup */
    assert(n_used <= n_inputs);
    assert(n_inputs < max_n_inputs);
    secp256k1_rand256(seed);

    key_index = (((size_t) seed[0] << 8) + seed[1]) % n_inputs;

    for (i = 0; i < n_inputs + 1; i++) {
        input_blinding_key[i] = malloc(32);
        secp256k1_rand256(input_blinding_key[i]);
        /* choose random fixed tag, except that for the output one copy from the key_index */
        if (i < n_inputs) {
            secp256k1_rand256(fixed_input_tags[i].data);
        } else {
            memcpy(&fixed_input_tags[i], &fixed_input_tags[key_index], sizeof(fixed_input_tags[i]));
        }
        assert(secp256k1_generator_generate_blinded(ctx, &ephemeral_input_tags[i], fixed_input_tags[i].data, input_blinding_key[i]));
    }

    /* test */
    result = secp256k1_surjectionproof_initialize(ctx, &proof, &input_index, fixed_input_tags, n_inputs, n_used, &fixed_input_tags[key_index], try_count, seed);
    if (n_used == 0) {
        assert(result == 0);
        return;
    }
    assert(result > 0);
    assert(input_index == key_index);

    result = secp256k1_surjectionproof_generate(ctx, &proof, ephemeral_input_tags, n_inputs, &ephemeral_input_tags[n_inputs], input_index, input_blinding_key[input_index], input_blinding_key[n_inputs]);
    assert(result == 1);

    assert(secp256k1_surjectionproof_serialize(ctx, serialized_proof, &serialized_len, &proof));
    assert(serialized_len == secp256k1_surjectionproof_serialized_size(ctx, &proof));
    assert(serialized_len == SECP256K1_SURJECTIONPROOF_SERIALIZATION_BYTES(n_inputs, n_used));
    assert(secp256k1_surjectionproof_parse(ctx, &proof, serialized_proof, serialized_len));
    result = secp256k1_surjectionproof_verify(ctx, &proof, ephemeral_input_tags, n_inputs, &ephemeral_input_tags[n_inputs]);
    assert(result == 1);
    /* various fail cases */
    if (n_inputs > 1) {
        result = secp256k1_surjectionproof_verify(ctx, &proof, ephemeral_input_tags, n_inputs, &ephemeral_input_tags[n_inputs - 1]);
        assert(result == 0);

        /* number of entries in ephemeral_input_tags array is less than proof.n_inputs */
        n_inputs -= 1;
        result = secp256k1_surjectionproof_generate(ctx, &proof, ephemeral_input_tags, n_inputs, &ephemeral_input_tags[n_inputs], input_index, input_blinding_key[input_index], input_blinding_key[n_inputs]);
        assert(result == 0);
        result = secp256k1_surjectionproof_verify(ctx, &proof, ephemeral_input_tags, n_inputs, &ephemeral_input_tags[n_inputs - 1]);
        assert(result == 0);
        n_inputs += 1;
    }

    /* cleanup */
    for (i = 0; i < n_inputs + 1; i++) {
        free(input_blinding_key[i]);
    }
}

/* check that a proof with empty n_used_inputs is invalid */
static void run_no_used_inputs_verify(void) {
    secp256k1_surjectionproof proof;
    secp256k1_fixed_asset_tag fixed_input_tag;
    secp256k1_fixed_asset_tag fixed_output_tag;
    secp256k1_generator ephemeral_input_tags[1];
    size_t n_ephemeral_input_tags = 1;
    secp256k1_generator ephemeral_output_tag;
    unsigned char blinding_key[32];
    secp256k1_ge inputs[1];
    secp256k1_ge output;
    secp256k1_sha256_t sha256_e0;
    int result;

    /* Create proof that doesn't use inputs. secp256k1_surjectionproof_initialize
     * will not work here since it insists on selecting an input that matches the output. */
    proof.n_inputs = 1;
    memset(proof.used_inputs, 0, SECP256K1_SURJECTIONPROOF_MAX_N_INPUTS / 8);

    /* create different fixed input and output tags */
    secp256k1_rand256(fixed_input_tag.data);
    secp256k1_rand256(fixed_output_tag.data);

    /* blind fixed output tags with random blinding key */
    secp256k1_rand256(blinding_key);
    assert(secp256k1_generator_generate_blinded(ctx, &ephemeral_input_tags[0], fixed_input_tag.data, blinding_key));
    assert(secp256k1_generator_generate_blinded(ctx, &ephemeral_output_tag, fixed_output_tag.data, blinding_key));

    /* create "borromean signature" which is just a hash of metadata (pubkeys, etc) in this case */
    secp256k1_generator_load(&output, &ephemeral_output_tag);
    secp256k1_generator_load(&inputs[0], &ephemeral_input_tags[0]);
    secp256k1_surjection_genmessage(proof.data, inputs, 1, &output);
    secp256k1_sha256_initialize(&sha256_e0);
    secp256k1_sha256_write(&sha256_e0, proof.data, 32);
    secp256k1_sha256_finalize(&sha256_e0, proof.data);

    result = secp256k1_surjectionproof_verify(ctx, &proof, ephemeral_input_tags, n_ephemeral_input_tags, &ephemeral_output_tag);
    assert(result == 0);
}

void run_bad_serialize(void) {
    secp256k1_surjectionproof proof;
    unsigned char serialized_proof[SECP256K1_SURJECTIONPROOF_SERIALIZATION_BYTES_MAX];
    size_t serialized_len;

    proof.n_inputs = 0;
    serialized_len = 2 + 31;
    /* e0 is one byte too short */
    assert(secp256k1_surjectionproof_serialize(ctx, serialized_proof, &serialized_len, &proof) == 0);
}

void run_bad_parse(void) {
    secp256k1_surjectionproof proof;
    unsigned char serialized_proof0[] = { 0x00 };
    unsigned char serialized_proof1[] = { 0x01, 0x00 };
    unsigned char serialized_proof2[33] = { 0 };

    /* Missing total input count */
    assert(secp256k1_surjectionproof_parse(ctx, &proof, serialized_proof0, sizeof(serialized_proof0)) == 0);
    /* Missing bitmap */
    assert(secp256k1_surjectionproof_parse(ctx, &proof, serialized_proof1, sizeof(serialized_proof1)) == 0);
    /* Missing e0 value */
    assert(secp256k1_surjectionproof_parse(ctx, &proof, serialized_proof2, sizeof(serialized_proof2)) == 0);
}

void run_surjection_tests(void) {
    run_input_selection_tests(0);
    run_input_selection_tests(1);
    run_input_selection_tests(5);
    run_input_selection_tests(100);

    run_input_selection_distribution_tests();
    run_gen_verify(10, 3);
    run_no_used_inputs_verify();
    run_bad_serialize();
    run_bad_parse();
}

#endif
