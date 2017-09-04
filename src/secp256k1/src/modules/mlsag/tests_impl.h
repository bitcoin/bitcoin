/**********************************************************************
 * Copyright (c) 2017 The Particl Core developers                     *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef SECP256K1_MODULE_MLSAG_TESTS
#define SECP256K1_MODULE_MLSAG_TESTS

#include "include/secp256k1_mlsag.h"

static int mlsag_count = 32;

#define MAX_N_INPUTS  32
#define MAX_N_OUTPUTS 32
#define MAX_N_COLUMNS 32
void test_mlsag(void)
{
    const size_t n_inputs = (secp256k1_rand32() % (MAX_N_INPUTS))+1;
    const size_t n_outputs = (secp256k1_rand32() % (MAX_N_OUTPUTS))+1;
    const size_t n_blinded = (secp256k1_rand32() % (n_outputs))+1;
    const size_t n_columns = (secp256k1_rand32() % (MAX_N_COLUMNS))+1;
    const size_t n_real_col = secp256k1_rand32() % (n_columns);
    const size_t n_rows = n_inputs+1;
    uint8_t preimage[32];
    uint8_t tmp32[32];
    size_t k, i, sz;
    uint64_t value[MAX_N_INPUTS + MAX_N_OUTPUTS];
    uint8_t blinds_out[MAX_N_OUTPUTS * 32];
    uint8_t blinds_in[MAX_N_INPUTS * 32];
    uint8_t keys_in[(MAX_N_INPUTS+1) * 32]; /* 1 extra for blind sum */
    uint8_t *pkeys[MAX_N_INPUTS+1];
    uint8_t m[(MAX_N_INPUTS+1) * MAX_N_COLUMNS * 33];
    int64_t total_value;
    const uint8_t *pcm_in[MAX_N_INPUTS * MAX_N_COLUMNS], *pcm_out[MAX_N_OUTPUTS], *pblinds[MAX_N_INPUTS + MAX_N_OUTPUTS];
    secp256k1_pedersen_commitment cm_in[MAX_N_INPUTS * MAX_N_COLUMNS];
    secp256k1_pedersen_commitment cm_out[MAX_N_OUTPUTS];
    secp256k1_scalar s;
    secp256k1_gej gej;
    secp256k1_ge ge;
    uint8_t pc[32];
    uint8_t ki[MAX_N_INPUTS * 33];
    uint8_t ss[(MAX_N_INPUTS+1) * MAX_N_COLUMNS * 33]; /* max_rows * max_cols */
    
    secp256k1_rand256(preimage);
    
    total_value = 0;
    for (i = 0; i < n_outputs; i++)
    {
        value[n_inputs + i] = secp256k1_rands64(0, (INT64_MAX/n_outputs));
        total_value += value[n_inputs + i];
    }
    for (i = 0; i < n_inputs - 1; i++)
    {
        value[i] = secp256k1_rands64(0, total_value);
        total_value -= value[i];
    }
    value[i] = total_value;
    
    for (k = 0; k < n_blinded; ++k)
    {
        random_scalar_order(&s);
        secp256k1_scalar_get_b32(&blinds_out[k * 32], &s);
        pblinds[n_inputs + k] = &blinds_out[k * 32];
        
        CHECK(secp256k1_pedersen_commit(ctx, &cm_out[k], pblinds[n_inputs + k], value[n_inputs + k], secp256k1_generator_h));
        pcm_out[k] = cm_out[k].data;
    }
    
    memset(tmp32, 0, 32); /* use tmp32 for zero here */
    for (k = n_blinded; k < n_outputs; ++k)
    {
        /* NOTE: fails if value <= 0 */
        CHECK(secp256k1_pedersen_commit(ctx, &cm_out[k], tmp32, value[n_inputs + k], secp256k1_generator_h));
        pcm_out[k] = cm_out[k].data;
    }
    
    for (k = 0; k < n_inputs; ++k) /* rows */
    for (i = 0; i < n_columns; ++i) /* cols */
    {
        if (i == n_real_col)
        {
            random_scalar_order(&s);
            secp256k1_scalar_get_b32(&keys_in[k * 32], &s);
            pkeys[k] = &keys_in[k * 32];
            
            secp256k1_ecmult_gen(&ctx->ecmult_gen_ctx, &gej, &s);
            secp256k1_ge_set_gej(&ge, &gej);
            secp256k1_eckey_pubkey_serialize(&ge, &m[(i+k*n_columns)*33], &sz, 1);
            
            random_scalar_order(&s);
            secp256k1_scalar_get_b32(&blinds_in[k * 32], &s);
            pblinds[k] = &blinds_in[k * 32];
            
            CHECK(secp256k1_pedersen_commit(ctx, &cm_in[i+k*n_columns], pblinds[k], value[k], secp256k1_generator_h));
            pcm_in[i+k*n_columns] = cm_in[i+k*n_columns].data;
            
            continue;
        }
        
        /* Fake input */
        random_scalar_order(&s);
        secp256k1_ecmult_gen(&ctx->ecmult_gen_ctx, &gej, &s);
        secp256k1_ge_set_gej(&ge, &gej);
        secp256k1_eckey_pubkey_serialize(&ge, &m[(i+k*n_columns)*33], &sz, 1);
        
        random_scalar_order(&s);
        secp256k1_ecmult_gen(&ctx->ecmult_gen_ctx, &gej, &s);
        secp256k1_ge_set_gej(&ge, &gej);
        secp256k1_eckey_pubkey_serialize(&ge, cm_in[i+k*n_columns].data, &sz, 1);
        pcm_in[i+k*n_columns] = cm_in[i+k*n_columns].data;
    }
    
    pkeys[n_inputs] = &keys_in[n_inputs * 32]; /* blind sum */
    
    CHECK(0 == secp256k1_prepare_mlsag(m, pkeys[n_inputs],
        n_outputs, n_blinded, n_columns, n_inputs+1,
        pcm_in, pcm_out, pblinds));
    
    secp256k1_rand256(tmp32); /* random seed */
    CHECK(0 == secp256k1_generate_mlsag(ctx, ki, pc, ss,
        tmp32, preimage, n_columns, n_rows, n_real_col,
        (const uint8_t**)pkeys, m));
    
    CHECK(0 == secp256k1_verify_mlsag(ctx,
        preimage, n_columns, n_rows, 
        m, ki, pc, ss));
    
    
    /* --- Test for failure --- */
    
    /* Bad preimage */
    CHECK(2 == secp256k1_verify_mlsag(ctx,
        tmp32, n_columns, n_rows,
        m, ki, pc, ss));
    
    
    /* Bad c */
    CHECK(2 == secp256k1_verify_mlsag(ctx,
        preimage, n_columns, n_rows,
        m, ki, tmp32, ss));
    
    
    /* Bad sum */
    value[0] -= 1;
    CHECK(secp256k1_pedersen_commit(ctx, &cm_in[n_real_col+0*n_columns], pblinds[0], value[0], secp256k1_generator_h));
    CHECK(0 == secp256k1_prepare_mlsag(m, pkeys[n_inputs],
        n_outputs, n_blinded, n_columns, n_inputs+1,
        pcm_in, pcm_out, pblinds));
    CHECK(0 == secp256k1_generate_mlsag(ctx, ki, pc, ss,
        tmp32, preimage, n_columns, n_rows, n_real_col,
        (const uint8_t**)pkeys, m));
    CHECK(2 == secp256k1_verify_mlsag(ctx,
        preimage, n_columns, n_rows,
        m, ki, pc, ss));
    
    /* Pass repaired bad sum */
    value[0] += 1;
    CHECK(secp256k1_pedersen_commit(ctx, &cm_in[n_real_col+0*n_columns], pblinds[0], value[0], secp256k1_generator_h));
    CHECK(0 == secp256k1_prepare_mlsag(m, pkeys[n_inputs],
        n_outputs, n_blinded, n_columns, n_inputs+1,
        pcm_in, pcm_out, pblinds));
    CHECK(0 == secp256k1_generate_mlsag(ctx, ki, pc, ss,
        tmp32, preimage, n_columns, n_rows, n_real_col,
        (const uint8_t**)pkeys, m));
    CHECK(0 == secp256k1_verify_mlsag(ctx,
        preimage, n_columns, n_rows,
        m, ki, pc, ss));
    
    
    /* Bad secretkey */
    random_scalar_order(&s);
    secp256k1_scalar_get_b32(&keys_in[0], &s);
    pkeys[0] = &keys_in[0];
    CHECK(0 == secp256k1_generate_mlsag(ctx, ki, pc, ss,
        tmp32, preimage, n_columns, n_rows, n_real_col,
        (const uint8_t**)pkeys, m));
    CHECK(2 == secp256k1_verify_mlsag(ctx,
        preimage, n_columns, n_rows,
        m, ki, pc, ss));
}


void run_mlsag_tests(void) {
    int i;
    
    for (i = 0; i < mlsag_count; i++) {
        test_mlsag();
    }
}

#endif
