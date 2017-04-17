/**********************************************************************
 * Copyright (c) 2016 Pieter Wuille                                   *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#include <stdint.h>
#include <string.h>

#include "include/secp256k1_generator.h"
#include "util.h"
#include "bench.h"

typedef struct {
    secp256k1_context* ctx;
    unsigned char key[32];
    unsigned char blind[32];
} bench_generator_t;

static void bench_generator_setup(void* arg) {
    bench_generator_t *data = (bench_generator_t*)arg;
    memset(data->key, 0x31, 32);
    memset(data->blind, 0x13, 32);
}

static void bench_generator_generate(void* arg) {
    int i;
    bench_generator_t *data = (bench_generator_t*)arg;

    for (i = 0; i < 20000; i++) {
        secp256k1_generator gen;
        CHECK(secp256k1_generator_generate(data->ctx, &gen, data->key));
        data->key[i & 31]++;
    }
}

static void bench_generator_generate_blinded(void* arg) {
    int i;
    bench_generator_t *data = (bench_generator_t*)arg;

    for (i = 0; i < 20000; i++) {
        secp256k1_generator gen;
        CHECK(secp256k1_generator_generate_blinded(data->ctx, &gen, data->key, data->blind));
        data->key[1 + (i & 30)]++;
        data->blind[1 + (i & 30)]++;
    }
}

int main(void) {
    bench_generator_t data;

    data.ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);

    run_benchmark("generator_generate", bench_generator_generate, bench_generator_setup, NULL, &data, 10, 20000);
    run_benchmark("generator_generate_blinded", bench_generator_generate_blinded, bench_generator_setup, NULL, &data, 10, 20000);

    secp256k1_context_destroy(data.ctx);
    return 0;
}
