/**********************************************************************
 * Copyright (c) 2017 Jonas Nick                                      *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/
#include <stdio.h>

#include "include/secp256k1.h"

#include "include/secp256k1_whitelist.h"
#include "bench.h"
#include "util.h"
#include "hash_impl.h"
#include "num_impl.h"
#include "scalar_impl.h"
#include "testrand_impl.h"

#define MAX_N_KEYS 30

typedef struct {
    secp256k1_context* ctx;
    unsigned char online_seckey[MAX_N_KEYS][32];
    unsigned char summed_seckey[MAX_N_KEYS][32];
    secp256k1_pubkey online_pubkeys[MAX_N_KEYS];
    secp256k1_pubkey offline_pubkeys[MAX_N_KEYS];
    unsigned char csub[32];
    secp256k1_pubkey sub_pubkey;
    secp256k1_whitelist_signature sig;
    size_t n_keys;
} bench_data;

static void bench_whitelist(void* arg) {
    bench_data* data = (bench_data*)arg;
    CHECK(secp256k1_whitelist_verify(data->ctx, &data->sig, data->online_pubkeys, data->offline_pubkeys, data->n_keys, &data->sub_pubkey) == 1);
}

static void bench_whitelist_setup(void* arg) {
    bench_data* data = (bench_data*)arg;
    int i = 0;
    CHECK(secp256k1_whitelist_sign(data->ctx, &data->sig, data->online_pubkeys, data->offline_pubkeys, data->n_keys, &data->sub_pubkey, data->online_seckey[i], data->summed_seckey[i], i, NULL, NULL));
}

static void run_test(bench_data* data) {
    char str[32];
    sprintf(str, "whitelist_%i", (int)data->n_keys);
    run_benchmark(str, bench_whitelist, bench_whitelist_setup, NULL, data, 100, 1);
}

void random_scalar_order(secp256k1_scalar *num) {
    do {
        unsigned char b32[32];
        int overflow = 0;
        secp256k1_rand256(b32);
        secp256k1_scalar_set_b32(num, b32, &overflow);
        if (overflow || secp256k1_scalar_is_zero(num)) {
            continue;
        }
        break;
    } while(1);
}

int main(void) {
    bench_data data;
    size_t i;
    size_t n_keys = 30;
    secp256k1_scalar ssub;

    data.ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);

    /* Start with subkey */
    random_scalar_order(&ssub);
    secp256k1_scalar_get_b32(data.csub, &ssub);
    CHECK(secp256k1_ec_seckey_verify(data.ctx, data.csub) == 1);
    CHECK(secp256k1_ec_pubkey_create(data.ctx, &data.sub_pubkey, data.csub) == 1);
    /* Then offline and online whitelist keys */
    for (i = 0; i < n_keys; i++) {
        secp256k1_scalar son, soff;

        /* Create two keys */
        random_scalar_order(&son);
        secp256k1_scalar_get_b32(data.online_seckey[i], &son);
        CHECK(secp256k1_ec_seckey_verify(data.ctx, data.online_seckey[i]) == 1);
        CHECK(secp256k1_ec_pubkey_create(data.ctx, &data.online_pubkeys[i], data.online_seckey[i]) == 1);

        random_scalar_order(&soff);
        secp256k1_scalar_get_b32(data.summed_seckey[i], &soff);
        CHECK(secp256k1_ec_seckey_verify(data.ctx, data.summed_seckey[i]) == 1);
        CHECK(secp256k1_ec_pubkey_create(data.ctx, &data.offline_pubkeys[i], data.summed_seckey[i]) == 1);

        /* Make summed_seckey correspond to the sum of offline_pubkey and sub_pubkey */
        secp256k1_scalar_add(&soff, &soff, &ssub);
        secp256k1_scalar_get_b32(data.summed_seckey[i], &soff);
        CHECK(secp256k1_ec_seckey_verify(data.ctx, data.summed_seckey[i]) == 1);
    }

    /* Run test */
    for (i = 1; i <= n_keys; ++i) {
        data.n_keys = i;
        run_test(&data);
    }

    secp256k1_context_destroy(data.ctx);
    return(0);
}
