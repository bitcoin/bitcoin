/***********************************************************************
 * Copyright (c) 2018-2020 Andrew Poelstra, Jonas Nick                 *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_MODULE_SCHNORRSIG_BENCH_H
#define SECP256K1_MODULE_SCHNORRSIG_BENCH_H

#include "../../../include/secp256k1_schnorrsig.h"

#define MSGLEN 32

typedef struct {
    secp256k1_context *ctx;
    int n;

    const secp256k1_keypair **keypairs;
    const unsigned char **pk;
    const unsigned char **sigs;
    const unsigned char **msgs;
} bench_schnorrsig_data;

static void bench_schnorrsig_sign(void* arg, int iters) {
    bench_schnorrsig_data *data = (bench_schnorrsig_data *)arg;
    int i;
    unsigned char msg[MSGLEN] = {0};
    unsigned char sig[64];

    for (i = 0; i < iters; i++) {
        msg[0] = i;
        msg[1] = i >> 8;
        CHECK(secp256k1_schnorrsig_sign_custom(data->ctx, sig, msg, MSGLEN, data->keypairs[i], NULL));
    }
}

static void bench_schnorrsig_verify(void* arg, int iters) {
    bench_schnorrsig_data *data = (bench_schnorrsig_data *)arg;
    int i;

    for (i = 0; i < iters; i++) {
        secp256k1_xonly_pubkey pk;
        CHECK(secp256k1_xonly_pubkey_parse(data->ctx, &pk, data->pk[i]) == 1);
        CHECK(secp256k1_schnorrsig_verify(data->ctx, data->sigs[i], data->msgs[i], MSGLEN, &pk));
    }
}

static void run_schnorrsig_bench(int iters, int argc, char** argv) {
    int i;
    bench_schnorrsig_data data;
    int d = argc == 1;

    data.ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
    data.keypairs = (const secp256k1_keypair **)malloc(iters * sizeof(secp256k1_keypair *));
    data.pk = (const unsigned char **)malloc(iters * sizeof(unsigned char *));
    data.msgs = (const unsigned char **)malloc(iters * sizeof(unsigned char *));
    data.sigs = (const unsigned char **)malloc(iters * sizeof(unsigned char *));

    CHECK(MSGLEN >= 4);
    for (i = 0; i < iters; i++) {
        unsigned char sk[32];
        unsigned char *msg = (unsigned char *)malloc(MSGLEN);
        unsigned char *sig = (unsigned char *)malloc(64);
        secp256k1_keypair *keypair = (secp256k1_keypair *)malloc(sizeof(*keypair));
        unsigned char *pk_char = (unsigned char *)malloc(32);
        secp256k1_xonly_pubkey pk;
        msg[0] = sk[0] = i;
        msg[1] = sk[1] = i >> 8;
        msg[2] = sk[2] = i >> 16;
        msg[3] = sk[3] = i >> 24;
        memset(&msg[4], 'm', MSGLEN - 4);
        memset(&sk[4], 's', 28);

        data.keypairs[i] = keypair;
        data.pk[i] = pk_char;
        data.msgs[i] = msg;
        data.sigs[i] = sig;

        CHECK(secp256k1_keypair_create(data.ctx, keypair, sk));
        CHECK(secp256k1_schnorrsig_sign_custom(data.ctx, sig, msg, MSGLEN, keypair, NULL));
        CHECK(secp256k1_keypair_xonly_pub(data.ctx, &pk, NULL, keypair));
        CHECK(secp256k1_xonly_pubkey_serialize(data.ctx, pk_char, &pk) == 1);
    }

    if (d || have_flag(argc, argv, "schnorrsig") || have_flag(argc, argv, "sign") || have_flag(argc, argv, "schnorrsig_sign")) run_benchmark("schnorrsig_sign", bench_schnorrsig_sign, NULL, NULL, (void *) &data, 10, iters);
    if (d || have_flag(argc, argv, "schnorrsig") || have_flag(argc, argv, "verify") || have_flag(argc, argv, "schnorrsig_verify")) run_benchmark("schnorrsig_verify", bench_schnorrsig_verify, NULL, NULL, (void *) &data, 10, iters);

    for (i = 0; i < iters; i++) {
        free((void *)data.keypairs[i]);
        free((void *)data.pk[i]);
        free((void *)data.msgs[i]);
        free((void *)data.sigs[i]);
    }

    /* Casting to (void *) avoids a stupid warning in MSVC. */
    free((void *)data.keypairs);
    free((void *)data.pk);
    free((void *)data.msgs);
    free((void *)data.sigs);

    secp256k1_context_destroy(data.ctx);
}

#endif
