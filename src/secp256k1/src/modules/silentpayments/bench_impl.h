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
    unsigned char input_pubkey33[33];
    secp256k1_xonly_pubkey tx_outputs[2];
    secp256k1_xonly_pubkey tx_inputs[2];
    secp256k1_silentpayments_found_output found_outputs[2];
    unsigned char scalar[32];
    unsigned char smallest_outpoint[36];
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
    const unsigned char tx_outputs[2][32] = {
        {0x84,0x17,0x92,0xc3,0x3c,0x9d,0xc6,0x19,0x3e,0x76,0x74,0x41,0x34,0x12,0x5d,0x40,0xad,0xd8,0xf2,0xf4,0xa9,0x64,0x75,0xf2,0x8b,0xa1,0x50,0xbe,0x03,0x2d,0x64,0xe8},
        {0x2e,0x84,0x7b,0xb0,0x1d,0x1b,0x49,0x1d,0xa5,0x12,0xdd,0xd7,0x60,0xb8,0x50,0x96,0x17,0xee,0x38,0x05,0x70,0x03,0xd6,0x11,0x5d,0x00,0xba,0x56,0x24,0x51,0x32,0x3a},
    };
    const unsigned char static_tx_input[32] = {
        0xf2,0x07,0x16,0x2b,0x1a,0x7a,0xbc,0x51,
        0xc4,0x20,0x17,0xbe,0xf0,0x55,0xe9,0xec,
        0x1e,0xfc,0x3d,0x35,0x67,0xcb,0x72,0x03,
        0x57,0xe2,0xb8,0x43,0x25,0xdb,0x33,0xac
    };
    const unsigned char smallest_outpoint[36] = {
        0x16, 0x9e, 0x1e, 0x83, 0xe9, 0x30, 0x85, 0x33, 0x91,
        0xbc, 0x6f, 0x35, 0xf6, 0x05, 0xc6, 0x75, 0x4c, 0xfe,
        0xad, 0x57, 0xcf, 0x83, 0x87, 0x63, 0x9d, 0x3b, 0x40,
        0x96, 0xc5, 0x4f, 0x18, 0xf4, 0x00, 0x00, 0x00, 0x00,
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
    size_t pubkeylen = 33;

    for (i = 0; i < 32; i++) {
        data->scalar[i] = i + 1;
    }
    for (i = 0; i < 2; i++) {
        CHECK(secp256k1_xonly_pubkey_parse(data->ctx, &data->tx_outputs[i], tx_outputs[i]));
    }
    /* Create the first input public key from the scalar.
     * This input is also used to create the serialized public data object for the light client
     */
    CHECK(secp256k1_keypair_create(data->ctx, &input_keypair, data->scalar));
    CHECK(secp256k1_keypair_pub(data->ctx, &input_pubkey, &input_keypair));
    CHECK(secp256k1_ec_pubkey_serialize(data->ctx, data->input_pubkey33, &pubkeylen, &input_pubkey, SECP256K1_EC_COMPRESSED));
    /* Create the input public keys for the full scan */
    CHECK(secp256k1_keypair_xonly_pub(data->ctx, &data->tx_inputs[0], NULL, &input_keypair));
    CHECK(secp256k1_xonly_pubkey_parse(data->ctx, &data->tx_inputs[1], static_tx_input));
    CHECK(secp256k1_ec_pubkey_parse(data->ctx, &data->spend_pubkey, spend_pubkey, pubkeylen));
    memcpy(data->scan_key, scan_key, 32);
    memcpy(data->smallest_outpoint, smallest_outpoint, 36);
}

static void bench_silentpayments_output_scan(void* arg, int iters) {
    int i, k = 0;
    bench_silentpayments_data *data = (bench_silentpayments_data*)arg;
    secp256k1_silentpayments_recipient_prevouts_summary prevouts_summary;

    for (i = 0; i < iters; i++) {
        unsigned char shared_secret[33];
        secp256k1_xonly_pubkey xonly_output;
        CHECK(secp256k1_silentpayments_recipient_prevouts_summary_parse(data->ctx, &prevouts_summary, data->input_pubkey33));
        CHECK(secp256k1_silentpayments_recipient_create_shared_secret(data->ctx,
            shared_secret,
            data->scan_key,
            &prevouts_summary
        ));
        CHECK(secp256k1_silentpayments_recipient_create_output_pubkey(data->ctx,
            &xonly_output,
            shared_secret,
            &data->spend_pubkey,
            k
        ));
    }
}

static void bench_silentpayments_full_tx_scan(void* arg, int iters, int use_labels) {
    int i;
    size_t n_found = 0;
    secp256k1_silentpayments_found_output *found_output_ptrs[2];
    const secp256k1_xonly_pubkey *tx_output_ptrs[2];
    const secp256k1_xonly_pubkey *tx_input_ptrs[2];
    bench_silentpayments_data *data = (bench_silentpayments_data*)arg;
    secp256k1_silentpayments_recipient_prevouts_summary prevouts_summary;
    const secp256k1_silentpayments_label_lookup label_lookup_fn = use_labels ? label_lookup : NULL;
    const void *label_context = use_labels ? label_cache : NULL;

    for (i = 0; i < 2; i++) {
        found_output_ptrs[i] = &data->found_outputs[i];
        tx_output_ptrs[i] = &data->tx_outputs[i];
        tx_input_ptrs[i] = &data->tx_inputs[i];
    }
    for (i = 0; i < iters; i++) {
        CHECK(secp256k1_silentpayments_recipient_prevouts_summary_create(data->ctx,
            &prevouts_summary,
            data->smallest_outpoint,
            tx_input_ptrs, 2,
            NULL, 0
        ));
        CHECK(secp256k1_silentpayments_recipient_scan_outputs(data->ctx,
            found_output_ptrs, &n_found,
            tx_output_ptrs, 2,
            data->scan_key,
            &prevouts_summary,
            &data->spend_pubkey,
            label_lookup_fn, label_context)
        );
    }
}

static void bench_silentpayments_full_scan(void *arg, int iters) {
    bench_silentpayments_full_tx_scan(arg, iters, 0);
}

/* TODO: currently, the with_labels benchmark ensures all of the labels code paths
 * are hit during scanning, but should be extended to measure scanning for labels
 * with a sizable labels cache.
 */
static void bench_silentpayments_full_scan_with_labels(void *arg, int iters) {
    bench_silentpayments_full_tx_scan(arg, iters, 1);
}

static void run_silentpayments_bench(int iters, int argc, char** argv) {
    bench_silentpayments_data data;
    int d = argc == 1;

    /* create a context with no capabilities */
    data.ctx = secp256k1_context_create(SECP256K1_FLAGS_TYPE_CONTEXT);

    if (d || have_flag(argc, argv, "silentpayments") || have_flag(argc, argv, "silentpayments_output_scan")) run_benchmark("silentpayments_output_scan", bench_silentpayments_output_scan, bench_silentpayments_scan_setup, NULL, &data, 10, iters);
    if (d || have_flag(argc, argv, "silentpayments") || have_flag(argc, argv, "silentpayments_full_scan")) run_benchmark("silentpayments_full_scan", bench_silentpayments_full_scan, bench_silentpayments_scan_setup, NULL, &data, 10, iters);
    if (d || have_flag(argc, argv, "silentpayments") || have_flag(argc, argv, "silentpayments_full_scan_with_labels")) run_benchmark("silentpayments_full_scan_with_labels", bench_silentpayments_full_scan_with_labels, bench_silentpayments_scan_setup, NULL, &data, 10, iters);

    secp256k1_context_destroy(data.ctx);
}

#endif /* SECP256K1_MODULE_SILENTPAYMENTS_BENCH_H */
