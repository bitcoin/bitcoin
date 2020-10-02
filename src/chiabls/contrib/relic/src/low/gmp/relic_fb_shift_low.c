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
 * Implementation of the low-level binary field bit shifting functions.
 *
 * @ingroup bn
 */

#include <gmp.h>

#include "relic_fb.h"
#include "relic_util.h"
#include "relic_fb_low.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

dig_t fb_lsh1_low(dig_t *c, const dig_t *a) {
	return mpn_lshift(c, a, FB_DIGS, 1);
}

dig_t fb_lshb_low(dig_t *c, const dig_t *a, int bits) {
	return mpn_lshift(c, a, FB_DIGS, bits);
}

void fb_lshd_low(dig_t *c, const dig_t *a, int digits) {
	dig_t *top;
	const dig_t *bot;
	int i;

	top = c + FB_DIGS - 1;
	bot = a + FB_DIGS - 1 - digits;

	for (i = 0; i < FB_DIGS - digits; i++, top--, bot--) {
		*top = *bot;
	}
	for (i = 0; i < digits; i++, c++) {
		*c = 0;
	}
}

dig_t fb_rsh1_low(dig_t *c, const dig_t *a) {
	return mpn_rshift(c, a, FB_DIGS, 1);
}

dig_t fb_rshb_low(dig_t *c, const dig_t *a, int bits) {
	return mpn_rshift(c, a, FB_DIGS, bits);
}

void fb_rshd_low(dig_t *c, const dig_t *a, int digits) {
	const dig_t *top;
	dig_t *bot;
	int i;

	top = a + digits;
	bot = c;

	for (i = 0; i < FB_DIGS - digits; i++, top++, bot++) {
		*bot = *top;
	}
	for (; i < FB_DIGS; i++, bot++) {
		*bot = 0;
	}
}

dig_t fb_lsha_low(dig_t *c, const dig_t *a, int bits, int size) {
	int i, j;
	dig_t b1, b2;

	j = FB_DIGIT - bits;
	b1 = a[0];
	c[0] ^= (b1 << bits);
	if (size == FB_DIGS) {
		for (i = 1; i < FB_DIGS; i++) {
			b2 = a[i];
			c[i] ^= ((b2 << bits) | (b1 >> j));
			b1 = b2;
		}
	} else {
		for (i = 1; i < size; i++) {
			b2 = a[i];
			c[i] ^= ((b2 << bits) | (b1 >> j));
			b1 = b2;
		}
	}
	return (b1 >> j);
}
