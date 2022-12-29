/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2012 RELIC Authors
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

#include "lzcnt.inc"

/**
 * Renames the inline assembly macro to a prettier name.
 */
#define asm					__asm__ volatile

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

/**
 * Function pointer to underlying lznct implementation.
 */
static unsigned int (*lzcnt_ptr)(unsigned int);

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void arch_init(void) {
	lzcnt_ptr = (has_lzcnt_hard() ? lzcnt32_hard : lzcnt32_soft);
}

void arch_clean(void) {
	lzcnt_ptr = NULL;
}

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

unsigned int arch_lzcnt(dig_t x) {
	return lzcnt_ptr((unsigned int)x) - (8 * sizeof(unsigned int) - WSIZE);
}
