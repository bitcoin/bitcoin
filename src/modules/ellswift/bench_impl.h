/***********************************************************************
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_MODULE_ELLSWIFT_BENCH_H
#define SECP256K1_MODULE_ELLSWIFT_BENCH_H

#include "../../../include/secp256k1_ellswift.h"

typedef struct {
    secp256k1_context *ctx;
    secp256k1_pubkey point[256];
    unsigned char rnd64[64];
} bench_ellswift_data;

static void bench_ellswift_setup(void *arg) {
    int i;
    bench_ellswift_data *data = (bench_ellswift_data*)arg;
    static const unsigned char init[64] = {
        0x78, 0x1f, 0xb7, 0xd4, 0x67, 0x7f, 0x08, 0x68,
        0xdb, 0xe3, 0x1d, 0x7f, 0x1b, 0xb0, 0xf6, 0x9e,
        0x0a, 0x64, 0xca, 0x32, 0x9e, 0xc6, 0x20, 0x79,
        0x03, 0xf3, 0xd0, 0x46, 0x7a, 0x0f, 0xd2, 0x21,
        0xb0, 0x2c, 0x46, 0xd8, 0xba, 0xca, 0x26, 0x4f,
        0x8f, 0x8c, 0xd4, 0xdd, 0x2d, 0x04, 0xbe, 0x30,
        0x48, 0x51, 0x1e, 0xd4, 0x16, 0xfd, 0x42, 0x85,
        0x62, 0xc9, 0x02, 0xf9, 0x89, 0x84, 0xff, 0xdc
    };
    memcpy(data->rnd64, init, 64);
    for (i = 0; i < 256; ++i) {
        int j;
        CHECK(secp256k1_ellswift_decode(data->ctx, &data->point[i], data->rnd64));
        for (j = 0; j < 64; ++j) {
            data->rnd64[j] += 1;
        }
    }
    CHECK(secp256k1_ellswift_encode(data->ctx, data->rnd64, &data->point[255], init + 16));
}

static void bench_ellswift_encode(void *arg, int iters) {
    int i;
    bench_ellswift_data *data = (bench_ellswift_data*)arg;

    for (i = 0; i < iters; i++) {
        CHECK(secp256k1_ellswift_encode(data->ctx, data->rnd64, &data->point[i & 255], data->rnd64 + 16));
    }
}

static void bench_ellswift_create(void *arg, int iters) {
    int i;
    bench_ellswift_data *data = (bench_ellswift_data*)arg;

    for (i = 0; i < iters; i++) {
        unsigned char buf[64];
        CHECK(secp256k1_ellswift_create(data->ctx, buf, data->rnd64, data->rnd64 + 32));
        memcpy(data->rnd64, buf, 64);
    }
}

static void bench_ellswift_decode(void *arg, int iters) {
    int i;
    secp256k1_pubkey out;
    size_t len;
    bench_ellswift_data *data = (bench_ellswift_data*)arg;

    for (i = 0; i < iters; i++) {
        CHECK(secp256k1_ellswift_decode(data->ctx, &out, data->rnd64) == 1);
        len = 33;
        CHECK(secp256k1_ec_pubkey_serialize(data->ctx, data->rnd64 + (i % 32), &len, &out, SECP256K1_EC_COMPRESSED));
    }
}

static void bench_ellswift_xdh(void *arg, int iters) {
    int i;
    bench_ellswift_data *data = (bench_ellswift_data*)arg;

    for (i = 0; i < iters; i++) {
        int party = i & 1;
        CHECK(secp256k1_ellswift_xdh(data->ctx,
                                     data->rnd64 + (i % 33),
                                     data->rnd64,
                                     data->rnd64,
                                     data->rnd64 + ((i + 16) % 33),
                                     party,
                                     secp256k1_ellswift_xdh_hash_function_bip324,
                                     NULL) == 1);
    }
}

void run_ellswift_bench(int iters, int argc, char **argv) {
    bench_ellswift_data data;
    int d = argc == 1;

    /* create a context with signing capabilities */
    data.ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);

    if (d || have_flag(argc, argv, "ellswift") || have_flag(argc, argv, "encode") || have_flag(argc, argv, "ellswift_encode")) run_benchmark("ellswift_encode", bench_ellswift_encode, bench_ellswift_setup, NULL, &data, 10, iters);
    if (d || have_flag(argc, argv, "ellswift") || have_flag(argc, argv, "decode") || have_flag(argc, argv, "ellswift_decode")) run_benchmark("ellswift_decode", bench_ellswift_decode, bench_ellswift_setup, NULL, &data, 10, iters);
    if (d || have_flag(argc, argv, "ellswift") || have_flag(argc, argv, "keygen") || have_flag(argc, argv, "ellswift_keygen")) run_benchmark("ellswift_keygen", bench_ellswift_create, bench_ellswift_setup, NULL, &data, 10, iters);
    if (d || have_flag(argc, argv, "ellswift") || have_flag(argc, argv, "ecdh") || have_flag(argc, argv, "ellswift_ecdh")) run_benchmark("ellswift_ecdh", bench_ellswift_xdh, bench_ellswift_setup, NULL, &data, 10, iters);

    secp256k1_context_destroy(data.ctx);
}

#endif
