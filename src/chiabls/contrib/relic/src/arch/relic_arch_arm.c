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
 * Implementation of architecture-dependent routines.
 *
 * @ingroup arch
 */

#include "relic_types.h"

/**
 * Renames the inline assembly macro to a prettier name.
 */
#define asm					__asm__ volatile

#ifdef __ARM_ARCH_7M__
	/**
	 * Addresses of the registers.
	 */
 	/** @{ */
	volatile unsigned int *DWT_CYCCNT = (unsigned int *)0xE0001004;
	volatile unsigned int *DWT_CONTROL = (unsigned int *)0xE0001000;
	volatile unsigned int *SCB_DEMCR = (unsigned int *)0xE000EDFC;
	/** @} */
#endif

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void arch_init(void) {
#if TIMER == CYCLE

#if defined(__ARM_ARCH_6__) || defined(__ARM_ARCH_6J__) || defined(__ARM_ARCH_6K__) || defined(__ARM_ARCH_6Z__) || defined(__ARM_ARCH_6ZK__) || defined(__ARM_ARCH_6T2__)
	/*
	 * This relies on a Kernel module described in:
	 * http://blog.regehr.org/archives/794
	 */
	asm("mcr p15, 0, %0, c15, c12, 0" : : "r" (1));
#elif __ARM_ARCH_7A__
	/*
	 * This relies on a Kernel module described in:
	 * http://stackoverflow.com/questions/3247373/
	 * how-to-measure-program-execution-time-in-arm-cortex-a8-processor
	 */
	asm("mcr p15, 0, %0, c9, c12, 0" :: "r"(17));
	asm("mcr p15, 0, %0, c9, c12, 1" :: "r"(0x8000000f));
	asm("mcr p15, 0, %0, c9, c12, 3" :: "r"(0x8000000f));
#elif __ARM_ARCH_7M__
	*SCB_DEMCR = *SCB_DEMCR | 0x01000000;
	*DWT_CYCCNT = 0; // reset the counter
	*DWT_CONTROL = *DWT_CONTROL | 1 ; // enable the counter
#endif

#endif /* TIMER = CYCLE */
}

void arch_clean(void) {
}


ull_t arch_cycles(void) {
	unsigned int value = 0;

#if TIMER == CYCLE

#if defined(__ARM_ARCH_6__) || defined(__ARM_ARCH_6J__) || defined(__ARM_ARCH_6K__) || defined(__ARM_ARCH_6Z__) || defined(__ARM_ARCH_6ZK__) || defined(__ARM_ARCH_6T2__)
	asm("mrc p15, 0, %0, c15, c12, 1" : "=r"(value));
#elif __ARM_ARCH_7A__
	asm("mrc p15, 0, %0, c9, c13, 0" : "=r"(value));
#elif __ARM_ARCH_7M__
	value = *DWT_CYCCNT;
#else
	#error "Unsupported ARM architecture. Cycle count implementation missing for this ARM version."
#endif

#endif /* TIMER = CYCLE */

	return value;
}
