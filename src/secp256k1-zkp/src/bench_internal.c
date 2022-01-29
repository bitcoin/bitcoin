/**********************************************************************
 * Copyright (c) 2014-2015 Pieter Wuille                              *
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
#include "ecmult_const_impl.h"
#include "ecmult_impl.h"
#include "bench.h"
#include "secp256k1.c"

typedef struct {
    secp256k1_scalar scalar_x, scalar_y;
    secp256k1_fe fe_x, fe_y;
    secp256k1_ge ge_x, ge_y;
    secp256k1_gej gej_x, gej_y;
    unsigned char data[64];
    int wnaf[256];
} bench_inv;

void bench_setup(void* arg) {
    bench_inv *data = (bench_inv*)arg;

    static const unsigned char init_x[32] = {
        0x02, 0x03, 0x05, 0x07, 0x0b, 0x0d, 0x11, 0x13,
        0x17, 0x1d, 0x1f, 0x25, 0x29, 0x2b, 0x2f, 0x35,
        0x3b, 0x3d, 0x43, 0x47, 0x49, 0x4f, 0x53, 0x59,
        0x61, 0x65, 0x67, 0x6b, 0x6d, 0x71, 0x7f, 0x83
    };

    static const unsigned char init_y[32] = {
        0x82, 0x83, 0x85, 0x87, 0x8b, 0x8d, 0x81, 0x83,
        0x97, 0xad, 0xaf, 0xb5, 0xb9, 0xbb, 0xbf, 0xc5,
        0xdb, 0xdd, 0xe3, 0xe7, 0xe9, 0xef, 0xf3, 0xf9,
        0x11, 0x15, 0x17, 0x1b, 0x1d, 0xb1, 0xbf, 0xd3
    };

    secp256k1_scalar_set_b32(&data->scalar_x, init_x, NULL);
    secp256k1_scalar_set_b32(&data->scalar_y, init_y, NULL);
    secp256k1_fe_set_b32(&data->fe_x, init_x);
    secp256k1_fe_set_b32(&data->fe_y, init_y);
    CHECK(secp256k1_ge_set_xo_var(&data->ge_x, &data->fe_x, 0));
    CHECK(secp256k1_ge_set_xo_var(&data->ge_y, &data->fe_y, 1));
    secp256k1_gej_set_ge(&data->gej_x, &data->ge_x);
    secp256k1_gej_set_ge(&data->gej_y, &data->ge_y);
    memcpy(data->data, init_x, 32);
    memcpy(data->data + 32, init_y, 32);
}

void bench_scalar_add(void* arg) {
    int i;
    bench_inv *data = (bench_inv*)arg;

    for (i = 0; i < 2000000; i++) {
        secp256k1_scalar_add(&data->scalar_x, &data->scalar_x, &data->scalar_y);
    }
}

void bench_scalar_negate(void* arg) {
    int i;
    bench_inv *data = (bench_inv*)arg;

    for (i = 0; i < 2000000; i++) {
        secp256k1_scalar_negate(&data->scalar_x, &data->scalar_x);
    }
}

void bench_scalar_sqr(void* arg) {
    int i;
    bench_inv *data = (bench_inv*)arg;

    for (i = 0; i < 200000; i++) {
        secp256k1_scalar_sqr(&data->scalar_x, &data->scalar_x);
    }
}

void bench_scalar_mul(void* arg) {
    int i;
    bench_inv *data = (bench_inv*)arg;

    for (i = 0; i < 200000; i++) {
        secp256k1_scalar_mul(&data->scalar_x, &data->scalar_x, &data->scalar_y);
    }
}

#ifdef USE_ENDOMORPHISM
void bench_scalar_split(void* arg) {
    int i;
    bench_inv *data = (bench_inv*)arg;

    for (i = 0; i < 20000; i++) {
        secp256k1_scalar l, r;
        secp256k1_scalar_split_lambda(&l, &r, &data->scalar_x);
        secp256k1_scalar_add(&data->scalar_x, &data->scalar_x, &data->scalar_y);
    }
}
#endif

void bench_scalar_inverse(void* arg) {
    int i;
    bench_inv *data = (bench_inv*)arg;

    for (i = 0; i < 2000; i++) {
        secp256k1_scalar_inverse(&data->scalar_x, &data->scalar_x);
        secp256k1_scalar_add(&data->scalar_x, &data->scalar_x, &data->scalar_y);
    }
}

void bench_scalar_inverse_var(void* arg) {
    int i;
    bench_inv *data = (bench_inv*)arg;

    for (i = 0; i < 2000; i++) {
        secp256k1_scalar_inverse_var(&data->scalar_x, &data->scalar_x);
        secp256k1_scalar_add(&data->scalar_x, &data->scalar_x, &data->scalar_y);
    }
}

void bench_field_normalize(void* arg) {
    int i;
    bench_inv *data = (bench_inv*)arg;

    for (i = 0; i < 2000000; i++) {
        secp256k1_fe_normalize(&data->fe_x);
    }
}

void bench_field_normalize_weak(void* arg) {
    int i;
    bench_inv *data = (bench_inv*)arg;

    for (i = 0; i < 2000000; i++) {
        secp256k1_fe_normalize_weak(&data->fe_x);
    }
}

void bench_field_mul(void* arg) {
    int i;
    bench_inv *data = (bench_inv*)arg;

    for (i = 0; i < 200000; i++) {
        secp256k1_fe_mul(&data->fe_x, &data->fe_x, &data->fe_y);
    }
}

void bench_field_sqr(void* arg) {
    int i;
    bench_inv *data = (bench_inv*)arg;

    for (i = 0; i < 200000; i++) {
        secp256k1_fe_sqr(&data->fe_x, &data->fe_x);
    }
}

void bench_field_inverse(void* arg) {
    int i;
    bench_inv *data = (bench_inv*)arg;

    for (i = 0; i < 20000; i++) {
        secp256k1_fe_inv(&data->fe_x, &data->fe_x);
        secp256k1_fe_add(&data->fe_x, &data->fe_y);
    }
}

void bench_field_inverse_var(void* arg) {
    int i;
    bench_inv *data = (bench_inv*)arg;

    for (i = 0; i < 20000; i++) {
        secp256k1_fe_inv_var(&data->fe_x, &data->fe_x);
        secp256k1_fe_add(&data->fe_x, &data->fe_y);
    }
}

void bench_field_sqrt(void* arg) {
    int i;
    bench_inv *data = (bench_inv*)arg;

    for (i = 0; i < 20000; i++) {
        secp256k1_fe_sqrt(&data->fe_x, &data->fe_x);
        secp256k1_fe_add(&data->fe_x, &data->fe_y);
    }
}

void bench_group_double_var(void* arg) {
    int i;
    bench_inv *data = (bench_inv*)arg;

    for (i = 0; i < 200000; i++) {
        secp256k1_gej_double_var(&data->gej_x, &data->gej_x, NULL);
    }
}

void bench_group_add_var(void* arg) {
    int i;
    bench_inv *data = (bench_inv*)arg;

    for (i = 0; i < 200000; i++) {
        secp256k1_gej_add_var(&data->gej_x, &data->gej_x, &data->gej_y, NULL);
    }
}

void bench_group_add_affine(void* arg) {
    int i;
    bench_inv *data = (bench_inv*)arg;

    for (i = 0; i < 200000; i++) {
        secp256k1_gej_add_ge(&data->gej_x, &data->gej_x, &data->ge_y);
    }
}

void bench_group_add_affine_var(void* arg) {
    int i;
    bench_inv *data = (bench_inv*)arg;

    for (i = 0; i < 200000; i++) {
        secp256k1_gej_add_ge_var(&data->gej_x, &data->gej_x, &data->ge_y, NULL);
    }
}

void bench_group_jacobi_var(void* arg) {
    int i;
    bench_inv *data = (bench_inv*)arg;

    for (i = 0; i < 20000; i++) {
        secp256k1_gej_has_quad_y_var(&data->gej_x);
    }
}

void bench_ecmult_wnaf(void* arg) {
    int i;
    bench_inv *data = (bench_inv*)arg;

    for (i = 0; i < 20000; i++) {
        secp256k1_ecmult_wnaf(data->wnaf, 256, &data->scalar_x, WINDOW_A);
        secp256k1_scalar_add(&data->scalar_x, &data->scalar_x, &data->scalar_y);
    }
}

void bench_wnaf_const(void* arg) {
    int i;
    bench_inv *data = (bench_inv*)arg;

    for (i = 0; i < 20000; i++) {
        secp256k1_wnaf_const(data->wnaf, data->scalar_x, WINDOW_A, 256);
        secp256k1_scalar_add(&data->scalar_x, &data->scalar_x, &data->scalar_y);
    }
}


void bench_sha256(void* arg) {
    int i;
    bench_inv *data = (bench_inv*)arg;
    secp256k1_sha256 sha;

    for (i = 0; i < 20000; i++) {
        secp256k1_sha256_initialize(&sha);
        secp256k1_sha256_write(&sha, data->data, 32);
        secp256k1_sha256_finalize(&sha, data->data);
    }
}

void bench_hmac_sha256(void* arg) {
    int i;
    bench_inv *data = (bench_inv*)arg;
    secp256k1_hmac_sha256 hmac;

    for (i = 0; i < 20000; i++) {
        secp256k1_hmac_sha256_initialize(&hmac, data->data, 32);
        secp256k1_hmac_sha256_write(&hmac, data->data, 32);
        secp256k1_hmac_sha256_finalize(&hmac, data->data);
    }
}

void bench_rfc6979_hmac_sha256(void* arg) {
    int i;
    bench_inv *data = (bench_inv*)arg;
    secp256k1_rfc6979_hmac_sha256 rng;

    for (i = 0; i < 20000; i++) {
        secp256k1_rfc6979_hmac_sha256_initialize(&rng, data->data, 64);
        secp256k1_rfc6979_hmac_sha256_generate(&rng, data->data, 32);
    }
}

void bench_context_verify(void* arg) {
    int i;
    (void)arg;
    for (i = 0; i < 20; i++) {
        secp256k1_context_destroy(secp256k1_context_create(SECP256K1_CONTEXT_VERIFY));
    }
}

void bench_context_sign(void* arg) {
    int i;
    (void)arg;
    for (i = 0; i < 200; i++) {
        secp256k1_context_destroy(secp256k1_context_create(SECP256K1_CONTEXT_SIGN));
    }
}

#ifndef USE_NUM_NONE
void bench_num_jacobi(void* arg) {
    int i;
    bench_inv *data = (bench_inv*)arg;
    secp256k1_num nx, norder;

    secp256k1_scalar_get_num(&nx, &data->scalar_x);
    secp256k1_scalar_order_get_num(&norder);
    secp256k1_scalar_get_num(&norder, &data->scalar_y);

    for (i = 0; i < 200000; i++) {
        secp256k1_num_jacobi(&nx, &norder);
    }
}
#endif

int main(int argc, char **argv) {
    bench_inv data;
    if (have_flag(argc, argv, "scalar") || have_flag(argc, argv, "add")) run_benchmark("scalar_add", bench_scalar_add, bench_setup, NULL, &data, 10, 2000000);
    if (have_flag(argc, argv, "scalar") || have_flag(argc, argv, "negate")) run_benchmark("scalar_negate", bench_scalar_negate, bench_setup, NULL, &data, 10, 2000000);
    if (have_flag(argc, argv, "scalar") || have_flag(argc, argv, "sqr")) run_benchmark("scalar_sqr", bench_scalar_sqr, bench_setup, NULL, &data, 10, 200000);
    if (have_flag(argc, argv, "scalar") || have_flag(argc, argv, "mul")) run_benchmark("scalar_mul", bench_scalar_mul, bench_setup, NULL, &data, 10, 200000);
#ifdef USE_ENDOMORPHISM
    if (have_flag(argc, argv, "scalar") || have_flag(argc, argv, "split")) run_benchmark("scalar_split", bench_scalar_split, bench_setup, NULL, &data, 10, 20000);
#endif
    if (have_flag(argc, argv, "scalar") || have_flag(argc, argv, "inverse")) run_benchmark("scalar_inverse", bench_scalar_inverse, bench_setup, NULL, &data, 10, 2000);
    if (have_flag(argc, argv, "scalar") || have_flag(argc, argv, "inverse")) run_benchmark("scalar_inverse_var", bench_scalar_inverse_var, bench_setup, NULL, &data, 10, 2000);

    if (have_flag(argc, argv, "field") || have_flag(argc, argv, "normalize")) run_benchmark("field_normalize", bench_field_normalize, bench_setup, NULL, &data, 10, 2000000);
    if (have_flag(argc, argv, "field") || have_flag(argc, argv, "normalize")) run_benchmark("field_normalize_weak", bench_field_normalize_weak, bench_setup, NULL, &data, 10, 2000000);
    if (have_flag(argc, argv, "field") || have_flag(argc, argv, "sqr")) run_benchmark("field_sqr", bench_field_sqr, bench_setup, NULL, &data, 10, 200000);
    if (have_flag(argc, argv, "field") || have_flag(argc, argv, "mul")) run_benchmark("field_mul", bench_field_mul, bench_setup, NULL, &data, 10, 200000);
    if (have_flag(argc, argv, "field") || have_flag(argc, argv, "inverse")) run_benchmark("field_inverse", bench_field_inverse, bench_setup, NULL, &data, 10, 20000);
    if (have_flag(argc, argv, "field") || have_flag(argc, argv, "inverse")) run_benchmark("field_inverse_var", bench_field_inverse_var, bench_setup, NULL, &data, 10, 20000);
    if (have_flag(argc, argv, "field") || have_flag(argc, argv, "sqrt")) run_benchmark("field_sqrt", bench_field_sqrt, bench_setup, NULL, &data, 10, 20000);

    if (have_flag(argc, argv, "group") || have_flag(argc, argv, "double")) run_benchmark("group_double_var", bench_group_double_var, bench_setup, NULL, &data, 10, 200000);
    if (have_flag(argc, argv, "group") || have_flag(argc, argv, "add")) run_benchmark("group_add_var", bench_group_add_var, bench_setup, NULL, &data, 10, 200000);
    if (have_flag(argc, argv, "group") || have_flag(argc, argv, "add")) run_benchmark("group_add_affine", bench_group_add_affine, bench_setup, NULL, &data, 10, 200000);
    if (have_flag(argc, argv, "group") || have_flag(argc, argv, "add")) run_benchmark("group_add_affine_var", bench_group_add_affine_var, bench_setup, NULL, &data, 10, 200000);
    if (have_flag(argc, argv, "group") || have_flag(argc, argv, "jacobi")) run_benchmark("group_jacobi_var", bench_group_jacobi_var, bench_setup, NULL, &data, 10, 20000);

    if (have_flag(argc, argv, "ecmult") || have_flag(argc, argv, "wnaf")) run_benchmark("wnaf_const", bench_wnaf_const, bench_setup, NULL, &data, 10, 20000);
    if (have_flag(argc, argv, "ecmult") || have_flag(argc, argv, "wnaf")) run_benchmark("ecmult_wnaf", bench_ecmult_wnaf, bench_setup, NULL, &data, 10, 20000);

    if (have_flag(argc, argv, "hash") || have_flag(argc, argv, "sha256")) run_benchmark("hash_sha256", bench_sha256, bench_setup, NULL, &data, 10, 20000);
    if (have_flag(argc, argv, "hash") || have_flag(argc, argv, "hmac")) run_benchmark("hash_hmac_sha256", bench_hmac_sha256, bench_setup, NULL, &data, 10, 20000);
    if (have_flag(argc, argv, "hash") || have_flag(argc, argv, "rng6979")) run_benchmark("hash_rfc6979_hmac_sha256", bench_rfc6979_hmac_sha256, bench_setup, NULL, &data, 10, 20000);

    if (have_flag(argc, argv, "context") || have_flag(argc, argv, "verify")) run_benchmark("context_verify", bench_context_verify, bench_setup, NULL, &data, 10, 20);
    if (have_flag(argc, argv, "context") || have_flag(argc, argv, "sign")) run_benchmark("context_sign", bench_context_sign, bench_setup, NULL, &data, 10, 200);

#ifndef USE_NUM_NONE
    if (have_flag(argc, argv, "num") || have_flag(argc, argv, "jacobi")) run_benchmark("num_jacobi", bench_num_jacobi, bench_setup, NULL, &data, 10, 200000);
#endif
    return 0;
}
