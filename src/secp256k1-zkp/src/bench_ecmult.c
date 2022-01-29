/**********************************************************************
 * Copyright (c) 2017 Pieter Wuille                                   *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/
#include <stdio.h>

#include "include/secp256k1.h"

#include "util.h"
#include "hash_impl.h"
#include "num_impl.h"
#include "field_impl.h"
#include "group_impl.h"
#include "scalar_impl.h"
#include "ecmult_impl.h"
#include "bench.h"
#include "secp256k1.c"

#define POINTS 32768
#define ITERS 10000

typedef struct {
    /* Setup once in advance */
    secp256k1_context* ctx;
    secp256k1_scratch_space* scratch;
    secp256k1_scalar* scalars;
    secp256k1_ge* pubkeys;
    secp256k1_scalar* seckeys;
    secp256k1_gej* expected_output;
    secp256k1_ecmult_multi_func ecmult_multi;

    /* Changes per test */
    size_t count;
    int includes_g;

    /* Changes per test iteration */
    size_t offset1;
    size_t offset2;

    /* Test output. */
    secp256k1_gej* output;
} bench_data;

static int bench_callback(secp256k1_scalar* sc, secp256k1_ge* ge, size_t idx, void* arg) {
    bench_data* data = (bench_data*)arg;
    if (data->includes_g) ++idx;
    if (idx == 0) {
        *sc = data->scalars[data->offset1];
        *ge = secp256k1_ge_const_g;
    } else {
        *sc = data->scalars[(data->offset1 + idx) % POINTS];
        *ge = data->pubkeys[(data->offset2 + idx - 1) % POINTS];
    }
    return 1;
}

static void bench_ecmult(void* arg) {
    bench_data* data = (bench_data*)arg;

    size_t count = data->count;
    int includes_g = data->includes_g;
    size_t iters = 1 + ITERS / count;
    size_t iter;

    for (iter = 0; iter < iters; ++iter) {
        data->ecmult_multi(&data->ctx->ecmult_ctx, data->scratch, &data->output[iter], data->includes_g ? &data->scalars[data->offset1] : NULL, bench_callback, arg, count - includes_g);
        data->offset1 = (data->offset1 + count) % POINTS;
        data->offset2 = (data->offset2 + count - 1) % POINTS;
    }
}

static void bench_ecmult_setup(void* arg) {
    bench_data* data = (bench_data*)arg;
    data->offset1 = (data->count * 0x537b7f6f + 0x8f66a481) % POINTS;
    data->offset2 = (data->count * 0x7f6f537b + 0x6a1a8f49) % POINTS;
}

static void bench_ecmult_teardown(void* arg) {
    bench_data* data = (bench_data*)arg;
    size_t iters = 1 + ITERS / data->count;
    size_t iter;
    /* Verify the results in teardown, to avoid doing comparisons while benchmarking. */
    for (iter = 0; iter < iters; ++iter) {
        secp256k1_gej tmp;
        secp256k1_gej_add_var(&tmp, &data->output[iter], &data->expected_output[iter], NULL);
        CHECK(secp256k1_gej_is_infinity(&tmp));
    }
}

static void generate_scalar(uint32_t num, secp256k1_scalar* scalar) {
    secp256k1_sha256 sha256;
    unsigned char c[11] = {'e', 'c', 'm', 'u', 'l', 't', 0, 0, 0, 0};
    unsigned char buf[32];
    int overflow = 0;
    c[6] = num;
    c[7] = num >> 8;
    c[8] = num >> 16;
    c[9] = num >> 24;
    secp256k1_sha256_initialize(&sha256);
    secp256k1_sha256_write(&sha256, c, sizeof(c));
    secp256k1_sha256_finalize(&sha256, buf);
    secp256k1_scalar_set_b32(scalar, buf, &overflow);
    CHECK(!overflow);
}

static void run_test(bench_data* data, size_t count, int includes_g) {
    char str[32];
    static const secp256k1_scalar zero = SECP256K1_SCALAR_CONST(0, 0, 0, 0, 0, 0, 0, 0);
    size_t iters = 1 + ITERS / count;
    size_t iter;

    data->count = count;
    data->includes_g = includes_g;

    /* Compute (the negation of) the expected results directly. */
    data->offset1 = (data->count * 0x537b7f6f + 0x8f66a481) % POINTS;
    data->offset2 = (data->count * 0x7f6f537b + 0x6a1a8f49) % POINTS;
    for (iter = 0; iter < iters; ++iter) {
        secp256k1_scalar tmp;
        secp256k1_scalar total = data->scalars[(data->offset1++) % POINTS];
        size_t i = 0;
        for (i = 0; i + 1 < count; ++i) {
            secp256k1_scalar_mul(&tmp, &data->seckeys[(data->offset2++) % POINTS], &data->scalars[(data->offset1++) % POINTS]);
            secp256k1_scalar_add(&total, &total, &tmp);
        }
        secp256k1_scalar_negate(&total, &total);
        secp256k1_ecmult(&data->ctx->ecmult_ctx, &data->expected_output[iter], NULL, &zero, &total);
    }

    /* Run the benchmark. */
    sprintf(str, includes_g ? "ecmult_%ig" : "ecmult_%i", (int)count);
    run_benchmark(str, bench_ecmult, bench_ecmult_setup, bench_ecmult_teardown, data, 10, count * (1 + ITERS / count));
}

int main(int argc, char **argv) {
    bench_data data;
    int i, p;
    secp256k1_gej* pubkeys_gej;
    size_t scratch_size;

    if (argc > 1) {
        if(have_flag(argc, argv, "pippenger_wnaf")) {
            printf("Using pippenger_wnaf:\n");
            data.ecmult_multi = secp256k1_ecmult_pippenger_batch_single;
        } else if(have_flag(argc, argv, "strauss_wnaf")) {
            printf("Using strauss_wnaf:\n");
            data.ecmult_multi = secp256k1_ecmult_strauss_batch_single;
        }
    } else {
        data.ecmult_multi = secp256k1_ecmult_multi_var;
    }

    /* Allocate stuff */
    data.ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
    scratch_size = secp256k1_strauss_scratch_size(POINTS) + STRAUSS_SCRATCH_OBJECTS*16;
    data.scratch = secp256k1_scratch_space_create(data.ctx, scratch_size);
    data.scalars = malloc(sizeof(secp256k1_scalar) * POINTS);
    data.seckeys = malloc(sizeof(secp256k1_scalar) * POINTS);
    data.pubkeys = malloc(sizeof(secp256k1_ge) * POINTS);
    data.expected_output = malloc(sizeof(secp256k1_gej) * (ITERS + 1));
    data.output = malloc(sizeof(secp256k1_gej) * (ITERS + 1));

    /* Generate a set of scalars, and private/public keypairs. */
    pubkeys_gej = malloc(sizeof(secp256k1_gej) * POINTS);
    secp256k1_gej_set_ge(&pubkeys_gej[0], &secp256k1_ge_const_g);
    secp256k1_scalar_set_int(&data.seckeys[0], 1);
    for (i = 0; i < POINTS; ++i) {
        generate_scalar(i, &data.scalars[i]);
        if (i) {
            secp256k1_gej_double_var(&pubkeys_gej[i], &pubkeys_gej[i - 1], NULL);
            secp256k1_scalar_add(&data.seckeys[i], &data.seckeys[i - 1], &data.seckeys[i - 1]);
        }
    }
    secp256k1_ge_set_all_gej_var(data.pubkeys, pubkeys_gej, POINTS, &data.ctx->error_callback);
    free(pubkeys_gej);

    for (i = 1; i <= 8; ++i) {
        run_test(&data, i, 1);
    }

    for (p = 0; p <= 11; ++p) {
        for (i = 9; i <= 16; ++i) {
            run_test(&data, i << p, 1);
        }
    }
    secp256k1_context_destroy(data.ctx);
    secp256k1_scratch_space_destroy(data.scratch);
    free(data.scalars);
    free(data.pubkeys);
    free(data.seckeys);
    free(data.output);
    free(data.expected_output);

    return(0);
}
