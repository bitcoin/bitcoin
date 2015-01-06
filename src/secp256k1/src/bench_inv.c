/**********************************************************************
 * Copyright (c) 2014 Pieter Wuille                                   *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

#include "include/secp256k1.h"

#include "util.h"
#include "num_impl.h"
#include "field_impl.h"
#include "group_impl.h"
#include "scalar_impl.h"
#include "bench.h"

typedef struct {
    secp256k1_scalar_t base, x;
} bench_inv_t;

void bench_inv_setup(void* arg) {
    bench_inv_t *data = (bench_inv_t*)arg;

    static const unsigned char init[32] = {
        0x02, 0x03, 0x05, 0x07, 0x0b, 0x0d, 0x11, 0x13,
        0x17, 0x1d, 0x1f, 0x25, 0x29, 0x2b, 0x2f, 0x35,
        0x3b, 0x3d, 0x43, 0x47, 0x49, 0x4f, 0x53, 0x59,
        0x61, 0x65, 0x67, 0x6b, 0x6d, 0x71, 0x7f, 0x83
    };

    secp256k1_scalar_set_b32(&data->base, init, NULL);
    secp256k1_scalar_set_b32(&data->x, init, NULL);
}

void bench_inv(void* arg, int iters) {
    bench_inv_t *data = (bench_inv_t*)arg;

    for (int i=0; i<iters; i++) {
        secp256k1_scalar_inverse(&data->x, &data->x);
        secp256k1_scalar_add(&data->x, &data->x, &data->base);
    }
}

int main(int argc, char **argv) {
    int iters=20000; int count=10;
    int oa;
    while ((oa = getopt(argc, argv, "c:i:w:")) != -1) {
        switch (oa) {
        case 'c':
            count=atoi(optarg);
            ( count<0 || count > 5000 ) ? (printf("Count %d out of sane bounds. Resetting to 10.\n",count),(count=10)):0x0;
            break;
        case 'i':
            iters=atoi(optarg);
            ( iters<0 || iters > 200000 ) ? (printf("Iterations %d out of sane bounds. Resetting to 20000.\n",iters),iters=20000):0x0;
            break;
        case 'w':
            ecmult_impl_windowG=atoi(optarg);
            ( ecmult_impl_windowG<2 || ecmult_impl_windowG > 30) ? (printf("WINDOW_G cache %d out of sane bounds. Resetting to 16.\n",ecmult_impl_windowG),ecmult_impl_windowG=16):0x0;
            break;
        case '?':
            printf("Missing argument to %c.", (char)optopt);
        default:
            return 1;
        }
    }

    secp256k1_ge_start();

    bench_inv_t data;
    run_benchmark(bench_inv, bench_inv_setup, NULL, &data, count, iters);

    secp256k1_ge_stop();
    return 0;
}
