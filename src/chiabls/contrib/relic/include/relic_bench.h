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
 * @defgroup bench Automated benchmarks
 */

/**
 * @file
 *
 * Interface of useful routines for benchmarking.
 *
 * @ingroup bench
 */

#ifndef RELIC_BENCH_H
#define RELIC_BENCH_H

#include "relic_conf.h"
#include "relic_label.h"
#include "relic_util.h"

/*============================================================================*/
/* Constant definitions                                                       */
/*============================================================================*/

/**
 * Shared parameter for these timer.
 */
#if TIMER == HREAL
#define CLOCK			CLOCK_REALTIME
#elif TIMER == HPROC
#define CLOCK			CLOCK_PROCESS_CPUTIME_ID
#elif TIMER == HTHRD
#define CLOCK			CLOCK_THREAD_CPUTIME_ID
#else
#define CLOCK			NULL
#endif

/*============================================================================*/
/* Macro definitions                                                          */
/*============================================================================*/

/**
 * Runs a new benchmark once.
 *
 * @param[in] LABEL			- the label for this benchmark.
 * @param[in] FUNCTION		- the function to benchmark.
 */
#define BENCH_ONCE(LABEL, FUNCTION)											\
	bench_reset();															\
	util_print("BENCH: " LABEL "%*c = ", (int)(32 - strlen(LABEL)), ' ');	\
	bench_before();															\
	FUNCTION;																\
	bench_after();															\
	bench_compute(1);														\
	bench_print();															\

/**
 * Runs a new benchmark a small number of times.
 *
 * @param[in] LABEL			- the label for this benchmark.
 * @param[in] FUNCTION		- the function to benchmark.
 */
#define BENCH_SMALL(LABEL, FUNCTION)										\
	bench_reset();															\
	util_print("BENCH: " LABEL "%*c = ", (int)(32 - strlen(LABEL)), ' ');	\
	bench_before();															\
	for (int i = 0; i < BENCH; i++)	{										\
		FUNCTION;															\
	}																		\
	bench_after();															\
	bench_compute(BENCH);													\
	bench_print();															\

/**
 * Runs a new benchmark.
 *
 * @param[in] LABEL			- the label for this benchmark.
 */
#define BENCH_BEGIN(LABEL)													\
	bench_reset();															\
	util_print("BENCH: " LABEL "%*c = ", (int)(32 - strlen(LABEL)), ' ');	\
	for (int i = 0; i < BENCH; i++)	{										\

/**
 * Prints the mean timing of each execution in nanoseconds.
 */
#define BENCH_END															\
	}																		\
	bench_compute(BENCH * BENCH);											\
	bench_print()															\

/**
 * Measures the time of one execution and adds it to the benchmark total.
 *
 * @param[in] FUNCTION		- the function executed.
 */
#define BENCH_ADD(FUNCTION)													\
	FUNCTION;																\
	bench_before();															\
	for (int j = 0; j < BENCH; j++) {										\
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

typedef uint32_t bench_t;

#elif TIMER == HREAL || TIMER == HPROC || TIMER == HTHRD

#include <sys/time.h>
#include <time.h>
typedef struct timespec bench_t;

#elif TIMER == ANSI

#include <time.h>
typedef clock_t bench_t;

#elif TIMER == POSIX

#include <sys/time.h>
typedef struct timeval bench_t;

#elif TIMER == CYCLE

typedef unsigned long long bench_t;

#else

typedef unsigned long long bench_t;

#endif

/*============================================================================*/
/* Function prototypes                                                        */
/*============================================================================*/

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

#endif /* !RELIC_BENCH_H */
