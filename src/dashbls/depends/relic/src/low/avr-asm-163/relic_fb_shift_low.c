/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2011 RELIC Authors
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
 * Implementation of the low-level binary field bit shifting functions.
 *
 * @ingroup fb
 */

#include "relic_fb.h"
#include "relic_util.h"
#include "relic_fb_low.h"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

/*@{ */
/**
 * Dedicated functions to shift left by every possible shift amount.
 *
 * @param c		- the result.
 * @param a		- the digit vector to shift and add.
 * @param size		- the size of the digit vector.
 * @return the carry of the last shift.
 */
dig_t fb_lsha1_low(dig_t *c, const dig_t *a, int size);
dig_t fb_lsha2_low(dig_t *c, const dig_t *a, int size);
dig_t fb_lsha3_low(dig_t *c, const dig_t *a, int size);
dig_t fb_lsha4_low(dig_t *c, const dig_t *a, int size);
dig_t fb_lsha5_low(dig_t *c, const dig_t *a, int size);
dig_t fb_lsha6_low(dig_t *c, const dig_t *a, int size);
dig_t fb_lsha7_low(dig_t *c, const dig_t *a, int size);
/*@} */

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

dig_t fb_lshb_low(dig_t *c, const dig_t *a, int bits) {
	int i;
	dig_t r, carry, shift;

	if (bits == 1)
		return fb_lsh1_low(c, a);

	/* Prepare the bit mask. */
	shift = RLC_DIG - bits;
	carry = 0;
	for (i = 0; i < RLC_FB_DIGS; i++, a++, c++) {
		/* Get the needed least significant bits. */
		r = ((*a) >> shift) & RLC_MASK(bits);
		/* Shift left the operand. */
		*c = ((*a) << bits) | carry;
		/* Update the carry. */
		carry = r;
	}
	return carry;
}

dig_t fb_rshb_low(dig_t *c, const dig_t *a, int bits) {
	int i;
	dig_t r, carry, mask, shift;

	if (bits == 1)
		return fb_rsh1_low(c, a);

	c += RLC_FB_DIGS - 1;
	a += RLC_FB_DIGS - 1;
	/* Prepare the bit mask. */
	mask = ((dig_t)1 << (dig_t)bits) - 1;
	shift = RLC_DIG - bits;
	carry = 0;
	for (i = RLC_FB_DIGS - 1; i >= 0; i--, a--, c--) {
		/* Get the needed least significant bits. */
		r = (*a) & mask;
		/* Shift left the operand. */
		*c = ((*a) >> bits) | (carry << shift);
		/* Update the carry. */
		carry = r;
	}
	return carry;
}

#if FB_INV == EXGCD || !defined(STRIP)

dig_t fb_lsha_low(dig_t *c, const dig_t *a, int bits, int size) {
	if (bits == 1)
		return fb_lsha1_low(c, a, size);
	if (bits == 2)
		return fb_lsha2_low(c, a, size);
	if (bits == 3)
		return fb_lsha3_low(c, a, size);
	if (bits == 4)
		return fb_lsha4_low(c, a, size);
	if (bits == 5)
		return fb_lsha5_low(c, a, size);
	if (bits == 6)
		return fb_lsha6_low(c, a, size);
	if (bits == 7)
		return fb_lsha7_low(c, a, size);

	for (int i = 0; i < RLC_FB_DIGS; i++, c++, a++)
		(*c) ^= (*a);
	return 0;
}

#else

dig_t fb_lsha_low(dig_t *c, const dig_t *a, int bits, int size) {
	int i, j;
	dig_t b1, b2;

	j = RLC_DIG - bits;
	b1 = a[0];
	c[0] ^= (b1 << bits);
	if (size == RLC_FB_DIGS) {
		for (i = 1; i < RLC_FB_DIGS; i++) {
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

#endif
