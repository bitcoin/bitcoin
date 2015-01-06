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
    unsigned char key[32];
    unsigned char nonce[32];
    unsigned char sig[72];
    int siglen;
    unsigned char pubkey[33];
    int pubkeylen;
} benchmark_verify_t;

static void benchmark_verify(void* arg, int iters) {
    benchmark_verify_t* data = (benchmark_verify_t*)arg;

    for (int i=0; i<iters; i++) {
        data->sig[data->siglen - 1] ^= (i & 0xFF);
        data->sig[data->siglen - 2] ^= ((i >> 8) & 0xFF);
        data->sig[data->siglen - 3] ^= ((i >> 16) & 0xFF);
        CHECK(secp256k1_ecdsa_verify(data->msg, data->sig, data->siglen, data->pubkey, data->pubkeylen) == (i == 0));
        data->sig[data->siglen - 1] ^= (i & 0xFF);
        data->sig[data->siglen - 2] ^= ((i >> 8) & 0xFF);
        data->sig[data->siglen - 3] ^= ((i >> 16) & 0xFF);
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

    secp256k1_start(SECP256K1_START_VERIFY | SECP256K1_START_SIGN);

    benchmark_verify_t data;

    for (int i = 0; i < 32; i++) data.msg[i] = 1 + i;
    for (int i = 0; i < 32; i++) data.key[i] = 33 + i;
    for (int i = 0; i < 32; i++) data.nonce[i] = 65 + i;
    data.siglen = 72;
    CHECK(secp256k1_ecdsa_sign(data.msg, data.sig, &data.siglen, data.key, data.nonce));
    data.pubkeylen = 33;
    CHECK(secp256k1_ec_pubkey_create(data.pubkey, &data.pubkeylen, data.key, 1));

    run_benchmark(benchmark_verify, NULL, NULL, &data, count, iters);

    secp256k1_stop();
    return 0;
}
