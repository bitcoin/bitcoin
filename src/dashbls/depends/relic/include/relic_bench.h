/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2009 RELIC Authors
 *
 * This file is part of RELIC. RELIC is legal property of its developers,
 * whose names are not listed here. Please refer to the COPYRIGHT file
 * for contact information.
 *
 * RELIC is free software; you can redistribute it and/or modify it under the
 * terms of the version 2.1 (or later) of the GNU Lesser General Public License
 * as published by the Free Software Foundation; or version 2.0 of the Apache
 * License as published by the Apache Software Foundation. See the LICENSE files
 * for more details.
 *
 * RELIC is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the LICENSE files for more details.
 *
 * You should have received a copy of the GNU Lesser General Public or the
 * Apache License along with RELIC. If not, see <https://www.gnu.org/licenses/>
 * or <https://www.apache.org/licenses/>.
 */

/**
 * @defgroup bench Automated benchmarks
 */

/**
 * @file
 *
 * Interface of useful routines for benchmarking.
 *
 * @ingroup bench
 */

#ifndef RLC_BENCH_H
#define RLC_BENCH_H

#include "relic_conf.h"
#include "relic_label.h"
#include "relic_util.h"

#if OPSYS == LINUX && TIMER == PERF
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <linux/perf_event.h>
#endif

/*============================================================================*/
/* Macro definitions                                                          */
/*============================================================================*/

/**
 * Runs a new benchmark once.
 *
 * @param[in] LABEL			- the label for this benchmark.
 * @param[in] FUNCTION		- the function to benchmark.
 * @param[in] N				- the amortization factor.
 */
#define BENCH_ONE(LABEL, FUNCTION, N)										\
	bench_reset();															\
	util_print("BENCH: " LABEL "%*c = ", (int)(32 - strlen(LABEL)), ' ');	\
	bench_before();															\
	FUNCTION;																\
	bench_after();															\
	bench_compute(N);														\
	bench_print();															\

/**
 * Runs a new benchmark a small number of times and prints the average timing
 * amortized by N.
 *
 * @param[in] LABEL			- the label for this benchmark.
 * @param[in] FUNCTION		- the function to benchmark.
 * @param[in] N				- the amortization factor.
 */
#define BENCH_FEW(LABEL, FUNCTION, N)										\
	bench_reset();															\
	util_print("BENCH: " LABEL "%*c = ", (int)(32 - strlen(LABEL)), ' ');	\
	bench_before();															\
	for (int i = 0; i < BENCH; i++)	{										\
		FUNCTION;															\
	}																		\
	bench_after();															\
	bench_compute(BENCH * (N));												\
	bench_print();															\

/**
 * Runs a new benchmark.
 *
 * @param[in] LABEL			- the label for this benchmark.
 */
#define BENCH_RUN(LABEL)													\
	bench_reset();															\
	util_print("BENCH: " LABEL "%*c = ", (int)(32 - strlen(LABEL)), ' ');	\
	for (int _b = 0; _b < BENCH; _b++)	{									\

/**
 * Prints the average timing of each execution in the chosen metric.
 */
#define BENCH_END															\
	}																		\
	bench_compute(BENCH * BENCH);											\
	bench_print()															\

/**
 * Prints the average timing of each execution amortized by N.
 *
 * @param N				- the amortization factor.
 */
#define BENCH_DIV(N)														\
	}																		\
	bench_compute(BENCH * BENCH * N);										\
	bench_print()															\

/**
 * Measures the time of one execution and adds it to the benchmark total.
 *
 * @param[in] FUNCTION		- the function executed.
 */
#define BENCH_ADD(FUNCTION)													\
	FUNCTION;																\
	bench_before();															\
	for (int _b = 0; _b < BENCH; _b++) {									\
		FUNCTION;															\
	}																		\
	bench_after();															\

/*============================================================================*/
/* Type definitions                                                           */
/*============================================================================*/

/**
 * Timer type.
 */
#if OPSYS == DUINO && TIMER == HREAL

typedef uint32_t ben_t;

#elif TIMER == HREAL || TIMER == HPROC || TIMER == HTHRD

#include <sys/time.h>
#include <time.h>
typedef struct timespec ben_t;

#elif TIMER == ANSI

#include <time.h>
typedef clock_t ben_t;

#elif TIMER == POSIX

#include <sys/time.h>
typedef struct timeval ben_t;

#else /* TIMER == CYCLE || TIMER == PERF */

typedef unsigned long long ben_t;

#endif

/*============================================================================*/
/* Function prototypes                                                        */
/*============================================================================*/

/**
 * Initializes the benchmarking module.
 */
void bench_init(void);

/**
 * Finalizes the benchmarking module.
 */
void bench_clean(void);

/**
 * Measures and prints benchmarking overhead.
 */
void bench_overhead(void);

/**
 * Resets the benchmark data.
 *
 * @param[in] label			- the benchmark label.
 */
void bench_reset(void);

/**
 * Measures the time before a benchmark is executed.
 */
void bench_before(void);

/**
 * Measures the time after a benchmark was started and adds it to the total.
 */
void bench_after(void);

/**
 * Computes the mean elapsed time between the start and the end of a benchmark.
 *
 * @param benches			- the number of executed benchmarks.
 */
void bench_compute(int benches);

/**
 * Prints the last benchmark.
 */
void bench_print(void);

/**
 * Returns the result of the last benchmark.
 *
 * @return the last benchmark.
 */
ull_t bench_total(void);

#endif /* !RLC_BENCH_H */
