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
 * Implementation of the low-level prime field multiplication functions.
 *
 * @ingroup fp
 */

#include "relic_fp.h"
#include "relic_fp_low.h"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

/**
 * Accumulates a double precision digit in a triple register variable.
 *
 * @param[in,out] R2		- most significant word of the triple register.
 * @param[in,out] R1		- middle word of the triple register.
 * @param[in,out] R0		- lowest significant word of the triple register.
 * @param[in] A				- the first digit to multiply.
 * @param[in] B				- the second digit to multiply.
 */
#define COMBA_STEP_FP_MUL_LOW(R2, R1, R0, A, B)								\
	dbl_t r = (dbl_t)(A) * (dbl_t)(B);										\
	dig_t _r = (R1);														\
	(R0) += (dig_t)(r);														\
	(R1) += (R0) < (dig_t)(r);												\
	(R2) += (R1) < _r;														\
	(R1) += (dig_t)((r) >> (dbl_t)BN_DIGIT);								\
	(R2) += (R1) < (dig_t)((r) >> (dbl_t)BN_DIGIT);							\

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

dig_t fp_mula_low(dig_t *c, const dig_t *a, dig_t digit) {
	int i;
	dig_t carry;
	dbl_t r;

	carry = 0;
	for (i = 0; i < FP_DIGS; i++, a++, c++) {
		/* Multiply the digit *tmpa by b and accumulate with the previous
		 * result in the same columns and the propagated carry. */
		r = (dbl_t)(*c) + (dbl_t)(*a) * (dbl_t)(digit) + (dbl_t)(carry);
		/* Increment the column and assign the result. */
		*c = (dig_t)r;
		/* Update the carry. */
		carry = (dig_t)(r >> (dbl_t)FP_DIGIT);
	}
	return carry;
}

dig_t fp_mul1_low(dig_t *c, const dig_t *a, dig_t digit) {
	int i;
	dig_t carry;
	dbl_t r;

	carry = 0;
	for (i = 0; i < FP_DIGS; i++, a++, c++) {
		/* Multiply the digit *tmpa by b and accumulate with the previous
		 * result in the same columns and the propagated carry. */
		r = (dbl_t)(*a) * (dbl_t)(digit) + (dbl_t)(carry);
		/* Increment the column and assign the result. */
		*c = (dig_t)r;
		/* Update the carry. */
		carry = (dig_t)(r >> (dbl_t)FP_DIGIT);
	}
	return carry;
}

void fp_mulm_low(dig_t *c, const dig_t *a, const dig_t *b) {
	relic_align dig_t t[2 * FP_DIGS];

	fp_muln_low(t, a, b);
	fp_rdc(c, t);
}
