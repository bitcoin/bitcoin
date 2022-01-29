/**********************************************************************
 * Copyright (c) 2015 Gregory Maxwell                                 *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef SECP256K1_MODULE_COMMITMENT_TESTS
#define SECP256K1_MODULE_COMMITMENT_TESTS

#include <string.h>

#include "group.h"
#include "scalar.h"
#include "testrand.h"
#include "util.h"

#include "include/secp256k1_commitment.h"

static void test_commitment_api(void) {
    secp256k1_pedersen_commitment commit;
    secp256k1_pedersen_commitment commit2;
    const secp256k1_pedersen_commitment *commit_ptr = &commit;
    unsigned char blind[32];
    unsigned char blind_out[32];
    const unsigned char *blind_ptr = blind;
    unsigned char *blind_out_ptr = blind_out;
    uint64_t val = secp256k1_rand32();

    secp256k1_context *none = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
    secp256k1_context *sign = secp256k1_context_create(SECP256K1_CONTEXT_SIGN);
    secp256k1_context *vrfy = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY);
    secp256k1_context *both = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
    int32_t ecount = 0;

    secp256k1_context_set_error_callback(none, counting_illegal_callback_fn, &ecount);
    secp256k1_context_set_error_callback(sign, counting_illegal_callback_fn, &ecount);
    secp256k1_context_set_error_callback(vrfy, counting_illegal_callback_fn, &ecount);
    secp256k1_context_set_error_callback(both, counting_illegal_callback_fn, &ecount);
    secp256k1_context_set_illegal_callback(none, counting_illegal_callback_fn, &ecount);
    secp256k1_context_set_illegal_callback(sign, counting_illegal_callback_fn, &ecount);
    secp256k1_context_set_illegal_callback(vrfy, counting_illegal_callback_fn, &ecount);
    secp256k1_context_set_illegal_callback(both, counting_illegal_callback_fn, &ecount);

    secp256k1_rand256(blind);
    CHECK(secp256k1_pedersen_commit(none, &commit, blind, val, &secp256k1_generator_const_h, &secp256k1_generator_const_g) == 1);
    CHECK(ecount == 0);
    CHECK(secp256k1_pedersen_commit(vrfy, &commit, blind, val, &secp256k1_generator_const_h, &secp256k1_generator_const_g) == 1);
    CHECK(ecount == 0);
    CHECK(secp256k1_pedersen_commit(sign, &commit, blind, val, &secp256k1_generator_const_h, &secp256k1_generator_const_g) == 1);
    CHECK(ecount == 0);
    ecount = 2;

    CHECK(secp256k1_pedersen_commit(sign, NULL, blind, val, &secp256k1_generator_const_h, &secp256k1_generator_const_g) == 0);
    CHECK(ecount == 3);
    CHECK(secp256k1_pedersen_commit(sign, &commit, NULL, val, &secp256k1_generator_const_h, &secp256k1_generator_const_g) == 0);
    CHECK(ecount == 4);
    CHECK(secp256k1_pedersen_commit(sign, &commit, blind, val, NULL, &secp256k1_generator_const_g) == 0);
    CHECK(ecount == 5);
    CHECK(secp256k1_pedersen_commit(sign, &commit, blind, val, &secp256k1_generator_const_h, NULL) == 0);
    CHECK(ecount == 6);

    CHECK(secp256k1_pedersen_blind_sum(none, blind_out, &blind_ptr, 1, 1) != 0);
    CHECK(ecount == 6);
    CHECK(secp256k1_pedersen_blind_sum(none, NULL, &blind_ptr, 1, 1) == 0);
    CHECK(ecount == 7);
    CHECK(secp256k1_pedersen_blind_sum(none, blind_out, NULL, 1, 1) == 0);
    CHECK(ecount == 8);
    CHECK(secp256k1_pedersen_blind_sum(none, blind_out, &blind_ptr, 0, 1) == 0);
    CHECK(ecount == 9);
    CHECK(secp256k1_pedersen_blind_sum(none, blind_out, &blind_ptr, 0, 0) != 0);
    CHECK(ecount == 9);

    CHECK(secp256k1_pedersen_commit(sign, &commit, blind, val, &secp256k1_generator_const_h, &secp256k1_generator_const_g) != 0);
    CHECK(secp256k1_pedersen_verify_tally(none, &commit_ptr, 1, &commit_ptr, 1) != 0);
    CHECK(secp256k1_pedersen_verify_tally(none, NULL, 0, &commit_ptr, 1) == 0);
    CHECK(secp256k1_pedersen_verify_tally(none, &commit_ptr, 1, NULL, 0) == 0);
    CHECK(secp256k1_pedersen_verify_tally(none, NULL, 0, NULL, 0) != 0);
    CHECK(ecount == 9);
    CHECK(secp256k1_pedersen_verify_tally(none, NULL, 1, &commit_ptr, 1) == 0);
    CHECK(ecount == 10);
    CHECK(secp256k1_pedersen_verify_tally(none, &commit_ptr, 1, NULL, 1) == 0);
    CHECK(ecount == 11);

    CHECK(secp256k1_pedersen_blind_generator_blind_sum(none, &val, &blind_ptr, &blind_out_ptr, 1, 0) != 0);
    CHECK(ecount == 11);
    CHECK(secp256k1_pedersen_blind_generator_blind_sum(none, &val, &blind_ptr, &blind_out_ptr, 1, 1) == 0);
    CHECK(ecount == 12);
    CHECK(secp256k1_pedersen_blind_generator_blind_sum(none, &val, &blind_ptr, &blind_out_ptr, 0, 0) == 0);
    CHECK(ecount == 13);
    CHECK(secp256k1_pedersen_blind_generator_blind_sum(none, NULL, &blind_ptr, &blind_out_ptr, 1, 0) == 0);
    CHECK(ecount == 14);
    CHECK(secp256k1_pedersen_blind_generator_blind_sum(none, &val, NULL, &blind_out_ptr, 1, 0) == 0);
    CHECK(ecount == 15);
    CHECK(secp256k1_pedersen_blind_generator_blind_sum(none, &val, &blind_ptr, NULL, 1, 0) == 0);
    CHECK(ecount == 16);

    /* Test commit with integer and blinding factor */
    /* Value: 1*/
    secp256k1_scalar tmp_s;
    unsigned char out[33];
    unsigned char out2[33];
    random_scalar_order_test(&tmp_s);
    secp256k1_scalar_get_b32(blind, &tmp_s);
    memset(blind_out, 0, 32);
    blind_out[31] = 1;
    val = 1;
    CHECK(secp256k1_pedersen_commit(sign, &commit, blind, val, &secp256k1_generator_const_h, &secp256k1_generator_const_g) == 1);
    CHECK(secp256k1_pedersen_commitment_serialize(sign, out, &commit) == 1);
    CHECK(secp256k1_pedersen_blind_commit(sign, &commit2, blind, blind_out, &secp256k1_generator_const_h, &secp256k1_generator_const_g) == 1);
    CHECK(secp256k1_pedersen_commitment_serialize(sign, out2, &commit2) == 1);
    CHECK(memcmp(out, out2, 33) == 0);
    /* Value: 1 and 2*/
    random_scalar_order_test(&tmp_s);
    secp256k1_scalar_get_b32(blind, &tmp_s);
    memset(blind_out, 0, 32);
    blind_out[31] = 1;
    val = 2;
    CHECK(secp256k1_pedersen_commit(sign, &commit, blind, val, &secp256k1_generator_const_h, &secp256k1_generator_const_g) == 1);
    CHECK(secp256k1_pedersen_commitment_serialize(sign, out, &commit) == 1);
    CHECK(secp256k1_pedersen_blind_commit(sign, &commit2, blind, blind_out, &secp256k1_generator_const_h, &secp256k1_generator_const_g) == 1);
    CHECK(secp256k1_pedersen_commitment_serialize(sign, out2, &commit2) == 1);
    CHECK(memcmp(out, out2, 33) != 0);
    /* Value: random*/
    random_scalar_order_test(&tmp_s);
    secp256k1_scalar_get_b32(blind, &tmp_s);
    memset(blind_out, 0, 32);
    blind_out[30] = secp256k1_rand32()%256;
    blind_out[31] = secp256k1_rand32()%256;
    val = blind_out[30]*256 + blind_out[31];
    CHECK(secp256k1_pedersen_commit(sign, &commit, blind, val, &secp256k1_generator_const_h, &secp256k1_generator_const_g) == 1);
    CHECK(secp256k1_pedersen_commitment_serialize(sign, out, &commit) == 1);
    CHECK(secp256k1_pedersen_blind_commit(sign, &commit2, blind, blind_out, &secp256k1_generator_const_h, &secp256k1_generator_const_g) == 1);
    CHECK(secp256k1_pedersen_commitment_serialize(sign, out2, &commit2) == 1);
    CHECK(memcmp(out, out2, 33) == 0);

    secp256k1_context_destroy(none);
    secp256k1_context_destroy(sign);
    secp256k1_context_destroy(vrfy);
    secp256k1_context_destroy(both);
}

static void test_pedersen(void) {
    secp256k1_pedersen_commitment commits[19];
    const secp256k1_pedersen_commitment *cptr[19];
    unsigned char blinds[32*19];
    const unsigned char *bptr[19];
    secp256k1_scalar s;
    uint64_t values[19];
    int64_t totalv;
    int i;
    int inputs;
    int outputs;
    int total;
    inputs = (secp256k1_rand32() & 7) + 1;
    outputs = (secp256k1_rand32() & 7) + 2;
    total = inputs + outputs;
    for (i = 0; i < 19; i++) {
        cptr[i] = &commits[i];
        bptr[i] = &blinds[i * 32];
    }
    totalv = 0;
    for (i = 0; i < inputs; i++) {
        values[i] = secp256k1_rands64(0, INT64_MAX - totalv);
        totalv += values[i];
    }
    for (i = 0; i < outputs - 1; i++) {
        values[i + inputs] = secp256k1_rands64(0, totalv);
        totalv -= values[i + inputs];
    }
    values[total - 1] = totalv;

    for (i = 0; i < total - 1; i++) {
        random_scalar_order(&s);
        secp256k1_scalar_get_b32(&blinds[i * 32], &s);
    }
    CHECK(secp256k1_pedersen_blind_sum(ctx, &blinds[(total - 1) * 32], bptr, total - 1, inputs));
    for (i = 0; i < total; i++) {
        CHECK(secp256k1_pedersen_commit(ctx, &commits[i], &blinds[i * 32], values[i], &secp256k1_generator_const_h, &secp256k1_generator_const_g));
    }
    CHECK(secp256k1_pedersen_verify_tally(ctx, cptr, inputs, &cptr[inputs], outputs));
    CHECK(secp256k1_pedersen_verify_tally(ctx, &cptr[inputs], outputs, cptr, inputs));
    if (inputs > 0 && values[0] > 0) {
        CHECK(!secp256k1_pedersen_verify_tally(ctx, cptr, inputs - 1, &cptr[inputs], outputs));
    }
    random_scalar_order(&s);
    for (i = 0; i < 4; i++) {
        secp256k1_scalar_get_b32(&blinds[i * 32], &s);
    }
    values[0] = INT64_MAX;
    values[1] = 0;
    values[2] = 1;
    for (i = 0; i < 3; i++) {
        CHECK(secp256k1_pedersen_commit(ctx, &commits[i], &blinds[i * 32], values[i], &secp256k1_generator_const_h, &secp256k1_generator_const_g));
    }
    CHECK(secp256k1_pedersen_verify_tally(ctx, &cptr[0], 1, &cptr[0], 1));
    CHECK(secp256k1_pedersen_verify_tally(ctx, &cptr[1], 1, &cptr[1], 1));
}

#define MAX_N_GENS	30
void test_multiple_generators(void) {
    const size_t n_inputs = (secp256k1_rand32() % (MAX_N_GENS / 2)) + 1;
    const size_t n_outputs = (secp256k1_rand32() % (MAX_N_GENS / 2)) + 1;
    const size_t n_generators = n_inputs + n_outputs;
    unsigned char *generator_blind[MAX_N_GENS];
    unsigned char *pedersen_blind[MAX_N_GENS];
    secp256k1_generator generator[MAX_N_GENS];
    secp256k1_pedersen_commitment commit[MAX_N_GENS];
    const secp256k1_pedersen_commitment *commit_ptr[MAX_N_GENS];
    size_t i;
    int64_t total_value;
    uint64_t value[MAX_N_GENS];

    secp256k1_scalar s;

    unsigned char generator_seed[32];
    random_scalar_order(&s);
    secp256k1_scalar_get_b32(generator_seed, &s);
    /* Create all the needed generators */
    for (i = 0; i < n_generators; i++) {
        generator_blind[i] = (unsigned char*) malloc(32);
        pedersen_blind[i] = (unsigned char*) malloc(32);

        random_scalar_order(&s);
        secp256k1_scalar_get_b32(generator_blind[i], &s);
        random_scalar_order(&s);
        secp256k1_scalar_get_b32(pedersen_blind[i], &s);

        CHECK(secp256k1_generator_generate_blinded(ctx, &generator[i], generator_seed, generator_blind[i]));

        commit_ptr[i] = &commit[i];
    }

    /* Compute all the values -- can be positive or negative */
    total_value = 0;
    for (i = 0; i < n_outputs; i++) {
        value[n_inputs + i] = secp256k1_rands64(0, INT64_MAX - total_value);
        total_value += value[n_inputs + i];
    }
    for (i = 0; i < n_inputs - 1; i++) {
        value[i] = secp256k1_rands64(0, total_value);
        total_value -= value[i];
    }
    value[i] = total_value;

    /* check total */
    total_value = 0;
    for (i = 0; i < n_outputs; i++) {
        total_value += value[n_inputs + i];
    }
    for (i = 0; i < n_inputs; i++) {
        total_value -= value[i];
    }
    CHECK(total_value == 0);

    /* Correct for blinding factors and do the commitments */
    CHECK(secp256k1_pedersen_blind_generator_blind_sum(ctx, value, (const unsigned char * const *) generator_blind, pedersen_blind, n_generators, n_inputs));
    for (i = 0; i < n_generators; i++) {
        CHECK(secp256k1_pedersen_commit(ctx, &commit[i], pedersen_blind[i], value[i], &generator[i], &secp256k1_generator_const_g));
    }

    /* Verify */
    CHECK(secp256k1_pedersen_verify_tally(ctx, &commit_ptr[0], n_inputs, &commit_ptr[n_inputs], n_outputs));

    /* Cleanup */
    for (i = 0; i < n_generators; i++) {
        free(generator_blind[i]);
        free(pedersen_blind[i]);
    }
}
#undef MAX_N_GENS

void test_switch(void) {
    secp256k1_context *local_ctx;
    secp256k1_pubkey tmp_pubkey;
    unsigned char blind[32];
    unsigned char blind_switch[32];
    unsigned char blind_switch_2[32];
    uint64_t val = secp256k1_rand32();

    secp256k1_generator GENERATOR_G = {{
        0x79, 0xbe, 0x66, 0x7e, 0xf9, 0xdc, 0xbb, 0xac,
        0x55, 0xa0, 0x62, 0x95, 0xce, 0x87, 0x0b, 0x07,
        0x02, 0x9b, 0xfc, 0xdb, 0x2d, 0xce, 0x28, 0xd9,
        0x59, 0xf2, 0x81, 0x5b, 0x16, 0xf8, 0x17, 0x98,
        0x48, 0x3a, 0xda, 0x77, 0x26, 0xa3, 0xc4, 0x65,
        0x5d, 0xa4, 0xfb, 0xfc, 0x0e, 0x11, 0x08, 0xa8,
        0xfd, 0x17, 0xb4, 0x48, 0xa6, 0x85, 0x54, 0x19,
        0x9c, 0x47, 0xd0, 0x8f, 0xfb, 0x10, 0xd4, 0xb8
    }};

    secp256k1_generator GENERATOR_H = {{
        0x50, 0x92, 0x9b, 0x74, 0xc1, 0xa0, 0x49, 0x54,
        0xb7, 0x8b, 0x4b, 0x60, 0x35, 0xe9, 0x7a, 0x5e,
        0x07, 0x8a, 0x5a, 0x0f, 0x28, 0xec, 0x96, 0xd5,
        0x47, 0xbf, 0xee, 0x9a, 0xce, 0x80, 0x3a, 0xc0,
        0x31, 0xd3, 0xc6, 0x86, 0x39, 0x73, 0x92, 0x6e,
        0x04, 0x9e, 0x63, 0x7c, 0xb1, 0xb5, 0xf4, 0x0a,
        0x36, 0xda, 0xc2, 0x8a, 0xf1, 0x76, 0x69, 0x68,
        0xc3, 0x0c, 0x23, 0x13, 0xf3, 0xa3, 0x89, 0x04
    }};

    secp256k1_pubkey GENERATOR_J_PUB = {{
        0x5f, 0x15, 0x21, 0x36, 0x93, 0x93, 0x01, 0x2a, 
        0x8d, 0x8b, 0x39, 0x7e, 0x9b, 0xf4, 0x54, 0x29, 
        0x2f, 0x5a, 0x1b, 0x3d, 0x38, 0x85, 0x16, 0xc2, 
        0xf3, 0x03, 0xfc, 0x95, 0x67, 0xf5, 0x60, 0xb8, 
        0x3a, 0xc4, 0xc5, 0xa6, 0xdc, 0xa2, 0x01, 0x59, 
        0xfc, 0x56, 0xcf, 0x74, 0x9a, 0xa6, 0xa5, 0x65, 
        0x31, 0x6a, 0xa5, 0x03, 0x74, 0x42, 0x3f, 0x42, 
        0x53, 0x8f, 0xaa, 0x2c, 0xd3, 0x09, 0x3f, 0xa4
    }};

    unsigned char GENERATOR_J_COMPR[33] = {
        0x02,
        0xb8, 0x60, 0xf5, 0x67, 0x95, 0xfc, 0x03, 0xf3,
        0xc2, 0x16, 0x85, 0x38, 0x3d, 0x1b, 0x5a, 0x2f,
        0x29, 0x54, 0xf4, 0x9b, 0x7e, 0x39, 0x8b, 0x8d,
        0x2a, 0x01, 0x93, 0x93, 0x36, 0x21, 0x15, 0x5f
    };

    local_ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);

    CHECK(secp256k1_ec_pubkey_parse(local_ctx, &tmp_pubkey, GENERATOR_J_COMPR, 33));
    CHECK(memcmp(GENERATOR_J_PUB.data, tmp_pubkey.data, 64) == 0);
    secp256k1_rand256(blind);
    CHECK(secp256k1_blind_switch(local_ctx, blind_switch, blind, val, &GENERATOR_H, &GENERATOR_G, &GENERATOR_J_PUB));
    CHECK(memcmp(blind_switch, blind, 32) != 0);
    CHECK(secp256k1_blind_switch(local_ctx, blind_switch_2, blind, val, &GENERATOR_H, &GENERATOR_G, &GENERATOR_J_PUB));
    CHECK(memcmp(blind_switch_2, blind_switch, 32) == 0);
}

void run_commitment_tests(void) {
    int i;
    test_commitment_api();
    for (i = 0; i < 10*count; i++) {
        test_pedersen();
    }
    test_multiple_generators();
    test_switch();
}

#endif
