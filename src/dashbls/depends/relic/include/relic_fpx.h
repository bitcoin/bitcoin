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
 * @defgroup fpx Prime field extensions.
 */

/**
 * @file
 *
 * Interface of the module for prime extension field arithmetic.
 *
 * @ingroup fpx
 */

#ifndef RLC_FPX_H
#define RLC_FPX_H

#include "relic_fp.h"
#include "relic_types.h"

/*============================================================================*/
/* Type definitions                                                           */
/*============================================================================*/

/**
 * Represents a quadratic extension prime field element.
 *
 * This extension is constructed with the basis {1, i}, where i is an adjoined
 * square root in the prime field.
 */
typedef fp_t fp2_t[2];

/**
 * Represents a double-precision quadratic extension field element.
 */
typedef dv_t dv2_t[2];

/**
 * Represents a quadratic extension field element with automatic memory
 * allocation.
 */
typedef fp_st fp2_st[2];

/**
 * Represents a cubic extension prime field element.
 *
 * This extension is constructed with the basis {1, j, j^2}, where j is an
 * adjoined cube root in the prime field.
 */
typedef fp_t fp3_t[3];

/**
 * Represents a double-precision cubic extension field element.
 */
typedef dv_t dv3_t[3];

/**
 * Represents a cubic extension field element with automatic memory
 * allocation.
 */
typedef fp_st fp3_st[3];

/**
 * Represents a quartic extension prime field element.
 *
 * This extension is constructed with the basis {1, s}, where s^2 = E is an
 * adjoined root in the underlying quadratic extension.
 */
typedef fp2_t fp4_t[2];

/**
 * Represents a double-precision quartic extension field element.
 */
typedef dv2_t dv4_t[2];

/**
 * Represents a quartic extension field element with automatic memory
 * allocation.
 */
typedef fp2_st fp4_st[2];

/**
 * Represents a sextic extension field element.
 *
 * This extension is constructed with the basis {1, v, v^2}, where v^3 = E is an
 * adjoined root in the underlying quadratic extension.
 */
typedef fp2_t fp6_t[3];

/**
 * Represents a double-precision sextic extension field element.
 */
typedef dv2_t dv6_t[3];

/**
 * Represents a sextic extension field element with automatic memory allocation.
 */
typedef fp2_st fp6_st[3];

/**
 * Represents an octic extension prime field element.
 *
 * This extension is constructed with the basis {1, v}, where v^2 = s is an
 * adjoined root in the underlying quadratic extension.
 */
typedef fp4_t fp8_t[2];

/**
 * Represents a double-precision octic extension field element.
 */
typedef dv4_t dv8_t[2];

/**
 * Represents an octic extension field element with automatic memory
 * allocation.
 */
typedef fp4_st fp8_st[2];

/**
 * Represents an octic extension prime field element.
 *
 * This extension is constructed with the basis {1, v, v^2}, where v^3 = j is an
 * adjoined root in the underlying quadratic extension.
 */
typedef fp3_t fp9_t[3];

/**
 * Represents a double-precision octic extension field element.
 */
typedef dv3_t dv9_t[3];

/**
 * Represents an octic extension field element with automatic memory
 * allocation.
 */
typedef fp3_st fp9_st[3];

/**
 * Represents a dodecic extension field element.
 *
 * This extension is constructed with the basis {1, w}, where w^2 = v is an
 * adjoined root in the underlying sextic extension.
 */
typedef fp6_t fp12_t[2];

/**
 * Represents a double-precision dodecic extension field element.
 */
typedef dv6_t dv12_t[2];

/**
 * Represents an octdecic extension field element.
 *
 * This extension is constructed with the basis {1, w}, where w^2 = v is an
 * adjoined root in the underlying sextic extension.
 */
typedef fp9_t fp18_t[2];

/**
 * Represents a double-precision octdecic extension field element.
 */
typedef dv9_t dv18_t[2];

/**
 * Represents a 24-degree extension field element.
 *
 * This extension is constructed with the basis {1, t, t^2}, where t^3 = w is an
 * adjoined root in the underlying dodecic extension.
 */
typedef fp8_t fp24_t[3];

/**
 * Represents a double-precision 24-degree extension field element.
 */
typedef dv8_t dv24_t[3];

/**
 * Represents a 48-degree extension field element.
 *
 * This extension is constructed with the basis {1, u}, where u^2 = t is an
 * adjoined root in the underlying dodecic extension.
 */
typedef fp24_t fp48_t[2];

/**
 * Represents a 54-degree extension field element.
 *
 * This extension is constructed with the basis {1, u, u^2}, where u^3 = t is an
 * adjoined root in the underlying dodecic extension.
 */
typedef fp18_t fp54_t[3];

/*============================================================================*/
/* Macro definitions                                                          */
/*============================================================================*/

/**
 * Initializes a double-precision quadratic extension field element with null.
 *
* @param[out] A			- the quadratic extension element to initialize.
 */
#define dv2_null(A)															\
		dv_null(A[0]); dv_null(A[1]);										\


/**
 * Allocates a double-precision quadratic extension field element.
 *
 * @param[out] A			- the new quadratic extension field element.
 */
#define dv2_new(A)															\
		dv_new(A[0]); dv_new(A[1]);											\

/**
 * Frees a double-precision quadratic extension field element.
 *
 * @param[out] A			- the quadratic extension field element to free.
 */
#define dv2_free(A)															\
		dv_free(A[0]); dv_free(A[1]); 										\

/**
 * Initializes a quadratic extension field element with null.
 *
* @param[out] A			- the quadratic extension element to initialize.
 */
#define fp2_null(A)															\
		fp_null(A[0]); fp_null(A[1]);										\

/**
 * Allocates a quadratic extension field element.
 *
 * @param[out] A			- the new quadratic extension field element.
 */
#define fp2_new(A)															\
		fp_new(A[0]); fp_new(A[1]);											\

/**
 * Frees a quadratic extension field element.
 *
 * @param[out] A			- the quadratic extension field element to free.
 */
#define fp2_free(A)															\
		fp_free(A[0]); fp_free(A[1]); 										\

/**
 * Adds two quadratic extension field elements. Computes C = A + B.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the first quadratic extension field element.
 * @param[in] B				- the second quadratic extension field element.
 */
#if FPX_QDR == BASIC
#define fp2_add(C, A, B)	fp2_add_basic(C, A, B)
#elif FPX_QDR == INTEG
#define fp2_add(C, A, B)	fp2_add_integ(C, A, B)
#endif

/**
 * Subtracts a quadratic extension field element from another.
 * Computes C = A - B.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the first quadratic extension field element.
 * @param[in] B				- the second quadratic extension field element.
 */
#if FPX_QDR == BASIC
#define fp2_sub(C, A, B)	fp2_sub_basic(C, A, B)
#elif FPX_QDR == INTEG
#define fp2_sub(C, A, B)	fp2_sub_integ(C, A, B)
#endif

/**
 * Doubles a quadratic extension field element. Computes C = A + A.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the quadratic extension field element.
 */
#if FPX_QDR == BASIC
#define fp2_dbl(C, A)		fp2_dbl_basic(C, A)
#elif FPX_QDR == INTEG
#define fp2_dbl(C, A)		fp2_dbl_integ(C, A)
#endif

/**
 * Adds a quadratic extension field element and a digit. Computes c = a + b.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the quadratic extension field element.
 * @param[in] b				- the digit to add.
 */
void fp2_add_dig(fp2_t c, const fp2_t a, dig_t b);

/**
 * Subtracts a quadratic extension field element and a digit. Computes c = a - b.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the quadratic extension field element.
 * @param[in] b				- the digit to subtract.
 */
void fp2_sub_dig(fp2_t c, const fp2_t a, dig_t b);

/**
 * Multiplies two quadratic extension field elements. Computes C = A * B.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the first quadratic extension field element.
 * @param[in] B				- the second quadratic extension field element.
 */
#if FPX_QDR == BASIC
#define fp2_mul(C, A, B)	fp2_mul_basic(C, A, B)
#elif FPX_QDR == INTEG
#define fp2_mul(C, A, B)	fp2_mul_integ(C, A, B)
#endif

/**
 * Multiplies a quadratic extension field by the quadratic/cubic non-residue.
 * Computes C = A * E, where E is a non-square/non-cube in the quadratic
 * extension.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the quadratic extension field element to multiply.
 */
#if FPX_QDR == BASIC
#define fp2_mul_nor(C, A)	fp2_mul_nor_basic(C, A)
#elif FPX_QDR == INTEG
#define fp2_mul_nor(C, A)	fp2_mul_nor_integ(C, A)
#endif

/**
 * Squares a quadratic extension field element. Computes C = A * A.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the quadratic extension field element to square.
 */
#if FPX_QDR == BASIC
#define fp2_sqr(C, A)		fp2_sqr_basic(C, A)
#elif FPX_QDR == INTEG
#define fp2_sqr(C, A)		fp2_sqr_integ(C, A)
#endif

/**
 * Initializes a double-precision cubic extension field element with a null
 * value.
 *
* @param[out] A			- the cubic extension element to initialize.
 */
#define dv3_null(A)															\
		dv_null(A[0]); dv_null(A[1]); dv_null(A[2]);						\

/**
 * Allocates a double-precision cubic extension field element.
 *
 * @param[out] A			- the new cubic extension field element.
 */
#define dv3_new(A)															\
		dv_new(A[0]); dv_new(A[1]);	dv_new(A[2]);							\

/**
 * Frees a double-precision cubic extension field element.
 *
 * @param[out] A			- the cubic extension field element to free.
 */
#define dv3_free(A)															\
		dv_free(A[0]); dv_free(A[1]); dv_free(A[2]);						\

/**
 * Initializes a cubic extension field element with null.
 *
* @param[out] A			- the cubic extension element to initialize.
 */
#define fp3_null(A)															\
		fp_null(A[0]); fp_null(A[1]); fp_null(A[2]);						\

/**
 * Allocates a cubic extension field element.
 *
 * @param[out] A			- the new cubic extension field element.
 */
#define fp3_new(A)															\
		fp_new(A[0]); fp_new(A[1]);	fp_new(A[2]);							\

/**
 * Frees a cubic extension field element.
 *
 * @param[out] A			- the cubic extension field element to free.
 */
#define fp3_free(A)															\
		fp_free(A[0]); fp_free(A[1]); fp_free(A[2]);						\

/**
 * Adds two cubic extension field elements. Computes C = A + B.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the first cubic extension field element.
 * @param[in] B				- the second cubic extension field element.
 */
#if FPX_CBC == BASIC
#define fp3_add(C, A, B)	fp3_add_basic(C, A, B)
#elif FPX_CBC == INTEG
#define fp3_add(C, A, B)	fp3_add_integ(C, A, B)
#endif

/**
 * Subtracts a cubic extension field element from another.
 * Computes C = A - B.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the first cubic extension field element.
 * @param[in] B				- the second cubic extension field element.
 */
#if FPX_CBC == BASIC
#define fp3_sub(C, A, B)	fp3_sub_basic(C, A, B)
#elif FPX_CBC == INTEG
#define fp3_sub(C, A, B)	fp3_sub_integ(C, A, B)
#endif

/**
 * Doubles a cubic extension field element. Computes C = A + A.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the cubic extension field element.
 */
#if FPX_CBC == BASIC
#define fp3_dbl(C, A)		fp3_dbl_basic(C, A)
#elif FPX_CBC == INTEG
#define fp3_dbl(C, A)		fp3_dbl_integ(C, A)
#endif

/**
 * Multiplies two cubic extension field elements. Computes C = A * B.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the first cubic extension field element.
 * @param[in] B				- the second cubic extension field element.
 */
#if FPX_CBC == BASIC
#define fp3_mul(C, A, B)	fp3_mul_basic(C, A, B)
#elif FPX_CBC == INTEG
#define fp3_mul(C, A, B)	fp3_mul_integ(C, A, B)
#endif

/**
 * Squares a cubic extension field element. Computes C = A * A.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the cubic extension field element to square.
 */
#if FPX_CBC == BASIC
#define fp3_sqr(C, A)		fp3_sqr_basic(C, A)
#elif FPX_CBC == INTEG
#define fp3_sqr(C, A)		fp3_sqr_integ(C, A)
#endif

/**
 * Initializes a double-precision quartic extension field with null.
 *
 * @param[out] A			- the quartic extension element to initialize.
 */
#define dv4_null(A)															\
		dv2_null(A[0]); dv2_null(A[1]);										\

/**
 * Allocates a double-precision quartic extension field element.
 *
 * @param[out] A			- the new quartic extension field element.
 */
#define dv4_new(A)															\
		dv2_new(A[0]); dv2_new(A[1]);										\

/**
 * Frees a double-precision quartic extension field element.
 *
 * @param[out] A			- the quartic extension field element to free.
 */
#define dv4_free(A)															\
		dv2_free(A[0]); dv2_free(A[1]);										\

/**
 * Initializes a quartic extension field with null.
 *
 * @param[out] A			- the quartic extension element to initialize.
 */
#define fp4_null(A)															\
		fp2_null(A[0]); fp2_null(A[1]);										\

/**
 * Allocates a quartic extension field element.
 *
 * @param[out] A			- the new quartic extension field element.
 */
#define fp4_new(A)															\
		fp2_new(A[0]); fp2_new(A[1]);										\

/**
 * Frees a quartic extension field element.
 *
 * @param[out] A			- the quartic extension field element to free.
 */
#define fp4_free(A)															\
		fp2_free(A[0]); fp2_free(A[1]);										\

/**
 * Multiplies two quartic extension field elements. Computes C = A * B.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the first quartic extension field element.
 * @param[in] B				- the second quartic extension field element.
 */
#if FPX_RDC == BASIC
#define fp4_mul(C, A, B)	fp4_mul_basic(C, A, B)
#elif FPX_RDC == LAZYR
#define fp4_mul(C, A, B)	fp4_mul_lazyr(C, A, B)
#endif

/**
 * Squares a quartic extension field element. Computes C = A * A.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the quartic extension field element to square.
 */
#if FPX_RDC == BASIC
#define fp4_sqr(C, A)		fp4_sqr_basic(C, A)
#elif FPX_RDC == LAZYR
#define fp4_sqr(C, A)		fp4_sqr_lazyr(C, A)
#endif

/**
 * Initializes a double-precision sextic extension field with null.
 *
 * @param[out] A			- the sextic extension element to initialize.
 */
#define dv6_null(A)															\
		dv2_null(A[0]); dv2_null(A[1]); dv2_null(A[2]);						\

/**
 * Allocates a double-precision sextic extension field element.
 *
 * @param[out] A			- the new sextic extension field element.
 */
#define dv6_new(A)															\
		dv2_new(A[0]); dv2_new(A[1]); dv2_new(A[2]);						\

/**
 * Frees a double-precision sextic extension field element.
 *
 * @param[out] A			- the sextic extension field element to free.
 */
#define dv6_free(A)															\
		dv2_free(A[0]); dv2_free(A[1]); dv2_free(A[2]); 					\

/**
 * Initializes a sextic extension field with null.
 *
 * @param[out] A			- the sextic extension element to initialize.
 */
#define fp6_null(A)															\
		fp2_null(A[0]); fp2_null(A[1]); fp2_null(A[2]);						\

/**
 * Allocates a sextic extension field element.
 *
 * @param[out] A			- the new sextic extension field element.
 */
#define fp6_new(A)															\
		fp2_new(A[0]); fp2_new(A[1]); fp2_new(A[2]);						\

/**
 * Frees a sextic extension field element.
 *
 * @param[out] A			- the sextic extension field element to free.
 */
#define fp6_free(A)															\
		fp2_free(A[0]); fp2_free(A[1]); fp2_free(A[2]); 					\

/**
 * Multiplies two sextic extension field elements. Computes C = A * B.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the first sextic extension field element.
 * @param[in] B				- the second sextic extension field element.
 */
#if FPX_RDC == BASIC
#define fp6_mul(C, A, B)	fp6_mul_basic(C, A, B)
#elif FPX_RDC == LAZYR
#define fp6_mul(C, A, B)	fp6_mul_lazyr(C, A, B)
#endif

/**
 * Squares a sextic extension field element. Computes C = A * A.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the sextic extension field element to square.
 */
#if FPX_RDC == BASIC
#define fp6_sqr(C, A)		fp6_sqr_basic(C, A)
#elif FPX_RDC == LAZYR
#define fp6_sqr(C, A)		fp6_sqr_lazyr(C, A)
#endif

/**
 * Initializes a double-precision octic extension field with null.
 *
 * @param[out] A			- the octic extension element to initialize.
 */
#define dv8_null(A)															\
		dv4_null(A[0]); dv4_null(A[1]);										\

/**
 * Allocates a double-precision octic extension field element.
 *
 * @param[out] A			- the new octic extension field element.
 */
#define dv8_new(A)															\
		dv4_new(A[0]); dv4_new(A[1]);										\

/**
 * Frees a double-precision octic extension field element.
 *
 * @param[out] A			- the octic extension field element to free.
 */
#define dv8_free(A)															\
		dv4_free(A[0]); dv4_free(A[1]);										\

/**
 * Initializes an octic extension field with null.
 *
 * @param[out] A			- the octic extension element to initialize.
 */
#define fp8_null(A)															\
		fp4_null(A[0]); fp4_null(A[1]);										\

/**
 * Allocates an octic extension field element.
 *
 * @param[out] A			- the new octic extension field element.
 */
#define fp8_new(A)															\
		fp4_new(A[0]); fp4_new(A[1]);										\

/**
 * Frees an octic extension field element.
 *
 * @param[out] A			- the octic extension field element to free.
 */
#define fp8_free(A)															\
		fp4_free(A[0]); fp4_free(A[1]);										\

/**
 * Multiplies two octic extension field elements. Computes C = A * B.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the first octic extension field element.
 * @param[in] B				- the second octic extension field element.
 */
#if FPX_RDC == BASIC
#define fp8_mul(C, A, B)	fp8_mul_basic(C, A, B)
#elif FPX_RDC == LAZYR
#define fp8_mul(C, A, B)	fp8_mul_lazyr(C, A, B)
#endif

/**
 * Squares an octic extension field element. Computes C = A * A.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the octic extension field element to square.
 */
#if FPX_RDC == BASIC
#define fp8_sqr(C, A)		fp8_sqr_basic(C, A)
#elif FPX_RDC == LAZYR
#define fp8_sqr(C, A)		fp8_sqr_lazyr(C, A)
#endif

/**
 * Initializes a double-precision nonic extension field with null.
 *
 * @param[out] A			- the octic extension element to initialize.
 */
#define dv9_null(A)															\
		dv3_null(A[0]); dv3_null(A[1]);	dv3_null(A[2]);						\

/**
 * Allocates a double-precision nonic extension field element.
 *
 * @param[out] A			- the new nonic extension field element.
 */
#define dv9_new(A)															\
		dv3_new(A[0]); dv3_new(A[1]); dv3_new(A[2]);						\

/**
 * Frees a double-precision nonic extension field element.
 *
 * @param[out] A			- the nonic extension field element to free.
 */
#define dv9_free(A)															\
		dv3_free(A[0]); dv3_free(A[1]); dv3_free(A[2]);						\

/**
 * Initializes a nonic extension field with null.
 *
 * @param[out] A			- the nonic extension element to initialize.
 */
#define fp9_null(A)															\
		fp3_null(A[0]); fp3_null(A[1]); fp3_null(A[2]);						\

/**
 * Allocates a nonic extension field element.
 *
 * @param[out] A			- the new nonic extension field element.
 */
#define fp9_new(A)															\
		fp3_new(A[0]); fp3_new(A[1]); fp3_new(A[2]);						\

/**
 * Frees a nonic extension field element.
 *
 * @param[out] A			- the nonic extension field element to free.
 */
#define fp9_free(A)															\
		fp3_free(A[0]); fp3_free(A[1]); fp3_free(A[2]);						\

/**
 * Multiplies two nonic extension field elements. Computes C = A * B.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the first nonic extension field element.
 * @param[in] B				- the second nonic extension field element.
 */
#if FPX_RDC == BASIC
#define fp9_mul(C, A, B)	fp9_mul_basic(C, A, B)
#elif FPX_RDC == LAZYR
#define fp9_mul(C, A, B)	fp9_mul_lazyr(C, A, B)
#endif

/**
 * Squares a nonic extension field element. Computes C = A * A.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the nonic extension field element to square.
 */
#if FPX_RDC == BASIC
#define fp9_sqr(C, A)		fp9_sqr_basic(C, A)
#elif FPX_RDC == LAZYR
#define fp9_sqr(C, A)		fp9_sqr_lazyr(C, A)
#endif

/**
 * Initializes a double-precision dodecic extension field with null.
 *
 * @param[out] A			- the dodecic extension element to initialize.
 */
#define dv12_null(A)														\
		dv6_null(A[0]); dv6_null(A[1]);										\

/**
 * Allocates a double-precision dodecic extension field element.
 *
 * @param[out] A			- the new dodecic extension field element.
 */
#define dv12_new(A)															\
		dv6_new(A[0]); dv6_new(A[1]);										\

/**
 * Frees a double-precision dodecic extension field element.
 *
 * @param[out] A			- the dodecic extension field element to free.
 */
#define dv12_free(A)														\
		dv6_free(A[0]); dv6_free(A[1]);										\

/**
 * Initializes a dodecic extension field with null.
 *
 * @param[out] A			- the dodecic extension element to initialize.
 */
#define fp12_null(A)														\
		fp6_null(A[0]); fp6_null(A[1]);										\

/**
 * Allocates a dodecic extension field element.
 *
 * @param[out] A			- the new dodecic extension field element.
 */
#define fp12_new(A)															\
		fp6_new(A[0]); fp6_new(A[1]);										\

/**
 * Frees a dodecic extension field element.
 *
 * @param[out] A			- the dodecic extension field element to free.
 */
#define fp12_free(A)														\
		fp6_free(A[0]); fp6_free(A[1]); 									\

/**
 * Multiplies two dodecic extension field elements. Computes C = A * B.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the first dodecic extension field element.
 * @param[in] B				- the second dodecic extension field element.
 */
#if FPX_RDC == BASIC
#define fp12_mul(C, A, B)		fp12_mul_basic(C, A, B)
#elif FPX_RDC == LAZYR
#define fp12_mul(C, A, B)		fp12_mul_lazyr(C, A, B)
#endif

/**
 * Multiplies a dense and a sparse dodecic extension field elements. Computes
 * C = A * B.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the dense dodecic extension field element.
 * @param[in] B				- the sparse dodecic extension field element.
 */
#if FPX_RDC == BASIC
#define fp12_mul_dxs(C, A, B)	fp12_mul_dxs_basic(C, A, B)
#elif FPX_RDC == LAZYR
#define fp12_mul_dxs(C, A, B)	fp12_mul_dxs_lazyr(C, A, B)
#endif

/**
 * Squares a dodecic extension field element. Computes C = A * A.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the dodecic extension field element to square.
 */
#if FPX_RDC == BASIC
#define fp12_sqr(C, A)			fp12_sqr_basic(C, A)
#elif FPX_RDC == LAZYR
#define fp12_sqr(C, A)			fp12_sqr_lazyr(C, A)
#endif

/**
 * Squares a dodecic extension field element in the cyclotomic subgroup.
 * Computes C = A * A.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the dodecic extension field element to square.
 */
#if FPX_RDC == BASIC
#define fp12_sqr_cyc(C, A)		fp12_sqr_cyc_basic(C, A)
#elif FPX_RDC == LAZYR
#define fp12_sqr_cyc(C, A)		fp12_sqr_cyc_lazyr(C, A)
#endif

/**
 * Squares a dodecic extension field element in the cyclotomic subgroup in
 * compressed form. Computes C = A * A.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the dodecic extension field element to square.
 */
#if FPX_RDC == BASIC
#define fp12_sqr_pck(C, A)		fp12_sqr_pck_basic(C, A)
#elif FPX_RDC == LAZYR
#define fp12_sqr_pck(C, A)		fp12_sqr_pck_lazyr(C, A)
#endif

/**
 * Initializes a double-precision sextic extension field with null.
 *
 * @param[out] A			- the sextic extension element to initialize.
 */
#define dv18_null(A)														\
		dv9_null(A[0]); dv9_null(A[1]);										\

/**
 * Allocates a double-precision sextic extension field element.
 *
 * @param[out] A			- the new sextic extension field element.
 */
#define dv18_new(A)															\
		dv9_new(A[0]); dv9_new(A[1]);										\

/**
 * Frees a double-precision sextic extension field element.
 *
 * @param[out] A			- the sextic extension field element to free.
 */
#define dv18_free(A)														\
		dv9_free(A[0]); dv9_free(A[1]);										\

/**
 * Initializes an octdecic extension field with null.
 *
 * @param[out] A			- the octdecic extension element to initialize.
 */
#define fp18_null(A)														\
		fp9_null(A[0]); fp9_null(A[1]);										\

/**
 * Allocates an octdecic extension field element.
 *
 * @param[out] A			- the new octdecic extension field element.
 */
#define fp18_new(A)															\
		fp9_new(A[0]); fp9_new(A[1]);										\

/**
 * Frees an octdecic extension field element.
 *
 * @param[out] A			- the octdecic extension field element to free.
 */
#define fp18_free(A)														\
		fp9_free(A[0]); fp9_free(A[1]);										\

/**
 * Multiplies two octdecic extension field elements. Computes C = A * B.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the first octdecic extension field element.
 * @param[in] B				- the second octdecic extension field element.
 */
#if FPX_RDC == BASIC
#define fp18_mul(C, A, B)		fp18_mul_basic(C, A, B)
#elif FPX_RDC == LAZYR
#define fp18_mul(C, A, B)		fp18_mul_lazyr(C, A, B)
#endif

/**
 * Multiplies a dense and a sparse octdecic extension field elements. Computes
 * C = A * B.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the dense octdecic extension field element.
 * @param[in] B				- the sparse octdecic extension field element.
 */
#if FPX_RDC == BASIC
#define fp18_mul_dxs(C, A, B)	fp18_mul_dxs_basic(C, A, B)
#elif FPX_RDC == LAZYR
#define fp18_mul_dxs(C, A, B)	fp18_mul_dxs_lazyr(C, A, B)
#endif

/**
 * Squares an octdecic extension field element. Computes C = A * A.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the octdecic extension field element to square.
 */
#if FPX_RDC == BASIC
#define fp18_sqr(C, A)			fp18_sqr_basic(C, A)
#elif FPX_RDC == LAZYR
#define fp18_sqr(C, A)			fp18_sqr_lazyr(C, A)
#endif

/**
 * Initializes a double-precision 24-degree extension field with null.
 *
 * @param[out] A			- the 24-degree extension element to initialize.
 */
#define dv24_null(A)														\
		dv8_null(A[0]); dv8_null(A[1]); dv8_null(A[2]);						\

/**
 * Allocates a double-precision 24-degree extension field element.
 *
 * @param[out] A			- the new 24-degree extension field element.
 */
#define dv24_new(A)															\
		dv8_new(A[0]); dv8_new(A[1]); dv8_new(A[2]);						\

/**
 * Frees a double-precision 24-degree extension field element.
 *
 * @param[out] A			- the 24-degree extension field element to free.
 */
#define dv24_free(A)														\
		dv8_free(A[0]); dv8_free(A[1]); dv8_free(A[2]); 					\

/**
 * Initializes a 24-degree extension field with null.
 *
 * @param[out] A			- the 24-degree extension element to initialize.
 */
#define fp24_null(A)														\
		fp8_null(A[0]); fp8_null(A[1]); fp8_null(A[2]);						\

/**
 * Allocates a 24-degree extension field element.
 *
 * @param[out] A			- the new 24-degree extension field element.
 */
#define fp24_new(A)															\
		fp8_new(A[0]); fp8_new(A[1]); fp8_new(A[2]);						\

/**
 * Frees a 24-degree extension field element.
 *
 * @param[out] A			- the 24-degree extension field element to free.
 */
#define fp24_free(A)														\
		fp8_free(A[0]); fp8_free(A[1]); fp8_free(A[2]); 					\

/**
 * Multiplies two 24-degree extension field elements. Computes C = A * B.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the first 24-degree extension field element.
 * @param[in] B				- the second 24-degree extension field element.
 */
#if FPX_RDC == BASIC
#define fp24_mul(C, A, B)		fp24_mul_basic(C, A, B)
#elif FPX_RDC == LAZYR
#define fp24_mul(C, A, B)		fp24_mul_lazyr(C, A, B)
#endif

/**
 * Squares a 24-degree extension field element. Computes C = A * A.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the 24-degree extension field element to square.
 */
#if FPX_RDC == BASIC
#define fp24_sqr(C, A)			fp24_sqr_basic(C, A)
#elif FPX_RDC == LAZYR
#define fp24_sqr(C, A)			fp24_sqr_lazyr(C, A)
#endif

/**
 * Squares a 24-degree extension field element in the cyclotomic subgroup.
 * Computes C = A * A.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the 24-degree extension field element to square.
 */
#if FPX_RDC == BASIC
#define fp24_sqr_cyc(C, A)		fp24_sqr_cyc_basic(C, A)
#elif FPX_RDC == LAZYR
#define fp24_sqr_cyc(C, A)		fp24_sqr_cyc_lazyr(C, A)
#endif

/**
 * Squares a 24-degree extension field element in the cyclotomic subgroup in
 * compressed form. Computes C = A * A.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the 24-degree extension field element to square.
 */
#if FPX_RDC == BASIC
#define fp24_sqr_pck(C, A)		fp24_sqr_pck_basic(C, A)
#elif FPX_RDC == LAZYR
#define fp24_sqr_pck(C, A)		fp24_sqr_pck_lazyr(C, A)
#endif

/**
 * Initializes a double-precision 48-degree extension field with null.
 *
 * @param[out] A			- the 48-degree extension element to initialize.
 */
#define dv48_null(A)														\
		dv24_null(A[0]); dv24_null(A[1]);									\

/**
 * Allocates a double-precision 48-degree extension field element.
 *
 * @param[out] A			- the new 48-degree extension field element.
 */
#define dv48_new(A)															\
		dv24_new(A[0]); dv24_new(A[1]);										\

/**
 * Frees a double-precision 48-degree extension field element.
 *
 * @param[out] A			- the 48-degree extension field element to free.
 */
#define dv48_free(A)														\
		dv24_free(A[0]); dv24_free(A[1]);									\

/**
 * Initializes a 48-degree extension field with null.
 *
 * @param[out] A			- the 48-degree extension element to initialize.
 */
#define fp48_null(A)														\
		fp24_null(A[0]); fp24_null(A[1]);									\

/**
 * Allocates a 48-degree extension field element.
 *
 * @param[out] A			- the new 48-degree extension field element.
 */
#define fp48_new(A)															\
		fp24_new(A[0]); fp24_new(A[1]);										\

/**
 * Frees a 48-degree extension field element.
 *
 * @param[out] A			- the 48-degree extension field element to free.
 */
#define fp48_free(A)														\
		fp24_free(A[0]); fp24_free(A[1]); 									\

/**
 * Multiplies two 48-degree extension field elements. Computes C = A * B.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the first 48-degree extension field element.
 * @param[in] B				- the second 48-degree extension field element.
 */
#if FPX_RDC == BASIC
#define fp48_mul(C, A, B)		fp48_mul_basic(C, A, B)
#elif FPX_RDC == LAZYR
#define fp48_mul(C, A, B)		fp48_mul_lazyr(C, A, B)
#endif

/**
 * Squares a 48-degree extension field element. Computes C = A * A.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the 48-degree extension field element to square.
 */
#if FPX_RDC == BASIC
#define fp48_sqr(C, A)			fp48_sqr_basic(C, A)
#elif FPX_RDC == LAZYR
#define fp48_sqr(C, A)			fp48_sqr_lazyr(C, A)
#endif

/**
 * Squares a 48-degree extension field element in the cyclotomic subgroup.
 * Computes C = A * A.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the 48-degree extension field element to square.
 */
#if FPX_RDC == BASIC
#define fp48_sqr_cyc(C, A)		fp48_sqr_cyc_basic(C, A)
#elif FPX_RDC == LAZYR
#define fp48_sqr_cyc(C, A)		fp48_sqr_cyc_lazyr(C, A)
#endif

/**
 * Squares a 48-degree extension field element in the cyclotomic subgroup in
 * compressed form. Computes C = A * A.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the 48-degree extension field element to square.
 */
#if FPX_RDC == BASIC
#define fp48_sqr_pck(C, A)		fp48_sqr_pck_basic(C, A)
#elif FPX_RDC == LAZYR
#define fp48_sqr_pck(C, A)		fp48_sqr_pck_lazyr(C, A)
#endif

/**
 * Initializes a double-precision 54-degree extension field with null.
 *
 * @param[out] A			- the 54-degree extension element to initialize.
 */
#define dv54_null(A)														\
		dv18_null(A[0]); dv18_null(A[1]); dv18_null(A[2]);					\

/**
 * Allocates a double-precision 54-degree extension field element.
 *
 * @param[out] A			- the new 54-degree extension field element.
 */
#define dv54_new(A)															\
		dv18_new(A[0]); dv18_new(A[1]);	dv18_new(A[2]);						\

/**
 * Frees a double-precision 54-degree extension field element.
 *
 * @param[out] A			- the 54-degree extension field element to free.
 */
#define dv54_free(A)														\
		dv18_free(A[0]); dv18_free(A[1]); dv18_free(A[2]);					\

/**
 * Initializes a 54-degree extension field with null.
 *
 * @param[out] A			- the 54-degree extension element to initialize.
 */
#define fp54_null(A)														\
		fp18_null(A[0]); fp18_null(A[1]); fp18_null(A[2]);					\

/**
 * Allocates a 54-degree extension field element.
 *
 * @param[out] A			- the new 54-degree extension field element.
 */
#define fp54_new(A)															\
		fp18_new(A[0]); fp18_new(A[1]);	fp18_new(A[2]);						\

/**
 * Frees a 54-degree extension field element.
 *
 * @param[out] A			- the 54-degree extension field element to free.
 */
#define fp54_free(A)														\
		fp18_free(A[0]); fp18_free(A[1]); fp18_free(A[2]);					\

/**
 * Multiplies two 54-degree extension field elements. Computes C = A * B.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the first 54-degree extension field element.
 * @param[in] B				- the second 54-degree extension field element.
 */
#if FPX_RDC == BASIC
#define fp54_mul(C, A, B)		fp54_mul_basic(C, A, B)
#elif FPX_RDC == LAZYR
#define fp54_mul(C, A, B)		fp54_mul_lazyr(C, A, B)
#endif

/**
 * Squares a 54-degree extension field element. Computes C = A * A.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the 54-degree extension field element to square.
 */
#if FPX_RDC == BASIC
#define fp54_sqr(C, A)			fp54_sqr_basic(C, A)
#elif FPX_RDC == LAZYR
#define fp54_sqr(C, A)			fp54_sqr_lazyr(C, A)
#endif

/**
 * Squares a 54-degree extension field element in the cyclotomic subgroup.
 * Computes C = A * A.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the 54-degree extension field element to square.
 */
#if FPX_RDC == BASIC
#define fp54_sqr_cyc(C, A)		fp54_sqr_cyc_basic(C, A)
#elif FPX_RDC == LAZYR
#define fp54_sqr_cyc(C, A)		fp54_sqr_cyc_lazyr(C, A)
#endif

/**
 * Squares a 54-degree extension field element in the cyclotomic subgroup in
 * compressed form. Computes C = A * A.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the 54-degree extension field element to square.
 */
#if FPX_RDC == BASIC
#define fp54_sqr_pck(C, A)		fp54_sqr_pck_basic(C, A)
#elif FPX_RDC == LAZYR
#define fp54_sqr_pck(C, A)		fp54_sqr_pck_lazyr(C, A)
#endif

/*============================================================================*/
/* Function prototypes                                                        */
/*============================================================================*/

/**
 * Initializes the quadratic extension field arithmetic module.
 */
void fp2_field_init(void);

/**
 * Return the integer part (u) of the quadratic non-residue (i + u).
 */
int fp2_field_get_qnr(void);

/**
 * Copies the second argument to the first argument.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the quadratic extension field element to copy.
 */
void fp2_copy(fp2_t c, fp2_t a);

/**
 * Assigns zero to a quadratic extension field element.
 *
 * @param[out] a			- the quadratic extension field element to zero.
 */
void fp2_zero(fp2_t a);

/**
 * Tests if a quadratic extension field element is zero or not.
 *
 * @param[in] a				- the quadratic extension field element to test.
 * @return 1 if the argument is zero, 0 otherwise.
 */
int fp2_is_zero(fp2_t a);

/**
 * Assigns a random value to a quadratic extension field element.
 *
 * @param[out] a			- the quadratic extension field element to assign.
 */
void fp2_rand(fp2_t a);

/**
 * Prints a quadratic extension field element to standard output.
 *
 * @param[in] a				- the quadratic extension field element to print.
 */
void fp2_print(fp2_t a);

/**
 * Returns the number of bytes necessary to store a quadratic extension field
 * element.
 *
 * @param[in] a				- the extension field element.
 * @param[in] pack			- the flag to indicate compression.
 * @return the number of bytes.
 */
int fp2_size_bin(fp2_t a, int pack);

/**
 * Reads a quadratic extension field element from a byte vector in big-endian
 * format.
 *
 * @param[out] a			- the result.
 * @param[in] bin			- the byte vector.
 * @param[in] len			- the buffer capacity.
 * @throw ERR_NO_BUFFER		- if the buffer capacity is not correct.
 */
void fp2_read_bin(fp2_t a, const uint8_t *bin, int len);

/**
 * Writes a quadratic extension field element to a byte vector in big-endian
 * format.
 *
 * @param[out] bin			- the byte vector.
 * @param[in] len			- the buffer capacity.
 * @param[in] a				- the extension field element to write.
 * @param[in] pack			- the flag to indicate compression.
 * @throw ERR_NO_BUFFER		- if the buffer capacity is not correct.
 */
void fp2_write_bin(uint8_t *bin, int len, fp2_t a, int pack);

/**
 * Returns the result of a comparison between two quadratic extension field
 * elements.
 *
 * @param[in] a				- the first quadratic extension field element.
 * @param[in] b				- the second quadratic extension field element.
 * @return RLC_EQ if a == b, and RLC_NE otherwise.
 */
int fp2_cmp(fp2_t a, fp2_t b);

/**
 * Returns the result of a signed comparison between a quadratic extension field
 * element and a digit.
 *
 * @param[in] a				- the quadratic extension field element.
 * @param[in] b				- the digit.
 * @return RLC_EQ if a == b, and RLC_NE otherwise.
 */
int fp2_cmp_dig(fp2_t a, dig_t b);

/**
 * Assigns a quadratic extension field element to a digit.
 *
 * @param[in] a				- the quadratic extension field element.
 * @param[in] b				- the digit.
 */
void fp2_set_dig(fp2_t a, dig_t b);

/**
 * Adds two quadratic extension field elements using basic arithmetic.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first quadratic extension field element.
 * @param[in] b				- the second quadratic extension field element.
 */
void fp2_add_basic(fp2_t c, fp2_t a, fp2_t b);

/**
 * Adds two quadratic extension field elements using integrated modular
 * reduction.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first quadratic extension field element.
 * @param[in] b				- the second quadratic extension field element.
 */
void fp2_add_integ(fp2_t c, fp2_t a, fp2_t b);

/**
 * Subtracts a quadratic extension field element from another using basic
 * arithmetic.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first quadratic extension field element.
 * @param[in] b				- the second quadratic extension field element.
 */
void fp2_sub_basic(fp2_t c, fp2_t a, fp2_t b);

/**
 * Subtracts a quadratic extension field element from another using integrated
 * modular reduction.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first quadratic extension field element.
 * @param[in] b				- the second quadratic extension field element.
 */
void fp2_sub_integ(fp2_t c, fp2_t a, fp2_t b);

/**
 * Negates a quadratic extension field element.
 *
 * @param[out] c			- the result.
 * @param[out] a			- the quadratic extension field element to negate.
 */
void fp2_neg(fp2_t c, fp2_t a);

/**
 * Doubles a quadratic extension field element using basic arithmetic.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the quadratic extension field element to double.
 */
void fp2_dbl_basic(fp2_t c, fp2_t a);

/**
 * Doubles a quadratic extension field element using integrated modular
 * reduction.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the quadratic extension field element to double.
 */
void fp2_dbl_integ(fp2_t c, fp2_t a);

/**
 * Multiples two quadratic extension field elements using basic arithmetic.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first quadratic extension field element.
 * @param[in] b				- the second quadratic extension field element.
 */
void fp2_mul_basic(fp2_t c, fp2_t a, fp2_t b);

/**
 * Multiples two quadratic extension field elements using integrated modular
 * reduction.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first quadratic extension field element.
 * @param[in] b				- the second quadratic extension field element.
 */
void fp2_mul_integ(fp2_t c, fp2_t a, fp2_t b);

/**
 * Multiplies a quadratic extension field element by the adjoined root.
 * Computes c = a * i.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the quadratic extension field element to multiply.
 */
void fp2_mul_art(fp2_t c, fp2_t a);

/**
 * Multiplies a quadratic extension field element by a quadratic/cubic
 * non-residue.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the quadratic extension field element to multiply.
 */
void fp2_mul_nor_basic(fp2_t c, fp2_t a);

/**
 * Multiplies a quadratic extension field element by a quadratic/cubic
 * non-residue using integrated modular reduction.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the quadratic extension field element to multiply.
 */
void fp2_mul_nor_integ(fp2_t c, fp2_t a);

/**
 * Multiplies a quadratic extension field element by a power of the constant
 * needed to compute a power of the Frobenius map.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the field element to multiply.
 * @param[in] i				- the power of the Frobenius map.
 * @param[in] j				- the power of the constant.
 */
void fp2_mul_frb(fp2_t c, fp2_t a, int i, int j);

/**
 * Multiplies a quadratic extension field element by a digit.
 * Computes c = a * b.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the quadratic extension field element.
 * @param[in] b				- the digit to multiply.
 */
void fp2_mul_dig(fp2_t c, const fp2_t a, dig_t b);

/**
 * Computes the square of a quadratic extension field element using basic
 * arithmetic.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the quadratic extension field element to square.
 */
void fp2_sqr_basic(fp2_t c, fp2_t a);

/**
 * Computes the square of a quadratic extension field element using integrated
 * modular reduction.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the quadratic extension field element to square.
 */
void fp2_sqr_integ(fp2_t c, fp2_t a);

/**
 * Inverts a quadratic extension field element. Computes c = 1/a.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the quadratic extension field element to invert.
 */
void fp2_inv(fp2_t c, fp2_t a);

/**
 * Computes the inverse of a cyclotomic quadratic extension field element.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the quadratic extension field element to invert.
 */
void fp2_inv_cyc(fp2_t c, fp2_t a);

/**
 * Inverts multiple quadratic extension field elements simultaneously.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the quadratic extension field elements to invert.
 * @param[in] n				- the number of elements.
 */
void fp2_inv_sim(fp2_t *c, fp2_t *a, int n);

/**
 * Tests if a quadratic extension field element is cyclotomic.
 *
 * @param[in] a				- the quadratic extension field element to test.
 * @return 1 if the extension field element is cyclotomic, 0 otherwise.
 */
int fp2_test_cyc(fp2_t a);

/**
 * Converts a quadratic extension field element to a cyclotomic element.
 * Computes c = a^(p - 1).
 *
 * @param[out] c			- the result.
 * @param[in] a				- the quadratic extension field element.
 */
void fp2_conv_cyc(fp2_t c, fp2_t a);

/**
 * Computes a power of a quadratic extension field element.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the quadratic extension element to exponentiate.
 * @param[in] b				- the exponent.
 */
void fp2_exp(fp2_t c, fp2_t a, bn_t b);

/**
 * Computes a power of a quadratic extension field element by a small exponent.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the basis.
 * @param[in] b				- the exponent.
 */
void fp2_exp_dig(fp2_t c, fp2_t a, dig_t b);

/**
 * Computes a power of a cyclotomic quadratic extension field element.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the cyclotomic element to exponentiate.
 * @param[in] b				- the exponent.
 */
void fp2_exp_cyc(fp2_t c, fp2_t a, bn_t b);

/**
 * Computes a power of the Frobenius map of a quadratic extension field element.
 * When i is odd, this is the same as computing the conjugate of the extension
 * field element.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the quadratic extension element to conjugate.
 * @param[in] i				- the power of the Frobenius map.
 */
void fp2_frb(fp2_t c, fp2_t a, int i);

/**
 * Extracts the square root of a quadratic extension field element. Computes
 * c = sqrt(a). The other square root is the negation of c.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the extension field element.
 * @return					- 1 if there is a square root, 0 otherwise.
 */
int fp2_srt(fp2_t c, fp2_t a);

/**
 * Compresses an extension field element.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the extension field element to compress.
 */
void fp2_pck(fp2_t c, fp2_t a);

/**
 * Decompresses a quadratic extension field element.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the quadratic extension field element.
 * @return if the decompression was successful
 */
int fp2_upk(fp2_t c, fp2_t a);

/**
 * Initializes the cubic extension field arithmetic module.
 */
void fp3_field_init(void);

/**
 * Copies the second argument to the first argument.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the cubic extension field element to copy.
 */
void fp3_copy(fp3_t c, fp3_t a);

/**
 * Assigns zero to a cubic extension field element.
 *
 * @param[out] a			- the cubic extension field element to zero.
 */
void fp3_zero(fp3_t a);

/**
 * Tests if a cubic extension field element is zero or not.
 *
 * @param[in] a				- the cubic extension field element to test.
 * @return 1 if the argument is zero, 0 otherwise.
 */
int fp3_is_zero(fp3_t a);

/**
 * Assigns a random value to a cubic extension field element.
 *
 * @param[out] a			- the cubic extension field element to assign.
 */
void fp3_rand(fp3_t a);

/**
 * Prints a cubic extension field element to standard output.
 *
 * @param[in] a				- the cubic extension field element to print.
 */
void fp3_print(fp3_t a);

/**
 * Returns the number of bytes necessary to store a cubic extension field
 * element.
 *
 * @param[out] size			- the result.
 * @param[in] a				- the extension field element.
 */
int fp3_size_bin(fp3_t a);

/**
 * Reads a cubic extension field element from a byte vector in big-endian
 * format.
 *
 * @param[out] a			- the result.
 * @param[in] bin			- the byte vector.
 * @param[in] len			- the buffer capacity.
 * @throw ERR_NO_BUFFER		- if the buffer capacity is not correct.
 */
void fp3_read_bin(fp3_t a, const uint8_t *bin, int len);

/**
 * Writes a cubic extension field element to a byte vector in big-endian
 * format.
 *
 * @param[out] bin			- the byte vector.
 * @param[in] len			- the buffer capacity.
 * @param[in] a				- the extension field element to write.
 * @throw ERR_NO_BUFFER		- if the buffer capacity is not correct.
 */
void fp3_write_bin(uint8_t *bin, int len, fp3_t a);

/**
 * Returns the result of a comparison between two cubic extension field
 * elements.
 *
 * @param[in] a				- the first cubic extension field element.
 * @param[in] b				- the second cubic extension field element.
 * @return RLC_EQ if a == b, and RLC_NE otherwise.
 */
int fp3_cmp(fp3_t a, fp3_t b);

/**
 * Returns the result of a signed comparison between a cubic extension field
 * element and a digit.
 *
 * @param[in] a				- the cubic extension field element.
 * @param[in] b				- the digit.
 * @return RLC_EQ if a == b, and RLC_NE otherwise.
 */
int fp3_cmp_dig(fp3_t a, dig_t b);

/**
 * Assigns a cubic extension field element to a digit.
 *
 * @param[in] a				- the cubic extension field element.
 * @param[in] b				- the digit.
 */
void fp3_set_dig(fp3_t a, dig_t b);

/**
 * Adds two cubic extension field elements using basic arithmetic.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first cubic extension field element.
 * @param[in] b				- the second cubic extension field element.
 */
void fp3_add_basic(fp3_t c, fp3_t a, fp3_t b);

/**
 * Adds two cubic extension field elements using integrated modular reduction.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first cubic extension field element.
 * @param[in] b				- the second cubic extension field element.
 */
void fp3_add_integ(fp3_t c, fp3_t a, fp3_t b);

/**
 * Subtracts a cubic extension field element from another using basic
 * arithmetic.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first cubic extension field element.
 * @param[in] b				- the second cubic extension field element.
 */
void fp3_sub_basic(fp3_t c, fp3_t a, fp3_t b);

/**
 * Subtracts a cubic extension field element from another using integrated
 * modular reduction.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first cubic extension field element.
 * @param[in] b				- the second cubic extension field element.
 */
void fp3_sub_integ(fp3_t c, fp3_t a, fp3_t b);

/**
 * Negates a cubic extension field element. Computes c = -a.
 *
 * @param[out] c			- the result.
 * @param[out] a			- the cubic extension field element to negate.
 */
void fp3_neg(fp3_t c, fp3_t a);

/**
 * Doubles a cubic extension field element using basic arithmetic.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the cubic extension field element to double.
 */
void fp3_dbl_basic(fp3_t c, fp3_t a);

/**
 * Doubles a cubic extension field element using integrated modular reduction.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the cubic extension field element to double.
 */
void fp3_dbl_integ(fp3_t c, fp3_t a);

/**
 * Multiples two cubic extension field elements using basic arithmetic.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first cubic extension field element.
 * @param[in] b				- the second cubic extension field element.
 */
void fp3_mul_basic(fp3_t c, fp3_t a, fp3_t b);

/**
 * Multiples two cubic extension field elements using integrated modular
 * reduction.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first cubic extension field element.
 * @param[in] b				- the second cubic extension field element.
 */
void fp3_mul_integ(fp3_t c, fp3_t a, fp3_t b);

/**
 * Multiplies a cubic extension field element by a cubic non-residue.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the cubic extension field element to multiply.
 */
void fp3_mul_nor(fp3_t c, fp3_t a);

/**
 * Multiplies a cubic extension field element by a power of the constant
 * needed to compute a power of the Frobenius map. If the flag is zero, the map
 * is computed on the cubic extension directly; otherwise the map is computed on
 * a higher extension.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the field element to multiply.
 * @param[in] i				- the power of the Frobenius map.
 * @param[in] j				- the power of the constant.
 */
void fp3_mul_frb(fp3_t c, fp3_t a, int i, int j);

/**
 * Computes the square of a cubic extension field element using basic
 * arithmetic.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the cubic extension field element to square.
 */
void fp3_sqr_basic(fp3_t c, fp3_t a);

/**
 * Computes the square of a cubic extension field element using integrated
 * modular reduction.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the cubic extension field element to square.
 */
void fp3_sqr_integ(fp3_t c, fp3_t a);

/**
 * Inverts a cubic extension field element. Computes c = 1/a.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the cubic extension field element to invert.
 */
void fp3_inv(fp3_t c, fp3_t a);

/**
 * Inverts multiple cubic extension field elements simultaneously.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the cubic extension field elements to invert.
 * @param[in] n				- the number of elements.
 */
void fp3_inv_sim(fp3_t *c, fp3_t *a, int n);

/**
 * Computes a power of a cubic extension field element. Computes c = a^b.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the cubic extension element to exponentiate.
 * @param[in] b				- the exponent.
 */
void fp3_exp(fp3_t c, fp3_t a, bn_t b);

/**
 * Computes a power of the Frobenius map of a cubic extension field element.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the cubic extension element to exponentiate.
 * @param[in] i				- the power of the Frobenius map.
 */
void fp3_frb(fp3_t c, fp3_t a, int i);

/**
 * Extracts the square root of a cubic extension field element. Computes
 * c = sqrt(a). The other square root is the negation of c.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the extension field element.
 * @return					- 1 if there is a square root, 0 otherwise.
 */
int fp3_srt(fp3_t c, fp3_t a);

/**
 * Initializes the quartic extension field arithmetic module.
 */
void fp4_field_init(void);

/**
 * Copies the second argument to the first argument.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the sextic extension field element to copy.
 */
void fp4_copy(fp4_t c, fp4_t a);

/**
 * Assigns zero to a quartic extension field element.
 *
 * @param[out] a			- the quartic extension field element to zero.
 */
void fp4_zero(fp4_t a);

/**
 * Tests if a quartic extension field element is zero or not.
 *
 * @param[in] a				- the quartic extension field element to test.
 * @return 1 if the argument is zero, 0 otherwise.
 */
int fp4_is_zero(fp4_t a);

/**
 * Assigns a random value to a quartic extension field element.
 *
 * @param[out] a			- the quartic extension field element to assign.
 */
void fp4_rand(fp4_t a);

/**
 * Prints a quartic extension field element to standard output.
 *
 * @param[in] a				- the quartic extension field element to print.
 */
void fp4_print(fp4_t a);

/**
 * Returns the number of bytes necessary to store a quartic extension field
 * element.
 *
 * @param[out] size			- the result.
 * @param[in] a				- the extension field element.
 */
int fp4_size_bin(fp4_t a);

/**
 * Reads a quartic extension field element from a byte vector in big-endian
 * format.
 *
 * @param[out] a			- the result.
 * @param[in] bin			- the byte vector.
 * @param[in] len			- the buffer capacity.
 * @throw ERR_NO_BUFFER		- if the buffer capacity is not correct.
 */
void fp4_read_bin(fp4_t a, const uint8_t *bin, int len);

/**
 * Writes a quartic extension field element to a byte vector in big-endian
 * format.
 *
 * @param[out] bin			- the byte vector.
 * @param[in] len			- the buffer capacity.
 * @param[in] a				- the extension field element to write.
 * @throw ERR_NO_BUFFER		- if the buffer capacity is not correct.
 */
void fp4_write_bin(uint8_t *bin, int len, fp4_t a);

/**
 * Returns the result of a comparison between two quartic extension field
 * elements.
 *
 * @param[in] a				- the first quartic extension field element.
 * @param[in] b				- the second quartic extension field element.
 * @return RLC_EQ if a == b, and RLC_NE otherwise.
 */
int fp4_cmp(fp4_t a, fp4_t b);

/**
 * Returns the result of a signed comparison between a quartic extension field
 * element and a digit.
 *
 * @param[in] a				- the quartic extension field element.
 * @param[in] b				- the digit.
 * @return RLC_EQ if a == b, and RLC_NE otherwise.
 */
int fp4_cmp_dig(fp4_t a, dig_t b);

/**
 * Assigns a quartic extension field element to a digit.
 *
 * @param[in] a				- the quartic extension field element.
 * @param[in] b				- the digit.
 */
void fp4_set_dig(fp4_t a, dig_t b);

/**
 * Adds two quartic extension field elements. Computes c = a + b.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first quartic extension field element.
 * @param[in] b				- the second quartic extension field element.
 */
void fp4_add(fp4_t c, fp4_t a, fp4_t b);

/**
 * Subtracts a quartic extension field element from another. Computes
 * c = a - b.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the quartic extension field element.
 * @param[in] b				- the quartic extension field element.
 */
void fp4_sub(fp4_t c, fp4_t a, fp4_t b);

/**
 * Negates a quartic extension field element. Computes c = -a.
 *
 * @param[out] c			- the result.
 * @param[out] a			- the quartic extension field element to negate.
 */
void fp4_neg(fp4_t c, fp4_t a);

/**
 * Doubles a quartic extension field element. Computes c = 2 * a.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the quartic extension field element to double.
 */
void fp4_dbl(fp4_t c, fp4_t a);

/**
 * Multiples two quartic extension field elements without performing modular
 * reduction.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the quartic extension field element.
 * @param[in] b				- the quartic extension field element.
 */
void fp4_mul_unr(dv4_t c, fp4_t a, fp4_t b);

/**
 * Multiples two quartic extension field elements.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the quartic extension field element.
 * @param[in] b				- the quartic extension field element.
 */
void fp4_mul_basic(fp4_t c, fp4_t a, fp4_t b);

/**
 * Multiples two quartic extension field elements using lazy reduction.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the quartic extension field element.
 * @param[in] b				- the quartic extension field element.
 */
void fp4_mul_lazyr(fp4_t c, fp4_t a, fp4_t b);

/**
 * Multiplies a quartic extension field element by the adjoined root.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the quartic extension field element to multiply.
 */
void fp4_mul_art(fp4_t c, fp4_t a);

/**
 * Multiplies a quartic extension field element by a power of the constant
 * needed to compute a power of the Frobenius map.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the field element to multiply.
 * @param[in] i				- the power of the Frobenius map.
 * @param[in] j				- the power of the constant.
 */
void fp4_mul_frb(fp4_t c, fp4_t a, int i, int j);

/**
 * Multiples a dense quartic extension field element by a sparse element.
 *
 * @param[out] c			- the result.
 * @param[in] a				- a quartic extension field element.
 * @param[in] b				- a sparse quartic extension field element.
 */
void fp4_mul_dxs(fp4_t c, fp4_t a, fp4_t b);

/**
 * Computes the square of a quartic extension field element without performing
 * modular reduction.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the quartic extension field element to square.
 */
void fp4_sqr_unr(dv4_t c, fp4_t a);

/**
 * Computes the squares of a quartic extension field element using basic
 * arithmetic.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the quartic extension field element to square.
 */
void fp4_sqr_basic(fp4_t c, fp4_t a);

/**
 * Computes the square of a quartic extension field element using lazy reduction.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the quartic extension field element to square.
 */
void fp4_sqr_lazyr(fp4_t c, fp4_t a);

/**
 * Inverts a quartic extension field element. Computes c = 1/a.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the quartic extension field element to invert.
 */
void fp4_inv(fp4_t c, fp4_t a);

/**
 * Inverts multiple quartic extension field elements simultaneously.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the quartic extension field elements to invert.
 * @param[in] n				- the number of elements.
 */
void fp4_inv_sim(fp4_t *c, fp4_t *a, int n);

/**
 * Computes the inverse of a cyclotomic quartic extension field element.
 *
 * For cyclotomic elements, this is equivalent to computing the conjugate.
 * A cyclotomic element is one previously raised to the (p^2 - 1)-th power.
 *
 * @param[out] c                        - the result.
 * @param[in] a                         - the quartic extension field element to invert.
 */
void fp4_inv_cyc(fp4_t c, fp4_t a);

/**
 * Computes a power of a quartic extension field element. Computes c = a^b.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the quartic extension element to exponentiate.
 * @param[in] b				- the exponent.
 */
void fp4_exp(fp4_t c, fp4_t a, bn_t b);

/**
 * Computes a power of the Frobenius endomorphism of a quartic extension field
 * element. Computes c = a^p^i.
 *
 * @param[out] c			- the result.
 * @param[in] a				- a quartic extension field element.
 * @param[in] i				- the power of the Frobenius map.
 */
void fp4_frb(fp4_t c, fp4_t a, int i);

/**
 * Extracts the square root of a quartic extension field element. Computes
 * c = sqrt(a). The other square root is the negation of c.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the extension field element.
 * @return					- 1 if there is a square root, 0 otherwise.
 */
int fp4_srt(fp4_t c, fp4_t a);

/**
 * Copies the second argument to the first argument.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the sextic extension field element to copy.
 */
void fp6_copy(fp6_t c, fp6_t a);

/**
 * Assigns zero to a sextic extension field element.
 *
 * @param[out] a			- the sextic extension field element to zero.
 */
void fp6_zero(fp6_t a);

/**
 * Tests if a sextic extension field element is zero or not.
 *
 * @param[in] a				- the sextic extension field element to test.
 * @return 1 if the argument is zero, 0 otherwise.
 */
int fp6_is_zero(fp6_t a);

/**
 * Assigns a random value to a sextic extension field element.
 *
 * @param[out] a			- the sextic extension field element to assign.
 */
void fp6_rand(fp6_t a);

/**
 * Prints a sextic extension field element to standard output.
 *
 * @param[in] a				- the sextic extension field element to print.
 */
void fp6_print(fp6_t a);

/**
 * Returns the number of bytes necessary to store a quadratic extension field
 * element.
 *
 * @param[out] size			- the result.
 * @param[in] a				- the extension field element.
 */
int fp6_size_bin(fp6_t a);

/**
 * Reads a quadratic extension field element from a byte vector in big-endian
 * format.
 *
 * @param[out] a			- the result.
 * @param[in] bin			- the byte vector.
 * @param[in] len			- the buffer capacity.
 * @throw ERR_NO_BUFFER		- if the buffer capacity is not correct.
 */
void fp6_read_bin(fp6_t a, const uint8_t *bin, int len);

/**
 * Writes a sextic extension field element to a byte vector in big-endian
 * format.
 *
 * @param[out] bin			- the byte vector.
 * @param[in] len			- the buffer capacity.
 * @param[in] a				- the extension field element to write.
 * @throw ERR_NO_BUFFER		- if the buffer capacity is not correct.
 */
void fp6_write_bin(uint8_t *bin, int len, fp6_t a);

/**
 * Returns the result of a comparison between two sextic extension field
 * elements.
 *
 * @param[in] a				- the first sextic extension field element.
 * @param[in] b				- the second sextic extension field element.
 * @return RLC_EQ if a == b, and RLC_NE otherwise.
 */
int fp6_cmp(fp6_t a, fp6_t b);

/**
 * Returns the result of a signed comparison between a sextic extension field
 * element and a digit.
 *
 * @param[in] a				- the sextic extension field element.
 * @param[in] b				- the digit.
 * @return RLC_EQ if a == b, and RLC_NE otherwise.
 */
int fp6_cmp_dig(fp6_t a, dig_t b);

/**
 * Assigns a sextic extension field element to a digit.
 *
 * @param[in] a				- the sextic extension field element.
 * @param[in] b				- the digit.
 */
void fp6_set_dig(fp6_t a, dig_t b);

/**
 * Adds two sextic extension field elements. Computes c = a + b.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first sextic extension field element.
 * @param[in] b				- the second sextic extension field element.
 */
void fp6_add(fp6_t c, fp6_t a, fp6_t b);

/**
 * Subtracts a sextic extension field element from another. Computes
 * c = a - b.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the sextic extension field element.
 * @param[in] b				- the sextic extension field element.
 */
void fp6_sub(fp6_t c, fp6_t a, fp6_t b);

/**
 * Negates a sextic extension field element. Computes c = -a.
 *
 * @param[out] c			- the result.
 * @param[out] a			- the sextic extension field element to negate.
 */
void fp6_neg(fp6_t c, fp6_t a);

/**
 * Doubles a sextic extension field element. Computes c = 2 * a.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the sextic extension field element to double.
 */
void fp6_dbl(fp6_t c, fp6_t a);

/**
 * Multiples two sextic extension field elements without performing modular
 * reduction.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the sextic extension field element.
 * @param[in] b				- the sextic extension field element.
 */
void fp6_mul_unr(dv6_t c, fp6_t a, fp6_t b);

/**
 * Multiples two sextic extension field elements.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the sextic extension field element.
 * @param[in] b				- the sextic extension field element.
 */
void fp6_mul_basic(fp6_t c, fp6_t a, fp6_t b);

/**
 * Multiples two sextic extension field elements using lazy reduction.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the sextic extension field element.
 * @param[in] b				- the sextic extension field element.
 */
void fp6_mul_lazyr(fp6_t c, fp6_t a, fp6_t b);

/**
 * Multiplies a sextic extension field element by the adjoined root.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the sextic extension field element to multiply.
 */
void fp6_mul_art(fp6_t c, fp6_t a);

/**
 * Multiples a dense sextic extension field element by a sparse element.
 *
 * @param[out] c			- the result.
 * @param[in] a				- a sextic extension field element.
 * @param[in] b				- a sparse sextic extension field element.
 */
void fp6_mul_dxs(fp6_t c, fp6_t a, fp6_t b);

/**
 * Computes the square of a sextic extension field element without performing
 * modular reduction.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the sextic extension field element to square.
 */
void fp6_sqr_unr(dv6_t c, fp6_t a);

/**
 * Computes the squares of a sextic extension field element using basic
 * arithmetic.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the sextic extension field element to square.
 */
void fp6_sqr_basic(fp6_t c, fp6_t a);

/**
 * Computes the square of a sextic extension field element using lazy reduction.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the sextic extension field element to square.
 */
void fp6_sqr_lazyr(fp6_t c, fp6_t a);

/**
 * Inverts a sextic extension field element. Computes c = 1/a.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the sextic extension field element to invert.
 */
void fp6_inv(fp6_t c, fp6_t a);

/**
 * Computes a power of a sextic extension field element. Computes c = a^b.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the sextic extension element to exponentiate.
 * @param[in] b				- the exponent.
 */
void fp6_exp(fp6_t c, fp6_t a, bn_t b);

/**
 * Computes a power of the Frobenius endomorphism of a sextic extension field
 * element. Computes c = a^p^i.
 *
 * @param[out] c			- the result.
 * @param[in] a				- a sextic extension field element.
 * @param[in] i				- the power of the Frobenius map.
 */
void fp6_frb(fp6_t c, fp6_t a, int i);

/**
 * Copies the second argument to the first argument.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the octic extension field element to copy.
 */
void fp8_copy(fp8_t c, fp8_t a);

/**
 * Assigns zero to an octic extension field element.
 *
 * @param[out] a			- the octic extension field element to zero.
 */
void fp8_zero(fp8_t a);

/**
 * Tests if an octic extension field element is zero or not.
 *
 * @param[in] a				- the octic extension field element to test.
 * @return 1 if the argument is zero, 0 otherwise.
 */
int fp8_is_zero(fp8_t a);

/**
 * Assigns a random value to an octic extension field element.
 *
 * @param[out] a			- the octic extension field element to assign.
 */
void fp8_rand(fp8_t a);

/**
 * Prints an octic extension field element to standard output.
 *
 * @param[in] a				- the octic extension field element to print.
 */
void fp8_print(fp8_t a);

/**
 * Returns the number of bytes necessary to store an octic extension field
 * element.
 *
 * @param[in] a				- the extension field element.
 * @param[in] pack			- the flag to indicate compression.
 * @return the number of bytes.
 */
int fp8_size_bin(fp8_t a, int pack);

/**
 * Reads an octic extension field element from a byte vector in big-endian
 * format.
 *
 * @param[out] a			- the result.
 * @param[in] bin			- the byte vector.
 * @param[in] len			- the buffer capacity.
 * @throw ERR_NO_BUFFER		- if the buffer capacity is not correct.
 */
void fp8_read_bin(fp8_t a, const uint8_t *bin, int len);

/**
 * Writes an octic extension field element to a byte vector in big-endian
 * format.
 *
 * @param[out] bin			- the byte vector.
 * @param[in] len			- the buffer capacity.
 * @param[in] a				- the extension field element to write.
 * @throw ERR_NO_BUFFER		- if the buffer capacity is not correct.
 */
void fp8_write_bin(uint8_t *bin, int len, fp8_t a);

/**
 * Returns the result of a comparison between two octic extension field
 * elements.
 *
 * @param[in] a				- the first octic extension field element.
 * @param[in] b				- the second octic extension field element.
 * @return RLC_EQ if a == b, and RLC_NE otherwise.
 */
int fp8_cmp(fp8_t a, fp8_t b);

/**
 * Returns the result of a signed comparison between an octic extension field
 * element and a digit.
 *
 * @param[in] a				- the octic extension field element.
 * @param[in] b				- the digit.
 * @return RLC_EQ if a == b, and RLC_NE otherwise.
 */
int fp8_cmp_dig(fp8_t a, dig_t b);

/**
 * Assigns an octic extension field element to a digit.
 *
 * @param[in] a				- the octic extension field element.
 * @param[in] b				- the digit.
 */
void fp8_set_dig(fp8_t a, dig_t b);

/**
 * Adds two octic extension field elements. Computes c = a + b.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first octic extension field element.
 * @param[in] b				- the second octic extension field element.
 */
void fp8_add(fp8_t c, fp8_t a, fp8_t b);

/**
 * Subtracts an octic extension field element from another. Computes
 * c = a - b.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the octic extension field element.
 * @param[in] b				- the octic extension field element.
 */
void fp8_sub(fp8_t c, fp8_t a, fp8_t b);

/**
 * Negates an octic extension field element. Computes c = -a.
 *
 * @param[out] c			- the result.
 * @param[out] a			- the octic extension field element to negate.
 */
void fp8_neg(fp8_t c, fp8_t a);

/**
 * Doubles an octic extension field element. Computes c = 2 * a.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the octic extension field element to double.
 */
void fp8_dbl(fp8_t c, fp8_t a);

/**
 * Multiples two octic extension field elements without performing modular
 * reduction.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the octic extension field element.
 * @param[in] b				- the octic extension field element.
 */
void fp8_mul_unr(dv8_t c, fp8_t a, fp8_t b);

/**
 * Multiples two octic extension field elements.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the octic extension field element.
 * @param[in] b				- the octic extension field element.
 */
void fp8_mul_basic(fp8_t c, fp8_t a, fp8_t b);

/**
 * Multiples two octic extension field elements using lazy reduction.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the octic extension field element.
 * @param[in] b				- the octic extension field element.
 */
void fp8_mul_lazyr(fp8_t c, fp8_t a, fp8_t b);

/**
 * Multiplies an octic extension field element by the adjoined root.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the octic extension field element to multiply.
 */
void fp8_mul_art(fp8_t c, fp8_t a);

/**
 * Multiples a dense octic extension field element by a sparse element.
 *
 * @param[out] c			- the result.
 * @param[in] a				- an octic extension field element.
 * @param[in] b				- a sparse octic extension field element.
 */
void fp8_mul_dxs(fp8_t c, fp8_t a, fp8_t b);

/**
 * Computes the square of an octic extension field element without performing
 * modular reduction.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the octic extension field element to square.
 */
void fp8_sqr_unr(dv8_t c, fp8_t a);

/**
 * Computes the squares of an octic extension field element using basic
 * arithmetic.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the octic extension field element to square.
 */
void fp8_sqr_basic(fp8_t c, fp8_t a);

/**
 * Computes the square of an octic extension field element using lazy reduction.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the octic extension field element to square.
 */
void fp8_sqr_lazyr(fp8_t c, fp8_t a);

/**
 * Computes the square of a cyclotomic octic extension field element.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the cyclotomic extension element to square.
 */
void fp8_sqr_cyc(fp8_t c, fp8_t a);

/**
 * Inverts an octic extension field element. Computes c = 1/a.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the octic extension field element to invert.
 */
void fp8_inv(fp8_t c, fp8_t a);

/**
 * Computes the inverse of a cyclotomic octic extension field element.
 *
 * For cyclotomic elements, this is equivalent to computing the conjugate.
 * A cyclotomic element is one previously raised to the (p^4 - 1)-th power.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the octic extension field element to invert.
 */
void fp8_inv_cyc(fp8_t c, fp8_t a);

/**
 * Inverts multiple octic extension field elements simultaneously.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the octic extension field elements to invert.
 * @param[in] n				- the number of elements.
 */
void fp8_inv_sim(fp8_t *c, fp8_t *a, int n);

/**
 * Tests if an octic extension field element is cyclotomic.
 *
 * @param[in] a				- the octic extension field element to test.
 * @return 1 if the extension field element is cyclotomic, 0 otherwise.
 */
int fp8_test_cyc(fp8_t a);

/**
 * Converts an octic extension field element to a cyclotomic element. Computes
 * c = a^(p^4 - 1).
 *
 * @param[out] c			- the result.
 * @param[in] a				- the octic extension field element.
 */
void fp8_conv_cyc(fp8_t c, fp8_t a);

/**
 * Computes a power of an octic extension field element. Computes c = a^b.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the octic extension element to exponentiate.
 * @param[in] b				- the exponent.
 */
void fp8_exp(fp8_t c, fp8_t a, bn_t b);

/**
 * Computes a power of a cyclotomic octic extension field element.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the basis.
 * @param[in] b				- the exponent.
 */
void fp8_exp_cyc(fp8_t c, fp8_t a, bn_t b);

/**
 * Computes a power of the Frobenius endomorphism of an octic extension field
 * element. Computes c = a^p^i.
 *
 * @param[out] c			- the result.
 * @param[in] a				- an octic extension field element.
 * @param[in] i				- the power of the Frobenius map.
 */
void fp8_frb(fp8_t c, fp8_t a, int i);

/**
 * Copies the second argument to the first argument.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the nonic extension field element to copy.
 */
void fp9_copy(fp9_t c, fp9_t a);

/**
 * Assigns zero to a nonic extension field element.
 *
 * @param[out] a			- the nonic extension field element to zero.
 */
void fp9_zero(fp9_t a);

/**
 * Tests if a nonic extension field element is zero or not.
 *
 * @param[in] a				- the nonic extension field element to test.
 * @return 1 if the argument is zero, 0 otherwise.
 */
int fp9_is_zero(fp9_t a);

/**
 * Assigns a random value to a nonic extension field element.
 *
 * @param[out] a			- the nonic extension field element to assign.
 */
void fp9_rand(fp9_t a);

/**
 * Prints a nonic extension field element to standard output.
 *
 * @param[in] a				- the nonic extension field element to print.
 */
void fp9_print(fp9_t a);

/**
 * Returns the number of bytes necessary to store a quadratic extension field
 * element.
 *
 * @param[out] size			- the result.
 * @param[in] a				- the extension field element.
 */
int fp9_size_bin(fp9_t a);

/**
 * Reads a quadratic extension field element from a byte vector in big-endian
 * format.
 *
 * @param[out] a			- the result.
 * @param[in] bin			- the byte vector.
 * @param[in] len			- the buffer capacity.
 * @throw ERR_NO_BUFFER		- if the buffer capacity is not correct.
 */
void fp9_read_bin(fp9_t a, const uint8_t *bin, int len);

/**
 * Writes a nonic extension field element to a byte vector in big-endian
 * format.
 *
 * @param[out] bin			- the byte vector.
 * @param[in] len			- the buffer capacity.
 * @param[in] a				- the extension field element to write.
 * @throw ERR_NO_BUFFER		- if the buffer capacity is not correct.
 */
void fp9_write_bin(uint8_t *bin, int len, fp9_t a);

/**
 * Returns the result of a comparison between two nonic extension field
 * elements.
 *
 * @param[in] a				- the first nonic extension field element.
 * @param[in] b				- the second nonic extension field element.
 * @return RLC_EQ if a == b, and RLC_NE otherwise.
 */
int fp9_cmp(fp9_t a, fp9_t b);

/**
 * Returns the result of a signed comparison between a nonic extension field
 * element and a digit.
 *
 * @param[in] a				- the nonic extension field element.
 * @param[in] b				- the digit.
 * @return RLC_EQ if a == b, and RLC_NE otherwise.
 */
int fp9_cmp_dig(fp9_t a, dig_t b);

/**
 * Assigns a nonic extension field element to a digit.
 *
 * @param[in] a				- the nonic extension field element.
 * @param[in] b				- the digit.
 */
void fp9_set_dig(fp9_t a, dig_t b);

/**
 * Adds two nonic extension field elements. Computes c = a + b.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first nonic extension field element.
 * @param[in] b				- the second nonic extension field element.
 */
void fp9_add(fp9_t c, fp9_t a, fp9_t b);

/**
 * Subtracts a nonic extension field element from another. Computes
 * c = a - b.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the nonic extension field element.
 * @param[in] b				- the nonic extension field element.
 */
void fp9_sub(fp9_t c, fp9_t a, fp9_t b);

/**
 * Negates a nonic extension field element. Computes c = -a.
 *
 * @param[out] c			- the result.
 * @param[out] a			- the nonic extension field element to negate.
 */
void fp9_neg(fp9_t c, fp9_t a);

/**
 * Doubles a nonic extension field element. Computes c = 2 * a.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the nonic extension field element to double.
 */
void fp9_dbl(fp9_t c, fp9_t a);

/**
 * Multiples two nonic extension field elements without performing modular
 * reduction.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the nonic extension field element.
 * @param[in] b				- the nonic extension field element.
 */
void fp9_mul_unr(dv9_t c, fp9_t a, fp9_t b);

/**
 * Multiples two nonic extension field elements.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the nonic extension field element.
 * @param[in] b				- the nonic extension field element.
 */
void fp9_mul_basic(fp9_t c, fp9_t a, fp9_t b);

/**
 * Multiples two nonic extension field elements using lazy reduction.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the nonic extension field element.
 * @param[in] b				- the nonic extension field element.
 */
void fp9_mul_lazyr(fp9_t c, fp9_t a, fp9_t b);

/**
 * Multiplies a nonic extension field element by the adjoined root.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the nonic extension field element to multiply.
 */
void fp9_mul_art(fp9_t c, fp9_t a);

/**
 * Multiples a dense nonic extension field element by a sparse element.
 *
 * @param[out] c			- the result.
 * @param[in] a				- a nonic extension field element.
 * @param[in] b				- a sparse nonic extension field element.
 */
void fp9_mul_dxs(fp9_t c, fp9_t a, fp9_t b);

/**
 * Computes the square of a nonic extension field element without performing
 * modular reduction.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the nonic extension field element to square.
 */
void fp9_sqr_unr(dv9_t c, fp9_t a);

/**
 * Computes the squares of a nonic extension field element using basic
 * arithmetic.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the nonic extension field element to square.
 */
void fp9_sqr_basic(fp9_t c, fp9_t a);

/**
 * Computes the square of a nonic extension field element using lazy reduction.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the nonic extension field element to square.
 */
void fp9_sqr_lazyr(fp9_t c, fp9_t a);

/**
 * Inverts a nonic extension field element. Computes c = 1/a.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the nonic extension field element to invert.
 */
void fp9_inv(fp9_t c, fp9_t a);

/**
 * Inverts multiple noinc extension field elements simultaneously.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the nonic extension field elements to invert.
 * @param[in] n				- the number of elements.
 */
void fp9_inv_sim(fp9_t *c, fp9_t *a, int n);

/**
 * Computes a power of a nonic extension field element. Computes c = a^b.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the nonic extension element to exponentiate.
 * @param[in] b				- the exponent.
 */
void fp9_exp(fp9_t c, fp9_t a, bn_t b);

/**
 * Computes a power of the Frobenius endomorphism of a nonic extension field
 * element. Computes c = a^p^i.
 *
 * @param[out] c			- the result.
 * @param[in] a				- a nonic extension field element.
 * @param[in] i				- the power of the Frobenius map.
 */
void fp9_frb(fp9_t c, fp9_t a, int i);

/**
 * Copies the second argument to the first argument.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the dodecic extension field element to copy.
 */
void fp12_copy(fp12_t c, fp12_t a);

/**
 * Assigns zero to a dodecic extension field element.
 *
 * @param[out] a			- the dodecic extension field element to zero.
 */
void fp12_zero(fp12_t a);

/**
 * Tests if a dodecic extension field element is zero or not.
 *
 * @param[in] a				- the dodecic extension field element to test.
 * @return 1 if the argument is zero, 0 otherwise.
 */
int fp12_is_zero(fp12_t a);

/**
 * Assigns a random value to a dodecic extension field element.
 *
 * @param[out] a			- the dodecic extension field element to assign.
 */
void fp12_rand(fp12_t a);

/**
 * Prints a dodecic extension field element to standard output.
 *
 * @param[in] a				- the dodecic extension field element to print.
 */
void fp12_print(fp12_t a);

/**
 * Returns the number of bytes necessary to store a dodecic extension field
 * element.
 *
 * @param[in] a				- the extension field element.
 * @param[in] pack			- the flag to indicate compression.
 * @return the number of bytes.
 */
int fp12_size_bin(fp12_t a, int pack);

/**
 * Reads a dodecic extension field element from a byte vector in big-endian
 * format.
 *
 * @param[out] a			- the result.
 * @param[in] bin			- the byte vector.
 * @param[in] len			- the buffer capacity.
 * @throw ERR_NO_BUFFER		- if the buffer capacity is not correct.
 */
void fp12_read_bin(fp12_t a, const uint8_t *bin, int len);

/**
 * Writes a dodecic extension field element to a byte vector in big-endian
 * format.
 *
 * @param[out] bin			- the byte vector.
 * @param[in] len			- the buffer capacity.
 * @param[in] a				- the extension field element to write.
 * @param[in] pack			- the flag to indicate compression.
 * @throw ERR_NO_BUFFER		- if the buffer capacity is not correct.
 */
void fp12_write_bin(uint8_t *bin, int len, fp12_t a, int pack);

/**
 * Returns the result of a comparison between two dodecic extension field
 * elements.
 *
 * @param[in] a				- the first dodecic extension field element.
 * @param[in] b				- the second dodecic extension field element.
 * @return RLC_EQ if a == b, and RLC_NE otherwise.
 */
int fp12_cmp(fp12_t a, fp12_t b);

/**
 * Returns the result of a signed comparison between a dodecic extension field
 * element and a digit.
 *
 * @param[in] a				- the dodecic extension field element.
 * @param[in] b				- the digit.
 * @return RLC_EQ if a == b, and RLC_NE otherwise.
 */
int fp12_cmp_dig(fp12_t a, dig_t b);

/**
 * Assigns a dodecic extension field element to a digit.
 *
 * @param[in] a				- the dodecic extension field element.
 * @param[in] b				- the digit.
 */
void fp12_set_dig(fp12_t a, dig_t b);

/**
 * Adds two dodecic extension field elements. Computes c = a + b.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first dodecic extension field element.
 * @param[in] b				- the second dodecic extension field element.
 */
void fp12_add(fp12_t c, fp12_t a, fp12_t b);

/**
 * Subtracts a dodecic extension field element from another. Computes
 * c = a - b.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first dodecic extension field element.
 * @param[in] b				- the second dodecic extension field element.
 */
void fp12_sub(fp12_t c, fp12_t a, fp12_t b);

/**
 * Negates a dodecic extension field element.
 *
 * @param[out] c			- the result.
 * @param[out] a			- the dodecic extension field element to negate.
 */
void fp12_neg(fp12_t c, fp12_t a);

/**
 * Doubles a dodecic extension field element. Computes c = 2 * a.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the dodecic extension field element to double.
 */
void fp12_dbl(fp12_t c, fp12_t a);

/**
 * Multiples two dodecic extension field elements without performing modular
 * reduction.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the dodecic extension field element.
 * @param[in] b				- the dodecic extension field element.
 */
void fp12_mul_unr(dv12_t c, fp12_t a, fp12_t b);

/**
 * Multiples two dodecic extension field elements using basic arithmetic.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the dodecic extension field element.
 * @param[in] b				- the dodecic extension field element.
 */
void fp12_mul_basic(fp12_t c, fp12_t a, fp12_t b);

/**
 * Multiples two dodecic extension field elements using lazy reduction.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the dodecic extension field element.
 * @param[in] b				- the dodecic extension field element.
 */
void fp12_mul_lazyr(fp12_t c, fp12_t a, fp12_t b);

/**
 * Multiplies a dodecic extension field element by the adjoined root.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the dodecic extension field element to multiply.
 */
void fp12_mul_art(fp12_t c, fp12_t a);

/**
 * Multiples a dense dodecic extension field element by a sparse element using
 * basic arithmetic.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the dense dodecic extension field element.
 * @param[in] b				- the sparse dodecic extension field element.
 */
void fp12_mul_dxs_basic(fp12_t c, fp12_t a, fp12_t b);

/**
 * Multiples a dense dodecic extension field element by a sparse element using
 * lazy reduction.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the dense dodecic extension field element.
 * @param[in] b				- the sparse dodecic extension field element.
 */
void fp12_mul_dxs_lazyr(fp12_t c, fp12_t a, fp12_t b);

/**
 * Computes the square of a dodecic extension field element without performing
 * modular reduction.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the dodecic extension field element to square.
 */
void fp12_sqr_unr(dv12_t c, fp12_t a);

/**
 * Computes the square of a dodecic extension field element using basic
 * arithmetic.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the dodecic extension field element to square.
 */
void fp12_sqr_basic(fp12_t c, fp12_t a);

/**
 * Computes the square of a dodecic extension field element using lazy
 * reduction.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the dodecic extension field element to square.
 */
void fp12_sqr_lazyr(fp12_t c, fp12_t a);

/**
 * Computes the square of a cyclotomic dodecic extension field element using
 * basic arithmetic.
 *
 * A cyclotomic element is one raised to the (p^6 - 1)(p^2 + 1)-th power.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the cyclotomic extension element to square.
 */
void fp12_sqr_cyc_basic(fp12_t c, fp12_t a);

/**
 * Computes the square of a cyclotomic dodecic extension field element using
 * lazy reduction.
 *
 * A cyclotomic element is one raised to the (p^6 - 1)(p^2 + 1)-th power.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the cyclotomic extension element to square.
 */
void fp12_sqr_cyc_lazyr(fp12_t c, fp12_t a);

/**
 * Computes the square of a compressed cyclotomic extension field element.
 *
 * A cyclotomic element is one raised to the (p^6 - 1)(p^2 + 1)-th power.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the cyclotomic extension element to square.
 */
void fp12_sqr_pck_basic(fp12_t c, fp12_t a);

/**
 * Computes the square of a compressed cyclotomic extension field element using
 * lazy reduction.
 *
 * A cyclotomic element is one raised to the (p^6 - 1)(p^2 + 1)-th power.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the cyclotomic extension element to square.
 */
void fp12_sqr_pck_lazyr(fp12_t c, fp12_t a);

/**
 * Tests if a dodecic extension field element belongs to the cyclotomic
 * subgroup.
 *
 * @param[in] a				- the dodecic extension field element to test.
 * @return 1 if the extension field element is in the subgroup, 0 otherwise.
 */
int fp12_test_cyc(fp12_t a);

/**
 * Converts a dodecic extension field element to a cyclotomic element.
 * Computes c = a^(p^6 - 1)*(p^2 + 1).
 *
 * @param[out] c			- the result.
 * @param[in] a				- a dodecic extension field element.
 */
void fp12_conv_cyc(fp12_t c, fp12_t a);

/**
 * Decompresses a compressed cyclotomic extension field element.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the dodecic extension field element to decompress.
 */
void fp12_back_cyc(fp12_t c, fp12_t a);

/**
 * Decompresses multiple compressed cyclotomic extension field elements.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the dodecic field elements to decompress.
 * @param[in] n				- the number of field elements to decompress.
 */
void fp12_back_cyc_sim(fp12_t *c, fp12_t *a, int n);

/**
 * Inverts a dodecic extension field element. Computes c = 1/a.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the dodecic extension field element to invert.
 */
void fp12_inv(fp12_t c, fp12_t a);

/**
 * Computes the inverse of a cyclotomic dodecic extension field element.
 * For unitary elements, this is equivalent to computing the conjugate.
 * A unitary element is one previously raised to the (p^6 - 1)-th power.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the dodecic extension field element to invert.
 */
void fp12_inv_cyc(fp12_t c, fp12_t a);

/**
 * Computes the Frobenius endomorphism of a dodecic extension element.
 * Computes c = a^p.
 *
 * @param[out] c			- the result.
 * @param[in] a				- a dodecic extension field element.
 * @param[in] i				- the power of the Frobenius map.
 */
void fp12_frb(fp12_t c, fp12_t a, int i);

/**
 * Computes a power of a dodecic extension field element.
 * Faster formulas are used if the extension field element is cyclotomic.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the basis.
 * @param[in] b				- the exponent.
 */
void fp12_exp(fp12_t c, fp12_t a, bn_t b);

/**
 * Computes a power of a dodecic extension field element by a small exponent.
 * Faster formulas are used if the extension field element is cyclotomic.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the basis.
 * @param[in] b				- the exponent.
 */
void fp12_exp_dig(fp12_t c, fp12_t a, dig_t b);

/**
 * Computes a power of a cyclotomic dodecic extension field element.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the basis.
 * @param[in] b				- the exponent.
 */
void fp12_exp_cyc(fp12_t c, fp12_t a, bn_t b);

/**
 * Computes a power of a cyclotomic quadratic extension field element.
 *
 * @param[out] e			- the result.
 * @param[in] a				- the first element to exponentiate.
 * @param[in] b				- the first exponent.
 * @param[in] c				- the second element to exponentiate.
 * @param[in] d				- the second exponent.
 */
void fp2_exp_cyc_sim(fp2_t e, fp2_t a, bn_t b, fp2_t c, bn_t d);

/**
 * Computes a power of a cyclotomic dodecic extension field element.
 *
 * @param[out] e			- the result.
 * @param[in] a				- the first element to exponentiate.
 * @param[in] b				- the first exponent.
 * @param[in] c				- the second element to exponentiate.
 * @param[in] d				- the second exponent.
 */
void fp12_exp_cyc_sim(fp12_t e, fp12_t a, bn_t b, fp12_t c, bn_t d);

/**
 * Computes a power of a cyclotomic dodecic extension field element.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the basis.
 * @param[in] b				- the exponent in sparse form.
 * @param[in] l				- the length of the exponent in sparse form.
 * @param[in] s				- the sign of the exponent.
 */
void fp12_exp_cyc_sps(fp12_t c, fp12_t a, const int *b, int l, int s);

/**
 * Compresses a dodecic extension field element.
 *
 * @param[out] r			- the result.
 * @param[in] p				- the dodecic extension field element to compress.
 */
void fp12_pck(fp12_t c, fp12_t a);

/**
 * Decompresses a dodecic extension field element.
 *
 * @param[out] r			- the result.
 * @param[in] p				- the dodecic extension field element to decompress.
 * @return if the decompression was successful
 */
int fp12_upk(fp12_t c, fp12_t a);

/**
 * Compresses a dodecic extension field element at the maximum rate.
 *
 * @param[out] r			- the result.
 * @param[in] p				- the dodecic extension field element to compress.
 */
void fp12_pck_max(fp12_t c, fp12_t a);

/**
 * Decompresses a dodecic extension field element at the maximum rate.
 *
 * @param[out] r			- the result.
 * @param[in] p				- the dodecic extension field element to decompress.
 * @return if the decompression was successful
 */
int fp12_upk_max(fp12_t c, fp12_t a);

/**
 * Copies the second argument to the first argument.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the octdecic extension field element to copy.
 */
void fp18_copy(fp18_t c, fp18_t a);

/**
 * Assigns zero to an octdecic extension field element.
 *
 * @param[out] a			- the octdecic extension field element to zero.
 */
void fp18_zero(fp18_t a);

/**
 * Tests if an octdecic extension field element is zero or not.
 *
 * @param[in] a				- the octdecic extension field element to test.
 * @return 1 if the argument is zero, 0 otherwise.
 */
int fp18_is_zero(fp18_t a);

/**
 * Assigns a random value to an octdecic extension field element.
 *
 * @param[out] a			- the octdecic extension field element to assign.
 */
void fp18_rand(fp18_t a);

/**
 * Prints an octdecic extension field element to standard output.
 *
 * @param[in] A				- the octdecic extension field element to print.
 */
void fp18_print(fp18_t a);

/**
 * Returns the number of bytes necessary to store an octdecic extension field
 * element.
 *
 * @param[in] a				- the extension field element.
 * @return the number of bytes.
 */
int fp18_size_bin(fp18_t a);

/**
 * Reads an octdecic extension field element from a byte vector in big-endian
 * format.
 *
 * @param[out] a			- the result.
 * @param[in] bin			- the byte vector.
 * @param[in] len			- the buffer capacity.
 * @throw ERR_NO_BUFFER		- if the buffer capacity is not correct.
 */
void fp18_read_bin(fp18_t a, const uint8_t *bin, int len);

/**
 * Writes an octdecic extension field element to a byte vector in big-endian
 * format.
 *
 * @param[out] bin			- the byte vector.
 * @param[in] len			- the buffer capacity.
 * @param[in] a				- the extension field element to write.
 * @throw ERR_NO_BUFFER		- if the buffer capacity is not correct.
 */
void fp18_write_bin(uint8_t *bin, int len, fp18_t a);

/**
 * Returns the result of a comparison between two octdecic extension field
 * elements.
 *
 * @param[in] a				- the first octdecic extension field element.
 * @param[in] b				- the second octdecic extension field element.
 * @return RLC_EQ if a == b, and RLC_NE otherwise.
 */
int fp18_cmp(fp18_t a, fp18_t b);

/**
 * Returns the result of a signed comparison between an octdecic extension
 * field element and a digit.
 *
 * @param[in] a				- the octdecic extension field element.
 * @param[in] b				- the digit.
 * @return RLC_EQ if a == b, and RLC_NE otherwise.
 */
int fp18_cmp_dig(fp18_t a, dig_t b);

/**
 * Assigns an octdecic extension field element to a digit.
 *
 * @param[in] a				- the octdecic extension field element.
 * @param[in] b				- the digit.
 */
void fp18_set_dig(fp18_t a, dig_t b);

/**
 * Adds two octdecic extension field elements. Computes c = a + b.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first octdecic extension field element.
 * @param[in] b				- the second octdecic extension field element.
 */
void fp18_add(fp18_t c, fp18_t a, fp18_t b);

/**
 * Subtracts an octdecic extension field element from another. Computes
 * c = a - b.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first octdecic extension field element.
 * @param[in] b				- the second octdecic extension field element.
 */
void fp18_sub(fp18_t c, fp18_t a, fp18_t b);

/**
 * Negates an octdecic extension field element.
 *
 * @param[out] c			- the result.
 * @param[out] a			- the octdecic extension field element to negate.
 */
void fp18_neg(fp18_t c, fp18_t a);

/**
 * Doubles an octdecic extension field element. Computes c = 2 * a.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the octdecic extension field element to double.
 */
void fp18_dbl(fp18_t c, fp18_t a);

/**
 * Multiples two octdecic extension field elements without performing modular
 * reduction.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the octdecic extension field element.
 * @param[in] b				- the octdecic extension field element.
 */
void fp18_mul_unr(dv18_t c, fp18_t a, fp18_t b);

/**
 * Multiples two octdecic extension field elements using basic arithmetic.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the octdecic extension field element.
 * @param[in] b				- the octdecic extension field element.
 */
void fp18_mul_basic(fp18_t c, fp18_t a, fp18_t b);

/**
 * Multiples two octdecic extension field elements using lazy reduction.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the octdecic extension field element.
 * @param[in] b				- the octdecic extension field element.
 */
void fp18_mul_lazyr(fp18_t c, fp18_t a, fp18_t b);

/**
 * Multiplies an octdecic extension field element by the adjoined root.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the octdecic extension field element to multiply.
 */
void fp18_mul_art(fp18_t c, fp18_t a);

/**
 * Multiples a dense octdecic extension field element by a sparse element using
 * basic arithmetic.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the dense octdecic extension field element.
 * @param[in] b				- the sparse octdecic extension field element.
 */
void fp18_mul_dxs_basic(fp18_t c, fp18_t a, fp18_t b);

/**
 * Multiples a dense octdecic extension field element by a sparse element using
 * lazy reduction.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the dense octdecic extension field element.
 * @param[in] b				- the sparse octdecic extension field element.
 */
void fp18_mul_dxs_lazyr(fp18_t c, fp18_t a, fp18_t b);

/**
 * Computes the square of an octdecic extension field element without performing
 * modular reduction.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the octdecic extension field element to square.
 */
void fp18_sqr_unr(dv18_t c, fp18_t a);

/**
 * Computes the square of an octdecic extension field element using basic
 * arithmetic.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the octdecic extension field element to square.
 */
void fp18_sqr_basic(fp18_t c, fp18_t a);

/**
 * Computes the square of an octdecic extension field element using lazy
 * reduction.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the octdecic extension field element to square.
 */
void fp18_sqr_lazyr(fp18_t c, fp18_t a);

/**
 * Inverts an octdecic extension field element. Computes c = 1/a.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the octdecic extension field element to invert.
 */
void fp18_inv(fp18_t c, fp18_t a);

/**
 * Computes the inverse of a cyclotomic octdecic extension field element.
 * For unitary elements, this is equivalent to computing the conjugate.
 * A unitary element is one previously raised to the (p^9 - 1)-th power.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the octdecic extension field element to invert.
 */
void fp18_inv_cyc(fp18_t c, fp18_t a);

/**
 * Converts an octdecic extension field element to a cyclotomic element.
 * Computes c = a^(p^9 - 1).
 *
 * @param[out] c			- the result.
 * @param[in] a				- an octdecic extension field element.
 */
void fp18_conv_cyc(fp18_t c, fp18_t a);

/**
 * Computes the Frobenius endomorphism of an octdecic extension element.
 * Computes c = a^(p^i).
 *
 * @param[out] c			- the result.
 * @param[in] a				- an octdecic extension field element.
 * @param[in] i				- the power of the Frobenius map.
 */
void fp18_frb(fp18_t c, fp18_t a, int i);

/**
 * Computes a power of an octdecic extension field element.
 * Faster formulas are used if the field element is cyclotomic.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the basis.
 * @param[in] b				- the exponent.
 */
void fp18_exp(fp18_t c, fp18_t a, bn_t b);

/**
 * Copies the second argument to the first argument.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the 24-degree extension field element to copy.
 */
void fp24_copy(fp24_t c, fp24_t a);

/**
 * Assigns zero to a 24-degree extension field element.
 *
 * @param[out] a			- the 24-degree extension field element to zero.
 */
void fp24_zero(fp24_t a);

/**
 * Tests if a 24-degree extension field element is zero or not.
 *
 * @param[in] a				- the 24-degree extension field element to test.
 * @return 1 if the argument is zero, 0 otherwise.
 */
int fp24_is_zero(fp24_t a);

/**
 * Assigns a random value to a 24-degree extension field element.
 *
 * @param[out] A			- the 24-degree extension field element to assign.
 */
void fp24_rand(fp24_t a);

/**
 * Prints a 24-degree extension field element to standard output.
 *
 * @param[in] A				- the 24-degree extension field element to print.
 */
void fp24_print(fp24_t a);

/**
 * Returns the number of bytes necessary to store a 24-degree extension field
 * element.
 *
 * @param[in] a				- the extension field element.
 * @param[in] pack			- the flag to indicate compression.
 * @return the number of bytes.
 */
int fp24_size_bin(fp24_t a, int pack);

/**
 * Reads a 24-degree extension field element from a byte vector in big-endian
 * format.
 *
 * @param[out] a			- the result.
 * @param[in] bin			- the byte vector.
 * @param[in] len			- the buffer capacity.
 * @throw ERR_NO_BUFFER		- if the buffer capacity is not correct.
 */
void fp24_read_bin(fp24_t a, const uint8_t *bin, int len);

/**
 * Writes a 24-degree extension field element to a byte vector in big-endian
 * format.
 *
 * @param[out] bin			- the byte vector.
 * @param[in] len			- the buffer capacity.
 * @param[in] a				- the extension field element to write.
 * @param[in] pack			- the flag to indicate compression.
 * @throw ERR_NO_BUFFER		- if the buffer capacity is not correct.
 */
void fp24_write_bin(uint8_t *bin, int len, fp24_t a, int pack);

/**
 * Returns the result of a comparison between two 24-degree extension field
 * elements.
 *
 * @param[in] a				- the first 24-degree extension field element.
 * @param[in] b				- the second 24-degree extension field element.
 * @return RLC_EQ if a == b, and RLC_NE otherwise.
 */
int fp24_cmp(fp24_t a, fp24_t b);

/**
 * Returns the result of a signed comparison between a 24-degree extension field
 * element and a digit.
 *
 * @param[in] a				- the 24-degree extension field element.
 * @param[in] b				- the digit.
 * @return RLC_EQ if a == b, and RLC_NE otherwise.
 */
int fp24_cmp_dig(fp24_t a, dig_t b);

/**
 * Assigns a 24-degree extension field element to a digit.
 *
 * @param[in] a				- the 24-degree extension field element.
 * @param[in] b				- the digit.
 */
void fp24_set_dig(fp24_t a, dig_t b);

/**
 * Adds two 24-degree extension field elements. Computes c = a + b.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first 24-degree extension field element.
 * @param[in] b				- the second 24-degree extension field element.
 */
void fp24_add(fp24_t c, fp24_t a, fp24_t b);

/**
 * Subtracts a 24-degree extension field element from another. Computes
 * c = a - b.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first 24-degree extension field element.
 * @param[in] b				- the second 24-degree extension field element.
 */
void fp24_sub(fp24_t c, fp24_t a, fp24_t b);

/**
 * Negates a 24-degree extension field element.
 *
 * @param[out] c			- the result.
 * @param[out] a			- the 24-degree extension field element to negate.
 */
void fp24_neg(fp24_t c, fp24_t a);

/**
 * Doubles a 24-degree extension field element. Computes c = 2 * a.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the octic extension field element to double.
 */
void fp24_dbl(fp24_t c, fp24_t a);

/**
 * Multiples two 24-degree extension field elements without performing modular
 * reduction.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the 24-degree extension field element.
 * @param[in] b				- the 24-degree extension field element.
 */
void fp24_mul_unr(dv24_t c, fp24_t a, fp24_t b);

/**
 * Multiples two 24-degree extension field elements using basic arithmetic.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the 24-degree extension field element.
 * @param[in] b				- the 24-degree extension field element.
 */
void fp24_mul_basic(fp24_t c, fp24_t a, fp24_t b);

/**
 * Multiples two 24-degree extension field elements using lazy reduction.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the 24-degree extension field element.
 * @param[in] b				- the 24-degree extension field element.
 */
void fp24_mul_lazyr(fp24_t c, fp24_t a, fp24_t b);

/**
 * Multiplies a 24-degree extension field element by the adjoined root.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the dodecic extension field element to multiply.
 */
void fp24_mul_art(fp24_t c, fp24_t a);

/**
 * Multiples a dense 24-degree extension field element by a sparse element.
 *
 * @param[out] c			- the result.
 * @param[in] a				- a 24-degree extension field element.
 * @param[in] b				- a 24-degree quartic extension field element.
 */
void fp24_mul_dxs(fp24_t c, fp24_t a, fp24_t b);

/**
 * Computes the square of a 24-degree extension field element without performing
 * modular reduction.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the 24-degree extension field element to square.
 */
void fp24_sqr_unr(dv24_t c, fp24_t a);

/**
 * Computes the square of a 24-degree extension field element using basic
 * arithmetic.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the 24-degree extension field element to square.
 */
void fp24_sqr_basic(fp24_t c, fp24_t a);

/**
 * Computes the square of a 24-degree extension field element using lazy
 * reduction.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the 24-degree extension field element to square.
 */
void fp24_sqr_lazyr(fp24_t c, fp24_t a);

/**
 * Computes the square of a cyclotomic 24-extension field element using
 * basic arithmetic.
 *
 * A cyclotomic element is one raised to the (p^6 - 1)(p^2 + 1)-th power.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the cyclotomic extension element to square.
 */
void fp24_sqr_cyc_basic(fp24_t c, fp24_t a);

/**
 * Computes the square of a cyclotomic 24-extension field element using
 * lazy reduction.
 *
 * A cyclotomic element is one raised to the (p^6 - 1)(p^2 + 1)-th power.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the cyclotomic extension element to square.
 */
void fp24_sqr_cyc_lazyr(fp24_t c, fp24_t a);

/**
 * Computes the square of a compressed cyclotomic extension field element.
 *
 * A cyclotomic element is one raised to the (p^6 - 1)(p^2 + 1)-th power.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the cyclotomic extension element to square.
 */
void fp24_sqr_pck_basic(fp24_t c, fp24_t a);

/**
 * Computes the square of a compressed cyclotomic extension field element using
 * lazy reduction.
 *
 * A cyclotomic element is one raised to the (p^6 - 1)(p^2 + 1)-th power.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the cyclotomic extension element to square.
 */
void fp24_sqr_pck_lazyr(fp24_t c, fp24_t a);

/**
 * Tests if a 24-extension field element belongs to the cyclotomic
 * subgroup.
 *
 * @param[in] a				- the 24-extension field element to test.
 * @return 1 if the extension field element is in the subgroup, 0 otherwise.
 */
int fp24_test_cyc(fp24_t a);

/**
 * Converts a 24-extension field element to a cyclotomic element.
 * Computes c = a^(p^6 - 1)*(p^2 + 1).
 *
 * @param[out] c			- the result.
 * @param[in] a				- a 24-extension field element.
 */
void fp24_conv_cyc(fp24_t c, fp24_t a);

/**
 * Decompresses a compressed cyclotomic extension field element.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the 24-extension field element to decompress.
 */
void fp24_back_cyc(fp24_t c, fp24_t a);

/**
 * Decompresses multiple compressed cyclotomic extension field elements.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the 24 field elements to decompress.
 * @param[in] n				- the number of field elements to decompress.
 */
void fp24_back_cyc_sim(fp24_t *c, fp24_t *a, int n);

/**
 * Inverts a 24-degree extension field element. Computes c = 1/a.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the 24-degree extension field element to invert.
 */
void fp24_inv(fp24_t c, fp24_t a);

/**
 * Computes the inverse of a cyclotomic octdecic extension field element.
 * For unitary elements, this is equivalent to computing the conjugate.
 * A unitary element is one previously raised to the (p^12 - 1)-th power.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the 24-degree extension field element to invert.
 */
void fp24_inv_cyc(fp24_t c, fp24_t a);

/**
 * Computes the Frobenius endomorphism of a 24-degree extension element.
 * Computes c = a^p.
 *
 * @param[out] c			- the result.
 * @param[in] a				- a 24-degree extension field element.
 * @param[in] i				- the power of the Frobenius map.
 */
void fp24_frb(fp24_t c, fp24_t a, int i);

/**
 * Computes a power of a 24-degree extension field element.
 * Faster formulas are used if the extension field element is cyclotomic.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the basis.
 * @param[in] b				- the exponent.
 */
void fp24_exp(fp24_t c, fp24_t a, bn_t b);

/**
 * Computes a power of a 24-extension field element by a small exponent.
 * Faster formulas are used if the extension field element is cyclotomic.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the basis.
 * @param[in] b				- the exponent.
 */
void fp24_exp_dig(fp24_t c, fp24_t a, dig_t b);

/**
 * Computes a power of a cyclotomic 24-extension field element.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the basis.
 * @param[in] b				- the exponent.
 */
void fp24_exp_cyc(fp24_t c, fp24_t a, bn_t b);

/**
 * Computes a power of a cyclotomic dodecic extension field element.
 *
 * @param[out] e			- the result.
 * @param[in] a				- the first element to exponentiate.
 * @param[in] b				- the first exponent.
 * @param[in] c				- the second element to exponentiate.
 * @param[in] d				- the second exponent.
 */
void fp24_exp_cyc_sim(fp24_t e, fp24_t a, bn_t b, fp24_t c, bn_t d);

/**
 * Computes a power of a cyclotomic 24-extension field element.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the basis.
 * @param[in] b				- the exponent in sparse form.
 * @param[in] l				- the length of the exponent in sparse form.
 * @param[in] s				- the sign of the exponent.
 */
void fp24_exp_cyc_sps(fp24_t c, fp24_t a, const int *b, int l, int s);

/**
 * Compresses a 24-extension field element.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the 24-extension field element to compress.
 */
void fp24_pck(fp24_t c, fp24_t a);

/**
 * Decompresses a 24-extension field element.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the 24-extension field element to decompress.
 * @return if the decompression was successful
 */
int fp24_upk(fp24_t c, fp24_t a);

/**
 * Copies the second argument to the first argument.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the 48-extension field element to copy.
 */
void fp48_copy(fp48_t c, fp48_t a);

/**
 * Assigns zero to a 48-extension field element.
 *
 * @param[out] a			- the 48-extension field element to zero.
 */
void fp48_zero(fp48_t a);

/**
 * Tests if a 48-extension field element is zero or not.
 *
 * @param[in] a				- the 48-extension field element to test.
 * @return 1 if the argument is zero, 0 otherwise.
 */
int fp48_is_zero(fp48_t a);

/**
 * Assigns a random value to a 48-extension field element.
 *
 * @param[out] a			- the 48-extension field element to assign.
 */
void fp48_rand(fp48_t a);

/**
 * Prints a 48-extension field element to standard output.
 *
 * @param[in] a				- the 48-extension field element to print.
 */
void fp48_print(fp48_t a);

/**
 * Returns the number of bytes necessary to store a 48-extension field
 * element.
 *
 * @param[in] a				- the extension field element.
 * @param[in] pack			- the flag to indicate compression.
 * @return the number of bytes.
 */
int fp48_size_bin(fp48_t a, int pack);

/**
 * Reads a 48-extension field element from a byte vector in big-endian
 * format.
 *
 * @param[out] a			- the result.
 * @param[in] bin			- the byte vector.
 * @param[in] len			- the buffer capacity.
 * @throw ERR_NO_BUFFER		- if the buffer capacity is not correct.
 */
void fp48_read_bin(fp48_t a, const uint8_t *bin, int len);

/**
 * Writes a 48-extension field element to a byte vector in big-endian
 * format.
 *
 * @param[out] bin			- the byte vector.
 * @param[in] len			- the buffer capacity.
 * @param[in] a				- the extension field element to write.
 * @param[in] pack			- the flag to indicate compression.
 * @throw ERR_NO_BUFFER		- if the buffer capacity is not correct.
 */
void fp48_write_bin(uint8_t *bin, int len, fp48_t a, int pack);

/**
 * Returns the result of a comparison between two 48-extension field
 * elements.
 *
 * @param[in] a				- the first 48-extension field element.
 * @param[in] b				- the second 48-extension field element.
 * @return RLC_EQ if a == b, and RLC_NE otherwise.
 */
int fp48_cmp(fp48_t a, fp48_t b);

/**
 * Returns the result of a signed comparison between a 48-extension field
 * element and a digit.
 *
 * @param[in] a				- the 48-extension field element.
 * @param[in] b				- the digit.
 * @return RLC_EQ if a == b, and RLC_NE otherwise.
 */
int fp48_cmp_dig(fp48_t a, dig_t b);

/**
 * Assigns a 48-extension field element to a digit.
 *
 * @param[in] a				- the 48-extension field element.
 * @param[in] b				- the digit.
 */
void fp48_set_dig(fp48_t a, dig_t b);

/**
 * Adds two 48-extension field elements. Computes c = a + b.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first 48-extension field element.
 * @param[in] b				- the second 48-extension field element.
 */
void fp48_add(fp48_t c, fp48_t a, fp48_t b);

/**
 * Subtracts a 48-extension field element from another. Computes
 * c = a - b.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first 48-extension field element.
 * @param[in] b				- the second 48-extension field element.
 */
void fp48_sub(fp48_t c, fp48_t a, fp48_t b);

/**
 * Negates a 48-extension field element.
 *
 * @param[out] c			- the result.
 * @param[out] a			- the 48-extension field element to negate.
 */
void fp48_neg(fp48_t c, fp48_t a);

/**
 * Doubles a 48-extension field element. Computes c = 2 * a.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the 48-extension field element to double.
 */
void fp48_dbl(fp48_t c, fp48_t a);

/**
 * Multiples two 48-extension field elements using basic arithmetic.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the 48-extension field element.
 * @param[in] b				- the 48-extension field element.
 */
void fp48_mul_basic(fp48_t c, fp48_t a, fp48_t b);

/**
 * Multiples two 48-extension field elements using lazy reduction.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the 48-extension field element.
 * @param[in] b				- the 48-extension field element.
 */
void fp48_mul_lazyr(fp48_t c, fp48_t a, fp48_t b);

/**
 * Multiplies a 48-extension field element by the adjoined root.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the 48-extension field element to multiply.
 */
void fp48_mul_art(fp48_t c, fp48_t a);

/**
 * Multiples a dense 48-extension field element by a sparse element using
 * basic arithmetic. Computes C = A * B.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the dense 48-extension field element.
 * @param[in] b				- the sparse 48-extension field element.
 */
void fp48_mul_dxs(fp48_t c, fp48_t a, fp48_t b);

/**
 * Computes the square of a 48-extension field element using basic
 * arithmetic.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the 48-extension field element to square.
 */
void fp48_sqr_basic(fp48_t c, fp48_t a);

/**
 * Computes the square of a 48-extension field element using lazy
 * reduction.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the 48-extension field element to square.
 */
void fp48_sqr_lazyr(fp48_t c, fp48_t a);

/**
 * Computes the square of a cyclotomic 48-extension field element using
 * basic arithmetic.
 *
 * A cyclotomic element is one raised to the (p^6 - 1)(p^2 + 1)-th power.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the cyclotomic extension element to square.
 */
void fp48_sqr_cyc_basic(fp48_t c, fp48_t a);

/**
 * Computes the square of a cyclotomic 48-extension field element using
 * lazy reduction.
 *
 * A cyclotomic element is one raised to the (p^6 - 1)(p^2 + 1)-th power.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the cyclotomic extension element to square.
 */
void fp48_sqr_cyc_lazyr(fp48_t c, fp48_t a);

/**
 * Computes the square of a compressed cyclotomic extension field element.
 *
 * A cyclotomic element is one raised to the (p^6 - 1)(p^2 + 1)-th power.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the cyclotomic extension element to square.
 */
void fp48_sqr_pck_basic(fp48_t c, fp48_t a);

/**
 * Computes the square of a compressed cyclotomic extension field element using
 * lazy reduction.
 *
 * A cyclotomic element is one raised to the (p^6 - 1)(p^2 + 1)-th power.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the cyclotomic extension element to square.
 */
void fp48_sqr_pck_lazyr(fp48_t c, fp48_t a);

/**
 * Tests if a 48-extension field element belongs to the cyclotomic
 * subgroup.
 *
 * @param[in] a				- the 48-extension field element to test.
 * @return 1 if the extension field element is in the subgroup, 0 otherwise.
 */
int fp48_test_cyc(fp48_t a);

/**
 * Converts a 48-extension field element to a cyclotomic element.
 * Computes c = a^(p^6 - 1)*(p^2 + 1).
 *
 * @param[out] c			- the result.
 * @param[in] a				- a 48-extension field element.
 */
void fp48_conv_cyc(fp48_t c, fp48_t a);

/**
 * Decompresses a compressed cyclotomic extension field element.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the 48-extension field element to decompress.
 */
void fp48_back_cyc(fp48_t c, fp48_t a);

/**
 * Decompresses multiple compressed cyclotomic extension field elements.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the 48 field elements to decompress.
 * @param[in] n				- the number of field elements to decompress.
 */
void fp48_back_cyc_sim(fp48_t *c, fp48_t *a, int n);

/**
 * Inverts a 48-extension field element. Computes c = 1/a.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the 48-extension field element to invert.
 */
void fp48_inv(fp48_t c, fp48_t a);

/**
 * Computes the inverse of a cyclotomic 48-extension field element.
 *
 * For unitary elements, this is equivalent to computing the conjugate.
 * A unitary element is one previously raised to the (p^24 - 1)-th power.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the 48-extension field element to invert.
 */
void fp48_inv_cyc(fp48_t c, fp48_t a);

/**
 * Converts a 48-extension field element to a cyclotomic element. Computes
 * c = a^(p^6 - 1).
 *
 * @param[out] c			- the result.
 * @param[in] a				- the 48-extension field element.
 */
void fp48_conv_cyc(fp48_t c, fp48_t a);

/**
 * Computes the Frobenius endomorphism of a 48-extension element.
 * Computes c = a^p.
 *
 * @param[out] c			- the result.
 * @param[in] a				- a 48-extension field element.
 * @param[in] i				- the power of the Frobenius map.
 */
void fp48_frb(fp48_t c, fp48_t a, int i);

/**
 * Computes a power of a 48-extension field element.
 * Faster formulas are used if the extension field element is cyclotomic.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the basis.
 * @param[in] b				- the exponent.
 */
void fp48_exp(fp48_t c, fp48_t a, bn_t b);

/**
 * Computes a power of a 48-extension field element by a small exponent.
 * Faster formulas are used if the extension field element is cyclotomic.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the basis.
 * @param[in] b				- the exponent.
 */
void fp48_exp_dig(fp48_t c, fp48_t a, dig_t b);

/**
 * Computes a power of a cyclotomic 48-extension field element.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the basis.
 * @param[in] b				- the exponent.
 */
void fp48_exp_cyc(fp48_t c, fp48_t a, bn_t b);

/**
 * Computes a power of a cyclotomic 48-extension field element.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the basis.
 * @param[in] b				- the exponent in sparse form.
 * @param[in] l				- the length of the exponent in sparse form.
 * @param[in] s				- the sign of the exponent.
 */
void fp48_exp_cyc_sps(fp48_t c, fp48_t a, const int *b, int l, int s);

/**
 * Compresses a 48-extension field element.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the 48-extension field element to compress.
 */
void fp48_pck(fp48_t c, fp48_t a);

/**
 * Decompresses a 48-extension field element.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the 48-extension field element to decompress.
 * @return if the decompression was successful
 */
int fp48_upk(fp48_t c, fp48_t a);

/**
 * Copies the second argument to the first argument.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the 54-extension field element to copy.
 */
void fp54_copy(fp54_t c, fp54_t a);

/**
 * Assigns zero to a 54-extension field element.
 *
 * @param[out] a			- the 54-extension field element to zero.
 */
void fp54_zero(fp54_t a);

/**
 * Tests if a 54-extension field element is zero or not.
 *
 * @param[in] a				- the 54-extension field element to test.
 * @return 1 if the argument is zero, 0 otherwise.
 */
int fp54_is_zero(fp54_t a);

/**
 * Assigns a random value to a 54-extension field element.
 *
 * @param[out] a			- the 54-extension field element to assign.
 */
void fp54_rand(fp54_t a);

/**
 * Prints a 54-extension field element to standard output.
 *
 * @param[in] a				- the 54-extension field element to print.
 */
void fp54_print(fp54_t a);

/**
 * Returns the number of bytes necessary to store a 54-extension field
 * element.
 *
 * @param[in] a				- the extension field element.
 * @param[in] pack			- the flag to indicate compression.
 * @return the number of bytes.
 */
int fp54_size_bin(fp54_t a, int pack);

/**
 * Reads a 54-extension field element from a byte vector in big-endian
 * format.
 *
 * @param[out] a			- the result.
 * @param[in] bin			- the byte vector.
 * @param[in] len			- the buffer capacity.
 * @throw ERR_NO_BUFFER		- if the buffer capacity is not correct.
 */
void fp54_read_bin(fp54_t a, const uint8_t *bin, int len);

/**
 * Writes a 54-extension field element to a byte vector in big-endian
 * format.
 *
 * @param[out] bin			- the byte vector.
 * @param[in] len			- the buffer capacity.
 * @param[in] a				- the extension field element to write.
 * @param[in] pack			- the flag to indicate compression.
 * @throw ERR_NO_BUFFER		- if the buffer capacity is not correct.
 */
void fp54_write_bin(uint8_t *bin, int len, fp54_t a, int pack);

/**
 * Returns the result of a comparison between two 54-extension field
 * elements.
 *
 * @param[in] a				- the first 54-extension field element.
 * @param[in] b				- the second 54-extension field element.
 * @return RLC_EQ if a == b, and RLC_NE otherwise.
 */
int fp54_cmp(fp54_t a, fp54_t b);

/**
 * Returns the result of a signed comparison between a 54-extension field
 * element and a digit.
 *
 * @param[in] a				- the 54-extension field element.
 * @param[in] b				- the digit.
 * @return RLC_EQ if a == b, and RLC_NE otherwise.
 */
int fp54_cmp_dig(fp54_t a, dig_t b);

/**
 * Assigns a 54-extension field element to a digit.
 *
 * @param[in] a				- the 54-extension field element.
 * @param[in] b				- the digit.
 */
void fp54_set_dig(fp54_t a, dig_t b);

/**
 * Adds two 54-extension field elements. Computes c = a + b.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first 54-extension field element.
 * @param[in] b				- the second 54-extension field element.
 */
void fp54_add(fp54_t c, fp54_t a, fp54_t b);

/**
 * Subtracts a 54-extension field element from another. Computes
 * c = a - b.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first 54-extension field element.
 * @param[in] b				- the second 54-extension field element.
 */
void fp54_sub(fp54_t c, fp54_t a, fp54_t b);

/**
 * Negates a 54-extension field element.
 *
 * @param[out] c			- the result.
 * @param[out] a			- the 54-extension field element to negate.
 */
void fp54_neg(fp54_t c, fp54_t a);

/**
 * Doubles a 54-extension field element. Computes c = 2 * a.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the 54-extension field element to double.
 */
void fp54_dbl(fp54_t c, fp54_t a);

/**
 * Multiples two 54-extension field elements using basic arithmetic.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the 54-extension field element.
 * @param[in] b				- the 54-extension field element.
 */
void fp54_mul_basic(fp54_t c, fp54_t a, fp54_t b);

/**
 * Multiples two 54-extension field elements using lazy reduction.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the 54-extension field element.
 * @param[in] b				- the 54-extension field element.
 */
void fp54_mul_lazyr(fp54_t c, fp54_t a, fp54_t b);

/**
 * Multiplies a 54-extension field element by the adjoined root.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the 54-extension field element to multiply.
 */
void fp54_mul_art(fp54_t c, fp54_t a);

/**
 * Multiples a dense 54-extension field element by a sparse element using
 * basic arithmetic. Computes C = A * B.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the dense 54-extension field element.
 * @param[in] b				- the sparse 54-extension field element.
 */
void fp54_mul_dxs(fp54_t c, fp54_t a, fp54_t b);

/**
 * Computes the square of a 54-extension field element using basic
 * arithmetic.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the 54-extension field element to square.
 */
void fp54_sqr_basic(fp54_t c, fp54_t a);

/**
 * Computes the square of a 54-extension field element using lazy
 * reduction.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the 54-extension field element to square.
 */
void fp54_sqr_lazyr(fp54_t c, fp54_t a);

/**
 * Computes the square of a cyclotomic 54-extension field element using
 * basic arithmetic.
 *
 * A cyclotomic element is one raised to the (p^6 - 1)(p^2 + 1)-th power.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the cyclotomic extension element to square.
 */
void fp54_sqr_cyc_basic(fp54_t c, fp54_t a);

/**
 * Computes the square of a cyclotomic 54-extension field element using
 * lazy reduction.
 *
 * A cyclotomic element is one raised to the (p^6 - 1)(p^2 + 1)-th power.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the cyclotomic extension element to square.
 */
void fp54_sqr_cyc_lazyr(fp54_t c, fp54_t a);

/**
 * Computes the square of a compressed cyclotomic extension field element.
 *
 * A cyclotomic element is one raised to the (p^6 - 1)(p^2 + 1)-th power.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the cyclotomic extension element to square.
 */
void fp54_sqr_pck_basic(fp54_t c, fp54_t a);

/**
 * Computes the square of a compressed cyclotomic extension field element using
 * lazy reduction.
 *
 * A cyclotomic element is one raised to the (p^6 - 1)(p^2 + 1)-th power.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the cyclotomic extension element to square.
 */
void fp54_sqr_pck_lazyr(fp54_t c, fp54_t a);

/**
 * Tests if a 54-extension field element belongs to the cyclotomic
 * subgroup.
 *
 * @param[in] a				- the 54-extension field element to test.
 * @return 1 if the extension field element is in the subgroup, 0 otherwise.
 */
int fp54_test_cyc(fp54_t a);

/**
 * Converts a 54-extension field element to a cyclotomic element.
 * Computes c = a^(p^6 - 1)*(p^2 + 1).
 *
 * @param[out] c			- the result.
 * @param[in] a				- a 54-extension field element.
 */
void fp54_conv_cyc(fp54_t c, fp54_t a);

/**
 * Decompresses a compressed cyclotomic extension field element.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the 54-extension field element to decompress.
 */
void fp54_back_cyc(fp54_t c, fp54_t a);

/**
 * Decompresses multiple compressed cyclotomic extension field elements.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the 54 field elements to decompress.
 * @param[in] n				- the number of field elements to decompress.
 */
void fp54_back_cyc_sim(fp54_t *c, fp54_t *a, int n);

/**
 * Inverts a 54-extension field element. Computes c = 1/a.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the 54-extension field element to invert.
 */
void fp54_inv(fp54_t c, fp54_t a);

/**
 * Computes the inverse of a cyclotomic 54-extension field element.
 *
 * For unitary elements, this is equivalent to computing the conjugate.
 * A unitary element is one previously raised to the (p^24 - 1)-th power.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the 54-extension field element to invert.
 */
void fp54_inv_cyc(fp54_t c, fp54_t a);

/**
 * Converts a 54-extension field element to a cyclotomic element. Computes
 * c = a^(p^6 - 1).
 *
 * @param[out] c			- the result.
 * @param[in] a				- the 54-extension field element.
 */
void fp54_conv_cyc(fp54_t c, fp54_t a);

/**
 * Computes the Frobenius endomorphism of a 54-extension element.
 * Computes c = a^p.
 *
 * @param[out] c			- the result.
 * @param[in] a				- a 54-extension field element.
 * @param[in] i				- the power of the Frobenius map.
 */
void fp54_frb(fp54_t c, fp54_t a, int i);

/**
 * Computes a power of a 54-extension field element.
 * Faster formulas are used if the extension field element is cyclotomic.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the basis.
 * @param[in] b				- the exponent.
 */
void fp54_exp(fp54_t c, fp54_t a, bn_t b);

/**
 * Computes a power of a 54-extension field element by a small exponent.
 * Faster formulas are used if the extension field element is cyclotomic.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the basis.
 * @param[in] b				- the exponent.
 */
void fp54_exp_dig(fp54_t c, fp54_t a, dig_t b);

/**
 * Computes a power of a cyclotomic 54-extension field element.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the basis.
 * @param[in] b				- the exponent.
 */
void fp54_exp_cyc(fp54_t c, fp54_t a, bn_t b);

/**
 * Computes a power of a cyclotomic 54-extension field element.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the basis.
 * @param[in] b				- the exponent in sparse form.
 * @param[in] l				- the length of the exponent in sparse form.
 * @param[in] s				- the sign of the exponent.
 */
void fp54_exp_cyc_sps(fp54_t c, fp54_t a, const int *b, int l, int s);

/**
 * Compresses a 54-extension field element.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the 54-extension field element to compress.
 */
void fp54_pck(fp54_t c, fp54_t a);

/**
 * Decompresses a 54-extension field element.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the 54-extension field element to decompress.
 * @return if the decompression was successful
 */
int fp54_upk(fp54_t c, fp54_t a);

#endif /* !RLC_FPX_H */
