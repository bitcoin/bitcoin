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
 * Implementation of architecture-dependent routines.
 *
 * @ingroup arch
 */

#include "relic_util.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void arch_init(void) {
}

void arch_clean(void) {
}

ull_t arch_cycles(void) {
	return 0;
}

unsigned int arch_lzcnt(dig_t a) {
#if WSIZE == 8 || WSIZE == 16
	static const uint8_t table[16] = {
		4, 3, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0
	};
#endif
#if WSIZE == 8
	if (a >> 4 == 0) {
		return table[a & 0xF] + 4;
	} else {
		return table[a >> 4];
	}
	return 0;
#elif WSIZE == 16
	int offset;

	if (a >= ((dig_t)1 << 8)) {
		offset = 8;
	} else {
		offset = 0;
	}
	a = a >> offset;
	if (a >> 4 == 0) {
		return table[a & 0xF] + offset;
	} else {
		return table[a >> 4] + 4 + offset;
	}
	return 0;
#elif WSIZE == 32
#ifdef _MSC_VER
    return __lzcnt(a);
#else
	return __builtin_clz(a);
#endif
#elif WSIZE == 64
#ifdef _MSC_VER
    return __lzcnt64(a);
#else
	return __builtin_clzll(a);
#endif
#endif
}
