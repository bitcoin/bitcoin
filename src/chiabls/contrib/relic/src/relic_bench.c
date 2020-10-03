/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (C) 2007-2017 RELIC Authors
 *
 * This file is part of RELIC. RELIC is legal property of its developers,
 * whose names are not listed here. Please refer to the COPYRIGHT file
 * for contact information.
 *
 * RELIC is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * RELIC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with RELIC. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file
 *
 * Implementation of useful benchmark routines.
 *
 * @ingroup relic
 */

#include <stdio.h>
#include <string.h>

#include "relic_core.h"
#include "relic_conf.h"

#if OPSYS == DUINO && TIMER == HREAL
/*
 * Prototype for Arduino timing function.
 *
 * @return The time in microseconds since the program began running.
 */
uint32_t micros();
#endif

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

#if defined(OVERH) && defined(TIMER) && BENCH > 1

/**
 * Dummy function for measuring benchmarking overhead.
 *
 * @param a				- the dummy parameter.
 */
static void empty(int *a) {
	(*a)++;
}

#endif /* OVER && TIMER && BENCH > 1 */

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

#if defined(OVERH) && defined(TIMER) && BENCH > 1

void bench_overhead(void) {
	ctx_t *ctx = core_get();
	int a[BENCH + 1];
	int *tmpa;

	do {
		ctx->over = 0;
		for (int l = 0; l < BENCH; l++) {
			ctx->total = 0;
			/* Measure the cost of (n^2 + over). */
			bench_before();
			for (int i = 0; i < BENCH; i++) {
				tmpa = a;
				for (int j = 0; j < BENCH; j++) {
					empty(tmpa++);
				}
			}
			bench_after();
			/* Add the cost of (n^2 + over). */
			ctx->over += ctx->total;
		}
		/* Overhead stores the cost of n*(n^2 + over) = n^3 + n*over. */
		ctx->total = 0;
		/* Measure the cost of (n^3 + over). */
		bench_before();
		for (int i = 0; i < BENCH; i++) {
			for (int k = 0; k < BENCH; k++) {
				tmpa = a;
				for (int j = 0; j < BENCH; j++) {
					empty(tmpa++);
				}
			}
		}
		bench_after();
		/* Subtract the cost of (n^3 + over). */
		ctx->over -= ctx->total;
		/* Now overhead stores (n - 1)*over, so take the average to obtain the
		 * overhead to execute BENCH operations inside a benchmark. */
		ctx->over /= (BENCH - 1);
		/* Divide to obtain the overhead of one operation pair. */
		ctx->over /= BENCH;
	} while (ctx->over < 0);
	ctx->total = ctx->over;
	bench_print();
}

#endif /* OVER && TIMER && BENCH > 1 */

void bench_reset(void) {
#ifdef TIMER
	core_get()->total = 0;
#endif
}

void bench_before(void) {
#if OPSYS == DUINO && TIMER == HREAL
	core_get()->before = micros();
#elif TIMER == HREAL || TIMER == HPROC || TIMER == HTHRD
	clock_gettime(CLOCK, &(core_get()->before));
#elif TIMER == ANSI
	core_get()->before = clock();
#elif TIMER == POSIX
	gettimeofday(&(core_get()->before), NULL);
#elif TIMER == CYCLE
	core_get()->before = arch_cycles();
#endif
}

void bench_after(void) {
	ctx_t *ctx = core_get();
	long long result;

#if OPSYS == DUINO && TIMER == HREAL
	core_get()->after = micros();
	result = (ctx->after - ctx->before);
#elif TIMER == HREAL || TIMER == HPROC || TIMER == HTHRD
	clock_gettime(CLOCK, &(ctx->after));
	result = ((long)ctx->after.tv_sec - (long)ctx->before.tv_sec) * 1000000000;
	result += (ctx->after.tv_nsec - ctx->before.tv_nsec);
#elif TIMER == ANSI
	ctx->after = clock();
	result = (ctx->after - ctx->before) * 1000000 / CLOCKS_PER_SEC;
#elif TIMER == POSIX
	gettimeofday(&(ctx->after), NULL);
	result = ((long)ctx->after.tv_sec - (long)ctx->before.tv_sec) * 1000000;
	result += (ctx->after.tv_usec - ctx->before.tv_usec);
#elif TIMER == CYCLE
	ctx->after = arch_cycles();
  	result = (ctx->after - ctx->before);
#endif

#ifdef TIMER
	ctx->total += result;
#else
	(void)result;
	(void)ctx;
#endif
}

void bench_compute(int benches) {
	ctx_t *ctx = core_get();
#ifdef TIMER
	ctx->total = ctx->total / benches;
#ifdef OVERH
	ctx->total = ctx->total - ctx->over;
#endif /* OVERH */
#else
	(void)benches;
	(void)ctx;
#endif /* TIMER */
}

void bench_print(void) {
	ctx_t *ctx = core_get();

#if TIMER == POSIX || TIMER == ANSI || (OPSYS == DUINO && TIMER == HREAL)
	util_print("%lld microsec", ctx->total);
#elif TIMER == CYCLE
	util_print("%lld cycles", ctx->total);
#else
	util_print("%lld nanosec", ctx->total);
#endif
	if (ctx->total < 0) {
		util_print(" (overflow or bad overhead estimation)\n");
	} else {
		util_print("\n");
	}
}

ull_t bench_total(void) {
	return core_get()->total;
}
