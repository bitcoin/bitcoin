/**********************************************************************
 * Copyright (c) 2014 Pieter Wuille                                   *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#include "include/secp256k1.h"
#include "util.h"
#include "bench.h"

typedef struct {
    secp256k1_context_t* ctx;
    unsigned char msg[32];
    unsigned char key[32];
} bench_sign_t;

static void bench_sign_setup(void* arg) {
    int i;
    bench_sign_t *data = (bench_sign_t*)arg;

    for (i = 0; i < 32; i++) data->msg[i] = i + 1;
    for (i = 0; i < 32; i++) data->key[i] = i + 65;
}

static void bench_sign(void* arg) {
    int i;
    bench_sign_t *data = (bench_sign_t*)arg;

    unsigned char sig[64];
    for (i = 0; i < 20000; i++) {
        int j;
        int recid = 0;
        secp256k1_ecdsa_signature_t signature;
        CHECK(secp256k1_ecdsa_sign(data->ctx, data->msg, &signature, data->key, NULL, NULL));
        CHECK(secp256k1_ecdsa_signature_serialize_compact(data->ctx, sig, &recid, &signature));
        for (j = 0; j < 32; j++) {
            data->msg[j] = sig[j];             /* Move former R to message. */
            data->key[j] = sig[j + 32];        /* Move former S to key.     */
        }
    }
}

int main(void) {
    bench_sign_t data;

    data.ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN);

    run_benchmark("ecdsa_sign", bench_sign, bench_sign_setup, NULL, &data, 10, 20000);

    secp256k1_context_destroy(data.ctx);
    return 0;
}
