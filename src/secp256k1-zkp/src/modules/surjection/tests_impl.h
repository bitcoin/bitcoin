/**********************************************************************
 * Copyright (c) 2016 Andrew Poelstra                                 *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef SECP256K1_MODULE_SURJECTIONPROOF_TESTS
#define SECP256K1_MODULE_SURJECTIONPROOF_TESTS

#include "testrand.h"
#include "group.h"
#include "include/secp256k1_generator.h"
#include "include/secp256k1_rangeproof.h"
#include "include/secp256k1_surjectionproof.h"

static void test_surjectionproof_api(void) {
    unsigned char seed[32];
    secp256k1_context *none = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
    secp256k1_context *sign = secp256k1_context_create(SECP256K1_CONTEXT_SIGN);
    secp256k1_context *vrfy = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY);
    secp256k1_context *both = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
    secp256k1_fixed_asset_tag fixed_input_tags[10];
    secp256k1_fixed_asset_tag fixed_output_tag;
    secp256k1_generator ephemeral_input_tags[10];
    secp256k1_generator ephemeral_output_tag;
    unsigned char input_blinding_key[10][32];
    unsigned char output_blinding_key[32];
    unsigned char serialized_proof[SECP256K1_SURJECTIONPROOF_SERIALIZATION_BYTES_MAX];
    size_t  serialized_len;
    secp256k1_surjectionproof proof;
    size_t n_inputs = sizeof(fixed_input_tags) / sizeof(fixed_input_tags[0]);
    size_t input_index;
    int32_t ecount = 0;
    size_t i;

    secp256k1_rand256(seed);
    secp256k1_context_set_error_callback(none, counting_illegal_callback_fn, &ecount);
    secp256k1_context_set_error_callback(sign, counting_illegal_callback_fn, &ecount);
    secp256k1_context_set_error_callback(vrfy, counting_illegal_callback_fn, &ecount);
    secp256k1_context_set_error_callback(both, counting_illegal_callback_fn, &ecount);
    secp256k1_context_set_illegal_callback(none, counting_illegal_callback_fn, &ecount);
    secp256k1_context_set_illegal_callback(sign, counting_illegal_callback_fn, &ecount);
    secp256k1_context_set_illegal_callback(vrfy, counting_illegal_callback_fn, &ecount);
    secp256k1_context_set_illegal_callback(both, counting_illegal_callback_fn, &ecount);

    for (i = 0; i < n_inputs; i++) {
        secp256k1_rand256(input_blinding_key[i]);
        secp256k1_rand256(fixed_input_tags[i].data);
        CHECK(secp256k1_generator_generate_blinded(ctx, &ephemeral_input_tags[i], fixed_input_tags[i].data, input_blinding_key[i]));
    }
    secp256k1_rand256(output_blinding_key);
    memcpy(&fixed_output_tag, &fixed_input_tags[0], sizeof(fixed_input_tags[0]));
    CHECK(secp256k1_generator_generate_blinded(ctx, &ephemeral_output_tag, fixed_output_tag.data, output_blinding_key));

    /* check initialize */
    CHECK(secp256k1_surjectionproof_initialize(none, &proof, &input_index, fixed_input_tags, n_inputs, 0, &fixed_input_tags[0], 100, seed) == 0);
    CHECK(ecount == 0);
    CHECK(secp256k1_surjectionproof_initialize(none, &proof, &input_index, fixed_input_tags, n_inputs, 3, &fixed_input_tags[0], 100, seed) != 0);
    CHECK(ecount == 0);
    CHECK(secp256k1_surjectionproof_initialize(none, NULL, &input_index, fixed_input_tags, n_inputs, 3, &fixed_input_tags[0], 100, seed) == 0);
    CHECK(ecount == 1);
    CHECK(secp256k1_surjectionproof_initialize(none, &proof, NULL, fixed_input_tags, n_inputs, 3, &fixed_input_tags[0], 100, seed) == 0);
    CHECK(ecount == 2);
    CHECK(secp256k1_surjectionproof_initialize(none, &proof, &input_index, NULL, n_inputs, 3, &fixed_input_tags[0], 100, seed) == 0);
    CHECK(ecount == 3);
    CHECK(secp256k1_surjectionproof_initialize(none, &proof, &input_index, fixed_input_tags, SECP256K1_SURJECTIONPROOF_MAX_N_INPUTS + 1, 3, &fixed_input_tags[0], 100, seed) == 0);
    CHECK(ecount == 4);
    CHECK(secp256k1_surjectionproof_initialize(none, &proof, &input_index, fixed_input_tags, n_inputs, n_inputs, &fixed_input_tags[0], 100, seed) != 0);
    CHECK(ecount == 4);
    CHECK(secp256k1_surjectionproof_initialize(none, &proof, &input_index, fixed_input_tags, n_inputs, n_inputs + 1, &fixed_input_tags[0], 100, seed) == 0);
    CHECK(ecount == 5);
    CHECK(secp256k1_surjectionproof_initialize(none, &proof, &input_index, fixed_input_tags, n_inputs, 3, NULL, 100, seed) == 0);
    CHECK(ecount == 6);
    CHECK((secp256k1_surjectionproof_initialize(none, &proof, &input_index, fixed_input_tags, n_inputs, 0, &fixed_input_tags[0], 0, seed) & 1) == 0);
    CHECK(ecount == 6);
    CHECK(secp256k1_surjectionproof_initialize(none, &proof, &input_index, fixed_input_tags, n_inputs, 0, &fixed_input_tags[0], 100, NULL) == 0);
    CHECK(ecount == 7);

    CHECK(secp256k1_surjectionproof_initialize(none, &proof, &input_index, fixed_input_tags, n_inputs, 3, &fixed_input_tags[0], 100, seed) != 0);
    /* check generate */
    CHECK(secp256k1_surjectionproof_generate(none, &proof, ephemeral_input_tags, n_inputs, &ephemeral_output_tag, 0, input_blinding_key[0], output_blinding_key) == 0);
    CHECK(ecount == 8);
    CHECK(secp256k1_surjectionproof_generate(vrfy, &proof, ephemeral_input_tags, n_inputs, &ephemeral_output_tag, 0, input_blinding_key[0], output_blinding_key) == 0);
    CHECK(ecount == 9);

    CHECK(secp256k1_surjectionproof_generate(sign, &proof, ephemeral_input_tags, n_inputs, &ephemeral_output_tag, 0, input_blinding_key[0], output_blinding_key) == 0);
    CHECK(ecount == 10);
    CHECK(secp256k1_surjectionproof_generate(both, &proof, ephemeral_input_tags, n_inputs, &ephemeral_output_tag, 0, input_blinding_key[0], output_blinding_key) != 0);
    CHECK(ecount == 10);

    CHECK(secp256k1_surjectionproof_generate(both, NULL, ephemeral_input_tags, n_inputs, &ephemeral_output_tag, 0, input_blinding_key[0], output_blinding_key) == 0);
    CHECK(ecount == 11);
    CHECK(secp256k1_surjectionproof_generate(both, &proof, NULL, n_inputs, &ephemeral_output_tag, 0, input_blinding_key[0], output_blinding_key) == 0);
    CHECK(ecount == 12);
    CHECK(secp256k1_surjectionproof_generate(both, &proof, ephemeral_input_tags, n_inputs + 1, &ephemeral_output_tag, 0, input_blinding_key[0], output_blinding_key) == 0);
    CHECK(ecount == 12);
    CHECK(secp256k1_surjectionproof_generate(both, &proof, ephemeral_input_tags, n_inputs - 1, &ephemeral_output_tag, 0, input_blinding_key[0], output_blinding_key) == 0);
    CHECK(ecount == 12);
    CHECK(secp256k1_surjectionproof_generate(both, &proof, ephemeral_input_tags, 0, &ephemeral_output_tag, 0, input_blinding_key[0], output_blinding_key) == 0);
    CHECK(ecount == 12);
    CHECK(secp256k1_surjectionproof_generate(both, &proof, ephemeral_input_tags, n_inputs, NULL, 0, input_blinding_key[0], output_blinding_key) == 0);
    CHECK(ecount == 13);
    CHECK(secp256k1_surjectionproof_generate(both, &proof, ephemeral_input_tags, n_inputs, &ephemeral_output_tag, 1, input_blinding_key[0], output_blinding_key) != 0);
    CHECK(ecount == 13);  /* the above line "succeeds" but generates an invalid proof as the input_index is wrong. it is fairly expensive to detect this. should we? */
    CHECK(secp256k1_surjectionproof_generate(both, &proof, ephemeral_input_tags, n_inputs, &ephemeral_output_tag, n_inputs + 1, input_blinding_key[0], output_blinding_key) != 0);
    CHECK(ecount == 13);
    CHECK(secp256k1_surjectionproof_generate(both, &proof, ephemeral_input_tags, n_inputs, &ephemeral_output_tag, 0, NULL, output_blinding_key) == 0);
    CHECK(ecount == 14);
    CHECK(secp256k1_surjectionproof_generate(both, &proof, ephemeral_input_tags, n_inputs, &ephemeral_output_tag, 0, input_blinding_key[0], NULL) == 0);
    CHECK(ecount == 15);

    CHECK(secp256k1_surjectionproof_generate(both, &proof, ephemeral_input_tags, n_inputs, &ephemeral_output_tag, 0, input_blinding_key[0], output_blinding_key) != 0);
    /* check verify */
    CHECK(secp256k1_surjectionproof_verify(none, &proof, ephemeral_input_tags, n_inputs, &ephemeral_output_tag) == 0);
    CHECK(ecount == 16);
    CHECK(secp256k1_surjectionproof_verify(sign, &proof, ephemeral_input_tags, n_inputs, &ephemeral_output_tag) == 0);
    CHECK(ecount == 17);
    CHECK(secp256k1_surjectionproof_verify(vrfy, &proof, ephemeral_input_tags, n_inputs, &ephemeral_output_tag) != 0);
    CHECK(ecount == 17);

    CHECK(secp256k1_surjectionproof_verify(vrfy, NULL, ephemeral_input_tags, n_inputs, &ephemeral_output_tag) == 0);
    CHECK(ecount == 18);
    CHECK(secp256k1_surjectionproof_verify(vrfy, &proof, NULL, n_inputs, &ephemeral_output_tag) == 0);
    CHECK(ecount == 19);
    CHECK(secp256k1_surjectionproof_verify(vrfy, &proof, ephemeral_input_tags, n_inputs - 1, &ephemeral_output_tag) == 0);
    CHECK(ecount == 19);
    CHECK(secp256k1_surjectionproof_verify(vrfy, &proof, ephemeral_input_tags, n_inputs + 1, &ephemeral_output_tag) == 0);
    CHECK(ecount == 19);
    CHECK(secp256k1_surjectionproof_verify(vrfy, &proof, ephemeral_input_tags, n_inputs, NULL) == 0);
    CHECK(ecount == 20);

    /* Check serialize */
    serialized_len = sizeof(serialized_proof);
    CHECK(secp256k1_surjectionproof_serialize(none, serialized_proof, &serialized_len, &proof) != 0);
    CHECK(ecount == 20);
    serialized_len = sizeof(serialized_proof);
    CHECK(secp256k1_surjectionproof_serialize(none, NULL, &serialized_len, &proof) == 0);
    CHECK(ecount == 21);
    serialized_len = sizeof(serialized_proof);
    CHECK(secp256k1_surjectionproof_serialize(none, serialized_proof, NULL, &proof) == 0);
    CHECK(ecount == 22);
    serialized_len = sizeof(serialized_proof);
    CHECK(secp256k1_surjectionproof_serialize(none, serialized_proof, &serialized_len, NULL) == 0);
    CHECK(ecount == 23);

    serialized_len = sizeof(serialized_proof);
    CHECK(secp256k1_surjectionproof_serialize(none, serialized_proof, &serialized_len, &proof) != 0);
    /* Check parse */
    CHECK(secp256k1_surjectionproof_parse(none, &proof, serialized_proof, serialized_len) != 0);
    CHECK(ecount == 23);
    CHECK(secp256k1_surjectionproof_parse(none, NULL, serialized_proof, serialized_len) == 0);
    CHECK(ecount == 24);
    CHECK(secp256k1_surjectionproof_parse(none, &proof, NULL, serialized_len) == 0);
    CHECK(ecount == 25);
    CHECK(secp256k1_surjectionproof_parse(none, &proof, serialized_proof, 0) == 0);
    CHECK(ecount == 25);

    secp256k1_context_destroy(none);
    secp256k1_context_destroy(sign);
    secp256k1_context_destroy(vrfy);
    secp256k1_context_destroy(both);
}

static void test_input_selection(size_t n_inputs) {
    unsigned char seed[32];
    size_t i;
    size_t result;
    size_t input_index;
    size_t try_count = n_inputs * 100;
    secp256k1_surjectionproof proof;
    secp256k1_fixed_asset_tag fixed_input_tags[1000];
    const size_t max_n_inputs = sizeof(fixed_input_tags) / sizeof(fixed_input_tags[0]) - 1;

    CHECK(n_inputs < max_n_inputs);
    secp256k1_rand256(seed);

    for (i = 0; i < n_inputs + 1; i++) {
        secp256k1_rand256(fixed_input_tags[i].data);
    }

    /* cannot match output when told to use zero keys */
    result = secp256k1_surjectionproof_initialize(ctx, &proof, &input_index, fixed_input_tags, n_inputs, 0, &fixed_input_tags[0], try_count, seed);
    CHECK(result == 0);
    CHECK(secp256k1_surjectionproof_n_used_inputs(ctx, &proof) == 0);
    CHECK(secp256k1_surjectionproof_n_total_inputs(ctx, &proof) == n_inputs);
    CHECK(secp256k1_surjectionproof_serialized_size(ctx, &proof) == 34 + (n_inputs + 7) / 8);
    if (n_inputs > 0) {
        /* succeed in 100*n_inputs tries (probability of failure e^-100) */
        result = secp256k1_surjectionproof_initialize(ctx, &proof, &input_index, fixed_input_tags, n_inputs, 1, &fixed_input_tags[0], try_count, seed);
        CHECK(result > 0);
        CHECK(result < n_inputs * 10);
        CHECK(secp256k1_surjectionproof_n_used_inputs(ctx, &proof) == 1);
        CHECK(secp256k1_surjectionproof_n_total_inputs(ctx, &proof) == n_inputs);
        CHECK(secp256k1_surjectionproof_serialized_size(ctx, &proof) == 66 + (n_inputs + 7) / 8);
        CHECK(input_index == 0);
    }

    if (n_inputs >= 3) {
        /* succeed in 10*n_inputs tries (probability of failure e^-10) */
        result = secp256k1_surjectionproof_initialize(ctx, &proof, &input_index, fixed_input_tags, n_inputs, 3, &fixed_input_tags[1], try_count, seed);
        CHECK(result > 0);
        CHECK(secp256k1_surjectionproof_n_used_inputs(ctx, &proof) == 3);
        CHECK(secp256k1_surjectionproof_n_total_inputs(ctx, &proof) == n_inputs);
        CHECK(secp256k1_surjectionproof_serialized_size(ctx, &proof) == 130 + (n_inputs + 7) / 8);
        CHECK(input_index == 1);

        /* fail, key not found */
        result = secp256k1_surjectionproof_initialize(ctx, &proof, &input_index, fixed_input_tags, n_inputs, 3, &fixed_input_tags[n_inputs], try_count, seed);
        CHECK(result == 0);

        /* succeed on first try when told to use all keys */
        result = secp256k1_surjectionproof_initialize(ctx, &proof, &input_index, fixed_input_tags, n_inputs, n_inputs, &fixed_input_tags[0], try_count, seed);
        CHECK(result == 1);
        CHECK(secp256k1_surjectionproof_n_used_inputs(ctx, &proof) == n_inputs);
        CHECK(secp256k1_surjectionproof_n_total_inputs(ctx, &proof) == n_inputs);
        CHECK(secp256k1_surjectionproof_serialized_size(ctx, &proof) == 2 + 32 * (n_inputs + 1) + (n_inputs + 7) / 8);
        CHECK(input_index == 0);

        /* succeed in less than 64 tries when told to use half keys. (probability of failure 2^-64) */
        result = secp256k1_surjectionproof_initialize(ctx, &proof, &input_index, fixed_input_tags, n_inputs, n_inputs / 2, &fixed_input_tags[0], 64, seed);
        CHECK(result > 0);
        CHECK(result < 64);
        CHECK(secp256k1_surjectionproof_n_used_inputs(ctx, &proof) == n_inputs / 2);
        CHECK(secp256k1_surjectionproof_n_total_inputs(ctx, &proof) == n_inputs);
        CHECK(secp256k1_surjectionproof_serialized_size(ctx, &proof) == 2 + 32 * (n_inputs / 2 + 1) + (n_inputs + 7) / 8);
        CHECK(input_index == 0);
    }
}

/** Runs surjectionproof_initilize multiple times and records the number of times each input was used.
 */
static void test_input_selection_distribution_helper(const secp256k1_fixed_asset_tag* fixed_input_tags, const size_t n_input_tags, const size_t n_input_tags_to_use, size_t *used_inputs) {
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
        CHECK(result > 0);

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
static void test_input_selection_distribution(void) {
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
    test_input_selection_distribution_helper(fixed_input_tags, n_inputs, n_input_tags_to_use, used_inputs);
    CHECK(used_inputs[0] == 10000);
    CHECK(used_inputs[1] == 0);
    CHECK(used_inputs[2] == 0);
    CHECK(used_inputs[3] == 0);

    n_input_tags_to_use = 2;
    /* The input equal to the fixed_output_tag must be included in all used_inputs sets.
     * For each fixed_input_tag != fixed_output_tag the probability that it's included
     * in the used_inputs set is P(used_input|not fixed_output_tag) = 1/3.
     */
    test_input_selection_distribution_helper(fixed_input_tags, n_inputs, n_input_tags_to_use, used_inputs);
    CHECK(used_inputs[0] == 10000);
    CHECK(used_inputs[1] > 2725 && used_inputs[1] < 3961);
    CHECK(used_inputs[2] > 2725 && used_inputs[2] < 3961);
    CHECK(used_inputs[3] > 2725 && used_inputs[3] < 3961);

    n_input_tags_to_use = 3;
    /* P(used_input|not fixed_output_tag) = 2/3 */
    test_input_selection_distribution_helper(fixed_input_tags, n_inputs, n_input_tags_to_use, used_inputs);
    CHECK(used_inputs[0] == 10000);
    CHECK(used_inputs[1] > 6039 && used_inputs[1] < 7275);
    CHECK(used_inputs[2] > 6039 && used_inputs[2] < 7275);
    CHECK(used_inputs[3] > 6039 && used_inputs[3] < 7275);


    n_input_tags_to_use = 1;
    /* Create second input tag that is equal to the output tag. Therefore, when using only
     * one input we have P(used_input|fixed_output_tag) = 1/2 and P(used_input|not fixed_output_tag) = 0
     */
    memcpy(fixed_input_tags[0].data, fixed_input_tags[1].data, 32);
    test_input_selection_distribution_helper(fixed_input_tags, n_inputs, n_input_tags_to_use, used_inputs);
    CHECK(used_inputs[0] > 4345 && used_inputs[0] < 5655);
    CHECK(used_inputs[1] > 4345 && used_inputs[1] < 5655);
    CHECK(used_inputs[2] == 0);
    CHECK(used_inputs[3] == 0);

    n_input_tags_to_use = 2;
    /* When choosing 2 inputs in initialization there are 5 possible combinations of
     * input indexes {(0, 1), (1, 2), (0, 3), (1, 3), (0, 2)}. Therefore we have
     * P(used_input|fixed_output_tag) = 3/5 and P(used_input|not fixed_output_tag) = 2/5.
     */
    test_input_selection_distribution_helper(fixed_input_tags, n_inputs, n_input_tags_to_use, used_inputs);
    CHECK(used_inputs[0] > 5352 && used_inputs[0] < 6637);
    CHECK(used_inputs[1] > 5352 && used_inputs[1] < 6637);
    CHECK(used_inputs[2] > 3363 && used_inputs[2] < 4648);
    CHECK(used_inputs[3] > 3363 && used_inputs[3] < 4648);

    n_input_tags_to_use = 3;
    /* There are 4 combinations, each with all inputs except one. Therefore we have
     * P(used_input|fixed_output_tag) = 3/4 and P(used_input|not fixed_output_tag) = 3/4.
     */
    test_input_selection_distribution_helper(fixed_input_tags, n_inputs, n_input_tags_to_use, used_inputs);
    CHECK(used_inputs[0] > 6918 && used_inputs[0] < 8053);
    CHECK(used_inputs[1] > 6918 && used_inputs[1] < 8053);
    CHECK(used_inputs[2] > 6918 && used_inputs[2] < 8053);
    CHECK(used_inputs[3] > 6918 && used_inputs[3] < 8053);
}

static void test_gen_verify(size_t n_inputs, size_t n_used) {
    unsigned char seed[32];
    secp256k1_surjectionproof proof;
    unsigned char serialized_proof[SECP256K1_SURJECTIONPROOF_SERIALIZATION_BYTES_MAX];
    unsigned char serialized_proof_trailing[SECP256K1_SURJECTIONPROOF_SERIALIZATION_BYTES_MAX + 1];
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
    CHECK(n_used <= n_inputs);
    CHECK(n_inputs < max_n_inputs);
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
        CHECK(secp256k1_generator_generate_blinded(ctx, &ephemeral_input_tags[i], fixed_input_tags[i].data, input_blinding_key[i]));
    }

    /* test */
    result = secp256k1_surjectionproof_initialize(ctx, &proof, &input_index, fixed_input_tags, n_inputs, n_used, &fixed_input_tags[key_index], try_count, seed);
    if (n_used == 0) {
        CHECK(result == 0);
        return;
    }
    CHECK(result > 0);
    CHECK(input_index == key_index);

    result = secp256k1_surjectionproof_generate(ctx, &proof, ephemeral_input_tags, n_inputs, &ephemeral_input_tags[n_inputs], input_index, input_blinding_key[input_index], input_blinding_key[n_inputs]);
    CHECK(result == 1);

    CHECK(secp256k1_surjectionproof_serialize(ctx, serialized_proof, &serialized_len, &proof));
    CHECK(serialized_len == secp256k1_surjectionproof_serialized_size(ctx, &proof));
    CHECK(serialized_len == SECP256K1_SURJECTIONPROOF_SERIALIZATION_BYTES(n_inputs, n_used));

    /* trailing garbage */
    memcpy(&serialized_proof_trailing, &serialized_proof, serialized_len);
    serialized_proof_trailing[serialized_len] = seed[0];
    CHECK(secp256k1_surjectionproof_parse(ctx, &proof, serialized_proof, serialized_len + 1) == 0);

    CHECK(secp256k1_surjectionproof_parse(ctx, &proof, serialized_proof, serialized_len));
    result = secp256k1_surjectionproof_verify(ctx, &proof, ephemeral_input_tags, n_inputs, &ephemeral_input_tags[n_inputs]);
    CHECK(result == 1);
    /* various fail cases */
    if (n_inputs > 1) {
        result = secp256k1_surjectionproof_verify(ctx, &proof, ephemeral_input_tags, n_inputs, &ephemeral_input_tags[n_inputs - 1]);
        CHECK(result == 0);

        /* number of entries in ephemeral_input_tags array is less than proof.n_inputs */
        n_inputs -= 1;
        result = secp256k1_surjectionproof_generate(ctx, &proof, ephemeral_input_tags, n_inputs, &ephemeral_input_tags[n_inputs], input_index, input_blinding_key[input_index], input_blinding_key[n_inputs]);
        CHECK(result == 0);
        result = secp256k1_surjectionproof_verify(ctx, &proof, ephemeral_input_tags, n_inputs, &ephemeral_input_tags[n_inputs - 1]);
        CHECK(result == 0);
        n_inputs += 1;
    }

    /* cleanup */
    for (i = 0; i < n_inputs + 1; i++) {
        free(input_blinding_key[i]);
    }
}

/* check that a proof with empty n_used_inputs is invalid */
static void test_no_used_inputs_verify(void) {
    secp256k1_surjectionproof proof;
    secp256k1_fixed_asset_tag fixed_input_tag;
    secp256k1_fixed_asset_tag fixed_output_tag;
    secp256k1_generator ephemeral_input_tags[1];
    size_t n_ephemeral_input_tags = 1;
    secp256k1_generator ephemeral_output_tag;
    unsigned char blinding_key[32];
    secp256k1_ge inputs[1];
    secp256k1_ge output;
    secp256k1_sha256 sha256_e0;
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
    CHECK(secp256k1_generator_generate_blinded(ctx, &ephemeral_input_tags[0], fixed_input_tag.data, blinding_key));
    CHECK(secp256k1_generator_generate_blinded(ctx, &ephemeral_output_tag, fixed_output_tag.data, blinding_key));

    /* create "borromean signature" which is just a hash of metadata (pubkeys, etc) in this case */
    secp256k1_generator_load(&output, &ephemeral_output_tag);
    secp256k1_generator_load(&inputs[0], &ephemeral_input_tags[0]);
    secp256k1_surjection_genmessage(proof.data, inputs, 1, &output);
    secp256k1_sha256_initialize(&sha256_e0);
    secp256k1_sha256_write(&sha256_e0, proof.data, 32);
    secp256k1_sha256_finalize(&sha256_e0, proof.data);

    result = secp256k1_surjectionproof_verify(ctx, &proof, ephemeral_input_tags, n_ephemeral_input_tags, &ephemeral_output_tag);
    CHECK(result == 0);
}

void test_bad_serialize(void) {
    secp256k1_surjectionproof proof;
    unsigned char serialized_proof[SECP256K1_SURJECTIONPROOF_SERIALIZATION_BYTES_MAX];
    size_t serialized_len;

    proof.n_inputs = 0;
    serialized_len = 2 + 31;
    /* e0 is one byte too short */
    CHECK(secp256k1_surjectionproof_serialize(ctx, serialized_proof, &serialized_len, &proof) == 0);
}

void test_bad_parse(void) {
    secp256k1_surjectionproof proof;
    unsigned char serialized_proof0[] = { 0x00 };
    unsigned char serialized_proof1[] = { 0x01, 0x00 };
    unsigned char serialized_proof2[33] = { 0 };

    /* Missing total input count */
    CHECK(secp256k1_surjectionproof_parse(ctx, &proof, serialized_proof0, sizeof(serialized_proof0)) == 0);
    /* Missing bitmap */
    CHECK(secp256k1_surjectionproof_parse(ctx, &proof, serialized_proof1, sizeof(serialized_proof1)) == 0);
    /* Missing e0 value */
    CHECK(secp256k1_surjectionproof_parse(ctx, &proof, serialized_proof2, sizeof(serialized_proof2)) == 0);
}

void run_surjection_tests(void) {
    int i;
    for (i = 0; i < count; i++) {
        test_surjectionproof_api();
    }

    test_input_selection(0);
    test_input_selection(1);
    test_input_selection(5);
    test_input_selection(100);
    test_input_selection(SECP256K1_SURJECTIONPROOF_MAX_N_INPUTS);

    test_input_selection_distribution();
    test_gen_verify(10, 3);
    test_gen_verify(SECP256K1_SURJECTIONPROOF_MAX_N_INPUTS, SECP256K1_SURJECTIONPROOF_MAX_N_INPUTS);
    test_no_used_inputs_verify();
    test_bad_serialize();
    test_bad_parse();
}

#endif
