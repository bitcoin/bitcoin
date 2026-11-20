/***********************************************************************
 * Copyright (c) 2026 The libsecp256k1 developers                      *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_MODULE_SCHNORRSIG_HALFAGG_BENCH_H
#define SECP256K1_MODULE_SCHNORRSIG_HALFAGG_BENCH_H

#include "../../../include/secp256k1_schnorrsig.h"
#include "../../../include/secp256k1_schnorrsig_halfagg.h"

/* Number of signatures in the benchmarked aggregate */
#define HALFAGG_BENCH_N_SIGS 3

typedef struct {
    secp256k1_context *ctx;
    secp256k1_xonly_pubkey pubkeys[HALFAGG_BENCH_N_SIGS];
    unsigned char msgs32[HALFAGG_BENCH_N_SIGS * 32];
    unsigned char sigs64[HALFAGG_BENCH_N_SIGS * 64];
    unsigned char aggsig[32 * (HALFAGG_BENCH_N_SIGS + 1)];
    size_t aggsig_len;
} bench_halfagg_data;

static void bench_halfagg_aggregate(void* arg, int iters) {
    bench_halfagg_data *data = (bench_halfagg_data *)arg;
    int i;

    for (i = 0; i < iters; i++) {
        unsigned char aggsig[32 * (HALFAGG_BENCH_N_SIGS + 1)];
        size_t aggsig_len = sizeof(aggsig);
        CHECK(secp256k1_schnorrsig_aggregate(data->ctx, aggsig, &aggsig_len, data->pubkeys,
                                             data->msgs32, data->sigs64, HALFAGG_BENCH_N_SIGS));
    }
}

static void bench_halfagg_verify(void* arg, int iters) {
    bench_halfagg_data *data = (bench_halfagg_data *)arg;
    int i;

    for (i = 0; i < iters; i++) {
        CHECK(secp256k1_schnorrsig_aggverify(data->ctx, data->pubkeys, data->msgs32,
                                             HALFAGG_BENCH_N_SIGS, data->aggsig, data->aggsig_len));
    }
}

static void run_halfagg_bench(int iters, int argc, char** argv) {
    int i;
    bench_halfagg_data data;
    int d = argc == 1;

    data.ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);

    for (i = 0; i < HALFAGG_BENCH_N_SIGS; i++) {
        unsigned char sk[32];
        secp256k1_keypair keypair;

        memset(sk, (unsigned char)(i + 1), sizeof(sk));
        memset(&data.msgs32[i * 32], (unsigned char)(i + 0x80), 32);
        CHECK(secp256k1_keypair_create(data.ctx, &keypair, sk));
        CHECK(secp256k1_keypair_xonly_pub(data.ctx, &data.pubkeys[i], NULL, &keypair));
        CHECK(secp256k1_schnorrsig_sign32(data.ctx, &data.sigs64[i * 64], &data.msgs32[i * 32], &keypair, NULL));
    }
    data.aggsig_len = sizeof(data.aggsig);
    CHECK(secp256k1_schnorrsig_aggregate(data.ctx, data.aggsig, &data.aggsig_len, data.pubkeys,
                                         data.msgs32, data.sigs64, HALFAGG_BENCH_N_SIGS));

    if (d || have_flag(argc, argv, "halfagg") || have_flag(argc, argv, "halfagg_aggregate")) run_benchmark("halfagg_aggregate", bench_halfagg_aggregate, NULL, NULL, (void *) &data, 10, iters);
    if (d || have_flag(argc, argv, "halfagg") || have_flag(argc, argv, "verify") || have_flag(argc, argv, "halfagg_verify")) run_benchmark("halfagg_verify", bench_halfagg_verify, NULL, NULL, (void *) &data, 10, iters);

    secp256k1_context_destroy(data.ctx);
}

#endif
