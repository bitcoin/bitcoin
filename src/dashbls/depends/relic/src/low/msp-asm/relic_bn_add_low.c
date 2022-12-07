/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2012 RELIC Authors
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
 * Implementation of the low-level multiple precision addition and subtraction
 * functions.
 *
 * @ingroup bn
 */

#include "relic_bn.h"
#include "relic_bn_low.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

dig_t bn_add1_low(dig_t *c, const dig_t *a, dig_t digit, int size) {
	int i;
	register dig_t carry, r0;

	carry = digit;
	for (i = 0; i < size && carry; i++, a++, c++) {
		r0 = (*a) + carry;
		carry = (r0 < carry);
		(*c) = r0;
	}
	for (; i < size; i++, a++, c++) {
		(*c) = (*a);
	}
	return carry;
}

//dig_t bn_addn_low(dig_t *c, dig_t *a, dig_t *b, int size) {
//	int i;
//	register dig_t carry, c0, c1, r0, r1;
//
//	carry = 0;
//	for (i = 0; i < size; i++, a++, b++, c++) {
//		r0 = (*a) + (*b);
//		c0 = (r0 < (*a));
//		r1 = r0 + carry;
//		c1 = (r1 < r0);
//		carry = c0 | c1;
//		(*c) = r1;
//	}
//	return carry;
//}

dig_t bn_sub1_low(dig_t *c, const dig_t *a, dig_t digit, int size) {
	int i;
	dig_t carry, r0;

	carry = digit;
	for (i = 0; i < size && carry; i++, c++, a++) {
		r0 = (*a) - carry;
		carry = (r0 > (*a));
		(*c) = r0;
	}
	for (; i < size; i++, a++, c++) {
		(*c) = (*a);
	}
	return carry;
}

//dig_t bn_subn_low(dig_t *c, dig_t *a, dig_t *b, int size) {
//	int i;
//	dig_t carry, r0, diff;
//
//	/* Zero the carry. */
//	carry = 0;
//	for (i = 0; i < size; i++, a++, b++, c++) {
//		diff = (*a) - (*b);
//		r0 = diff - carry;
//		carry = ((*a) < (*b)) || (carry && !diff);
//		(*c) = r0;
//	}
//	return carry;
//}
