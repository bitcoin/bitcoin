/***********************************************************************
 * Copyright (c) 2024 josibake                                         *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_MODULE_SILENTPAYMENTS_BENCH_H
#define SECP256K1_MODULE_SILENTPAYMENTS_BENCH_H

#include "../../../include/secp256k1_silentpayments.h"

/* upper bound of maximum taproot outputs per block: floor(1_000_000/43)
 * (needed for constructing the "worst-case scanning attack", where a single
 *  tx fills up a full bock of taproot outputs that all go to the same scankey group) */
#define MAX_P2TR_OUTPUTS_PER_BLOCK 23255

#define SP_BENCH_MAX_INPUTS  1
#define SP_BENCH_MAX_OUTPUTS MAX_P2TR_OUTPUTS_PER_BLOCK

typedef struct {
    secp256k1_context *ctx;
    secp256k1_pubkey spend_pubkey;
    unsigned char scan_key[32];
    secp256k1_xonly_pubkey *tx_outputs;
    secp256k1_xonly_pubkey **tx_outputs_ptrs;
    secp256k1_xonly_pubkey tx_inputs[SP_BENCH_MAX_INPUTS];
    const secp256k1_xonly_pubkey *tx_inputs_ptrs[SP_BENCH_MAX_INPUTS];
    secp256k1_silentpayments_found_output *found_outputs;
    secp256k1_silentpayments_found_output **found_outputs_ptrs;
    unsigned char smallest_outpoint[36];
    unsigned char label[33];
    unsigned char label_tweak[32];
    int num_outputs;
    int num_matches;
} bench_silentpayments_data;

const unsigned char* label_lookup(const unsigned char* key, const void* cache_ptr) {
    bench_silentpayments_data *data = (bench_silentpayments_data*)cache_ptr;
    if (secp256k1_memcmp_var(key, data->label, 33) == 0) {
        return data->label_tweak;
    }
    return NULL;
}

static void bench_silentpayments_scan_setup(void* arg) {
    int i;
    bench_silentpayments_data *data = (bench_silentpayments_data*)arg;
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
    unsigned char scalar[32];
    secp256k1_keypair input_keypair;
    size_t pubkeylen = 33;

    for (i = 0; i < 32; i++) {
        scalar[i] = i + 1;
    }
    /* Create the first input public key for the full scan from the scalar. */
    CHECK(secp256k1_keypair_create(data->ctx, &input_keypair, scalar));
    CHECK(secp256k1_keypair_xonly_pub(data->ctx, &data->tx_inputs[0], NULL, &input_keypair));
    data->tx_inputs_ptrs[0] = &data->tx_inputs[0];
    CHECK(secp256k1_ec_pubkey_parse(data->ctx, &data->spend_pubkey, spend_pubkey, pubkeylen));
    memcpy(data->scan_key, scan_key, 32);
    memcpy(data->smallest_outpoint, smallest_outpoint, 36);

    /* prepare transaction outputs for the "worst-case scanning attack",
     * can be used for typical scanning scenarios as well */
    {
        secp256k1_silentpayments_recipient *recipients = malloc(sizeof(secp256k1_silentpayments_recipient) * data->num_outputs);
        const secp256k1_silentpayments_recipient **recipients_ptrs = malloc(sizeof(secp256k1_silentpayments_recipient*) * data->num_outputs);
        const secp256k1_keypair *taproot_keypairs_ptrs[SP_BENCH_MAX_INPUTS];
        secp256k1_pubkey scan_pubkey, other_scan_pubkey;
        unsigned char other_scan_seckey[32] = {99};
        secp256k1_silentpayments_label label;
        secp256k1_pubkey labeled_spend_pubkey;
        int index_first_k;

        CHECK(data->num_outputs <= SP_BENCH_MAX_OUTPUTS);
        CHECK(data->num_matches <= data->num_outputs);
        index_first_k = data->num_outputs - data->num_matches;

        CHECK(secp256k1_ec_pubkey_create(data->ctx, &scan_pubkey, data->scan_key));
        CHECK(secp256k1_ec_pubkey_create(data->ctx, &other_scan_pubkey, other_scan_seckey));

        CHECK(secp256k1_silentpayments_recipient_label_create(data->ctx, &label, data->label_tweak, data->scan_key, 0));
        CHECK(secp256k1_silentpayments_recipient_label_serialize(data->ctx, data->label, &label));
        CHECK(secp256k1_silentpayments_recipient_create_labeled_spend_pubkey(data->ctx,
            &labeled_spend_pubkey, &data->spend_pubkey, &label));

        data->tx_outputs = malloc(sizeof(secp256k1_xonly_pubkey) * data->num_outputs);
        data->tx_outputs_ptrs = malloc(sizeof(secp256k1_xonly_pubkey*) * data->num_outputs);
        data->found_outputs = malloc(sizeof(secp256k1_silentpayments_found_output) * data->num_outputs);
        data->found_outputs_ptrs = malloc(sizeof(secp256k1_silentpayments_found_output*) * data->num_outputs);

        taproot_keypairs_ptrs[0] = &input_keypair;
        for (i = 0; i < data->num_outputs; i++) {
            data->tx_outputs_ptrs[i] = &data->tx_outputs[i];
            recipients_ptrs[i] = &recipients[i];
            recipients[i].spend_pubkey = labeled_spend_pubkey;
            if (i >= index_first_k) {
                recipients[i].scan_pubkey = scan_pubkey;
            } else {
                unsigned char tweak[32] = {0};
                tweak[31] = 1;
                /* tweak non-match scan pubkey in order to create single-recipient groups
                 * (we want to avoid running into the recipient group protocol limit) */
                CHECK(secp256k1_ec_pubkey_tweak_add(data->ctx, &other_scan_pubkey, tweak));
                recipients[i].scan_pubkey = other_scan_pubkey;
            }
            recipients[i].index = i;
        }
        CHECK(secp256k1_silentpayments_sender_create_outputs(data->ctx, data->tx_outputs_ptrs, recipients_ptrs,
            data->num_outputs, data->smallest_outpoint, taproot_keypairs_ptrs, SP_BENCH_MAX_INPUTS, NULL, 0));

        for (i = 0; i < data->num_outputs; i++) {
            data->found_outputs_ptrs[i] = &data->found_outputs[i];
        }
        /* reverse outputs within k group to simulate worst-case */
        for (i = 0; i < data->num_matches / 2; i++) {
            int pos = index_first_k + i;
            secp256k1_xonly_pubkey *tmp = data->tx_outputs_ptrs[pos];
            data->tx_outputs_ptrs[pos] = data->tx_outputs_ptrs[data->num_outputs - i - 1];
            data->tx_outputs_ptrs[data->num_outputs - i - 1] = tmp;
        }

        free(recipients_ptrs);
        free(recipients);
    }
}

static void bench_silentpayments_scan_teardown(void* arg, int iters) {
    bench_silentpayments_data *data = (bench_silentpayments_data*)arg;
    (void)iters;

    free(data->tx_outputs);
    free(data->tx_outputs_ptrs);
    free(data->found_outputs);
    free(data->found_outputs_ptrs);
}

static void bench_silentpayments_scan(void* arg, int iters) {
    bench_silentpayments_data *data = (bench_silentpayments_data*)arg;
    secp256k1_silentpayments_prevouts_summary prevouts_summary;
    uint32_t n_found = 0;
    int i;
    const secp256k1_silentpayments_label_lookup label_lookup_fn = label_lookup;
    const void *label_context = data;

    CHECK(data->num_outputs <= SP_BENCH_MAX_OUTPUTS);

    for (i = 0; i < iters; i++) {
        CHECK(secp256k1_silentpayments_recipient_prevouts_summary_create(data->ctx, &prevouts_summary,
            data->smallest_outpoint, data->tx_inputs_ptrs, SP_BENCH_MAX_INPUTS, NULL, 0));
        CHECK(secp256k1_silentpayments_recipient_scan_outputs(data->ctx,
            data->found_outputs_ptrs, &n_found,
            (const secp256k1_xonly_pubkey**)data->tx_outputs_ptrs, data->num_outputs,
            data->scan_key, &prevouts_summary, &data->spend_pubkey,
            label_lookup_fn, label_context)
        );
        CHECK(n_found == (uint32_t)data->num_matches);
    }
}

static void run_silentpayments_bench(int iters, int argc, char** argv) {
    bench_silentpayments_data data;
    int d = argc == 1;

    data.ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);

    if (d || have_flag(argc, argv, "silentpayments") || have_flag(argc, argv, "silentpayments_scan_nomatch")) {
        const int num_outputs_bench[] = {2, 5, 10, 100, 1000, MAX_P2TR_OUTPUTS_PER_BLOCK/10, MAX_P2TR_OUTPUTS_PER_BLOCK};
        size_t o;
        for (o = 0; o < sizeof(num_outputs_bench)/sizeof(num_outputs_bench[0]); o++) {
            const int num_outputs = num_outputs_bench[o];
            char str[64];
            data.num_outputs = num_outputs;
            data.num_matches = 0;
            sprintf(str, "silentpayments_scan_nomatch_N=%i", num_outputs);
            run_benchmark(str, bench_silentpayments_scan, bench_silentpayments_scan_setup, bench_silentpayments_scan_teardown, &data, 10, num_outputs < 100 ? iters : 1);
        }
    }

    if (d || have_flag(argc, argv, "silentpayments") || have_flag(argc, argv, "silentpayments_scan_worstcase")) {
        size_t k;
        const int num_matches_bench[] = {10, 100, 1000};
        for (k = 0; k < sizeof(num_matches_bench)/sizeof(num_matches_bench[0]); k++) {
            const int num_matches = num_matches_bench[k];
            char str[64];
            data.num_outputs = MAX_P2TR_OUTPUTS_PER_BLOCK;
            data.num_matches = num_matches;
            sprintf(str, "silentpayments_scan_worstcase_K=%i", num_matches);
            run_benchmark(str, bench_silentpayments_scan, bench_silentpayments_scan_setup, bench_silentpayments_scan_teardown, &data, 3, 1);
        }
    }

    secp256k1_context_destroy(data.ctx);
}

#endif /* SECP256K1_MODULE_SILENTPAYMENTS_BENCH_H */
