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

void run_benchmark(void (*benchmark)(void*), void (*setup)(void*), void (*teardown)(void*), void* data, int count, int iter) {
    double min = HUGE_VAL;
    double sum = 0.0;
    double max = 0.0;
    for (int i = 0; i < count; i++) {
        if (setup) setup(data);
        double begin = gettimedouble();
        benchmark(data);
        double total = gettimedouble() - begin;
        if (teardown) teardown(data);
        if (total < min) min = total;
        if (total > max) max = total;
        sum += total;
    }
    printf("min %.3fus / avg %.3fus / max %.3fus\n", min * 1000000.0 / iter, (sum / count) * 1000000.0 / iter, max * 1000000.0 / iter);
}

#endif
