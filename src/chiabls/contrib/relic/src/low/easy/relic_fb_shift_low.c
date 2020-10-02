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
 * @ingroup fb
 */

#include "relic_fb.h"
#include "relic_util.h"
#include "relic_fb_low.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

dig_t fb_lsh1_low(dig_t *c, const dig_t *a) {
	int i;
	dig_t r, carry;

	/* Prepare the bit mask. */
	carry = 0;
	for (i = 0; i < FB_DIGS; i++, a++, c++) {
		/* Get the most significant bit. */
		r = *a >> (FB_DIGIT - 1);
		/* Shift the operand and insert the carry, */
		*c = (*a << 1) | carry;
		/* Update the carry. */
		carry = r;
	}
	return carry;
}

dig_t fb_lshb_low(dig_t *c, const dig_t *a, int bits) {
	int i;
	dig_t r, carry, mask, shift;

	/* Prepare the bit mask. */
	shift = FB_DIGIT - bits;
	carry = 0;
	mask = MASK(bits);
	for (i = 0; i < FB_DIGS; i++, a++, c++) {
		/* Get the needed least significant bits. */
		r = ((*a) >> shift) & mask;
		/* Shift left the operand. */
		*c = ((*a) << bits) | carry;
		/* Update the carry. */
		carry = r;
	}
	return carry;
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
	int i;
	dig_t r, carry;

	c += FB_DIGS - 1;
	a += FB_DIGS - 1;
	carry = 0;
	for (i = FB_DIGS - 1; i >= 0; i--, a--, c--) {
		/* Get the least significant bit. */
		r = *a & 0x01;
		/* Shift the operand and insert the carry. */
		carry <<= FB_DIGIT - 1;
		*c = (*a >> 1) | carry;
		/* Update the carry. */
		carry = r;
	}
	return carry;
}

dig_t fb_rshb_low(dig_t *c, const dig_t *a, int bits) {
	int i;
	dig_t r, carry, mask, shift;

	c += FB_DIGS - 1;
	a += FB_DIGS - 1;
	/* Prepare the bit mask. */
	shift = FB_DIGIT - bits;
	carry = 0;
	mask = MASK(bits);
	for (i = FB_DIGS - 1; i >= 0; i--, a--, c--) {
		/* Get the needed least significant bits. */
		r = (*a) & mask;
		/* Shift left the operand. */
		*c = ((*a) >> bits) | (carry << shift);
		/* Update the carry. */
		carry = r;
	}
	return carry;
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
