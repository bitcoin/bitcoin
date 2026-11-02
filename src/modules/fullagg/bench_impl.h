/***********************************************************************
 * Copyright (c) 2026 The libsecp256k1 developers                      *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_MODULE_FULLAGG_BENCH_H
#define SECP256K1_MODULE_FULLAGG_BENCH_H

#include "../../../include/secp256k1_fullagg.h"

/* Number of signers in the benchmarked signing session */
#define FULLAGG_BENCH_N_SIGNERS 3

typedef struct {
    secp256k1_context *ctx;

    secp256k1_keypair keypairs[FULLAGG_BENCH_N_SIGNERS];
    secp256k1_xonly_pubkey pks[FULLAGG_BENCH_N_SIGNERS];
    const secp256k1_xonly_pubkey *pk_ptrs[FULLAGG_BENCH_N_SIGNERS];
    unsigned char msgs[FULLAGG_BENCH_N_SIGNERS][32];
    const unsigned char *msg_ptrs[FULLAGG_BENCH_N_SIGNERS];
    secp256k1_fullagg_pubnonce pubnonces[FULLAGG_BENCH_N_SIGNERS];
    const secp256k1_fullagg_pubnonce *pubnonce_ptrs[FULLAGG_BENCH_N_SIGNERS];
    secp256k1_fullagg_secnonce secnonce;
    secp256k1_fullagg_session session;
    unsigned char sig[64];
} bench_fullagg_data;

static void bench_fullagg_sign(void* arg, int iters) {
    bench_fullagg_data *data = (bench_fullagg_data *)arg;
    int i;
    secp256k1_fullagg_partial_sig partial_sig;

    for (i = 0; i < iters; i++) {
        secp256k1_fullagg_secnonce secnonce;
        /* The secnonce is copied because signing invalidates it and this
         * benchmark signs repeatedly with the same session. Application code
         * must never copy a secnonce (see secp256k1_fullagg_secnonce). */
        memcpy(&secnonce, &data->secnonce, sizeof(secnonce));
        CHECK(secp256k1_fullagg_partial_sign(data->ctx, &partial_sig, &secnonce, &data->keypairs[0],
                                             data->msgs[0], &data->session, data->pk_ptrs,
                                             data->msg_ptrs, data->pubnonce_ptrs,
                                             FULLAGG_BENCH_N_SIGNERS, 0));
    }
}

static void bench_fullagg_verify(void* arg, int iters) {
    bench_fullagg_data *data = (bench_fullagg_data *)arg;
    int i;

    for (i = 0; i < iters; i++) {
        CHECK(secp256k1_fullagg_verify(data->ctx, data->sig, data->pk_ptrs, data->msg_ptrs,
                                       FULLAGG_BENCH_N_SIGNERS));
    }
}

static void run_fullagg_bench(int iters, int argc, char** argv) {
    int i;
    bench_fullagg_data data;
    secp256k1_fullagg_secnonce secnonces[FULLAGG_BENCH_N_SIGNERS];
    secp256k1_fullagg_aggnonce aggnonce;
    secp256k1_fullagg_partial_sig psigs[FULLAGG_BENCH_N_SIGNERS];
    const secp256k1_fullagg_partial_sig *psig_ptrs[FULLAGG_BENCH_N_SIGNERS];
    int d = argc == 1;

    data.ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);

    for (i = 0; i < FULLAGG_BENCH_N_SIGNERS; i++) {
        unsigned char sk[32];
        unsigned char secrand[32];

        memset(sk, (unsigned char)(i + 1), sizeof(sk));
        memset(secrand, (unsigned char)(i + 0x40), sizeof(secrand));
        memset(data.msgs[i], (unsigned char)(i + 0x80), sizeof(data.msgs[i]));
        data.pk_ptrs[i] = &data.pks[i];
        data.msg_ptrs[i] = data.msgs[i];
        data.pubnonce_ptrs[i] = &data.pubnonces[i];
        psig_ptrs[i] = &psigs[i];

        CHECK(secp256k1_keypair_create(data.ctx, &data.keypairs[i], sk));
        CHECK(secp256k1_keypair_xonly_pub(data.ctx, &data.pks[i], NULL, &data.keypairs[i]));
        CHECK(secp256k1_fullagg_nonce_gen(data.ctx, &secnonces[i], &data.pubnonces[i], secrand,
                                          sk, &data.pks[i], NULL));
    }
    /* Save signer 0's secnonce for bench_fullagg_sign before it is consumed */
    memcpy(&data.secnonce, &secnonces[0], sizeof(data.secnonce));

    CHECK(secp256k1_fullagg_nonce_agg(data.ctx, &aggnonce, data.pubnonce_ptrs, FULLAGG_BENCH_N_SIGNERS));
    CHECK(secp256k1_fullagg_session_init(data.ctx, &data.session, &aggnonce, data.pk_ptrs,
                                         data.msg_ptrs, data.pubnonce_ptrs, FULLAGG_BENCH_N_SIGNERS));
    for (i = 0; i < FULLAGG_BENCH_N_SIGNERS; i++) {
        CHECK(secp256k1_fullagg_partial_sign(data.ctx, &psigs[i], &secnonces[i], &data.keypairs[i],
                                             data.msgs[i], &data.session, data.pk_ptrs,
                                             data.msg_ptrs, data.pubnonce_ptrs,
                                             FULLAGG_BENCH_N_SIGNERS, i));
    }
    CHECK(secp256k1_fullagg_partial_sig_agg(data.ctx, data.sig, &data.session, psig_ptrs, FULLAGG_BENCH_N_SIGNERS));

    if (d || have_flag(argc, argv, "fullagg") || have_flag(argc, argv, "sign") || have_flag(argc, argv, "fullagg_sign")) run_benchmark("fullagg_sign", bench_fullagg_sign, NULL, NULL, (void *) &data, 10, iters);
    if (d || have_flag(argc, argv, "fullagg") || have_flag(argc, argv, "verify") || have_flag(argc, argv, "fullagg_verify")) run_benchmark("fullagg_verify", bench_fullagg_verify, NULL, NULL, (void *) &data, 10, iters);

    secp256k1_context_destroy(data.ctx);
}

#endif
