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
 * @defgroup dv Digit vector handling
 */

/**
 * @file
 *
 * Interface of the module for manipulating temporary double-precision digit
 * vectors.
 *
 * @ingroup dv
 */

#ifndef RLC_DV_H
#define RLC_DV_H

#include "relic_bn.h"
#include "relic_conf.h"
#include "relic_types.h"
#include "relic_util.h"
#include "relic_label.h"

/*============================================================================*/
/* Constant definitions                                                       */
/*============================================================================*/

/**
 * Size in bits of the largest field element.
 */
#ifdef WITH_FP

#ifdef WITH_FB
#define RLC_DV_MAX		(RLC_MAX(FP_PRIME, FB_POLYN))
#else /* !WITH_FB */
#define RLC_DV_MAX		(FP_PRIME)
#endif

#else /* !WITH_FP */

#ifdef WITH_FB
#define RLC_DV_MAX		(FB_POLYN)
#else /* !WITH_FB */
#define RLC_DV_MAX		(0)
#endif

#endif /* WITH_FP */

/**
 * Size in digits of a temporary vector.
 *
 * A temporary vector has enough size to store a multiplication/squaring result
 * produced by any module.
 */
#define RLC_DV_DIGS		(RLC_MAX(RLC_CEIL(RLC_DV_MAX, RLC_DIG), RLC_BN_SIZE))

/**
 * Size in bytes of a temporary vector.
 */
#define RLC_DV_BYTES	(RLC_DV_DIGS * (RLC_DIG / 8))

/*============================================================================*/
/* Type definitions                                                           */
/*============================================================================*/

/**
 * Represents a temporary double-precision digit vector.
 */
#if ALLOC == AUTO
typedef rlc_align dig_t dv_t[RLC_DV_DIGS + RLC_PAD(RLC_DV_BYTES)/(RLC_DIG / 8)];
#else
typedef dig_t *dv_t;
#endif

/*============================================================================*/
/* Macro definitions                                                          */
/*============================================================================*/

/**
 * Initializes a digit vector with a null value.
 *
 * @param[out] A			- the digit vector to initialize.
 */
#if ALLOC == AUTO
#define dv_null(A)			/* empty */
#else
#define dv_null(A)			A = NULL;
#endif

/**
 * Calls a function to allocate a temporary double-precision digit vector.
 *
 * @param[out] A			- the double-precision result.
 */
#if ALLOC == DYNAMIC
#define dv_new(A)			dv_new_dynam(&(A), RLC_DV_DIGS)
#elif ALLOC == AUTO
#define dv_new(A)			/* empty */
#endif

/**
 * Calls a function to clean and free a temporary double-precision digit vector.
 *
 * @param[out] A			- the temporary digit vector to clean and free.
 */
#if ALLOC == DYNAMIC
#define dv_free(A)			dv_free_dynam(&(A))
#elif ALLOC == AUTO
#define dv_free(A)			(void)A
#endif

/*============================================================================*/
/* Function prototypes                                                        */
/*============================================================================*/

/**
 * Prints a temporary digit vector.
 *
 * @param[in] a				- the temporary digit vector to print.
 * @param[in] digits		- the number of digits to print.
 */
void dv_print(dig_t *a, int digits);

/**
 * Assigns zero to a temporary double-precision digit vector.
 *
 * @param[out] a			- the temporary digit vector to assign.
 * @param[in] digits		- the number of words to initialize with zero.
 */
void dv_zero(dig_t *a, int digits);

/**
 * Copies some digits from a digit vector to another digit vector.
 *
 * @param[out] c			- the destination.
 * @param[in] a				- the source.
 * @param[in] digits		- the number of digits to copy.
 */
void dv_copy(dig_t *c, const dig_t *a, int digits);

/**
 * Conditionally copies some digits from a digit vector to another digit vector.
 *
 * @param[out] c			- the destination.
 * @paraim[in] a			- the source.
 * @param[in] digits		- the number of digits to copy.
 * @param[in] cond			- the condition to evaluate.
 */
void dv_copy_cond(dig_t *c, const dig_t *a, int digits, dig_t cond);

/**
 * Conditionally swap two digit vectors.
 *
 * @param[in,out] c			- the destination.
 * @paraim[in,out] a		- the source.
 * @param[in] digits		- the number of digits to copy.
 * @param[in] cond			- the condition to evaluate.
 */
void dv_swap_cond(dig_t *c, dig_t *a, int digits, dig_t cond);

/**
 * Returns the result of a comparison between two digit vectors.
 *
 * @param[in] a				- the first digit vector.
 * @param[in] b				- the second digit vector.
 * @param[in] size			- the length in digits of the vectors.
 * @return RLC_LT if a < b, RLC_EQ if a == b and RLC_GT if a > b.
 */
int dv_cmp(const dig_t *a, const dig_t *b, int size);

/**
 * Compares two digit vectors in constant time.
 *
 * @param[in] a				- the first digit vector.
 * @param[in] b				- the second digit vector.
 * @param[in] size			- the length in digits of the vectors.
 * @return RLC_EQ if they are equal and RLC_NE otherwise.
 */
int dv_cmp_const(const dig_t *a, const dig_t *b, int size);

/**
 * Allocates and initializes a temporary double-precision digit vector.
 *
 * @param[out] a			- the new temporary digit vector.
 * @param[in] digits		- the required precision in digits.
 * @throw ERR_NO_MEMORY		- if there is no available memory.
 * @throw ERR_PRECISION		- if the required precision cannot be represented
 * 							by the library.
 */
#if ALLOC == DYNAMIC
void dv_new_dynam(dv_t *a, int digits);
#endif

/**
 * Cleans and frees a temporary double-precision digit vector.
 *
 * @param[out] a			- the temporary digit vector to clean and free.
 */
#if ALLOC == DYNAMIC
void dv_free_dynam(dv_t *a);
#endif

/**
 * Shifts a digit vector to the left by some digits.
 * Computes c = a << (digits * RLC_DIG).
 *
 * @param[out] c			- the result.
 * @param[in] a				- the digit vector to shift.
 * @param[in] size			- the number of digits to shift.
 * @param[in] digits		- the shift amount.
 */
void dv_lshd(dig_t *c, const dig_t *a, int size, int digits);

/**
 * Shifts a digit vector to the right by some digits.
 * Computes c = a >> (digits * RLC_DIG).
 *
 * @param[out] c			- the result.
 * @param[in] a				- the digit vector to shift.
 * @param[in] size			- the number of digits to shift.
 * @param[in] digits		- the shift amount.
 */
void dv_rshd(dig_t *c, const dig_t *a, int size, int digits);

#endif /* !RLC_DV_H */
