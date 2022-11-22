/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2009 RELIC Authors
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
 * Interface of the low-level binary field arithmetic module.
 *
 * All functions assume a configured polynomial basis f(z) and that the
 * destination has enough capacity to store the result of the computation.
 *
 * @ingroup fb
 */

#ifndef RLC_FB_LOW_H
#define RLC_FB_LOW_H

/*============================================================================*/
/* Constant definitions                                                       */
/*============================================================================*/

#ifdef ASM

#include "relic_conf.h"

#undef RLC_FB_DIGS
#if (FB_POLYN % WSIZE) > 0
#define RLC_FB_DIGS		(FB_POLYN/WSIZE + 1)
#else
#define RLC_FB_DIGS		(FB_POLYN/WSIZE)
#endif

#else

#include "relic_types.h"

/*============================================================================*/
/* Function prototypes                                                        */
/*============================================================================*/

/**
 * Adds a digit vector and a digit. Computes c = a + digit.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first digit vector to add.
 * @param[in] digit			- the digit to add.
 */
void fb_add1_low(dig_t *c, const dig_t *a, dig_t digit);

/**
 * Adds two digit vectors of the same size. Computes c = a + b.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first digit vector to add.
 * @param[in] b				- the second digit vector to add.
 */
void fb_addn_low(dig_t *c, const dig_t *a, const dig_t *b);

/**
 * Adds two digit vectors of the same size, with this size different than the
 * standard precision and specified in the last parameter. Computes c = a + b.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first digit vector to add.
 * @param[in] b				- the second digit vector to add.
 * @param[in] size			- the number of digits to add.
 */
void fb_addd_low(dig_t *c, const dig_t *a, const dig_t *b, int size);

/**
 * Shifts a digit vector to the left by 1 bit. Computes c = a * z.
 *
 * @param[out] c			- the result
 * @param[in] a				- the digit vector to shift.
 * @return the carry of the last digit shift.
 */
dig_t fb_lsh1_low(dig_t *c, const dig_t *a);

/**
 * Shifts a digit vector to the left by an amount smaller than a digit.
 * The shift amount must be bigger than 0 and smaller than RLC_DIG. Computes
 * c = a * z^bits.
 *
 * @param[out] c			- the result
 * @param[in] a				- the digit vector to shift.
 * @param[in] bits			- the shift ammount.
 * @return the carry of the last digit shift.
 */
dig_t fb_lshb_low(dig_t *c, const dig_t *a, int bits);

/**
 * Shifts a digit vector to the right by 1 bit. Computes c = a / z.
 *
 * @param[out] c			- the result
 * @param[in] a				- the digit vector to shift.
 * @return the carry of the last digit shift.
 */
dig_t fb_rsh1_low(dig_t *c, const dig_t *a);

/**
 * Shifts a digit vector to the right by an amount smaller than a digit.
 * The shift amount must be bigger than 0 and smaller than RLC_DIG.
 * Computes c = a / (z^bits).
 *
 * @param[out] c			- the result
 * @param[in] a				- the digit vector to shift.
 * @param[in] bits			- the shift amount.
 * @return the carry of the last digit shift.
 */
dig_t fb_rshb_low(dig_t *c, const dig_t *a, int bits);

/**
 * Adds a left-shifted digit vector to another digit vector.
 * The shift amount must be shorter than the digit size.
 * Computes c = c + (a * z^bits).
 *
 * @param[out] c			- the result.
 * @param[in] a				- the digit vector to shift and add.
 * @param[in] size			- the number of digits to add.
 * @param[in] bits			- the shift amount.
 * @return the carry of the last shift.
 */
dig_t fb_lsha_low(dig_t *c, const dig_t *a, int bits, int size);

/**
 * Multiplies a digit vector by a digit.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the digit vector to multiply.
 * @param[in] digit			- the digit to multiply.
 */
void fb_mul1_low(dig_t *c, const dig_t *a, dig_t digit);

/**
 * Multiplies two digit vectors of the same size.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first digit vector to multiply.
 * @param[in] b				- the second digit vector to multiply.
 */
void fb_muln_low(dig_t *c, const dig_t *a, const dig_t *b);

/**
 * Multiplies two digit vectors of the same size but smaller than the standard
 * precision.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first digit vector to multiply.
 * @param[in] b				- the second digit vector to multiply.
 * @param[in] size			- the size of the digit vectors.
 */
void fb_muld_low(dig_t *c, const dig_t *a, const dig_t *b, int size);

/**
 * Multiplies two digit vectors of the same size with embedded modular
 * reduction.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first digit vector to multiply.
 * @param[in] b				- the second digit vector to multiply.
 */
void fb_mulm_low(dig_t *c, const dig_t *a, const dig_t *b);

/**
 * Squares a digit vector using bit manipulation.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the digit vector to square.
 */
void fb_sqrn_low(dig_t *c, const dig_t *a);

/**
 * Squares a digit vector using a lookup table.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the digit vector to square.
 */
void fb_sqrl_low(dig_t *c, const dig_t *a);

/**
 * Squares a digit vector with embedded modular reduction.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the digit vector to square.
 */
void fb_sqrm_low(dig_t *c, const dig_t *a);

/**
 * Exponentiates consecutively a digit vector to a fixed power 2^k/2^-k given a
 * precomputed table.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the digit vector to square.
 * @param[in] t				- the
 */
void fb_itrn_low(dig_t *c, const dig_t *a, dig_t *t);

/**
 * Extracts the square root of a digit vector.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the digit vector to extract the square root.
 */
void fb_srtn_low(dig_t *c, const dig_t *a);

/**
 * Solves a quadratic equation for c^2 + c = a.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the digit vector.
 */
void fb_slvn_low(dig_t *c, const dig_t *a);

/**
 * Computes the trace of a digit vector.
 *
 * @param[in] a				- the digit vector.
 * @return the trace of the argument.
 */
dig_t fb_trcn_low(const dig_t *a);

/**
 * Reduces a digit vector modulo the configured irreducible polynomial.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the digit vector to reduce.
 */
void fb_rdcn_low(dig_t *c, dig_t *a);

/**
 * Reduces the most significant bits of a digit vector modulo the configured
 * irreducible polynomial. The maximum number of bits to be reduced is equal
 * to the size of the digit.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the digit vector to reduce.
 */
void fb_rdc1_low(dig_t *c, dig_t *a);

/**
 * Inverts a digit vector modulo the configured irreducible polynomial.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the digit vector to invert.
 */
void fb_invn_low(dig_t *c, const dig_t *a);

#endif /* !ASM */

#endif /* !RLC_FB_LOW_H */
