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
 * @defgroup fpx Extensions of prime fields
 */

/**
 * @file
 *
 * Interface of the module for extension field arithmetic over prime fields.
 *
 * @ingroup fpx
 */

#ifndef RELIC_FPX_H
#define RELIC_FPX_H

#include "relic_fp.h"
#include "relic_types.h"

/*============================================================================*/
/* Type definitions                                                           */
/*============================================================================*/

/**
 * Represents a quadratic extension prime field element.
 *
 * This extension is constructed with the basis {1, i}, where i is an
 * adjoined square root in the prime field. If p = 3 mod 8, u = i, i^2 = -1;
 * otherwise i^2 = -2.
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
 * This extension is constructed with the basis {1, j}, where j is an
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
 * Represents a sextic extension field element.
 *
 * This extension is constructed with the basis {1, v, v^2}, where v^3 = E is an
 * adjoined cube root in the underlying quadratic extension.  If p = 3 mod 8,
 * E = u + 1; if p = 5 mod 8, E = u; if p = 7 mod 8 and p = 2,3 mod 5,
 * E = u + 2.
 */
typedef fp2_t fp6_t[3];

/**
 * Represents a double-precision sextic extension field element.
 */
typedef dv2_t dv6_t[3];

/**
 * Represents a dodecic extension field element.
 *
 * This extension is constructed with the basis {1, w}, where w^2 = v is an
 * adjoined square root in the underlying sextic extension.
 */
typedef fp6_t fp12_t[2];

/**
 * Represents a octdecic extension field element.
 *
 * This extension is constructed with the basis {1, w}, where w^3 = v is an
 * adjoined cube root in the underlying sextic extension.
 */
typedef fp6_t fp18_t[3];

/*============================================================================*/
/* Macro definitions                                                          */
/*============================================================================*/

/**
 * Initializes a quadratic extension field element with a null value.
 *
* @param[out] A			- the quadratic extension element to initialize.
 */
#define fp2_null(A)															\
		fp_null(A[0]); fp_null(A[1]);										\

/**
 * Initializes a double-precision quadratic extension field element with a null
 * value.
 *
* @param[out] A			- the quadratic extension element to initialize.
 */
#define dv2_null(A)															\
		dv_null(A[0]); dv_null(A[1]);										\

/**
 * Allocate and initializes a quadratic extension field element.
 *
 * @param[out] A			- the new quadratic extension field element.
 */
#define fp2_new(A)															\
		fp_new(A[0]); fp_new(A[1]);											\

/**
 * Allocate and initializes a double-precision quadratic extension field element.
 *
 * @param[out] A			- the new quadratic extension field element.
 */
#define dv2_new(A)															\
		dv_new(A[0]); dv_new(A[1]);											\

/**
 * Calls a function to clean and free a quadratic extension field element.
 *
 * @param[out] A			- the quadratic extension field element to free.
 */
#define fp2_free(A)															\
		fp_free(A[0]); fp_free(A[1]); 										\

/**
 * Calls a function to clean and free a double-precision quadratic extension
 * field element.
 *
 * @param[out] A			- the quadratic extension field element to free.
 */
#define dv2_free(A)															\
		dv_free(A[0]); dv_free(A[1]); 										\

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
 * Subtracts a quadratic extension field elements from another.
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
 * Doubles a quadratic extension field elements. Computes C = A + A.
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
 * Squares a quadratic extension field elements. Computes C = A * A.
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
 * Initializes a cubic extension field element with a null value.
 *
* @param[out] A			- the cubic extension element to initialize.
 */
#define fp3_null(A)															\
		fp_null(A[0]); fp_null(A[1]); fp_null(A[2]);						\

/**
 * Initializes a double-precision cubic extension field element with a null
 * value.
 *
* @param[out] A			- the cubic extension element to initialize.
 */
#define dv3_null(A)															\
		dv_null(A[0]); dv_null(A[1]); dv_null(A[2]);						\

/**
 * Allocate and initializes a cubic extension field element.
 *
 * @param[out] A			- the new cubic extension field element.
 */
#define fp3_new(A)															\
		fp_new(A[0]); fp_new(A[1]);	fp_new(A[2]);							\

/**
 * Allocate and initializes a double-precision cubic extension field element.
 *
 * @param[out] A			- the new cubic extension field element.
 */
#define dv3_new(A)															\
		dv_new(A[0]); dv_new(A[1]);	dv_new(A[2]);							\

/**
 * Calls a function to clean and free a cubic extension field element.
 *
 * @param[out] A			- the cubic extension field element to free.
 */
#define fp3_free(A)															\
		fp_free(A[0]); fp_free(A[1]); fp_free(A[2]);						\

/**
 * Calls a function to clean and free a double-precision cubic extension
 * field element.
 *
 * @param[out] A			- the cubic extension field element to free.
 */
#define dv3_free(A)															\
		dv_free(A[0]); dv_free(A[1]); dv_free(A[2]);						\

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
 * Subtracts a cubic extension field elements from another.
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
 * Doubles a cubic extension field elements. Computes C = A + A.
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
 * Multiplies a cubic extension field by the cubic non-residue. Computes
 * C = A * E, where E is a non-square/non-cube in the cubic extension.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the cubic extension field element to multiply.
 */
#if FPX_CBC == BASIC
#define fp3_mul_nor(C, A)	fp3_mul_nor_basic(C, A)
#elif FPX_CBC == INTEG
#define fp3_mul_nor(C, A)	fp3_mul_nor_integ(C, A)
#endif

/**
 * Squares a cubic extension field elements. Computes C = A * A.
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
 * Initializes a sextic extension field with a null value.
 *
 * @param[out] A			- the sextic extension element to initialize.
 */
#define fp6_null(A)															\
		fp2_null(A[0]); fp2_null(A[1]); fp2_null(A[2]);						\

/**
 * Initializes a double-precision sextic extension field with a null value.
 *
 * @param[out] A			- the sextic extension element to initialize.
 */
#define dv6_null(A)															\
		dv2_null(A[0]); dv2_null(A[1]); dv2_null(A[2]);						\

/**
 * Allocate and initializes a sextic extension field element.
 *
 * @param[out] A			- the new sextic extension field element.
 */
#define fp6_new(A)															\
		fp2_new(A[0]); fp2_new(A[1]); fp2_new(A[2]);						\

/**
 * Calls a function to clean and free a sextic extension field element.
 *
 * @param[out] A			- the sextic extension field element to free.
 */
#define fp6_free(A)															\
		fp2_free(A[0]); fp2_free(A[1]); fp2_free(A[2]); 					\

/**
 * Allocate and initializes a double-precision sextic extension field element.
 *
 * @param[out] A			- the new sextic extension field element.
 */
#define dv6_new(A)															\
		dv2_new(A[0]); dv2_new(A[1]); dv2_new(A[2]);						\

/**
 * Calls a function to clean and free a double-precision sextic extension field
 * element.
 *
 * @param[out] A			- the sextic extension field element to free.
 */
#define dv6_free(A)															\
		dv2_free(A[0]); dv2_free(A[1]); dv2_free(A[2]); 					\

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
 * Initializes a dodecic extension field with a null value.
 *
 * @param[out] A			- the dodecic extension element to initialize.
 */
#define fp12_null(A)														\
		fp6_null(A[0]); fp6_null(A[1]);										\

/**
 * Allocate and initializes a dodecic extension field element.
 *
 * @param[out] A			- the new dodecic extension field element.
 */
#define fp12_new(A)															\
		fp6_new(A[0]); fp6_new(A[1]);										\

/**
 * Calls a function to clean and free a dodecic extension field element.
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
 * Squares a dodecic extension field elements. Computes C = A * A.
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
 * Squares a dodecic extension field elements in the cyclotomic subgroup.
 * Computes C = A * B.
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
 * Squares a dodecic extension field elements in the cyclotomic subgroup in.
 * compressed form. Computes C = A * B.
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
 * Initializes a octdecic extension field with a null value.
 *
 * @param[out] A			- the octdecic extension element to initialize.
 */
#define fp18_null(A)														\
		fp6_null(A[0]); fp6_null(A[1]);	fp6_null(A[2]);						\

/**
 * Allocate and initializes a octdecic extension field element.
 *
 * @param[out] A			- the new octdecic extension field element.
 */
#define fp18_new(A)															\
		fp6_new(A[0]); fp6_new(A[1]); fp6_new(A[2]);						\

/**
 * Calls a function to clean and free a octdecic extension field element.
 *
 * @param[out] A			- the octdecic extension field element to free.
 */
#define fp18_free(A)														\
		fp6_free(A[0]); fp6_free(A[1]); fp6_free(A[2]);						\

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
 * Squares a octdecic extension field element. Computes C = A * A.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the octdecic extension field element to square.
 */
#if FPX_RDC == BASIC
#define fp18_sqr(C, A)			fp18_sqr_basic(C, A)
#elif FPX_RDC == LAZYR
#define fp18_sqr(C, A)			fp18_sqr_lazyr(C, A)
#endif

/*============================================================================*/
/* Function prototypes                                                        */
/*============================================================================*/

/**
 * Copies the second argument to the first argument.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the quadratic extension field element to copy.
 */
void fp2_copy(fp2_t c, fp2_t a);

/**
 * Assigns zero to a quadratic extension field element.
 *
 * @param[out] A			- the quadratic extension field element to zero.
 */
void fp2_zero(fp2_t a);

/**
 * Tests if a quadratic extension field element is zero or not.
 *
 * @param[in] A				- the quadratic extension field element to test.
 * @return 1 if the argument is zero, 0 otherwise.
 */
int fp2_is_zero(fp2_t a);

/**
 * Assigns a random value to a quadratic extension field element.
 *
 * @param[out] A			- the quadratic extension field element to assign.
 */
void fp2_rand(fp2_t a);

/**
 * Prints a quadratic extension field element to standard output.
 *
 * @param[in] A				- the quadratic extension field element to print.
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
void fp2_read_bin(fp2_t a, uint8_t *bin, int len);

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
 * @param[in] A				- the first quadratic extension field element.
 * @param[in] B				- the second quadratic extension field element.
 * @return CMP_LT if a < b, CMP_EQ if a == b and CMP_GT if a > b.
 */
int fp2_cmp(fp2_t a, fp2_t b);

/**
 * Returns the result of a signed comparison between a quadratic extension field
 * element and a digit.
 *
 * @param[in] a				- the quadratic extension field element.
 * @param[in] b				- the digit.
 * @return CMP_LT if a < b, CMP_EQ if a == b and CMP_GT if a > b.
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
 * @param[out] C			- the result.
 * @param[in] A				- the first quadratic extension field element.
 * @param[in] B				- the second quadratic extension field element.
 */
void fp2_sub_basic(fp2_t c, fp2_t a, fp2_t b);

/**
 * Subtracts a quadratic extension field element from another using integrated
 * modular reduction.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the first quadratic extension field element.
 * @param[in] B				- the second quadratic extension field element.
 */
void fp2_sub_integ(fp2_t c, fp2_t a, fp2_t b);

/**
 * Negates a quadratic extension field element.
 *
 * @param[out] C			- the result.
 * @param[out] A			- the quadratic extension field element to negate.
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
 * Multiplies a quadratic extension field element by the adjoined square root.
 * Computes c = a * u.
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
 * Computes the inverse of a unitary quadratic extension field element.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the quadratic extension field element to invert.
 */
void fp2_inv_uni(fp2_t c, fp2_t a);

/**
 * Inverts multiple quadratic extension field elements simultaneously.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the quadratic extension field elements to invert.
 * @param[in] n				- the number of elements.
 */
void fp2_inv_sim(fp2_t *c, fp2_t *a, int n);

/**
 * Tests if a quadratic extension field element is unitary.
 *
 * @param[in] a				- the quadratic extension field element to test.
 * @return 1 if the extension field element is unitary, 0 otherwise.
 */
int fp2_test_uni(fp2_t a);

/**
 * Converts a quadratic extension field element to a unitary element. Computes
 * c = a^(p - 1).
 *
 * @param[out] c			- the result.
 * @param[in] a				- the quadratic extension field element.
 */
void fp2_conv_uni(fp2_t c, fp2_t a);

/**
 * Computes a power of a quadratic extension field element.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the quadratic extension element to exponentiate.
 * @param[in] b				- the exponent.
 */
void fp2_exp(fp2_t c, fp2_t a, bn_t b);

/**
 * Computes a power of a unitary quadratic extension field element.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the unitary element to exponentiate.
 * @param[in] b				- the exponent.
 */
void fp2_exp_uni(fp2_t c, fp2_t a, bn_t b);

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
 * @param[out] r			- the result.
 * @param[in] p				- the extension field element to compress.
 */
void fp2_pck(fp2_t c, fp2_t a);

/**
 * Decompresses an extension field element.
 *
 * @param[out] r			- the result.
 * @param[in] p				- the extension field element to decompress.
 * @return if the decompression was successful
 */
int fp2_upk(fp2_t c, fp2_t a);

/**
 * Copies the second argument to the first argument.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the cubic extension field element to copy.
 */
void fp3_copy(fp3_t c, fp3_t a);

/**
 * Assigns zero to a cubic extension field element.
 *
 * @param[out] A			- the cubic extension field element to zero.
 */
void fp3_zero(fp3_t a);

/**
 * Tests if a cubic extension field element is zero or not.
 *
 * @param[in] A				- the cubic extension field element to test.
 * @return 1 if the argument is zero, 0 otherwise.
 */
int fp3_is_zero(fp3_t a);

/**
 * Assigns a random value to a cubic extension field element.
 *
 * @param[out] A			- the cubic extension field element to assign.
 */
void fp3_rand(fp3_t a);

/**
 * Prints a cubic extension field element to standard output.
 *
 * @param[in] A				- the cubic extension field element to print.
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
void fp3_read_bin(fp3_t a, uint8_t *bin, int len);

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
 * @param[in] A				- the first cubic extension field element.
 * @param[in] B				- the second cubic extension field element.
 * @return CMP_LT if a < b, CMP_EQ if a == b and CMP_GT if a > b.
 */
int fp3_cmp(fp3_t a, fp3_t b);

/**
 * Returns the result of a signed comparison between a cubic extension field
 * element and a digit.
 *
 * @param[in] a				- the cubic extension field element.
 * @param[in] b				- the digit.
 * @return CMP_LT if a < b, CMP_EQ if a == b and CMP_GT if a > b.
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
 * @param[out] C			- the result.
 * @param[in] A				- the first cubic extension field element.
 * @param[in] B				- the second cubic extension field element.
 */
void fp3_sub_basic(fp3_t c, fp3_t a, fp3_t b);

/**
 * Subtracts a cubic extension field element from another using integrated
 * modular reduction.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the first cubic extension field element.
 * @param[in] B				- the second cubic extension field element.
 */
void fp3_sub_integ(fp3_t c, fp3_t a, fp3_t b);

/**
 * Negates a cubic extension field element. Computes c = -a.
 *
 * @param[out] C			- the result.
 * @param[out] A			- the cubic extension field element to negate.
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
 * Multiplies a cubic extension field element by the adjoined cubic root.
 * Computes c = a * u.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the cubic extension field element to multiply.
 */
void fp3_mul_art(fp3_t c, fp3_t a);

/**
 * Multiplies a cubic extension field element by a cubic non-residue.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the cubic extension field element to multiply.
 */
void fp3_mul_nor_basic(fp3_t c, fp3_t a);

/**
 * Multiplies a cubic extension field element by a cubic non-residue using
 * integrated modular reduction.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the cubic extension field element to multiply.
 */
void fp3_mul_nor_integ(fp3_t c, fp3_t a);

/**
 * Multiplies a cubic extension field element by a power of the constant
 * needed to compute a power of the Frobenius map. If the flag is zero, the map
 * is computed on the cubic extension directly; otherwise the map is computed on
 * a higher extension.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the field element to multiply.
 * @param[in] i				- the flag to indicate the current extension.
 * @param[in] j				- the power of the Frobenius map.
 * @param[in] k				- the power of the constant.
 */
void fp3_mul_frb(fp3_t c, fp3_t a, int i, int j, int k);

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
 * Copies the second argument to the first argument.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the sextic extension field element to copy.
 */
void fp6_copy(fp6_t c, fp6_t a);

/**
 * Assigns zero to a sextic extension field element.
 *
 * @param[out] A			- the sextic extension field element to zero.
 */
void fp6_zero(fp6_t a);

/**
 * Tests if a sextic extension field element is zero or not.
 *
 * @param[in] A				- the sextic extension field element to test.
 * @return 1 if the argument is zero, 0 otherwise.
 */
int fp6_is_zero(fp6_t a);

/**
 * Assigns a random value to a sextic extension field element.
 *
 * @param[out] A			- the sextic extension field element to assign.
 */
void fp6_rand(fp6_t a);

/**
 * Prints a sextic extension field element to standard output.
 *
 * @param[in] A				- the sextic extension field element to print.
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
void fp6_read_bin(fp6_t a, uint8_t *bin, int len);

/**
 * Writes a setix extension field element to a byte vector in big-endian
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
 * @param[in] A				- the first sextic extension field element.
 * @param[in] B				- the second sextic extension field element.
 * @return CMP_LT if a < b, CMP_EQ if a == b and CMP_GT if a > b.
 */
int fp6_cmp(fp6_t a, fp6_t b);

/**
 * Returns the result of a signed comparison between a sextic extension field
 * element and a digit.
 *
 * @param[in] a				- the sextic extension field element.
 * @param[in] b				- the digit.
 * @return CMP_LT if a < b, CMP_EQ if a == b and CMP_GT if a > b.
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
 * @param[out] C			- the result.
 * @param[out] A			- the sextic extension field element to negate.
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
 * Multiplies a sextic extension field element by the adjoined cube root.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the sextic extension field element to multiply.
 */
void fp6_mul_art(fp6_t c, fp6_t a);

/**
 * Multiples a dense sextic extension field element by a sparse element.
 *
 * The sparse element must have a[2] = 0.
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
 * @param[out] C			- the result.
 * @param[in] A				- the dodecic extension field element to copy.
 */
void fp12_copy(fp12_t c, fp12_t a);

/**
 * Assigns zero to a dodecic extension field element.
 *
 * @param[out] A			- the dodecic extension field element to zero.
 */
void fp12_zero(fp12_t a);

/**
 * Tests if a dodecic extension field element is zero or not.
 *
 * @param[in] A				- the dodecic extension field element to test.
 * @return 1 if the argument is zero, 0 otherwise.
 */
int fp12_is_zero(fp12_t a);

/**
 * Assigns a random value to a dodecic extension field element.
 *
 * @param[out] A			- the dodecic extension field element to assign.
 */
void fp12_rand(fp12_t a);

/**
 * Prints a dodecic extension field element to standard output.
 *
 * @param[in] A				- the dodecic extension field element to print.
 */
void fp12_print(fp12_t a);

/**
 * Returns the number of bytes necessary to store a quadratic extension field
 * element.
 *
 * @param[in] a				- the extension field element.
 * @param[in] pack			- the flag to indicate compression.
 * @return the number of bytes.
 */
int fp12_size_bin(fp12_t a, int pack);

/**
 * Reads a quadratic extension field element from a byte vector in big-endian
 * format.
 *
 * @param[out] a			- the result.
 * @param[in] bin			- the byte vector.
 * @param[in] len			- the buffer capacity.
 * @throw ERR_NO_BUFFER		- if the buffer capacity is not correct.
 */
void fp12_read_bin(fp12_t a, uint8_t *bin, int len);

/**
 * Writes a setix extension field element to a byte vector in big-endian
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
 * @return CMP_LT if a < b, CMP_EQ if a == b and CMP_GT if a > b.
 */
int fp12_cmp(fp12_t a, fp12_t b);

/**
 * Returns the result of a signed comparison between a dodecic extension field
 * element and a digit.
 *
 * @param[in] a				- the dodecic extension field element.
 * @param[in] b				- the digit.
 * @return CMP_LT if a < b, CMP_EQ if a == b and CMP_GT if a > b.
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
 * Adds two dodecic extension field elements. Computes C = A + B.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the first dodecic extension field element.
 * @param[in] B				- the second dodecic extension field element.
 */
void fp12_add(fp12_t c, fp12_t a, fp12_t b);

/**
 * Subtracts a dodecic extension field element from another. Computes
 * C = A - B.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the first dodecic extension field element.
 * @param[in] B				- the second dodecic extension field element.
 */
void fp12_sub(fp12_t c, fp12_t a, fp12_t b);

/**
 * Negates a dodecic extension field element.
 *
 * @param[out] C			- the result.
 * @param[out] A			- the dodecic extension field element to negate.
 */
void fp12_neg(fp12_t c, fp12_t a);

/**
 * Multiples two dodecic extension field elements without performing modular
 * reduction.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the dodecic extension field element.
 * @param[in] b				- the dodecic extension field element.
 */
void fp12_mul_unr(dv6_t c, fp12_t a, fp12_t b);

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
 * Decompresses a compressed cyclotomic extension field element to its
 * usual representation.
 *
 * @param[out] c			- the result.
 * @param[in] a				- a dodecic extension field element to decompress.
 */
void fp12_back_cyc(fp12_t c, fp12_t a);

/**
 * Decompresses multiple compressed cyclotomic extension field elements to their
 * usual representations.
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
 * Computes the inverse of a unitary dodecic extension field element.
 *
 * For unitary elements, this is equivalent to computing the conjugate.
 * A unitary element is one previously raised to the (p^6 - 1)-th power.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the dodecic extension field element to invert.
 */
void fp12_inv_uni(fp12_t c, fp12_t a);

/**
 * Converts a dodecic extension field element to a unitary element. Computes
 * c = a^(p^6 - 1).
 *
 * @param[out] c			- the result.
 * @param[in] a				- the dodecic extension field element.
 */
void fp12_conv_uni(fp12_t c, fp12_t a);

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
 * Computes a power of a dodecic extension field element. Detects if the
 * extension field element is in a cyclotomic subgroup and if this is the case,
 * faster formulae are used.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the basis.
 * @param[in] b				- the exponent.
 */
void fp12_exp(fp12_t c, fp12_t a, bn_t b);

/**
 * Computes a power of a cyclotomic dodecic extension field element.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the basis.
 * @param[in] b				- the exponent.
 */
void fp12_exp_cyc(fp12_t c, fp12_t a, bn_t b);

/**
 * Computes a power of a cyclotomic dodecic extension field element.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the basis.
 * @param[in] b				- the exponent in sparse form.
 * @param[in] l				- the length of the exponent in sparse form.
 */
void fp12_exp_cyc_sps(fp12_t c, fp12_t a, int *b, int l);

/**
 * Compresses an extension field element.
 *
 * @param[out] r			- the result.
 * @param[in] p				- the extension field element to compress.
 */
void fp12_pck(fp12_t c, fp12_t a);

/**
 * Decompresses an extension field element.
 *
 * @param[out] r			- the result.
 * @param[in] p				- the extension field element to decompress.
 * @return if the decompression was successful
 */
int fp12_upk(fp12_t c, fp12_t a);

/**
 * Copies the second argument to the first argument.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the octodecic extension field element to copy.
 */
void fp18_copy(fp18_t c, fp18_t a);

/**
 * Assigns zero to a octodecic extension field element.
 *
 * @param[out] A			- the octodecic extension field element to zero.
 */
void fp18_zero(fp18_t a);

/**
 * Tests if a octodecic extension field element is zero or not.
 *
 * @param[in] A				- the octodecic extension field element to test.
 * @return 1 if the argument is zero, 0 otherwise.
 */
int fp18_is_zero(fp18_t a);

/**
 * Assigns a random value to a octodecic extension field element.
 *
 * @param[out] A			- the octodecic extension field element to assign.
 */
void fp18_rand(fp18_t a);

/**
 * Prints a octodecic extension field element to standard output.
 *
 * @param[in] A				- the octodecic extension field element to print.
 */
void fp18_print(fp18_t a);

/**
 * Returns the result of a comparison between two octodecic extension field
 * elements.
 *
 * @param[in] a				- the first octodecic extension field element.
 * @param[in] b				- the second octodecic extension field element.
 * @return CMP_LT if a < b, CMP_EQ if a == b and CMP_GT if a > b.
 */
int fp18_cmp(fp18_t a, fp18_t b);

/**
 * Returns the result of a signed comparison between a octodecic extension field
 * element and a digit.
 *
 * @param[in] a				- the octodecic extension field element.
 * @param[in] b				- the digit.
 * @return CMP_LT if a < b, CMP_EQ if a == b and CMP_GT if a > b.
 */
int fp18_cmp_dig(fp18_t a, dig_t b);

/**
 * Assigns a octodecic extension field element to a digit.
 *
 * @param[in] a				- the octodecic extension field element.
 * @param[in] b				- the digit.
 */
void fp18_set_dig(fp18_t a, dig_t b);

/**
 * Adds two octodecic extension field elements. Computes C = A + B.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the first octodecic extension field element.
 * @param[in] B				- the second octodecic extension field element.
 */
void fp18_add(fp18_t c, fp18_t a, fp18_t b);

/**
 * Subtracts a octodecic extension field element from another. Computes
 * C = A - B.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the first octodecic extension field element.
 * @param[in] B				- the second octodecic extension field element.
 */
void fp18_sub(fp18_t c, fp18_t a, fp18_t b);

/**
 * Negates a octodecic extension field element.
 *
 * @param[out] C			- the result.
 * @param[out] A			- the octodecic extension field element to negate.
 */
void fp18_neg(fp18_t c, fp18_t a);

/**
 * Multiples two octodecic extension field elements using basic arithmetic.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the octodecic extension field element.
 * @param[in] b				- the octodecic extension field element.
 */
void fp18_mul_basic(fp18_t c, fp18_t a, fp18_t b);

/**
 * Multiples two octodecic extension field elements using lazy reduction.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the octodecic extension field element.
 * @param[in] b				- the octodecic extension field element.
 */
void fp18_mul_lazyr(fp18_t c, fp18_t a, fp18_t b);

/**
 * Multiples a dense octodecic extension field element by a sparse element using
 * basic arithmetic.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the dense octodecic extension field element.
 * @param[in] b				- the sparse octodecic extension field element.
 */
void fp18_mul_dxs_basic(fp18_t c, fp18_t a, fp18_t b);

/**
 * Multiples a dense octodecic extension field element by a sparse element using
 * lazy reduction.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the dense octodecic extension field element.
 * @param[in] b				- the sparse octodecic extension field element.
 */
void fp18_mul_dxs_lazyr(fp18_t c, fp18_t a, fp18_t b);

/**
 * Computes the square of a octodecic extension field element using basic
 * arithmetic.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the octodecic extension field element to square.
 */
void fp18_sqr_basic(fp18_t c, fp18_t a);

/**
 * Computes the square of a octodecic extension field element using lazy
 * reduction.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the octodecic extension field element to square.
 */
void fp18_sqr_lazyr(fp18_t c, fp18_t a);

/**
 * Computes the square of a cyclotomic octodecic extension field element.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the cyclotomic extension element to square.
 */
void fp18_sqr_cyc(fp18_t c, fp18_t a);

/**
 * Computes the square of a compressed cyclotomic extension field element.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the cyclotomic extension element to square.
 */
void fp18_sqr_pck(fp18_t c, fp18_t a);

/**
 * Tests if a octodecic extension field element belongs to the cyclotomic
 * subgroup.
 *
 * @param[in] a				- the octodecic extension field element to test.
 * @return 1 if the extension field element is in the subgroup. 0 otherwise.
 */
int fp18_test_cyc(fp18_t a);

/**
 * Converts a octodecic extension field element to a cyclotomic element.
 * Computes c = a^(p^9 - 1)*(p^2 + 1).
 *
 * @param[out] c			- the result.
 * @param[in] a				- a octodecic extension field element.
 */
void fp18_conv_cyc(fp18_t c, fp18_t a);

/**
 * Decompresses a compressed cyclotomic extension field element to its
 * usual representation.
 *
 * @param[out] c			- the result.
 * @param[in] a				- a octodecic extension field element to decompress.
 */
void fp18_back_cyc(fp18_t c, fp18_t a);

/**
 * Decompresses multiple compressed cyclotomic extension field elements to their
 * usual representations.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the octodecic field elements to decompress.
 * @param[in] n				- the number of field elements to decompress.
 */
void fp18_back_cyc_sim(fp18_t *c, fp18_t *a, int n);

/**
 * Inverts a octodecic extension field element. Computes c = 1/a.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the octodecic extension field element to invert.
 */
void fp18_inv(fp18_t c, fp18_t a);

/**
 * Computes the inverse of a unitary octodecic extension field element.
 *
 * For unitary elements, this is equivalent to computing the conjugate.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the octodecic extension field element to invert.
 */
void fp18_inv_uni(fp18_t c, fp18_t a);

/**
 * Converts a octodecic extension field element to a unitary element. Computes
 * c = a^(p^9 - 1).
 *
 * @param[out] c			- the result.
 * @param[in] a				- a octodecic extension field element.
 */
void fp18_conv_uni(fp18_t c, fp18_t a);

/**
 * Computes the Frobenius endomorphism of a octodecic extension element.
 * Computes c = a^p.
 *
 * @param[out] c			- the result.
 * @param[in] a				- a octodecic extension field element.
 * @param[in] i				- the power of the Frobenius map.
 */
void fp18_frb(fp18_t c, fp18_t a, int i);

/**
 * Computes a power of a octodecic extension field element. Detects if the
 * extension field element is in a cyclotomic subgroup and if this is the case,
 * faster formulae are used.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the basis.
 * @param[in] b				- the exponent.
 */
void fp18_exp(fp18_t c, fp18_t a, bn_t b);

/**
 * Computes a power of a cyclotomic octodecic extension field element.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the basis.
 * @param[in] b				- the exponent.
 */
void fp18_exp_cyc(fp18_t c, fp18_t a, bn_t b);

/**
 * Computes a power of a cyclotomic octodecic extension field element.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the basis.
 * @param[in] b				- the exponent in sparse form.
 * @param[in] l				- the length of the exponent in sparse form.
 */
void fp18_exp_cyc_sps(fp18_t c, fp18_t a, int *b, int l);

#endif /* !RELIC_FPX_H */
