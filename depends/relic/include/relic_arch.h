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
 * @defgroup arch Architecture-dependent utilities
 */

/**
 * @file
 *
 * Interface of architecture-dependent functions.
 *
 * @ingroup arch
 */

#ifndef RLC_ARCH_H
#define RLC_ARCH_H

#include "relic_types.h"
#include "relic_label.h"

#if ARCH == AVR
#include <avr/pgmspace.h>
#endif

/*============================================================================*/
/* Macro definitions                                                          */
/*============================================================================*/

/**
 * Chooses a proper way to store a string in the target architecture.
 *
 * @param[in] S					- the string to store.
 */
#if ARCH == AVR
#define RLC_STR(S)				PSTR(S)
#else
#define RLC_STR(S)				S
#endif

/**
 * Fetches a constant string to be used by the library.
 *
 * @param[out] S			- the resulting prepared parameter.
 * @param[in] ID			- the parameter represented as a string.
 * @param[in] L				- the length of the string.
 */
#if ARCH == AVR
#define RLC_GET(S, ID, L)		arch_copy_rom(S, RLC_STR(ID), L);
#else
#define RLC_GET(S, ID, L)		memcpy(S, ID, L);
#endif

/*============================================================================*/
/* Function prototypes                                                        */
/*============================================================================*/

/**
 * Performs architecture-dependent initialization.
 */
void arch_init(void);

/**
 * Performs architecture-dependent finalization.
 */
void arch_clean(void);

/**
 * Return the number of elapsed cycles.
 */
ull_t arch_cycles(void);

/**
 * Return the number of leading zeros in an integer.
 */
uint_t arch_lzcnt(dig_t);

#if ARCH == AVR

/**
 * Copies a string from the text section to the destination vector.
 *
 * @param[out] dest		- the destination vector.
 * @param[in] src		- the pointer to the string stored on the text section.
 * @param[in] len		- the length of the string.
 */
void arch_copy_rom(char *dest, const char *src, int len);

#endif

#endif /* !RLC_ARCH_H */
