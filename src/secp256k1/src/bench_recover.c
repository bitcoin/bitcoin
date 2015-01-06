/**********************************************************************
 * Copyright (c) 2014 Pieter Wuille                                   *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

#include "include/secp256k1.h"
#include "util.h"
#include "bench.h"

typedef struct {
    unsigned char msg[32];
    unsigned char sig[64];
} bench_recover_t;

void bench_recover(void* arg, int iters) {
    bench_recover_t *data = (bench_recover_t*)arg;

    unsigned char pubkey[33];
    for (int i=0; i<iters; i++) {
        int pubkeylen = 33;
        CHECK(secp256k1_ecdsa_recover_compact(data->msg, data->sig, pubkey, &pubkeylen, 1, i % 2));
        for (int j = 0; j < 32; j++) {
            data->sig[j + 32] = data->msg[j];    /* Move former message to S. */
            data->msg[j] = data->sig[j];         /* Move former R to message. */
            data->sig[j] = pubkey[j + 1];        /* Move recovered pubkey X coordinate to R (which must be a valid X coordinate). */
        }
    }
}

void bench_recover_setup(void* arg) {
    bench_recover_t *data = (bench_recover_t*)arg;

    for (int i = 0; i < 32; i++) data->msg[i] = 1 + i;
    for (int i = 0; i < 64; i++) data->sig[i] = 65 + i;
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

    secp256k1_start(SECP256K1_START_VERIFY);

    bench_recover_t data;
    run_benchmark(bench_recover, bench_recover_setup, NULL, &data, count, iters);

    secp256k1_stop();
    return 0;
}
