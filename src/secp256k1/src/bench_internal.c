/***********************************************************************
 * Copyright (c) 2014-2015 Pieter Wuille                               *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/
#include <stdio.h>

#include "secp256k1.c"
#include "../include/secp256k1.h"

#include "assumptions.h"
#include "util.h"
#include "hash_impl.h"
#include "field_impl.h"
#include "group_impl.h"
#include "scalar_impl.h"
#include "ecmult_const_impl.h"
#include "ecmult_impl.h"
#include "bench.h"

typedef struct {
    secp256k1_scalar scalar[2];
    secp256k1_fe fe[4];
    secp256k1_ge ge[2];
    secp256k1_gej gej[2];
    unsigned char data[64];
    int wnaf[256];
} bench_inv;

void bench_setup(void* arg) {
    bench_inv *data = (bench_inv*)arg;

    static const unsigned char init[4][32] = {
        /* Initializer for scalar[0], fe[0], first half of data, the X coordinate of ge[0],
           and the (implied affine) X coordinate of gej[0]. */
        {
            0x02, 0x03, 0x05, 0x07, 0x0b, 0x0d, 0x11, 0x13,
            0x17, 0x1d, 0x1f, 0x25, 0x29, 0x2b, 0x2f, 0x35,
            0x3b, 0x3d, 0x43, 0x47, 0x49, 0x4f, 0x53, 0x59,
            0x61, 0x65, 0x67, 0x6b, 0x6d, 0x71, 0x7f, 0x83
        },
        /* Initializer for scalar[1], fe[1], first half of data, the X coordinate of ge[1],
           and the (implied affine) X coordinate of gej[1]. */
        {
            0x82, 0x83, 0x85, 0x87, 0x8b, 0x8d, 0x81, 0x83,
            0x97, 0xad, 0xaf, 0xb5, 0xb9, 0xbb, 0xbf, 0xc5,
            0xdb, 0xdd, 0xe3, 0xe7, 0xe9, 0xef, 0xf3, 0xf9,
            0x11, 0x15, 0x17, 0x1b, 0x1d, 0xb1, 0xbf, 0xd3
        },
        /* Initializer for fe[2] and the Z coordinate of gej[0]. */
        {
            0x3d, 0x2d, 0xef, 0xf4, 0x25, 0x98, 0x4f, 0x5d,
            0xe2, 0xca, 0x5f, 0x41, 0x3f, 0x3f, 0xce, 0x44,
            0xaa, 0x2c, 0x53, 0x8a, 0xc6, 0x59, 0x1f, 0x38,
            0x38, 0x23, 0xe4, 0x11, 0x27, 0xc6, 0xa0, 0xe7
        },
        /* Initializer for fe[3] and the Z coordinate of gej[1]. */
        {
            0xbd, 0x21, 0xa5, 0xe1, 0x13, 0x50, 0x73, 0x2e,
            0x52, 0x98, 0xc8, 0x9e, 0xab, 0x00, 0xa2, 0x68,
            0x43, 0xf5, 0xd7, 0x49, 0x80, 0x72, 0xa7, 0xf3,
            0xd7, 0x60, 0xe6, 0xab, 0x90, 0x92, 0xdf, 0xc5
        }
    };

    secp256k1_scalar_set_b32(&data->scalar[0], init[0], NULL);
    secp256k1_scalar_set_b32(&data->scalar[1], init[1], NULL);
    secp256k1_fe_set_b32(&data->fe[0], init[0]);
    secp256k1_fe_set_b32(&data->fe[1], init[1]);
    secp256k1_fe_set_b32(&data->fe[2], init[2]);
    secp256k1_fe_set_b32(&data->fe[3], init[3]);
    CHECK(secp256k1_ge_set_xo_var(&data->ge[0], &data->fe[0], 0));
    CHECK(secp256k1_ge_set_xo_var(&data->ge[1], &data->fe[1], 1));
    secp256k1_gej_set_ge(&data->gej[0], &data->ge[0]);
    secp256k1_gej_rescale(&data->gej[0], &data->fe[2]);
    secp256k1_gej_set_ge(&data->gej[1], &data->ge[1]);
    secp256k1_gej_rescale(&data->gej[1], &data->fe[3]);
    memcpy(data->data, init[0], 32);
    memcpy(data->data + 32, init[1], 32);
}

void bench_scalar_add(void* arg, int iters) {
    int i, j = 0;
    bench_inv *data = (bench_inv*)arg;

    for (i = 0; i < iters; i++) {
        j += secp256k1_scalar_add(&data->scalar[0], &data->scalar[0], &data->scalar[1]);
    }
    CHECK(j <= iters);
}

void bench_scalar_negate(void* arg, int iters) {
    int i;
    bench_inv *data = (bench_inv*)arg;

    for (i = 0; i < iters; i++) {
        secp256k1_scalar_negate(&data->scalar[0], &data->scalar[0]);
    }
}

void bench_scalar_mul(void* arg, int iters) {
    int i;
    bench_inv *data = (bench_inv*)arg;

    for (i = 0; i < iters; i++) {
        secp256k1_scalar_mul(&data->scalar[0], &data->scalar[0], &data->scalar[1]);
    }
}

void bench_scalar_split(void* arg, int iters) {
    int i, j = 0;
    bench_inv *data = (bench_inv*)arg;

    for (i = 0; i < iters; i++) {
        secp256k1_scalar_split_lambda(&data->scalar[0], &data->scalar[1], &data->scalar[0]);
        j += secp256k1_scalar_add(&data->scalar[0], &data->scalar[0], &data->scalar[1]);
    }
    CHECK(j <= iters);
}

void bench_scalar_inverse(void* arg, int iters) {
    int i, j = 0;
    bench_inv *data = (bench_inv*)arg;

    for (i = 0; i < iters; i++) {
        secp256k1_scalar_inverse(&data->scalar[0], &data->scalar[0]);
        j += secp256k1_scalar_add(&data->scalar[0], &data->scalar[0], &data->scalar[1]);
    }
    CHECK(j <= iters);
}

void bench_scalar_inverse_var(void* arg, int iters) {
    int i, j = 0;
    bench_inv *data = (bench_inv*)arg;

    for (i = 0; i < iters; i++) {
        secp256k1_scalar_inverse_var(&data->scalar[0], &data->scalar[0]);
        j += secp256k1_scalar_add(&data->scalar[0], &data->scalar[0], &data->scalar[1]);
    }
    CHECK(j <= iters);
}

void bench_field_half(void* arg, int iters) {
    int i;
    bench_inv *data = (bench_inv*)arg;

    for (i = 0; i < iters; i++) {
        secp256k1_fe_half(&data->fe[0]);
    }
}

void bench_field_normalize(void* arg, int iters) {
    int i;
    bench_inv *data = (bench_inv*)arg;

    for (i = 0; i < iters; i++) {
        secp256k1_fe_normalize(&data->fe[0]);
    }
}

void bench_field_normalize_weak(void* arg, int iters) {
    int i;
    bench_inv *data = (bench_inv*)arg;

    for (i = 0; i < iters; i++) {
        secp256k1_fe_normalize_weak(&data->fe[0]);
    }
}

void bench_field_mul(void* arg, int iters) {
    int i;
    bench_inv *data = (bench_inv*)arg;

    for (i = 0; i < iters; i++) {
        secp256k1_fe_mul(&data->fe[0], &data->fe[0], &data->fe[1]);
    }
}

void bench_field_sqr(void* arg, int iters) {
    int i;
    bench_inv *data = (bench_inv*)arg;

    for (i = 0; i < iters; i++) {
        secp256k1_fe_sqr(&data->fe[0], &data->fe[0]);
    }
}

void bench_field_inverse(void* arg, int iters) {
    int i;
    bench_inv *data = (bench_inv*)arg;

    for (i = 0; i < iters; i++) {
        secp256k1_fe_inv(&data->fe[0], &data->fe[0]);
        secp256k1_fe_add(&data->fe[0], &data->fe[1]);
    }
}

void bench_field_inverse_var(void* arg, int iters) {
    int i;
    bench_inv *data = (bench_inv*)arg;

    for (i = 0; i < iters; i++) {
        secp256k1_fe_inv_var(&data->fe[0], &data->fe[0]);
        secp256k1_fe_add(&data->fe[0], &data->fe[1]);
    }
}

void bench_field_sqrt(void* arg, int iters) {
    int i, j = 0;
    bench_inv *data = (bench_inv*)arg;
    secp256k1_fe t;

    for (i = 0; i < iters; i++) {
        t = data->fe[0];
        j += secp256k1_fe_sqrt(&data->fe[0], &t);
        secp256k1_fe_add(&data->fe[0], &data->fe[1]);
    }
    CHECK(j <= iters);
}

void bench_group_double_var(void* arg, int iters) {
    int i;
    bench_inv *data = (bench_inv*)arg;

    for (i = 0; i < iters; i++) {
        secp256k1_gej_double_var(&data->gej[0], &data->gej[0], NULL);
    }
}

void bench_group_add_var(void* arg, int iters) {
    int i;
    bench_inv *data = (bench_inv*)arg;

    for (i = 0; i < iters; i++) {
        secp256k1_gej_add_var(&data->gej[0], &data->gej[0], &data->gej[1], NULL);
    }
}

void bench_group_add_affine(void* arg, int iters) {
    int i;
    bench_inv *data = (bench_inv*)arg;

    for (i = 0; i < iters; i++) {
        secp256k1_gej_add_ge(&data->gej[0], &data->gej[0], &data->ge[1]);
    }
}

void bench_group_add_affine_var(void* arg, int iters) {
    int i;
    bench_inv *data = (bench_inv*)arg;

    for (i = 0; i < iters; i++) {
        secp256k1_gej_add_ge_var(&data->gej[0], &data->gej[0], &data->ge[1], NULL);
    }
}

void bench_group_add_zinv_var(void* arg, int iters) {
    int i;
    bench_inv *data = (bench_inv*)arg;

    for (i = 0; i < iters; i++) {
        secp256k1_gej_add_zinv_var(&data->gej[0], &data->gej[0], &data->ge[1], &data->gej[0].y);
    }
}

void bench_group_to_affine_var(void* arg, int iters) {
    int i;
    bench_inv *data = (bench_inv*)arg;

    for (i = 0; i < iters; ++i) {
        secp256k1_ge_set_gej_var(&data->ge[1], &data->gej[0]);
        /* Use the output affine X/Y coordinates to vary the input X/Y/Z coordinates.
           Note that the resulting coordinates will generally not correspond to a point
           on the curve, but this is not a problem for the code being benchmarked here.
           Adding and normalizing have less overhead than EC operations (which could
           guarantee the point remains on the curve). */
        secp256k1_fe_add(&data->gej[0].x, &data->ge[1].y);
        secp256k1_fe_add(&data->gej[0].y, &data->fe[2]);
        secp256k1_fe_add(&data->gej[0].z, &data->ge[1].x);
        secp256k1_fe_normalize_var(&data->gej[0].x);
        secp256k1_fe_normalize_var(&data->gej[0].y);
        secp256k1_fe_normalize_var(&data->gej[0].z);
    }
}

void bench_ecmult_wnaf(void* arg, int iters) {
    int i, bits = 0, overflow = 0;
    bench_inv *data = (bench_inv*)arg;

    for (i = 0; i < iters; i++) {
        bits += secp256k1_ecmult_wnaf(data->wnaf, 256, &data->scalar[0], WINDOW_A);
        overflow += secp256k1_scalar_add(&data->scalar[0], &data->scalar[0], &data->scalar[1]);
    }
    CHECK(overflow >= 0);
    CHECK(bits <= 256*iters);
}

void bench_wnaf_const(void* arg, int iters) {
    int i, bits = 0, overflow = 0;
    bench_inv *data = (bench_inv*)arg;

    for (i = 0; i < iters; i++) {
        bits += secp256k1_wnaf_const(data->wnaf, &data->scalar[0], WINDOW_A, 256);
        overflow += secp256k1_scalar_add(&data->scalar[0], &data->scalar[0], &data->scalar[1]);
    }
    CHECK(overflow >= 0);
    CHECK(bits <= 256*iters);
}


void bench_sha256(void* arg, int iters) {
    int i;
    bench_inv *data = (bench_inv*)arg;
    secp256k1_sha256 sha;

    for (i = 0; i < iters; i++) {
        secp256k1_sha256_initialize(&sha);
        secp256k1_sha256_write(&sha, data->data, 32);
        secp256k1_sha256_finalize(&sha, data->data);
    }
}

void bench_hmac_sha256(void* arg, int iters) {
    int i;
    bench_inv *data = (bench_inv*)arg;
    secp256k1_hmac_sha256 hmac;

    for (i = 0; i < iters; i++) {
        secp256k1_hmac_sha256_initialize(&hmac, data->data, 32);
        secp256k1_hmac_sha256_write(&hmac, data->data, 32);
        secp256k1_hmac_sha256_finalize(&hmac, data->data);
    }
}

void bench_rfc6979_hmac_sha256(void* arg, int iters) {
    int i;
    bench_inv *data = (bench_inv*)arg;
    secp256k1_rfc6979_hmac_sha256 rng;

    for (i = 0; i < iters; i++) {
        secp256k1_rfc6979_hmac_sha256_initialize(&rng, data->data, 64);
        secp256k1_rfc6979_hmac_sha256_generate(&rng, data->data, 32);
    }
}

void bench_context_verify(void* arg, int iters) {
    int i;
    (void)arg;
    for (i = 0; i < iters; i++) {
        secp256k1_context_destroy(secp256k1_context_create(SECP256K1_CONTEXT_VERIFY));
    }
}

void bench_context_sign(void* arg, int iters) {
    int i;
    (void)arg;
    for (i = 0; i < iters; i++) {
        secp256k1_context_destroy(secp256k1_context_create(SECP256K1_CONTEXT_SIGN));
    }
}

int main(int argc, char **argv) {
    bench_inv data;
    int iters = get_iters(20000);
    int d = argc == 1; /* default */
    print_output_table_header_row();

    if (d || have_flag(argc, argv, "scalar") || have_flag(argc, argv, "add")) run_benchmark("scalar_add", bench_scalar_add, bench_setup, NULL, &data, 10, iters*100);
    if (d || have_flag(argc, argv, "scalar") || have_flag(argc, argv, "negate")) run_benchmark("scalar_negate", bench_scalar_negate, bench_setup, NULL, &data, 10, iters*100);
    if (d || have_flag(argc, argv, "scalar") || have_flag(argc, argv, "mul")) run_benchmark("scalar_mul", bench_scalar_mul, bench_setup, NULL, &data, 10, iters*10);
    if (d || have_flag(argc, argv, "scalar") || have_flag(argc, argv, "split")) run_benchmark("scalar_split", bench_scalar_split, bench_setup, NULL, &data, 10, iters);
    if (d || have_flag(argc, argv, "scalar") || have_flag(argc, argv, "inverse")) run_benchmark("scalar_inverse", bench_scalar_inverse, bench_setup, NULL, &data, 10, iters);
    if (d || have_flag(argc, argv, "scalar") || have_flag(argc, argv, "inverse")) run_benchmark("scalar_inverse_var", bench_scalar_inverse_var, bench_setup, NULL, &data, 10, iters);

    if (d || have_flag(argc, argv, "field") || have_flag(argc, argv, "half")) run_benchmark("field_half", bench_field_half, bench_setup, NULL, &data, 10, iters*100);
    if (d || have_flag(argc, argv, "field") || have_flag(argc, argv, "normalize")) run_benchmark("field_normalize", bench_field_normalize, bench_setup, NULL, &data, 10, iters*100);
    if (d || have_flag(argc, argv, "field") || have_flag(argc, argv, "normalize")) run_benchmark("field_normalize_weak", bench_field_normalize_weak, bench_setup, NULL, &data, 10, iters*100);
    if (d || have_flag(argc, argv, "field") || have_flag(argc, argv, "sqr")) run_benchmark("field_sqr", bench_field_sqr, bench_setup, NULL, &data, 10, iters*10);
    if (d || have_flag(argc, argv, "field") || have_flag(argc, argv, "mul")) run_benchmark("field_mul", bench_field_mul, bench_setup, NULL, &data, 10, iters*10);
    if (d || have_flag(argc, argv, "field") || have_flag(argc, argv, "inverse")) run_benchmark("field_inverse", bench_field_inverse, bench_setup, NULL, &data, 10, iters);
    if (d || have_flag(argc, argv, "field") || have_flag(argc, argv, "inverse")) run_benchmark("field_inverse_var", bench_field_inverse_var, bench_setup, NULL, &data, 10, iters);
    if (d || have_flag(argc, argv, "field") || have_flag(argc, argv, "sqrt")) run_benchmark("field_sqrt", bench_field_sqrt, bench_setup, NULL, &data, 10, iters);

    if (d || have_flag(argc, argv, "group") || have_flag(argc, argv, "double")) run_benchmark("group_double_var", bench_group_double_var, bench_setup, NULL, &data, 10, iters*10);
    if (d || have_flag(argc, argv, "group") || have_flag(argc, argv, "add")) run_benchmark("group_add_var", bench_group_add_var, bench_setup, NULL, &data, 10, iters*10);
    if (d || have_flag(argc, argv, "group") || have_flag(argc, argv, "add")) run_benchmark("group_add_affine", bench_group_add_affine, bench_setup, NULL, &data, 10, iters*10);
    if (d || have_flag(argc, argv, "group") || have_flag(argc, argv, "add")) run_benchmark("group_add_affine_var", bench_group_add_affine_var, bench_setup, NULL, &data, 10, iters*10);
    if (d || have_flag(argc, argv, "group") || have_flag(argc, argv, "add")) run_benchmark("group_add_zinv_var", bench_group_add_zinv_var, bench_setup, NULL, &data, 10, iters*10);
    if (d || have_flag(argc, argv, "group") || have_flag(argc, argv, "to_affine")) run_benchmark("group_to_affine_var", bench_group_to_affine_var, bench_setup, NULL, &data, 10, iters);

    if (d || have_flag(argc, argv, "ecmult") || have_flag(argc, argv, "wnaf")) run_benchmark("wnaf_const", bench_wnaf_const, bench_setup, NULL, &data, 10, iters);
    if (d || have_flag(argc, argv, "ecmult") || have_flag(argc, argv, "wnaf")) run_benchmark("ecmult_wnaf", bench_ecmult_wnaf, bench_setup, NULL, &data, 10, iters);

    if (d || have_flag(argc, argv, "hash") || have_flag(argc, argv, "sha256")) run_benchmark("hash_sha256", bench_sha256, bench_setup, NULL, &data, 10, iters);
    if (d || have_flag(argc, argv, "hash") || have_flag(argc, argv, "hmac")) run_benchmark("hash_hmac_sha256", bench_hmac_sha256, bench_setup, NULL, &data, 10, iters);
    if (d || have_flag(argc, argv, "hash") || have_flag(argc, argv, "rng6979")) run_benchmark("hash_rfc6979_hmac_sha256", bench_rfc6979_hmac_sha256, bench_setup, NULL, &data, 10, iters);

    if (d || have_flag(argc, argv, "context") || have_flag(argc, argv, "verify")) run_benchmark("context_verify", bench_context_verify, bench_setup, NULL, &data, 10, 1 + iters/1000);
    if (d || have_flag(argc, argv, "context") || have_flag(argc, argv, "sign")) run_benchmark("context_sign", bench_context_sign, bench_setup, NULL, &data, 10, 1 + iters/100);

    return 0;
}
