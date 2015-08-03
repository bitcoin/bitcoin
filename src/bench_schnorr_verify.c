/**********************************************************************
 * Copyright (c) 2014 Pieter Wuille                                   *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#include <stdio.h>
#include <string.h>

#include "include/secp256k1.h"
#include "include/secp256k1_schnorr.h"
#include "util.h"
#include "bench.h"

typedef struct {
    unsigned char key[32];
    unsigned char sig[64];
    unsigned char pubkey[33];
    int pubkeylen;
} benchmark_schnorr_sig_t;

typedef struct {
    secp256k1_context_t *ctx;
    unsigned char msg[32];
    benchmark_schnorr_sig_t sigs[64];
    int numsigs;
} benchmark_schnorr_verify_t;

static void benchmark_schnorr_init(void* arg) {
    int i, k;
    benchmark_schnorr_verify_t* data = (benchmark_schnorr_verify_t*)arg;

    for (i = 0; i < 32; i++) data->msg[i] = 1 + i;
    for (k = 0; k < data->numsigs; k++) {
        secp256k1_pubkey_t pubkey;
        for (i = 0; i < 32; i++) data->sigs[k].key[i] = 33 + i + k;
        secp256k1_schnorr_sign(data->ctx, data->msg, data->sigs[k].sig, data->sigs[k].key, NULL, NULL);
        data->sigs[k].pubkeylen = 33;
        CHECK(secp256k1_ec_pubkey_create(data->ctx, &pubkey, data->sigs[k].key));
        CHECK(secp256k1_ec_pubkey_serialize(data->ctx, data->sigs[k].pubkey, &data->sigs[k].pubkeylen, &pubkey, 1));
    }
}

static void benchmark_schnorr_verify(void* arg) {
    int i;
    benchmark_schnorr_verify_t* data = (benchmark_schnorr_verify_t*)arg;

    for (i = 0; i < 20000 / data->numsigs; i++) {
        secp256k1_pubkey_t pubkey;
        data->sigs[0].sig[(i >> 8) % 64] ^= (i & 0xFF);
        CHECK(secp256k1_ec_pubkey_parse(data->ctx, &pubkey, data->sigs[0].pubkey, data->sigs[0].pubkeylen));
        CHECK(secp256k1_schnorr_verify(data->ctx, data->msg, data->sigs[0].sig, &pubkey) == ((i & 0xFF) == 0));
        data->sigs[0].sig[(i >> 8) % 64] ^= (i & 0xFF);
    }
}



int main(void) {
    benchmark_schnorr_verify_t data;

    data.ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);

    data.numsigs = 1;
    run_benchmark("schnorr_verify", benchmark_schnorr_verify, benchmark_schnorr_init, NULL, &data, 10, 20000);

    secp256k1_context_destroy(data.ctx);
    return 0;
}
