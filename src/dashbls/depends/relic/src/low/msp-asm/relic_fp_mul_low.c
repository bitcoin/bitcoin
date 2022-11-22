/*
 * Copyright 2007-2009 RELIC Project
 *
 * This file is part of RELIC. RELIC is legal property of its developers,
 * whose names are not listed here. Please refer to the COPYRIGHT file.
 *
 * RELIC is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * RELIC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with RELIC. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file
 *
 * Implementation of the low-level prime field multiplication functions.
 *
 * @ingroup bn
 */

#include "relic_fp.h"
#include "relic_fp_low.h"


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
