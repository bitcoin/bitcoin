
#ifndef SECP256K1_MODULE_EXTRAKEYS_BENCH_H
#define SECP256K1_MODULE_EXTRAKEYS_BENCH_H

#include "../../../include/secp256k1_extrakeys.h"
#ifdef ENABLE_MODULE_BATCH
# include "../../../include/secp256k1_batch.h"
# include "../../../include/secp256k1_tweak_check_batch.h"
#endif

typedef struct {
    secp256k1_context *ctx;
#ifdef ENABLE_MODULE_BATCH
    secp256k1_batch *batch;
    /* number of tweak checks to batch verify.
     * it varies from 1 to iters with 20% increments */
    int n;
#endif

    const secp256k1_keypair **keypairs;
    const unsigned char **pks;
    const unsigned char **tweaked_pks;
    const int **tweaked_pk_parities;
    const unsigned char **tweaks;
} bench_tweak_check_data;

void bench_xonly_pubkey_tweak_add_check(void* arg, int iters) {
    bench_tweak_check_data *data = (bench_tweak_check_data *)arg;
    int i;

    for (i = 0; i < iters; i++) {
        secp256k1_xonly_pubkey pk;
        CHECK(secp256k1_xonly_pubkey_parse(data->ctx, &pk, data->pks[i]) == 1);
        CHECK(secp256k1_xonly_pubkey_tweak_add_check(data->ctx, data->tweaked_pks[i], *data->tweaked_pk_parities[i], &pk, data->tweaks[i]) == 1);
    }
}

#ifdef ENABLE_MODULE_BATCH
void bench_xonly_pubkey_tweak_add_check_n(void* arg, int iters) {
    bench_tweak_check_data *data = (bench_tweak_check_data *)arg;
    int i, j;

    for (j = 0; j < iters/data->n; j++) {
        for (i = 0; i < data->n; i++) {
            secp256k1_xonly_pubkey pk;
            CHECK(secp256k1_xonly_pubkey_parse(data->ctx, &pk, data->pks[j+i]) == 1);
            CHECK(secp256k1_batch_usable(data->ctx, data->batch) == 1);
            CHECK(secp256k1_batch_add_xonlypub_tweak_check(data->ctx, data->batch, data->tweaked_pks[j+i], *data->tweaked_pk_parities[j+i], &pk, data->tweaks[j+i]) == 1);
        }
        CHECK(secp256k1_batch_verify(data->ctx, data->batch) == 1);
    }
}
#endif

void run_extrakeys_bench(int iters, int argc, char** argv) {
    int i;
    bench_tweak_check_data data;
    int d = argc == 1;

    data.ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
    data.keypairs = (const secp256k1_keypair **)malloc(iters * sizeof(secp256k1_keypair *));
    data.pks = (const unsigned char **)malloc(iters * sizeof(unsigned char *));
    data.tweaked_pks = (const unsigned char **)malloc(iters * sizeof(unsigned char *));
    data.tweaked_pk_parities = (const int **)malloc(iters * sizeof(int *));
    data.tweaks = (const unsigned char **)malloc(iters * sizeof(unsigned char *));
#ifdef ENABLE_MODULE_BATCH
    data.batch = secp256k1_batch_create(data.ctx, iters, NULL);
    CHECK(data.batch != NULL);
#endif

    for (i = 0; i < iters; i++) {
        unsigned char sk[32];
        unsigned char *tweaked_pk_char = (unsigned char *)malloc(32);
        int *tweaked_pk_parity = (int *)malloc(sizeof(int)); /*todo: use sizeof(*twk_parity) instead?*/
        unsigned char *tweak = (unsigned char *)malloc(32);
        secp256k1_keypair *keypair = (secp256k1_keypair *)malloc(sizeof(*keypair));
        unsigned char *pk_char = (unsigned char *)malloc(32);
        secp256k1_xonly_pubkey pk;
        secp256k1_pubkey output_pk;
        secp256k1_xonly_pubkey output_pk_xonly;
        tweak[0] = sk[0] = i;
        tweak[1] = sk[1] = i >> 8;
        tweak[2] = sk[2] = i >> 16;
        tweak[3] = sk[3] = i >> 24;
        memset(&tweak[4], 't', 28);
        memset(&sk[4], 's', 28);

        data.keypairs[i] = keypair;
        data.pks[i] = pk_char;
        data.tweaked_pks[i] = tweaked_pk_char;
        data.tweaked_pk_parities[i] = tweaked_pk_parity;
        data.tweaks[i] = tweak;

        CHECK(secp256k1_keypair_create(data.ctx, keypair, sk));
        CHECK(secp256k1_keypair_xonly_pub(data.ctx, &pk, NULL, keypair));
        CHECK(secp256k1_xonly_pubkey_tweak_add(data.ctx, &output_pk, &pk, tweak));
        CHECK(secp256k1_xonly_pubkey_from_pubkey(data.ctx, &output_pk_xonly, tweaked_pk_parity, &output_pk));
        CHECK(secp256k1_xonly_pubkey_serialize(data.ctx, tweaked_pk_char, &output_pk_xonly) == 1);
        CHECK(secp256k1_xonly_pubkey_serialize(data.ctx, pk_char, &pk) == 1);
    }

    if (d || have_flag(argc, argv, "extrakeys") || have_flag(argc, argv, "tweak_add_check")) run_benchmark("tweak_add_check", bench_xonly_pubkey_tweak_add_check, NULL, NULL, (void *) &data, 10, iters);
#ifdef ENABLE_MODULE_BATCH
    if (d || have_flag(argc, argv, "extrakeys") || have_flag(argc, argv, "batch_verify") || have_flag(argc, argv, "tweak_check_batch_verify")) {
        for (i = 1; i <= iters; i = (int)(i*1.2 + 1)) {
            char name[64];
            int divisible_iters;
            sprintf(name, "tweak_check_batch_verify_%d", (int) i);

            data.n = i;
            divisible_iters = iters - (iters % data.n);
            run_benchmark(name, bench_xonly_pubkey_tweak_add_check_n, NULL, NULL, (void *) &data, 3, divisible_iters);
            fflush(stdout);
        }
    }
#endif

    for (i = 0; i < iters; i++) {
        free((void *)data.keypairs[i]);
        free((void *)data.pks[i]);
        free((void *)data.tweaked_pks[i]);
        free((void *)data.tweaked_pk_parities[i]);
        free((void *)data.tweaks[i]);
    }

    /* Casting to (void *) avoids a stupid warning in MSVC. */
    free((void *)data.keypairs);
    free((void *)data.pks);
    free((void *)data.tweaked_pks);
    free((void *)data.tweaked_pk_parities);
    free((void *)data.tweaks);

#ifdef ENABLE_MODULE_BATCH
    secp256k1_batch_destroy(data.ctx, data.batch);
#endif
    secp256k1_context_destroy(data.ctx);
}

#endif /* SECP256K1_MODULE_EXTRAKEYS_BENCH_H */
