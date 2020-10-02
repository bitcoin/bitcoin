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
 * Interface of the low-level prime field arithmetic module.
 *
 * @ingroup fp
 */

#ifndef RELIC_FP_LOW_H
#define RELIC_FP_LOW_H

/*============================================================================*/
/* Constant definitions                                                       */
/*============================================================================*/

#ifdef ASM

#include "relic_conf.h"
#include "relic_label.h"

#if (FP_PRIME % WSIZE) > 0
#define FP_DIGS	(FP_PRIME/WSIZE + 1)
#else
#define FP_DIGS	(FP_PRIME/WSIZE)
#endif
#else

/*============================================================================*/
/* Function prototypes                                                        */
/*============================================================================*/

/**
 * Adds a digit vector and a digit. Computes c = a + digit.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the digit vector to add.
 * @param[in] digit			- the digit to add.
 * @return the carry of the last digit addition.
 */
dig_t fp_add1_low(dig_t *c, const dig_t *a, dig_t digit);

/**
 * Adds to digit vectors of the same size. Computes c = a + b.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first digit vector to add.
 * @param[in] b				- the second digit vector to add.
 * @return the carry of the last digit addition.
 */
dig_t fp_addn_low(dig_t *c, const dig_t *a, const dig_t *b);

/**
 * Adds to digit vectors of the same size with integrated modular reduction.
 * Computes c = a + b.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first digit vector to add.
 * @param[in] b				- the second digit vector to add.
 */
void fp_addm_low(dig_t *c, const dig_t *a, const dig_t *b);

/**
 * Adds to double-length digit vectors. Computes c = a + b.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first digit vector to add.
 * @param[in] b				- the second digit vector to add.
 * @return the carry of the last digit addition.
 */
dig_t fp_addd_low(dig_t *c, const dig_t *a, const dig_t *b);

/**
 * Adds to double-length digit vectors and reduces modulo p * R. Computes
 * c = a + b.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first digit vector to add.
 * @param[in] b				- the second digit vector to add.
 */
void fp_addc_low(dig_t *c, const dig_t *a, const dig_t *b);

/**
 * Subtracts a digit from a digit vector. Computes c = a - digit.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the digit vector.
 * @param[in] digit			- the digit to subtract.
 * @return the carry of the last digit subtraction.
 */
dig_t fp_sub1_low(dig_t *c, const dig_t *a, dig_t digit);

/**
 * Subtracts a digit vector from another digit vector. Computes c = a - b.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the digit vector.
 * @param[in] b				- the digit vector to subtract.
 * @return the carry of the last digit subtraction.
 */
dig_t fp_subn_low(dig_t *c, const dig_t *a, const dig_t *b);

/**
 * Subtracts to digit vectors of the same size with integrated modular
 * reduction.
 * Computes c = a - b.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first digit vector to add.
 * @param[in] b				- the second digit vector to add.
 */
void fp_subm_low(dig_t *c, const dig_t *a, const dig_t *b);

/**
 * Subtracts a double-length digit vector from another digit vector.
 * Computes c = a - b.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first digit vector to add.
 * @param[in] b				- the second digit vector to add.
 * @return the carry of the last digit subtraction.
 */
dig_t fp_subd_low(dig_t *c, const dig_t *a, const dig_t *b);

/**
 * Subtracts a double-length digit vector from another digit vector.
 * Computes c = a - b. This version of the function should handle possible
 * carries by adding p * R.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first digit vector to add.
 * @param[in] b				- the second digit vector to add.
 */
void fp_subc_low(dig_t *c, const dig_t *a, const dig_t *b);

/**
 * Negates a digit vector. Computes c = -a.
 *
 * @param[out] c			- the result.
 * @param[out] a			- the prime field element to negate.
 */
void fp_negm_low(dig_t *c, const dig_t *a);

/**
 * Doubles a digit vector. Computes c = a + a.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the digit vector.
 * @return the carry of the last digit doubling.
 */
dig_t fp_dbln_low(dig_t *c, const dig_t *a);

/**
 * Doubles a digit vector with integrated modular reduction.
 * Computes c = a + a.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first digit vector to add.
 */
void fp_dblm_low(dig_t *c, const dig_t *a);

/**
 * Halves a digit vector with integrated modular reduction.
 * Computes c = a/2.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the digit vector to halve.
 */
void fp_hlvm_low(dig_t *c, const dig_t *a);

/**
 * Halves a double-precision digit vector. Computes c = a/2.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the digit vector to halve.
 */
void fp_hlvd_low(dig_t *c, const dig_t *a);

/**
 * Compares two digits.
 *
 * @param[in] a				- the first digit to compare.
 * @param[in] b				- the second digit to compare.
 * @return CMP_LT if a < b, CMP_EQ if a == b and CMP_GT if a > b.
 */
int fp_cmp1_low(dig_t a, dig_t b);

/**
 * Compares two digit vectors of the same size.
 *
 * @param[in] a				- the first digit vector to compare.
 * @param[in] b				- the second digit vector to compare.
 * @return CMP_LT if a < b, CMP_EQ if a == b and CMP_GT if a > b.
 */
int fp_cmpn_low(const dig_t *a, const dig_t *b);

/**
 * Shifts a digit vector to the left by 1 bits. Computes c = a << 1.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the digit vector to shift.
 * @return the carry of the last digit shift.
 */
dig_t fp_lsh1_low(dig_t *c, const dig_t *a);

/**
 * Shifts a digit vector to the left by an amount smaller than a digit. Computes
 * c = a << bits.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the digit vector to shift.
 * @param[in] bits			- the shift amount.
 * @return the carry of the last digit shift.
 */
dig_t fp_lshb_low(dig_t *c, const dig_t *a, int bits);

/**
 * Shifts a digit vector to the left by some digits.
 * Computes c = a << (digits * DIGIT).
 *
 * @param[out] c			- the result.
 * @param[in] a				- the multiple precision integer to shift.
 * @param[in] digits		- the shift ammount.
 */
void fp_lshd_low(dig_t *c, const dig_t *a, int digits);

/**
 * Shifts a digit vector to the right by 1 bit. Computes c = a >> 1.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the digit vector to shift.
 * @return the carry of the last digit shift.
 */
dig_t fp_rsh1_low(dig_t *c, const dig_t *a);

/**
 * Shifts a digit vector to the right by an amount smaller than a digit.
 * Computes c = a >> bits.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the digit vector to shift.
 * @param[in] bits			- the shift amount.
 * @return the carry of the last digit shift.
 */
dig_t fp_rshb_low(dig_t *c, const dig_t *a, int bits);

/**
 * Shifts a digit vector to the right by some digits.
 * Computes c = a >> (digits * DIGIT).
 *
 * @param[out] c			- the result.
 * @param[in] a				- the multiple precision integer to shift.
 * @param[in] digits		- the shift amount.
 */
void fp_rshd_low(dig_t *c, const dig_t *a, int digits);

/**
 * Multiplies a digit vector by a digit and adds this result to another digit
 * vector. Computes c = c + a * digit.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the digit vector to multiply.
 * @param[in] digit			- the digit to multiply.
 * @return the carry of the addition.
 */
dig_t fp_mula_low(dig_t *c, const dig_t *a, dig_t digit);

/**
 * Multiplies a digit vector by a digit and stores this result in another digit
 * vector. Computes c = a * digit.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the digit vector to multiply.
 * @param[in] digit			- the digit to multiply.
 * @return the most significant digit.
 */
dig_t fp_mul1_low(dig_t *c, const dig_t *a, dig_t digit);

/**
 * Multiplies two digit vectors of the same size. Computes c = a * b.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first digit vector to multiply.
 * @param[in] b				- the second digit vector to multiply.
 */
void fp_muln_low(dig_t *c, const dig_t *a, const dig_t *b);

/**
 * Multiplies two digit vectors of the same size with embedded modular
 * reduction. Computes c = (a * b) mod p.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first digit vector to multiply.
 * @param[in] b				- the second digit vector to multiply.
 */
void fp_mulm_low(dig_t *c, const dig_t *a, const dig_t *b);

/**
 * Squares a digit vector. Computes c = a * a.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the digit vector to square.
 */
void fp_sqrn_low(dig_t *c, const dig_t *a);

/**
 * Squares a digit vector with embedded modular reduction.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the digit vector to square.
 */
void fp_sqrm_low(dig_t *c, const dig_t *a);

/**
 * Reduces a digit vector modulo m represented in special form.
 * Computes c = a mod m.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the digit vector to reduce.
 * @param[in] m				- the modulus.
 */
void fp_rdcs_low(dig_t *c, const dig_t *a, const dig_t *m);

/**
 * Reduces a digit vector modulo the configured prime p. Computes c = a mod p.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the digit vector to reduce.
 */
void fp_rdcn_low(dig_t *c, dig_t *a);

/**
 * Inverts a digit vector modulo the configured prime.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the digit vector to invert.
 */
void fp_invn_low(dig_t *c, const dig_t *a);

#endif /* ASM */

#endif /* !RELIC_FP_LOW_H */
