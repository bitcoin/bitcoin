/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (C) 2007, 2008, 2009 RELIC Authors
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

#include "relic_bn.h"
#include "relic_bn_low.h"
#include "relic_util.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

dig_t bn_lsh1_low(dig_t *c, const dig_t *a, int size) {
	int i;
	dig_t r, carry;

	carry = 0;
	for (i = 0; i < size; i++, a++, c++) {
		/* Get the most significant bit. */
		r = *a >> (BN_DIGIT - 1);
		/* Shift the operand and insert the carry, */
		*c = (*a << 1) | carry;
		/* Update the carry. */
		carry = r;
	}
	return carry;
}

dig_t bn_lshb_low(dig_t *c, const dig_t *a, int size, int bits) {
	int i;
	dig_t r, carry, shift, mask;

	shift = BN_DIGIT - bits;
	carry = 0;
	mask = MASK(bits);
	for (i = 0; i < size; i++, a++, c++) {
		/* Get the needed least significant bits. */
		r = ((*a) >> shift) & mask;
		/* Shift left the operand. */
		*c = ((*a) << bits) | carry;
		/* Update the carry. */
		carry = r;
	}
	return carry;
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

//dig_t bn_rsh1_low(dig_t *c, dig_t *a, int size) {
//	int i;
//	dig_t r, carry;
//
//	c += size - 1;
//	a += size - 1;
//	carry = 0;
//	for (i = size - 1; i >= 0; i--, a--, c--) {
//		/* Get the least significant bit. */
//		r = *a & 0x01;
//		/* Shift the operand and insert the carry. */
//		carry <<= BN_DIGIT - 1;
//		*c = (*a >> 1) | carry;
//		/* Update the carry. */
//		carry = r;
//	}
//	return carry;
//}

dig_t bn_rshb_low(dig_t *c, const dig_t *a, int size, int bits) {
	int i;
	dig_t r, carry, shift, mask;

	c += size - 1;
	a += size - 1;
	/* Prepare the bit mask. */
	shift = BN_DIGIT - bits;
	carry = 0;
	mask = MASK(bits);
	for (i = size - 1; i >= 0; i--, a--, c--) {
		/* Get the needed least significant bits. */
		r = (*a) & mask;
		/* Shift left the operand. */
		*c = ((*a) >> bits) | (carry << shift);
		/* Update the carry. */
		carry = r;
	}
	return carry;
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
