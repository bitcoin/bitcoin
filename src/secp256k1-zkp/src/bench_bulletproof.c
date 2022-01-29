/**********************************************************************
 * Copyright (c) 2017 Andrew Poelstra                                 *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#include <stdint.h>

#include "include/secp256k1_generator.h"
#include "include/secp256k1_commitment.h"
#include "include/secp256k1_bulletproofs.h"
#include "util.h"
#include "bench.h"

#define MAX_PROOF_SIZE	2000

typedef struct {
    secp256k1_context *ctx;
    secp256k1_scratch_space *scratch;
    unsigned char nonce[32];
    unsigned char **proof;
    secp256k1_bulletproof_generators *generators;
    secp256k1_generator *value_gen;
    secp256k1_generator blind_gen;
    size_t n_proofs;
    size_t plen;
    size_t iters;
} bench_bulletproof_t;

typedef struct {
    bench_bulletproof_t *common;
    secp256k1_pedersen_commitment **commit;
    const unsigned char **blind;
    size_t *value;
    size_t n_commits;
    size_t nbits;
} bench_bulletproof_rangeproof_t;

static void bench_bulletproof_common_setup(bench_bulletproof_t *data) {
    size_t i;
    const unsigned char nonce[32] = "my kingdom for some randomness!!";
    const unsigned char genbd[32] = "yet more blinding, for the asset";

    memcpy(data->nonce, nonce, 32);
    data->proof = (unsigned char **)malloc(data->n_proofs * sizeof(*data->proof));
    data->value_gen = (secp256k1_generator *)malloc(data->n_proofs * sizeof(*data->value_gen));
    for (i = 0; i < data->n_proofs; i++) {
        data->proof[i] = (unsigned char *)malloc(MAX_PROOF_SIZE);
        CHECK(secp256k1_generator_generate(data->ctx, &data->value_gen[i], genbd));
    }
    data->plen = MAX_PROOF_SIZE;
}

static void bench_bulletproof_rangeproof_setup(void* arg) {
    bench_bulletproof_rangeproof_t *data = (bench_bulletproof_rangeproof_t*)arg;
    size_t i;
    size_t v;

    unsigned char blind[32] = "and my kingdom too for a blinder";

    bench_bulletproof_common_setup (data->common);

    data->commit = (secp256k1_pedersen_commitment **)malloc(data->common->n_proofs * sizeof(*data->commit));
    data->blind = (const unsigned char **)malloc(data->n_commits * sizeof(*data->blind));
    data->value = (size_t *)malloc(data->n_commits * sizeof(*data->commit));

    for (i = 0; i < data->common->n_proofs; i++) {
        data->commit[i] = (secp256k1_pedersen_commitment *)malloc(data->n_commits * sizeof(*data->commit[i]));
    }

    for (i = 0; i < data->n_commits; i++) {
        data->blind[i] = malloc(32);
        blind[0] = i;
        blind[1] = i >> 8;
        memcpy((unsigned char*) data->blind[i], blind, 32);
        data->value[i] = i * 17;
        CHECK(secp256k1_pedersen_commit(data->common->ctx, &data->commit[0][i], data->blind[i], data->value[i], &data->common->value_gen[0], &data->common->blind_gen));
    }
    for (i = 1; i < data->common->n_proofs; i++) {
        memcpy(data->commit[i], data->commit[0], data->n_commits * sizeof(*data->commit[0]));
    }

    CHECK(secp256k1_bulletproof_rangeproof_prove(data->common->ctx, data->common->scratch, data->common->generators, data->common->proof[0], &data->common->plen, NULL, NULL, NULL, data->value, NULL, data->blind, NULL, data->n_commits, data->common->value_gen, data->nbits, data->common->nonce, NULL, NULL, 0, NULL) == 1);
    for (i = 1; i < data->common->n_proofs; i++) {
        memcpy(data->common->proof[i], data->common->proof[0], data->common->plen);
        CHECK(secp256k1_bulletproof_rangeproof_verify(data->common->ctx, data->common->scratch, data->common->generators, data->common->proof[i], data->common->plen, NULL, data->commit[i], data->n_commits, data->nbits, &data->common->value_gen[0], NULL, 0) == 1);
    }
    CHECK(secp256k1_bulletproof_rangeproof_verify(data->common->ctx, data->common->scratch, data->common->generators, data->common->proof[0], data->common->plen, NULL, data->commit[0], data->n_commits, data->nbits, data->common->value_gen, NULL, 0) == 1);
    CHECK(secp256k1_bulletproof_rangeproof_verify_multi(data->common->ctx, data->common->scratch, data->common->generators, (const unsigned char **) data->common->proof, data->common->n_proofs, data->common->plen, NULL, (const secp256k1_pedersen_commitment **) data->commit, data->n_commits, data->nbits, data->common->value_gen, NULL, 0) == 1);
    if (data->n_commits == 1) {
        CHECK(secp256k1_bulletproof_rangeproof_rewind(data->common->ctx, &v, blind, data->common->proof[0], data->common->plen, 0, data->commit[0], &data->common->value_gen[0], data->common->nonce, NULL, 0, NULL) == 1);
    }
}

static void bench_bulletproof_common_teardown(bench_bulletproof_t *data) {
    size_t i;

    for (i = 0; i < data->n_proofs; i++) {
        free(data->proof[i]);
    }
    free(data->proof);
    free(data->value_gen);
}

static void bench_bulletproof_rangeproof_teardown(void* arg) {
    bench_bulletproof_rangeproof_t *data = (bench_bulletproof_rangeproof_t*)arg;
    size_t i;

    if (data->blind != NULL) {
        for (i = 0; i < data->n_commits; i++) {
            free((unsigned char*) data->blind[i]);
        }
    }
    if (data->commit != NULL) {
        for (i = 0; i < data->common->n_proofs; i++) {
            free(data->commit[i]);
        }
        free(data->commit);
    }
    free(data->blind);
    free(data->value);

    bench_bulletproof_common_teardown(data->common);
}

static void bench_bulletproof_rangeproof_prove(void* arg) {
    bench_bulletproof_rangeproof_t *data = (bench_bulletproof_rangeproof_t*)arg;
    size_t i;
    for (i = 0; i < 25; i++) {
        CHECK(secp256k1_bulletproof_rangeproof_prove(data->common->ctx, data->common->scratch, data->common->generators, data->common->proof[0], &data->common->plen, NULL, NULL, NULL, data->value, NULL, data->blind, NULL, data->n_commits, data->common->value_gen, data->nbits, data->common->nonce, NULL, NULL, 0, NULL) == 1);
    }
}

static void bench_bulletproof_rangeproof_verify(void* arg) {
    size_t i;
    bench_bulletproof_rangeproof_t *data = (bench_bulletproof_rangeproof_t*)arg;

    for (i = 0; i < data->common->iters; i++) {
        CHECK(secp256k1_bulletproof_rangeproof_verify_multi(data->common->ctx, data->common->scratch, data->common->generators, (const unsigned char **) data->common->proof, data->common->n_proofs, data->common->plen, NULL, (const secp256k1_pedersen_commitment **) data->commit, data->n_commits, data->nbits, data->common->value_gen, NULL, 0) == 1);
    }
}

static void bench_bulletproof_rangeproof_rewind_succeed(void* arg) {
    size_t i;
    size_t v;
    unsigned char blind[32];
    bench_bulletproof_rangeproof_t *data = (bench_bulletproof_rangeproof_t*)arg;

    for (i = 0; i < data->common->iters; i++) {
        CHECK(secp256k1_bulletproof_rangeproof_rewind(data->common->ctx, &v, blind, data->common->proof[0], data->common->plen, 0, data->commit[0], &data->common->value_gen[0], data->common->nonce, NULL, 0, NULL) == 1);
    }
}

static void bench_bulletproof_rangeproof_rewind_fail(void* arg) {
    size_t i;
    size_t v;
    unsigned char blind[32];
    bench_bulletproof_rangeproof_t *data = (bench_bulletproof_rangeproof_t*)arg;

    data->common->nonce[0] ^= 1;
    for (i = 0; i < data->common->iters; i++) {
        CHECK(secp256k1_bulletproof_rangeproof_rewind(data->common->ctx, &v, blind, data->common->proof[0], data->common->plen, 0, data->commit[0], &data->common->value_gen[0], data->common->nonce, NULL, 0, NULL) == 0);
    }
    data->common->nonce[0] ^= 1;
}

static void run_rangeproof_test(bench_bulletproof_rangeproof_t *data, size_t nbits, size_t n_commits) {
    char str[64];

    data->nbits = nbits;
    data->n_commits = n_commits;
    data->common->iters = 100;

    data->common->n_proofs = 1;
    sprintf(str, "bulletproof_prove, %i, %i, 0, ", (int)nbits, (int) n_commits);
    run_benchmark(str, bench_bulletproof_rangeproof_prove, bench_bulletproof_rangeproof_setup, bench_bulletproof_rangeproof_teardown, (void *)data, 5, 25);

    data->common->n_proofs = 1;
    sprintf(str, "bulletproof_verify, %i, %i, 1, ", (int)nbits, (int) n_commits);
    run_benchmark(str, bench_bulletproof_rangeproof_verify, bench_bulletproof_rangeproof_setup, bench_bulletproof_rangeproof_teardown, (void *)data, 5, data->common->iters);

    if (n_commits == 1) {
        sprintf(str, "bulletproof_rewind_succeed, %i, ", (int)nbits);
        run_benchmark(str, bench_bulletproof_rangeproof_rewind_succeed, bench_bulletproof_rangeproof_setup, bench_bulletproof_rangeproof_teardown, (void *)data, 5, data->common->iters);
        sprintf(str, "bulletproof_rewind_fail, %i, ", (int)nbits);
        run_benchmark(str, bench_bulletproof_rangeproof_rewind_fail, bench_bulletproof_rangeproof_setup, bench_bulletproof_rangeproof_teardown, (void *)data, 5, data->common->iters);
    }

    data->common->n_proofs = 2;
    sprintf(str, "bulletproof_verify, %i, %i, 2, ", (int)nbits, (int) n_commits);
    run_benchmark(str, bench_bulletproof_rangeproof_verify, bench_bulletproof_rangeproof_setup, bench_bulletproof_rangeproof_teardown, (void *)data, 5, data->common->iters);

    data->common->iters = 10;
    data->common->n_proofs = 50;
    sprintf(str, "bulletproof_verify, %i, %i, 50, ", (int)nbits, (int) n_commits);
    run_benchmark(str, bench_bulletproof_rangeproof_verify, bench_bulletproof_rangeproof_setup, bench_bulletproof_rangeproof_teardown, (void *)data, 5, data->common->iters);

    data->common->iters = 1;
    data->common->n_proofs = 100;
    sprintf(str, "bulletproof_verify, %i, %i, 100, ", (int)nbits, (int) n_commits);
    run_benchmark(str, bench_bulletproof_rangeproof_verify, bench_bulletproof_rangeproof_setup, bench_bulletproof_rangeproof_teardown, (void *)data, 5, data->common->iters);

    data->common->n_proofs = 500;
    sprintf(str, "bulletproof_verify, %i, %i, 500, ", (int)nbits, (int) n_commits);
    run_benchmark(str, bench_bulletproof_rangeproof_verify, bench_bulletproof_rangeproof_setup, bench_bulletproof_rangeproof_teardown, (void *)data, 5, data->common->iters);

    data->common->n_proofs = 1000;
    sprintf(str, "bulletproof_verify, %i, %i, 1000, ", (int)nbits, (int) n_commits);
    run_benchmark(str, bench_bulletproof_rangeproof_verify, bench_bulletproof_rangeproof_setup, bench_bulletproof_rangeproof_teardown, (void *)data, 5, data->common->iters);
}

int main(void) {
    bench_bulletproof_t data;
    bench_bulletproof_rangeproof_t rp_data;

    data.blind_gen = secp256k1_generator_const_g;
    data.ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
    data.scratch = secp256k1_scratch_space_create(data.ctx, 1024 * 1024 * 1024);
    data.generators = secp256k1_bulletproof_generators_create(data.ctx, &data.blind_gen, 64 * 1024);

    rp_data.common = &data;

    run_rangeproof_test(&rp_data, 8, 1);
    run_rangeproof_test(&rp_data, 16, 1);
    run_rangeproof_test(&rp_data, 32, 1);

    run_rangeproof_test(&rp_data, 64, 1);
    run_rangeproof_test(&rp_data, 64, 2);
    run_rangeproof_test(&rp_data, 64, 4);
    run_rangeproof_test(&rp_data, 64, 8);
    run_rangeproof_test(&rp_data, 64, 16);
    run_rangeproof_test(&rp_data, 64, 32);
    run_rangeproof_test(&rp_data, 64, 64);
    run_rangeproof_test(&rp_data, 64, 128);
    run_rangeproof_test(&rp_data, 64, 256);
    run_rangeproof_test(&rp_data, 64, 512);
    /* to add more, increase the number of generators above in `data.generators = ...` */

    secp256k1_bulletproof_generators_destroy(data.ctx, data.generators);
    secp256k1_scratch_space_destroy(data.scratch);
    secp256k1_context_destroy(data.ctx);
    return 0;
}
