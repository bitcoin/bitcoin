/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2013 RELIC Authors
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
 * @file
 *
 * Implementation of AMD64-dependent routines.
 *
 * @ingroup arch
 */

#include <stdio.h>

#include "relic_types.h"
#include "relic_arch.h"
#include "relic_core.h"

#include "lzcnt.inc"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

/**
 * Function pointer to underlying lznct implementation.
 */
static unsigned int (*lzcnt_ptr)(ull_t);

#if TIMER == CYCLE || TIMER == PERF
/**
 * Renames the inline assembly macro to a prettier name.
 */
#define asm					__asm__ volatile
#endif

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void arch_init(void) {
	lzcnt_ptr = (has_lzcnt_hard() ? lzcnt64_hard : lzcnt64_soft);
}

void arch_clean(void) {
	lzcnt_ptr = NULL;
}

#if TIMER == CYCLE

ull_t arch_cycles(void) {
	unsigned int hi, lo;
	asm (
		"cpuid\n\t"/*serialize*/
		"rdtsc\n\t"/*read the clock*/
		"mov %%edx, %0\n\t"
		"mov %%eax, %1\n\t"
		: "=r" (hi), "=r" (lo):: "%rax", "%rbx", "%rcx", "%rdx"
	);
	return ((ull_t) lo) | (((ull_t) hi) << 32);
}

#elif TIMER == PERF

ull_t arch_cycles(void) {
	unsigned int seq;
	ull_t index, offset, result = 0;
	if (core_get()->perf_buf != NULL) {
		do {
			seq = core_get()->perf_buf->lock;
			asm("" ::: "memory");
			index = core_get()->perf_buf->index;
			offset = core_get()->perf_buf->offset;
			asm(
				"rdpmc; shlq $32, %%rdx; orq %%rdx,%%rax"
				: "=a" (result) : "c" (index - 1) : "%rdx"
			);
			asm("" ::: "memory");
		} while (core_get()->perf_buf->lock != seq);

		result += offset;
		result &= RLC_MASK(48); /* Get lower 48 bits only. */
	}
	return result;
}
#endif

unsigned int arch_lzcnt(dig_t x) {
	return lzcnt_ptr((ull_t)x) - (8 * sizeof(ull_t) - WSIZE);
}
