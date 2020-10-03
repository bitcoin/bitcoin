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
 * Implementation of the multiple precision integer arithmetic multiplication
 * functions.
 *
 * @ingroup bn
 */

#include "relic_bn.h"
#include "relic_bn_low.h"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

/**
 * Computes the step of a Comba squaring.
 *
 * @param[in,out] R2		- most significant word of the triple register.
 * @param[in,out] R1		- middle word of the triple register.
 * @param[in,out] R0		- lowest significant word of the triple register.
 * @param[in] A				- the first digit to multiply.
 * @param[in] B				- the second digit to multiply.
 */
#define COMBA_STEP_BN_SQR_LOW(R2, R1, R0, A, B)								\
	dbl_t r = (dbl_t)(A) * (dbl_t)(B);										\
	dbl_t s = r + r;														\
	dig_t _r = (R1);														\
	(R0) += (dig_t)s;														\
	(R1) += (R0) < (dig_t)s;												\
	(R2) += (R1) < _r;														\
	(R1) += (dig_t)(s >> (dbl_t)BN_DIGIT);									\
	(R2) += (R1) < (dig_t)(s >> (dbl_t)BN_DIGIT);							\
	(R2) += (s < r);														\

/**
 * Computes the step of a Comba squaring when the loop length is odd.
 *
 * @param[in,out] R2		- most significant word of the triple register.
 * @param[in,out] R1		- middle word of the triple register.
 * @param[in,out] R0		- lowest significant word of the triple register.
 * @param[in] A				- the first digit to multiply.
 */
#define COMBA_FINAL(R2, R1, R0, A)											\
	dbl_t r = (dbl_t)(*tmpa) * (dbl_t)(*tmpa);								\
	dig_t _r = (R1);														\
	(R0) += (dig_t)(r);														\
	(R1) += (R0) < (dig_t)r;												\
	(R2) += (R1) < _r;														\
	(R1) += (dig_t)(r >> (dbl_t)BN_DIGIT);									\
	(R2) += (R1) < (dig_t)(r >> (dbl_t)BN_DIGIT);							\

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void bn_sqra_low(dig_t *c, const dig_t *a, int size) {
	int i;
	dig_t c0, c1;
	dig_t digit;
	dbl_t r, r0, r1;

	digit = *a;

	/* Accumulate this column with the square of a->dp[i]. */
	r = (dbl_t)(*c) + (dbl_t)(digit) * (dbl_t)(digit);

	*c = (dig_t)r;

	/* Update the carry. */
	c0 = (dig_t)(r >> (dbl_t)BN_DIGIT);
	c1 = 0;

	c++;
	a++;
	for (i = 0; i < size - 1; i++, a++, c++) {
		r = (dbl_t)(digit) * (dbl_t)(*a);
		r0 = r + r;
		r1 = r0 + (dbl_t)(*c) + (dbl_t)(c0);
		*c = (dig_t)r1;

		/* Accumulate the old delayed carry. */
		c0 = (dig_t)((r1 >> (dbl_t)BN_DIGIT) + c1);
		/* Compute the new delayed carry. */
		c1 = (r0 < r) || (r1 < r0) || (c0 < c1);
	}
	*c += c0;
	c1 += (*c++ < c0);
	*c += c1;
}

void bn_sqrn_low(dig_t *c, const dig_t *a, int size) {
	int i, j;
	const dig_t *tmpa, *tmpb;
	dig_t r0, r1, r2;

	/* Zero the accumulator. */
	r0 = r1 = r2 = 0;

	/* Comba squaring produces one column of the result per iteration. */
	for (i = 0; i < size; i++, c++) {
		tmpa = a;
		tmpb = a + i;

		/* Compute the number of additions in this column. */
		j = (i + 1);
		for (j = 0; j < (i + 1) / 2; j++, tmpa++, tmpb--) {
			COMBA_STEP_BN_SQR_LOW(r2, r1, r0, *tmpa, *tmpb);
		}
		if (!(i & 0x01)) {
			COMBA_FINAL(r2, r1, r0, *tmpa);
		}
		*c = r0;
		r0 = r1;
		r1 = r2;
		r2 = 0;
	}
	for (i = 0; i < size; i++, c++) {
		tmpa = a + (i + 1);
		tmpb = a + (size - 1);

		/* Compute the number of additions in this column. */
		for (j = 0; j < (size - 1 - i) / 2; j++, tmpa++, tmpb--) {
			COMBA_STEP_BN_SQR_LOW(r2, r1, r0, *tmpa, *tmpb);
		}
		if (!((size - i) & 0x01)) {
			COMBA_FINAL(r2, r1, r0, *tmpa);
		}
		*c = r0;
		r0 = r1;
		r1 = r2;
		r2 = 0;
	}
}
