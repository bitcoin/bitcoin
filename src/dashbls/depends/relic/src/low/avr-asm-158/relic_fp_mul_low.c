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
#define COMBA_STEP(R2, R1, R0, A, B)										\
	dbl_t r = (dbl_t)(A) * (dbl_t)(B);										\
	dig_t _r = (R1);														\
	(R0) += (dig_t)(r);														\
	(R1) += (R0) < (dig_t)(r);												\
	(R2) += (R1) < _r;														\
	(R1) += (dig_t)((r) >> (dbl_t)RLC_DIG);								\
	(R2) += (R1) < (dig_t)((r) >> (dbl_t)RLC_DIG);							\

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

dig_t fp_mula_low(dig_t *c, const dig_t *a, dig_t digit) {
	int i;
	dig_t carry;
	dbl_t r;

	carry = 0;
	for (i = 0; i < RLC_FP_DIGS; i++, a++, c++) {
		/* Multiply the digit *tmpa by b and accumulate with the previous
		 * result in the same columns and the propagated carry. */
		r = (dbl_t)(*c) + (dbl_t)(*a) * (dbl_t)(digit) + (dbl_t)(carry);
		/* Increment the column and assign the result. */
		*c = (dig_t)r;
		/* Update the carry. */
		carry = (dig_t)(r >> (dbl_t)RLC_DIG);
	}
	return carry;
}

void fp_mulm_low(dig_t *c, const dig_t *a, const dig_t *b) {
	dig_t rlc_align t[2 * RLC_FP_DIGS];

	fp_muln_low(t, a, b);
	fp_rdc(c, t);
}

dig_t fp_mul1_low(dig_t *c, const dig_t *a, dig_t digit) {
	int i;
	dig_t carry;
	dbl_t r;

	carry = 0;
	for (i = 0; i < RLC_FP_DIGS; i++, a++, c++) {
		/* Multiply the digit *tmpa by b and accumulate with the previous
		 * result in the same columns and the propagated carry. */
		r = (dbl_t)(*a) * (dbl_t)(digit) + (dbl_t)(carry);
		/* Increment the column and assign the result. */
		*c = (dig_t)r;
		/* Update the carry. */
		carry = (dig_t)(r >> (dbl_t)RLC_DIG);
	}
	return carry;
}
