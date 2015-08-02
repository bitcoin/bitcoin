/**********************************************************************
 * Copyright (c) 2015 Pieter Wuille, Andrew Poelstra                  *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#include <string.h>

#include "include/secp256k1.h"
#include "include/secp256k1_ecdh.h"
#include "util.h"
#include "bench.h"

typedef struct {
    secp256k1_context_t *ctx;
    secp256k1_pubkey_t point;
    unsigned char scalar[32];
} bench_ecdh_t;

static void bench_ecdh_setup(void* arg) {
    int i;
    bench_ecdh_t *data = (bench_ecdh_t*)arg;
    const unsigned char point[] = {
        0x03,
        0x54, 0x94, 0xc1, 0x5d, 0x32, 0x09, 0x97, 0x06,
        0xc2, 0x39, 0x5f, 0x94, 0x34, 0x87, 0x45, 0xfd,
        0x75, 0x7c, 0xe3, 0x0e, 0x4e, 0x8c, 0x90, 0xfb,
        0xa2, 0xba, 0xd1, 0x84, 0xf8, 0x83, 0xc6, 0x9f
    };

    data->ctx = secp256k1_context_create(0);
    for (i = 0; i < 32; i++) data->scalar[i] = i + 1;
    CHECK(secp256k1_ec_pubkey_parse(data->ctx, &data->point, point, sizeof(point)) == 1);
}

static void bench_ecdh(void* arg) {
    int i;
    unsigned char res[32];
    bench_ecdh_t *data = (bench_ecdh_t*)arg;

    for (i = 0; i < 20000; i++) {
        CHECK(secp256k1_ecdh(data->ctx, res, &data->point, data->scalar) == 1);
    }
}

int main(void) {
    bench_ecdh_t data;

    run_benchmark("ecdh", bench_ecdh, bench_ecdh_setup, NULL, &data, 10, 20000);
    return 0;
}
