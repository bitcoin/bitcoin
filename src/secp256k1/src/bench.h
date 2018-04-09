/**********************************************************************
 * Copyright (c) 2014 Pieter Wuille                                   *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef SECP256K1_BENCH_H
#define SECP256K1_BENCH_H

#include <stdio.h>
#include <math.h>
#include "sys/time.h"
#include <ctpl_stl.h>
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


	double begin, total;
	ctpl::thread_pool p(8);
	if (setup != NULL) {
		setup(data);
	}
	std::vector<std::future<void>> results(count);
	begin = gettimedouble();
	for (int i = 0; i < count; ++i) { // for 8 iterations,
		results[i] = p.push([&](int) {benchmark(data);});
	}
	for (int j = 0; j < count; ++j) {
		results[j].get();
	}
	total = gettimedouble() - begin;
	printf("%s: min ", name);
	printf("us / avg ");
	print_number((total / count) * 1000000.0 / iter);

    
}

#endif /* SECP256K1_BENCH_H */
