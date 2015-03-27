/**********************************************************************
 * Copyright (c) 2014 Pieter Wuille                                   *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#include <stdio.h>
#include <string.h>

#include "include/secp256k1.h"
#include "util.h"
#include "bench.h"

typedef struct {
    unsigned char msg[32];
    unsigned char key[32];
    unsigned char sig[72];
    int siglen;
    unsigned char pubkey[33];
    int pubkeylen;
} benchmark_verify_t;

static void benchmark_verify(void* arg) {
    int i;
    benchmark_verify_t* data = (benchmark_verify_t*)arg;

    for (i = 0; i < 20000; i++) {
        data->sig[data->siglen - 1] ^= (i & 0xFF);
        data->sig[data->siglen - 2] ^= ((i >> 8) & 0xFF);
        data->sig[data->siglen - 3] ^= ((i >> 16) & 0xFF);
        CHECK(secp256k1_ecdsa_verify(data->msg, data->sig, data->siglen, data->pubkey, data->pubkeylen) == (i == 0));
        data->sig[data->siglen - 1] ^= (i & 0xFF);
        data->sig[data->siglen - 2] ^= ((i >> 8) & 0xFF);
        data->sig[data->siglen - 3] ^= ((i >> 16) & 0xFF);
    }
}

int main(void) {
    int i;
    benchmark_verify_t data;

    secp256k1_start(SECP256K1_START_VERIFY | SECP256K1_START_SIGN);

    for (i = 0; i < 32; i++) data.msg[i] = 1 + i;
    for (i = 0; i < 32; i++) data.key[i] = 33 + i;
    data.siglen = 72;
    secp256k1_ecdsa_sign(data.msg, data.sig, &data.siglen, data.key, NULL, NULL);
    data.pubkeylen = 33;
    CHECK(secp256k1_ec_pubkey_create(data.pubkey, &data.pubkeylen, data.key, 1));

    run_benchmark("ecdsa_verify", benchmark_verify, NULL, NULL, &data, 10, 20000);

    secp256k1_stop();
    return 0;
}
