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
 * @defgroup fbx Binary field extensions.
 */

/**
 * @file
 *
 * Interface of the module for extension field arithmetic over binary fields.
 *
 * @ingroup fbx
 */

#ifndef RLC_FBX_H
#define RLC_FBX_H

#include "relic_fb.h"
#include "relic_types.h"

/*============================================================================*/
/* Type definitions                                                           */
/*============================================================================*/

/**
 * Represents a quadratic extension binary field element.
 *
 * This extension field is constructed with the basis {1, s}, where s is a
 * quadratic non-residue in the binary field.
 */
typedef fb_t fb2_t[2];

/*============================================================================*/
/* Macro definitions                                                          */
/*============================================================================*/

/**
 * Initializes a quadratic extension binary field with a null value.
 *
 * @param[out] A			- the quadratic extension element to initialize.
 */
#define fb2_null(A)															\
		fb_null(A[0]); fb_null(A[1]);										\

/**
 * Calls a function to allocate a quadratic extension binary field element.
 *
 * @param[out] A			- the new quadratic extension field element.
 */
#define fb2_new(A)															\
		fb_new(A[0]); fb_new(A[1]);											\

/**
 * Calls a function to free a quadratic extension binary field element.
 *
 * @param[out] A			- the quadratic extension field element to free.
 */
#define fb2_free(A)															\
		fb_free(A[0]); fb_free(A[1]); 										\

/**
 * Copies the second argument to the first argument.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the quadratic extension field element to copy.
 */
#define fb2_copy(C, A)														\
		fb_copy(C[0], A[0]); fb_copy(C[1], A[1]); 							\

/**
 * Negates a quadratic extension field element.
 *
 * f@param[out] C			- the result.
 * @param[out] A			- the quadratic extension field element to negate.
 */
#define fb2_neg(C, A)														\
		fb_neg(C[0], A[0]); fb_neg(C[1], A[1]); 							\

/**
 * Assigns zero to a quadratic extension field element.
 *
 * @param[out] A			- the quadratic extension field element to zero.
 */
#define fb2_zero(A)															\
		fb_zero(A[0]); fb_zero(A[1]); 										\

/**
 * Tests if a quadratic extension field element is zero or not.
 *
 * @param[in] A				- the quadratic extension field element to test.
 * @return 1 if the argument is zero, 0 otherwise.
 */
#define fb2_is_zero(A)														\
		(fb_is_zero(A[0]) && fb_is_zero(A[1]))								\

/**
 * Assigns a random value to a quadratic extension field element.
 *
 * @param[out] A			- the quadratic extension field element to assign.
 */
#define fb2_rand(A)															\
		fb_rand(A[0]); fb_rand(A[1]);										\

/**
 * Prints a quadratic extension field element to standard output.
 *
 * @param[in] A				- the quadratic extension field element to print.
 */
#define fb2_print(A)														\
		fb_print(A[0]); fb_print(A[1]);										\

/**
 * Returns the result of a comparison between two quadratic extension field
 * elements
 *
 * @param[in] A				- the first quadratic extension field element.
 * @param[in] B				- the second quadratic extension field element.
 * @return RLC_NE if a != b, RLC_EQ if a == b.
 */
#define fb2_cmp(A, B)														\
		((fb_cmp(A[0], B[0]) == RLC_EQ) && (fb_cmp(A[1], B[1]) == RLC_EQ)	\
		? RLC_EQ : RLC_NE)													\

/**
 * Adds two quadratic extension field elements. Computes c = a + b.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the first quadratic extension field element.
 * @param[in] B				- the second quadratic extension field element.
 */
#define fb2_add(C, A, B)													\
		fb_add(C[0], A[0], B[0]); fb_add(C[1], A[1], B[1]);					\

/*============================================================================*/
/* Function prototypes                                                        */
/*============================================================================*/

/**
 * Multiples two quadratic extension field elements. Computes c = a * b.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the quadratic extension binary field element.
 * @param[in] b				- the quadratic extension binary field element.
 */
void fb2_mul(fb2_t c, fb2_t a, fb2_t b);

 /**
  * Multiples a quadratic extension field element by a quadratic non-residue.
  * Computes c = a * s.
  *
  * @param[out] c			- the result.
  * @param[in] a				- the quadratic extension binary field element.
  * @param[in] b				- the quadratic extension binary field element.
  */
 void fb2_mul_nor(fb2_t c, fb2_t a);

/**
 * Computes the square of a quadratic extension field element. Computes
 * c = a * a.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the binary field element to square.
 */
void fb2_sqr(fb2_t c, fb2_t a);

/**
 * Solves a quadratic equation for c, Tr(a) = 0. Computes c such that
 * c^2 + c = a.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the quadratic extension field element.
 */
void fb2_slv(fb2_t c, fb2_t a);

/**
 * Inverts a quadratic extension field element. Computes c = a^{-1}.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the quadratic extension field element to invert.
 */
void fb2_inv(fb2_t c, fb2_t a);

#endif /* !RLC_FBX_H */
