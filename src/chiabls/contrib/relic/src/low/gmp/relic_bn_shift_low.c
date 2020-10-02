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
 * Implementation of the low-level multiple precision bit shifting functions.
 *
 * @ingroup bn
 */

#include <gmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "relic_bn.h"
#include "relic_bn_low.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

dig_t bn_lsh1_low(dig_t *c, const dig_t *a, int size) {
	return mpn_lshift(c, a, size, 1);
}

dig_t bn_lshb_low(dig_t *c, const dig_t *a, int size, int bits) {
	return mpn_lshift(c, a, size, bits);
}

void bn_lshd_low(dig_t *c, const dig_t *a, int size, int digits) {
	dig_t *top;
	const dig_t *bot;
	int i;

	top = c + size + digits - 1;
	bot = a + size - 1;

	for (i = 0; i < size; i++, top--, bot--) {
		*top = *bot;
	}
	for (i = 0; i < digits; i++, c++) {
		*c = 0;
	}
}

dig_t bn_rsh1_low(dig_t *c, const dig_t *a, int size) {
	return mpn_rshift(c, a, size, 1);
}

dig_t bn_rshb_low(dig_t *c, const dig_t *a, int size, int bits) {
	return mpn_rshift(c, a, size, bits);
}

void bn_rshd_low(dig_t *c, const dig_t *a, int size, int digits) {
	const dig_t *top;
	dig_t *bot;
	int i;

	top = a + digits;
	bot = c;

	for (i = 0; i < size - digits; i++, top++, bot++) {
		*bot = *top;
	}
}
