/**********************************************************************
 * Copyright (c) 2014 Pieter Wuille                                   *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#include "include/secp256k1.h"
#include "util.h"
#include "bench.h"

typedef struct {
    unsigned char msg[32];
    unsigned char nonce[32];
    unsigned char key[32];
} bench_sign_t;

static void bench_sign_setup(void* arg) {
    bench_sign_t *data = (bench_sign_t*)arg;

    for (int i = 0; i < 32; i++) data->msg[i] = i + 1;
    for (int i = 0; i < 32; i++) data->nonce[i] = i + 33;
    for (int i = 0; i < 32; i++) data->key[i] = i + 65;
}

static void bench_sign(void* arg) {
    bench_sign_t *data = (bench_sign_t*)arg;

    unsigned char sig[64];
    for (int i=0; i<20000; i++) {
        int recid = 0;
        CHECK(secp256k1_ecdsa_sign_compact(data->msg, sig, data->key, data->nonce, &recid));
        for (int j = 0; j < 32; j++) {
            data->nonce[j] = data->key[j];     /* Move former key to nonce  */
            data->msg[j] = sig[j];             /* Move former R to message. */
            data->key[j] = sig[j + 32];        /* Move former S to key.     */
        }
    }
}

int main(void) {
    secp256k1_start(SECP256K1_START_SIGN);

    bench_sign_t data;
    run_benchmark(bench_sign, bench_sign_setup, NULL, &data, 10, 20000);

    secp256k1_stop();
    return 0;
}
