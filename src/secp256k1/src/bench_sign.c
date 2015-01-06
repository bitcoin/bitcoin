/**********************************************************************
 * Copyright (c) 2014 Pieter Wuille                                   *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

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

static void bench_sign(void* arg, int iters) {
    bench_sign_t *data = (bench_sign_t*)arg;

    unsigned char sig[64];
    for (int i=0; i<iters; i++) {
        int recid = 0;
        CHECK(secp256k1_ecdsa_sign_compact(data->msg, sig, data->key, data->nonce, &recid));
        for (int j = 0; j < 32; j++) {
            data->nonce[j] = data->key[j];     /* Move former key to nonce  */
            data->msg[j] = sig[j];             /* Move former R to message. */
            data->key[j] = sig[j + 32];        /* Move former S to key.     */
        }
    }
}

int main(int argc, char **argv) {
    int iters=20000; int count=10;
    int oa;
    while ((oa = getopt(argc, argv, "c:i:w:")) != -1) {
        switch (oa) {
        case 'c':
            count=atoi(optarg);
            ( count<0 || count > 5000 ) ? (printf("Count %d out of sane bounds. Resetting to 10.\n",count),(count=10)):0x0;
            break;
        case 'i':
            iters=atoi(optarg);
            ( iters<0 || iters > 200000 ) ? (printf("Iterations %d out of sane bounds. Resetting to 20000.\n",iters),iters=20000):0x0;
            break;
        case 'w':
            ecmult_impl_windowG=atoi(optarg);
            ( ecmult_impl_windowG<2 || ecmult_impl_windowG > 30) ? (printf("WINDOW_G cache %d out of sane bounds. Resetting to 16.\n",ecmult_impl_windowG),ecmult_impl_windowG=16):0x0;
            break;
        case '?':
            printf("Missing argument to %c.", (char)optopt);
        default:
            return 1;
        }
    }

    secp256k1_start(SECP256K1_START_SIGN);

    bench_sign_t data;
    run_benchmark(bench_sign, bench_sign_setup, NULL, &data, count, iters);

    secp256k1_stop();
    return 0;
}
