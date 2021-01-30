/**********************************************************************
 * Copyright (c) 2014 Pieter Wuille                                   *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef SECP256K1_BENCH_H
#define SECP256K1_BENCH_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "sys/time.h"

static int64_t gettime_i64(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int64_t)tv.tv_usec + (int64_t)tv.tv_sec * 1000000LL;
}

#define FP_EXP (6)
#define FP_MULT (1000000LL)

/* Format fixed point number. */
void print_number(const int64_t x) {
    int64_t x_abs, y;
    int c, i, rounding;
    size_t ptr;
    char buffer[30];

    if (x == INT64_MIN) {
        /* Prevent UB. */
        printf("ERR");
        return;
    }
    x_abs = x < 0 ? -x : x;

    /* Determine how many decimals we want to show (more than FP_EXP makes no
     * sense). */
    y = x_abs;
    c = 0;
    while (y > 0LL && y < 100LL * FP_MULT && c < FP_EXP) {
        y *= 10LL;
        c++;
    }

    /* Round to 'c' decimals. */
    y = x_abs;
    rounding = 0;
    for (i = c; i < FP_EXP; ++i) {
        rounding = (y % 10) >= 5;
        y /= 10;
    }
    y += rounding;

    /* Format and print the number. */
    ptr = sizeof(buffer) - 1;
    buffer[ptr] = 0;
    if (c != 0) {
        for (i = 0; i < c; ++i) {
            buffer[--ptr] = '0' + (y % 10);
            y /= 10;
        }
        buffer[--ptr] = '.';
    }
    do {
        buffer[--ptr] = '0' + (y % 10);
        y /= 10;
    } while (y != 0);
    if (x < 0) {
        buffer[--ptr] = '-';
    }
    printf("%s", &buffer[ptr]);
}

void run_benchmark(char *name, void (*benchmark)(void*, int), void (*setup)(void*), void (*teardown)(void*, int), void* data, int count, int iter) {
    int i;
    int64_t min = INT64_MAX;
    int64_t sum = 0;
    int64_t max = 0;
    for (i = 0; i < count; i++) {
        int64_t begin, total;
        if (setup != NULL) {
            setup(data);
        }
        begin = gettime_i64();
        benchmark(data, iter);
        total = gettime_i64() - begin;
        if (teardown != NULL) {
            teardown(data, iter);
        }
        if (total < min) {
            min = total;
        }
        if (total > max) {
            max = total;
        }
        sum += total;
    }
    printf("%s: min ", name);
    print_number(min * FP_MULT / iter);
    printf("us / avg ");
    print_number(((sum * FP_MULT) / count) / iter);
    printf("us / max ");
    print_number(max * FP_MULT / iter);
    printf("us\n");
}

int have_flag(int argc, char** argv, char *flag) {
    char** argm = argv + argc;
    argv++;
    if (argv == argm) {
        return 1;
    }
    while (argv != NULL && argv != argm) {
        if (strcmp(*argv, flag) == 0) {
            return 1;
        }
        argv++;
    }
    return 0;
}

int get_iters(int default_iters) {
    char* env = getenv("SECP256K1_BENCH_ITERS");
    if (env) {
        return strtol(env, NULL, 0);
    } else {
        return default_iters;
    }
}

#endif /* SECP256K1_BENCH_H */
