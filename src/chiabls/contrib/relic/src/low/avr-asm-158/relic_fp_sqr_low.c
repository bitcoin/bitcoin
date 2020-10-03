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
 * Implementation of low-level prime field squaring functions.
 *
 * @ingroup fp
 */

#include "relic_fp.h"
#include "relic_fp_low.h"

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
#define COMBA_STEP(R2, R1, R0, A, B)										\
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

void fp_sqrm_low(dig_t *c, const dig_t *a) {
	dig_t relic_align t[2 * FP_DIGS];

	fp_sqrn_low(t, a);
	fp_rdc(c, t);
}
