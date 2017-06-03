/**********************************************************************
 * Copyright (c) 2014 Pieter Wuille                                   *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef _SECP256K1_BENCH_H_
#define _SECP256K1_BENCH_H_

#include <stdio.h>
#include <math.h>
#include "sys/time.h"

static double gettimedouble(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_usec * 0.000001 + tv.tv_sec;
}

void print_number(double x) {
    double y = x;
    int c = 0;
    if (y < 0.0) {
        y = -y;
    }
    while (y > 0 && y < 100.0) {
        y *= 10.0;
        c++;
    }
    printf("%.*f", c, x);
}

void run_benchmark(char *name, void (*benchmark)(void*), void (*setup)(void*), void (*teardown)(void*), void* data, int count, int iter) {
    int i;
    double min = HUGE_VAL;
    double sum = 0.0;
    double max = 0.0;
    for (i = 0; i < count; i++) {
        double begin, total;
        if (setup != NULL) {
            setup(data);
        }
        begin = gettimedouble();
        benchmark(data);
        total = gettimedouble() - begin;
        if (teardown != NULL) {
            teardown(data);
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
    print_number(min * 1000000.0 / iter);
    printf("us / avg ");
    print_number((sum / count) * 1000000.0 / iter);
    printf("us / max ");
    print_number(max * 1000000.0 / iter);
    printf("us\n");
}

#endif
