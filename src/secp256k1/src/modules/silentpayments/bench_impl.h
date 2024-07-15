/***********************************************************************
 * Copyright (c) 2024 josibake                                         *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_MODULE_SILENTPAYMENTS_BENCH_H
#define SECP256K1_MODULE_SILENTPAYMENTS_BENCH_H

#include "../../../include/secp256k1_silentpayments.h"

typedef struct {
    secp256k1_context *ctx;
    secp256k1_pubkey spend_pubkey;
    unsigned char scan_key[32];
    secp256k1_silentpayments_public_data public_data;
    secp256k1_xonly_pubkey tx_outputs[4];
    secp256k1_silentpayments_found_output found_outputs[4];
    unsigned char scalar[32];
} bench_silentpayments_data;

/* we need a non-null pointer for the cache */
static int noop;
void* label_cache = &noop;
const unsigned char* label_lookup(const unsigned char* key, const void* cache_ptr) {
    (void)key;
    (void)cache_ptr;
    return NULL;
}

static void bench_silentpayments_scan_setup(void* arg) {
    int i;
    bench_silentpayments_data *data = (bench_silentpayments_data*)arg;
    const unsigned char tx_outputs[4][32] = {
        {0x84,0x17,0x92,0xc3,0x3c,0x9d,0xc6,0x19,0x3e,0x76,0x74,0x41,0x34,0x12,0x5d,0x40,0xad,0xd8,0xf2,0xf4,0xa9,0x64,0x75,0xf2,0x8b,0xa1,0x50,0xbe,0x03,0x2d,0x64,0xe8},
        {0x2e,0x84,0x7b,0xb0,0x1d,0x1b,0x49,0x1d,0xa5,0x12,0xdd,0xd7,0x60,0xb8,0x50,0x96,0x17,0xee,0x38,0x05,0x70,0x03,0xd6,0x11,0x5d,0x00,0xba,0x56,0x24,0x51,0x32,0x3a},
        {0xf2,0x07,0x16,0x2b,0x1a,0x7a,0xbc,0x51,0xc4,0x20,0x17,0xbe,0xf0,0x55,0xe9,0xec,0x1e,0xfc,0x3d,0x35,0x67,0xcb,0x72,0x03,0x57,0xe2,0xb8,0x43,0x25,0xdb,0x33,0xac},
        {0xe9,0x76,0xa5,0x8f,0xbd,0x38,0xae,0xb4,0xe6,0x09,0x3d,0x4d,0xf0,0x2e,0x9c,0x1d,0xe0,0xc4,0x51,0x3a,0xe0,0xc5,0x88,0xce,0xf6,0x8c,0xda,0x5b,0x2f,0x88,0x34,0xca}
    };
    const unsigned char spend_pubkey[33] = {
        0x02,0xee,0x97,0xdf,0x83,0xb2,0x54,0x6a,
        0xf5,0xa7,0xd0,0x62,0x15,0xd9,0x8b,0xcb,
        0x63,0x7f,0xe0,0x5d,0xd0,0xfa,0x37,0x3b,
        0xd8,0x20,0xe6,0x64,0xd3,0x72,0xde,0x9a,0x01
    };
    const unsigned char scan_key[32] = {
        0xa8,0x90,0x54,0xc9,0x5b,0xe3,0xc3,0x01,
        0x56,0x65,0x74,0xf2,0xaa,0x93,0xad,0xe0,
        0x51,0x85,0x09,0x03,0xa6,0x9c,0xbd,0xd1,
        0xd4,0x7e,0xae,0x26,0x3d,0x7b,0xc0,0x31
    };
    secp256k1_keypair input_keypair;
    secp256k1_pubkey input_pubkey;
    unsigned char input_pubkey33[33];
    size_t pubkeylen = 33;

    for (i = 0; i < 32; i++) {
        data->scalar[i] = i + 1;
    }
    for (i = 0; i < 4; i++) {
        CHECK(secp256k1_xonly_pubkey_parse(data->ctx, &data->tx_outputs[i], tx_outputs[i]));
    }
    CHECK(secp256k1_keypair_create(data->ctx, &input_keypair, data->scalar));
    CHECK(secp256k1_keypair_pub(data->ctx, &input_pubkey, &input_keypair));
    CHECK(secp256k1_ec_pubkey_serialize(data->ctx, input_pubkey33, &pubkeylen, &input_pubkey, SECP256K1_EC_COMPRESSED));
    CHECK(secp256k1_silentpayments_recipient_public_data_parse(data->ctx, &data->public_data, input_pubkey33));
    CHECK(secp256k1_ec_pubkey_parse(data->ctx, &data->spend_pubkey, spend_pubkey, pubkeylen));
    memcpy(data->scan_key, scan_key, 32);
}

static void bench_silentpayments_scan(void* arg, int iters) {
    int i;
    size_t n_found = 0;
    secp256k1_silentpayments_found_output *found_output_ptrs[4];
    const secp256k1_xonly_pubkey *tx_output_ptrs[4];
    bench_silentpayments_data *data = (bench_silentpayments_data*)arg;

    for (i = 0; i < 4; i++) {
        found_output_ptrs[i] = &data->found_outputs[i];
        tx_output_ptrs[i] = &data->tx_outputs[i];
    }
    for (i = 0; i < iters; i++) {
        CHECK(secp256k1_silentpayments_recipient_scan_outputs(data->ctx,
            found_output_ptrs, &n_found,
            tx_output_ptrs, 4,
            data->scan_key,
            &data->public_data,
            &data->spend_pubkey,
            label_lookup, label_cache)
        );
    }
}

static void run_silentpayments_bench(int iters, int argc, char** argv) {
    bench_silentpayments_data data;
    int d = argc == 1;

    /* create a context with no capabilities */
    data.ctx = secp256k1_context_create(SECP256K1_FLAGS_TYPE_CONTEXT);

    if (d || have_flag(argc, argv, "silentpayments")) run_benchmark("silentpayments", bench_silentpayments_scan, bench_silentpayments_scan_setup, NULL, &data, 10, iters);

    secp256k1_context_destroy(data.ctx);
}

#endif /* SECP256K1_MODULE_SILENTPAYMENTS_BENCH_H */
