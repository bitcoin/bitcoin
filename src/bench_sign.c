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
    unsigned char key[32];
} bench_sign_t;

/** Very fast but insecure nonce generation function. Do not use for production code. */
static int insecure_nonce_function(unsigned char *nonce32, const unsigned char *msg32, const unsigned char *key32, unsigned int count, const void *data) {
   (void)data;
   for (int i = 0; i < 8; i++) {
       ((uint32_t*)nonce32)[i] = ((uint32_t*)msg32)[i] + ((uint32_t*)key32)[i] + count;
   }
   return 1;
}

static void bench_sign_setup(void* arg) {
    bench_sign_t *data = (bench_sign_t*)arg;

    for (int i = 0; i < 32; i++) data->msg[i] = i + 1;
    for (int i = 0; i < 32; i++) data->key[i] = i + 65;
}

static void bench_sign(void* arg) {
    bench_sign_t *data = (bench_sign_t*)arg;

    unsigned char sig[64];
    for (int i=0; i<20000; i++) {
        int recid = 0;
        secp256k1_ecdsa_sign_compact(data->msg, sig, data->key, insecure_nonce_function, NULL, &recid);
        for (int j = 0; j < 32; j++) {
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
