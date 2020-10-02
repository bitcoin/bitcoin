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
 * @defgroup epx Elliptic curves defined over extensions of prime fields.
 */

/**
 * @file
 *
 * Interface of the module for arithmetic on prime elliptic curves defined over
 * extension fields.
 *
 * @ingroup epx
 */

#ifndef RELIC_EPX_H
#define RELIC_EPX_H

#include "relic_fpx.h"
#include "relic_ep.h"
#include "relic_types.h"

/*============================================================================*/
/* Constant definitions                                                       */
/*============================================================================*/

/**
 * Size of a precomputation table using the binary method.
 */
#define RELIC_EPX_TABLE_BASIC		(2 * FP_BITS + 1)

/**
 * Size of a precomputation table using Yao's windowing method.
 */
#define RELIC_EPX_TABLE_YAOWI      (2 * FP_BITS / EP_DEPTH + 1)

/**
 * Size of a precomputation table using the NAF windowing method.
 */
#define RELIC_EPX_TABLE_NAFWI      (2 * FP_BITS / EP_DEPTH + 1)

/**
 * Size of a precomputation table using the single-table comb method.
 */
#define RELIC_EPX_TABLE_COMBS      (1 << EP_DEPTH)

/**
 * Size of a precomputation table using the double-table comb method.
 */
#define RELIC_EPX_TABLE_COMBD		(1 << (EP_DEPTH + 1))

/**
 * Size of a precomputation table using the w-(T)NAF method.
 */
#define RELIC_EPX_TABLE_LWNAF		(1 << (EP_DEPTH - 2))

/**
 * Size of a precomputation table using the chosen algorithm.
 */
#if EP_FIX == BASIC
#define RELIC_EPX_TABLE			RELIC_EPX_TABLE_BASIC
#elif EP_FIX == YAOWI
#define RELIC_EPX_TABLE			RELIC_EPX_TABLE_YAOWI
#elif EP_FIX == NAFWI
#define RELIC_EPX_TABLE			RELIC_EPX_TABLE_NAFWI
#elif EP_FIX == COMBS
#define RELIC_EPX_TABLE			RELIC_EPX_TABLE_COMBS
#elif EP_FIX == COMBD
#define RELIC_EPX_TABLE			RELIC_EPX_TABLE_COMBD
#elif EP_FIX == LWNAF
#define RELIC_EPX_TABLE			RELIC_EPX_TABLE_LWNAF
#endif

/**
 * Maximum size of a precomputation table.
 */
#ifdef STRIP
#define RELIC_EPX_TABLE_MAX RELIC_EPX_TABLE
#else
#define RELIC_EPX_TABLE_MAX MAX(RELIC_EPX_TABLE_BASIC, RELIC_EPX_TABLE_COMBD)
#endif


/*============================================================================*/
/* Type definitions                                                           */
/*============================================================================*/

/**
 * Represents an elliptic curve point over a quadratic extension over a prime
 * field.
 */
typedef struct {
	/** The first coordinate. */
	fp2_t x;
	/** The second coordinate. */
	fp2_t y;
	/** The third coordinate (projective representation). */
	fp2_t z;
	/** Flag to indicate that this point is normalized. */
	int norm;
} ep2_st;

/**
 * Pointer to an elliptic curve point.
 */
#if ALLOC == AUTO
typedef ep2_st ep2_t[1];
#else
typedef ep2_st *ep2_t;
#endif

/**
 * Represents an elliptic curve point over a cubic extension over a prime
 * field.
 */
typedef struct {
	/** The first coordinate. */
	fp3_t x;
	/** The second coordinate. */
	fp3_t y;
	/** The third coordinate (projective representation). */
	fp3_t z;
	/** Flag to indicate that this point is normalized. */
	int norm;
} ep3_st;

/**
 * Pointer to an elliptic curve point.
 */
#if ALLOC == AUTO
typedef ep3_st ep3_t[1];
#else
typedef ep3_st *ep3_t;
#endif

/*============================================================================*/
/* Macro definitions                                                          */
/*============================================================================*/

/**
 * Initializes a point on a elliptic curve with a null value.
 *
 * @param[out] A				- the point to initialize.
 */
#if ALLOC == AUTO
#define ep2_null(A)				/* empty */
#else
#define ep2_null(A)				A = NULL
#endif

/**
 * Calls a function to allocate a point on a elliptic curve.
 *
 * @param[out] A				- the new point.
 * @throw ERR_NO_MEMORY			- if there is no available memory.
 */
#if ALLOC == DYNAMIC
#define ep2_new(A)															\
	A = (ep2_t)calloc(1, sizeof(ep2_st));									\
	if (A == NULL) {														\
		THROW(ERR_NO_MEMORY);												\
	}																		\
	fp2_null((A)->x);														\
	fp2_null((A)->y);														\
	fp2_null((A)->z);														\
	fp2_new((A)->x);														\
	fp2_new((A)->y);														\
	fp2_new((A)->z);														\

#elif ALLOC == STATIC
#define ep2_new(A)															\
	A = (ep2_t)alloca(sizeof(ep2_st));										\
	if (A == NULL) {														\
		THROW(ERR_NO_MEMORY);												\
	}																		\
	fp2_null((A)->x);														\
	fp2_null((A)->y);														\
	fp2_null((A)->z);														\
	fp2_new((A)->x);														\
	fp2_new((A)->y);														\
	fp2_new((A)->z);														\

#elif ALLOC == AUTO
#define ep2_new(A)				/* empty */

#elif ALLOC == STACK
#define ep2_new(A)															\
	A = (ep2_t)alloca(sizeof(ep2_st));										\
	fp2_new((A)->x);														\
	fp2_new((A)->y);														\
	fp2_new((A)->z);														\

#endif

/**
 * Calls a function to clean and free a point on a elliptic curve.
 *
 * @param[out] A				- the point to free.
 */
#if ALLOC == DYNAMIC
#define ep2_free(A)															\
	if (A != NULL) {														\
		fp2_free((A)->x);													\
		fp2_free((A)->y);													\
		fp2_free((A)->z);													\
		free(A);															\
		A = NULL;															\
	}																		\

#elif ALLOC == STATIC
#define ep2_free(A)															\
	if (A != NULL) {														\
		fp2_free((A)->x);													\
		fp2_free((A)->y);													\
		fp2_free((A)->z);													\
		A = NULL;															\
	}																		\

#elif ALLOC == AUTO
#define ep2_free(A)				/* empty */
#elif ALLOC == STACK
#define ep2_free(A)				A = NULL;
#endif

/**
 * Negates a point in an elliptic curve over a quadratic extension field.
 * Computes R = -P.
 *
 * @param[out] R				- the result.
 * @param[in] P					- the point to negate.
 */
#if EP_ADD == BASIC
#define ep2_neg(R, P)			ep2_neg_basic(R, P)
#elif EP_ADD == PROJC
#define ep2_neg(R, P)			ep2_neg_projc(R, P)
#endif

/**
 * Adds two points in an elliptic curve over a quadratic extension field.
 * Computes R = P + Q.
 *
 * @param[out] R				- the result.
 * @param[in] P					- the first point to add.
 * @param[in] Q					- the second point to add.
 */
#if EP_ADD == BASIC
#define ep2_add(R, P, Q)		ep2_add_basic(R, P, Q);
#elif EP_ADD == PROJC
#define ep2_add(R, P, Q)		ep2_add_projc(R, P, Q);
#endif

/**
 * Subtracts a point in an elliptic curve over a quadratic extension field from
 * another point in this curve. Computes R = P - Q.
 *
 * @param[out] R				- the result.
 * @param[in] P					- the first point.
 * @param[in] Q					- the second point.
 */
#if EP_ADD == BASIC
#define ep2_sub(R, P, Q)		ep2_sub_basic(R, P, Q)
#elif EP_ADD == PROJC
#define ep2_sub(R, P, Q)		ep2_sub_projc(R, P, Q)
#endif

/**
 * Doubles a point in an elliptic curve over a quadratic extension field.
 * Computes R = 2P.
 *
 * @param[out] R				- the result.
 * @param[in] P					- the point to double.
 */
#if EP_ADD == BASIC
#define ep2_dbl(R, P)			ep2_dbl_basic(R, P);
#elif EP_ADD == PROJC
#define ep2_dbl(R, P)			ep2_dbl_projc(R, P);
#endif

/**
 * Multiplies a point in an elliptic curve over a quadratic extension field.
 * Computes R = kP.
 *
 * @param[out] R			- the result.
 * @param[in] P				- the point to multiply.
 * @param[in] K				- the integer.
 */
#if EP_MUL == BASIC
#define ep2_mul(R, P, K)		ep2_mul_basic(R, P, K)
#elif EP_MUL == SLIDE
#define ep2_mul(R, P, K)		ep2_mul_slide(R, P, K)
#elif EP_MUL == MONTY
#define ep2_mul(R, P, K)		ep2_mul_monty(R, P, K)
#elif EP_MUL == LWNAF
#define ep2_mul(R, P, K)		ep2_mul_lwnaf(R, P, K)
#endif

/**
 * Builds a precomputation table for multiplying a fixed prime elliptic point
 * over a quadratic extension.
 *
 * @param[out] T				- the precomputation table.
 * @param[in] P					- the point to multiply.
 */
#if EP_FIX == BASIC
#define ep2_mul_pre(T, P)		ep2_mul_pre_basic(T, P)
#elif EP_FIX == YAOWI
#define ep2_mul_pre(T, P)		ep2_mul_pre_yaowi(T, P)
#elif EP_FIX == NAFWI
#define ep2_mul_pre(T, P)		ep2_mul_pre_nafwi(T, P)
#elif EP_FIX == COMBS
#define ep2_mul_pre(T, P)		ep2_mul_pre_combs(T, P)
#elif EP_FIX == COMBD
#define ep2_mul_pre(T, P)		ep2_mul_pre_combd(T, P)
#elif EP_FIX == LWNAF
#define ep2_mul_pre(T, P)		ep2_mul_pre_lwnaf(T, P)
#elif EP_FIX == GLV
//TODO: implement ep2_mul_pre_glv
#define ep2_mul_pre(T, P)		ep2_mul_pre_lwnaf(T, P)
#endif

/**
 * Multiplies a fixed prime elliptic point over a quadratic extension using a
 * precomputation table. Computes R = kP.
 *
 * @param[out] R				- the result.
 * @param[in] T					- the precomputation table.
 * @param[in] K					- the integer.
 */
#if EP_FIX == BASIC
#define ep2_mul_fix(R, T, K)	ep2_mul_fix_basic(R, T, K)
#elif EP_FIX == YAOWI
#define ep2_mul_fix(R, T, K)	ep2_mul_fix_yaowi(R, T, K)
#elif EP_FIX == NAFWI
#define ep2_mul_fix(R, T, K)	ep2_mul_fix_nafwi(R, T, K)
#elif EP_FIX == COMBS
#define ep2_mul_fix(R, T, K)	ep2_mul_fix_combs(R, T, K)
#elif EP_FIX == COMBD
#define ep2_mul_fix(R, T, K)	ep2_mul_fix_combd(R, T, K)
#elif EP_FIX == LWNAF
#define ep2_mul_fix(R, T, K)	ep2_mul_fix_lwnaf(R, T, K)
#elif EP_FIX == GLV
//TODO: implement ep2_mul_pre_glv
#define ep2_mul_fix(R, T, K)	ep2_mul_fix_lwnaf(R, T, K)
#endif

/**
 * Multiplies and adds two prime elliptic curve points simultaneously. Computes
 * R = kP + lQ.
 *
 * @param[out] R				- the result.
 * @param[in] P					- the first point to multiply.
 * @param[in] K					- the first integer.
 * @param[in] Q					- the second point to multiply.
 * @param[in] M					- the second integer,
 */
#if EP_SIM == BASIC
#define ep2_mul_sim(R, P, K, Q, M)	ep2_mul_sim_basic(R, P, K, Q, M)
#elif EP_SIM == TRICK
#define ep2_mul_sim(R, P, K, Q, M)	ep2_mul_sim_trick(R, P, K, Q, M)
#elif EP_SIM == INTER
#define ep2_mul_sim(R, P, K, Q, M)	ep2_mul_sim_inter(R, P, K, Q, M)
#elif EP_SIM == JOINT
#define ep2_mul_sim(R, P, K, Q, M)	ep2_mul_sim_joint(R, P, K, Q, M)
#endif

/*============================================================================*/
/* Function prototypes                                                        */
/*============================================================================*/

/**
 * Initializes the elliptic curve over quadratic extension.
 */
void ep2_curve_init(void);

/**
 * Finalizes the elliptic curve over quadratic extension.
 */
void ep2_curve_clean(void);

/**
 * Returns the 'a' coefficient of the currently configured elliptic curve.
 *
 * @param[out] a			- the 'a' coefficient of the elliptic curve.
 */
void ep2_curve_get_a(fp2_t a);

/**
 * Returns the 'b' coefficient of the currently configured elliptic curve.
 *
 * @param[out] b			- the 'b' coefficient of the elliptic curve.
 */
void ep2_curve_get_b(fp2_t b);

/**
 * Returns the vector of coefficients required to perform GLV method.
 *
 * @param[out] b			- the vector of coefficients.
 */
void ep2_curve_get_vs(bn_t *v);

/**
 * Returns a optimization identifier based on the 'a' coefficient of the curve.
 *
 * @return the optimization identifier.
 */
int ep2_curve_opt_a(void);

/**
 * Tests if the configured elliptic curve is a twist.
 *
 * @return the type of the elliptic curve twist, 0 if non-twisted curve.
 */
int ep2_curve_is_twist(void);

/**
 * Returns the generator of the group of points in the elliptic curve.
 *
 * @param[out] g			- the returned generator.
 */
void ep2_curve_get_gen(ep2_t g);

/**
 * Returns the precomputation table for the generator.
 *
 * @return the table.
 */
ep2_t *ep2_curve_get_tab(void);

/**
 * Returns the order of the group of points in the elliptic curve.
 *
 * @param[out] n			- the returned order.
 */
void ep2_curve_get_ord(bn_t n);

/**
 * Returns the cofactor of the group order in the elliptic curve.
 *
 * @param[out] h			- the returned cofactor.
 */
void ep2_curve_get_cof(bn_t h);

/**
 * Returns the sqrt(-3) mod q in the curve, where q is the prime.
 *
 * @param[out] h			- the returned cofactor.
 */
void ep2_curve_get_s3(bn_t s3);

/**
 * Returns the (sqrt(-3) - 1) / 2 mod q in the curve, where q is the prime.
 *
 * @param[out] h			- the returned cofactor.
 */
void ep2_curve_get_s32(bn_t s32);

/**
 * Configures an elliptic curve over a quadratic extension by its coefficients.
 *
 * @param[in] a			- the 'a' coefficient of the curve.
 * @param[in] b			- the 'b' coefficient of the curve.
 * @param[in] g			- the generator.
 * @param[in] r			- the order of the group of points.
 * @param[in] h			- the cofactor of the group order.
 */
void ep2_curve_set(fp2_t a, fp2_t b, ep2_t g, bn_t r, bn_t h);

/**
 * Configures an elliptic curve by twisting the curve over the base prime field.
 *
 *  @param				- the type of twist (multiplicative or divisive)
 */
void ep2_curve_set_twist(int type);

/**
 * Tests if a point on a elliptic curve is at the infinity.
 *
 * @param[in] p				- the point to test.
 * @return 1 if the point is at infinity, 0 otherise.
 */
int ep2_is_infty(ep2_t p);

/**
 * Assigns a elliptic curve point to a point at the infinity.
 *
 * @param[out] p			- the point to assign.
 */
void ep2_set_infty(ep2_t p);

/**
 * Copies the second argument to the first argument.
 *
 * @param[out] q			- the result.
 * @param[in] p				- the elliptic curve point to copy.
 */
void ep2_copy(ep2_t r, ep2_t p);

/**
 * Compares two elliptic curve points.
 *
 * @param[in] p				- the first elliptic curve point.
 * @param[in] q				- the second elliptic curve point.
 * @return CMP_EQ if p == q and CMP_NE if p != q.
 */
int ep2_cmp(ep2_t p, ep2_t q);

/**
 * Assigns a random value to an elliptic curve point.
 *
 * @param[out] p			- the elliptic curve point to assign.
 */
void ep2_rand(ep2_t p);

/**
 * Computes the right-hand side of the elliptic curve equation at a certain
 * elliptic curve point.
 *
 * @param[out] rhs			- the result.
 * @param[in] p				- the point.
 */
void ep2_rhs(fp2_t rhs, ep2_t p);

/**
 * Tests if a point is in the curve.
 *
 * @param[in] p				- the point to test.
 */
int ep2_is_valid(ep2_t p);

/**
 * Builds a precomputation table for multiplying a random prime elliptic point.
 *
 * @param[out] t			- the precomputation table.
 * @param[in] p				- the point to multiply.
 * @param[in] w				- the window width.
 */
void ep2_tab(ep2_t *t, ep2_t p, int w);

/**
 * Prints a elliptic curve point.
 *
 * @param[in] p				- the elliptic curve point to print.
 */
void ep2_print(ep2_t p);

/**
 * Returns the number of bytes necessary to store a prime elliptic curve point
 * over a quadratic extension with optional point compression.
 *
 * @param[in] a				- the prime field element.
 * @param[in] pack			- the flag to indicate compression.
 * @return the number of bytes.
 */
int ep2_size_bin(ep2_t a, int pack);

/**
 * Reads a prime elliptic curve point over a quadratic extension from a byte
 * vector in big-endian format.
 *
 * @param[out] a			- the result.
 * @param[in] bin			- the byte vector.
 * @param[in] len			- the buffer capacity.
 * @throw ERR_NO_VALID		- if the encoded point is invalid.
 * @throw ERR_NO_BUFFER		- if the buffer capacity is invalid.
 */
void ep2_read_bin(ep2_t a, uint8_t *bin, int len);

/**
 * Writes a prime elliptic curve pointer over a quadratic extension to a byte
 * vector in big-endian format with optional point compression.
 *
 * @param[out] bin			- the byte vector.
 * @param[in] len			- the buffer capacity.
 * @param[in] a				- the prime elliptic curve point to write.
 * @param[in] pack			- the flag to indicate point compression.
 * @throw ERR_NO_BUFFER		- if the buffer capacity is invalid.
 */
void ep2_write_bin(uint8_t *bin, int len, ep2_t a, int pack);

/**
 * Negates a point represented in affine coordinates in an elliptic curve over
 * a quadratic extension.
 *
 * @param[out] r			- the result.
 * @param[out] p			- the point to negate.
 */
void ep2_neg_basic(ep2_t r, ep2_t p);

/**
 * Negates a point represented in projective coordinates in an elliptic curve
 * over a quadratic exyension.
 *
 * @param[out] r			- the result.
 * @param[out] p			- the point to negate.
 */
void ep2_neg_projc(ep2_t r, ep2_t p);

/**
 * Adds to points represented in affine coordinates in an elliptic curve over a
 * quadratic extension.
 *
 * @param[out] r			- the result.
 * @param[in] p				- the first point to add.
 * @param[in] q				- the second point to add.
 */
void ep2_add_basic(ep2_t r, ep2_t p, ep2_t q);

/**
 * Adds to points represented in affine coordinates in an elliptic curve over a
 * quadratic extension and returns the computed slope.
 *
 * @param[out] r			- the result.
 * @param[out] s			- the slope.
 * @param[in] p				- the first point to add.
 * @param[in] q				- the second point to add.
 */
void ep2_add_slp_basic(ep2_t r, fp2_t s, ep2_t p, ep2_t q);

/**
 * Subtracts a points represented in affine coordinates in an elliptic curve
 * over a quadratic extension from another point.
 *
 * @param[out] r			- the result.
 * @param[in] p				- the first point.
 * @param[in] q				- the point to subtract.
 */
void ep2_sub_basic(ep2_t r, ep2_t p, ep2_t q);

/**
 * Adds two points represented in projective coordinates in an elliptic curve
 * over a quadratic extension.
 *
 * @param[out] r			- the result.
 * @param[in] p				- the first point to add.
 * @param[in] q				- the second point to add.
 */
void ep2_add_projc(ep2_t r, ep2_t p, ep2_t q);

/**
 * Subtracts a points represented in projective coordinates in an elliptic curve
 * over a quadratic extension from another point.
 *
 * @param[out] r			- the result.
 * @param[in] p				- the first point.
 * @param[in] q				- the point to subtract.
 */
void ep2_sub_projc(ep2_t r, ep2_t p, ep2_t q);

/**
 * Doubles a points represented in affine coordinates in an elliptic curve over
 * a quadratic extension.
 *
 * @param[out] r			- the result.
 * @param[int] p			- the point to double.
 */
void ep2_dbl_basic(ep2_t r, ep2_t p);

/**
 * Doubles a points represented in affine coordinates in an elliptic curve over
 * a quadratic extension and returns the computed slope.
 *
 * @param[out] r			- the result.
 * @param[out] s			- the slope.
 * @param[in] p				- the point to double.
 */
void ep2_dbl_slp_basic(ep2_t r, fp2_t s, ep2_t p);

/**
 * Doubles a points represented in projective coordinates in an elliptic curve
 * over a quadratic extension.
 *
 * @param[out] r			- the result.
 * @param[in] p				- the point to double.
 */
void ep2_dbl_projc(ep2_t r, ep2_t p);

/**
 * Multiplies a prime elliptic point by an integer using the binary method.
 *
 * @param[out] r			- the result.
 * @param[in] p				- the point to multiply.
 * @param[in] k				- the integer.
 */
void ep2_mul_basic(ep2_t r, ep2_t p, const bn_t k);

/**
 * Multiplies a prime elliptic point by an integer using the sliding window
 * method.
 *
 * @param[out] r			- the result.
 * @param[in] p				- the point to multiply.
 * @param[in] k				- the integer.
 */
void ep2_mul_slide(ep2_t r, ep2_t p, const bn_t k);

/**
 * Multiplies a prime elliptic point by an integer using the constant-time
 * Montgomery laddering point multiplication method.
 *
 * @param[out] r			- the result.
 * @param[in] p				- the point to multiply.
 * @param[in] k				- the integer.
 */
void ep2_mul_monty(ep2_t r, ep2_t p, const bn_t k);

/**
 * Multiplies a prime elliptic point by an integer using the w-NAF method.
 *
 * @param[out] r			- the result.
 * @param[in] p				- the point to multiply.
 * @param[in] k				- the integer.
 */
void ep2_mul_lwnaf(ep2_t r, ep2_t p, const bn_t k);

/**
 * Multiplies a prime elliptic point by an integer using a regular method.
 *
 * @param[out] r			- the result.
 * @param[in] p				- the point to multiply.
 * @param[in] k				- the integer.
 */
void ep2_mul_lwreg(ep2_t r, ep2_t p, const bn_t k);

/**
 * Multiplies the generator of an elliptic curve over a qaudratic extension.
 *
 * @param[out] r			- the result.
 * @param[in] k				- the integer.
 */
void ep2_mul_gen(ep2_t r, bn_t k);

/**
 * Builds a precomputation table for multiplying a fixed prime elliptic point
 * using the binary method.
 *
 * @param[out] t			- the precomputation table.
 * @param[in] p				- the point to multiply.
 */
void ep2_mul_pre_basic(ep2_t *t, ep2_t p);

/**
 * Builds a precomputation table for multiplying a fixed prime elliptic point
 * using Yao's windowing method.
 *
 * @param[out] t			- the precomputation table.
 * @param[in] p				- the point to multiply.
 */
void ep2_mul_pre_yaowi(ep2_t *t, ep2_t p);

/**
 * Builds a precomputation table for multiplying a fixed prime elliptic point
 * using the NAF windowing method.
 *
 * @param[out] t			- the precomputation table.
 * @param[in] p				- the point to multiply.
 */
void ep2_mul_pre_nafwi(ep2_t *t, ep2_t p);

/**
 * Builds a precomputation table for multiplying a fixed prime elliptic point
 * using the single-table comb method.
 *
 * @param[out] t			- the precomputation table.
 * @param[in] p				- the point to multiply.
 */
void ep2_mul_pre_combs(ep2_t *t, ep2_t p);

/**
 * Builds a precomputation table for multiplying a fixed prime elliptic point
 * using the double-table comb method.
 *
 * @param[out] t			- the precomputation table.
 * @param[in] p				- the point to multiply.
 */
void ep2_mul_pre_combd(ep2_t *t, ep2_t p);

/**
 * Builds a precomputation table for multiplying a fixed prime elliptic point
 * using the w-(T)NAF method.
 *
 * @param[out] t			- the precomputation table.
 * @param[in] p				- the point to multiply.
 */
void ep2_mul_pre_lwnaf(ep2_t *t, ep2_t p);

/**
 * Multiplies a fixed prime elliptic point using a precomputation table and
 * the binary method.
 *
 * @param[out] r			- the result.
 * @param[in] t				- the precomputation table.
 * @param[in] k				- the integer.
 */
void ep2_mul_fix_basic(ep2_t r, ep2_t *t, bn_t k);

/**
 * Multiplies a fixed prime elliptic point using a precomputation table and
 * Yao's windowing method
 *
 * @param[out] r			- the result.
 * @param[in] t				- the precomputation table.
 * @param[in] k				- the integer.
 */
void ep2_mul_fix_yaowi(ep2_t r, ep2_t *t, bn_t k);

/**
 * Multiplies a fixed prime elliptic point using a precomputation table and
 * the w-(T)NAF method.
 *
 * @param[out] r			- the result.
 * @param[in] t				- the precomputation table.
 * @param[in] k				- the integer.
 */
void ep2_mul_fix_nafwi(ep2_t r, ep2_t *t, bn_t k);

/**
 * Multiplies a fixed prime elliptic point using a precomputation table and
 * the single-table comb method.
 *
 * @param[out] r			- the result.
 * @param[in] t				- the precomputation table.
 * @param[in] k				- the integer.
 */
void ep2_mul_fix_combs(ep2_t r, ep2_t *t, bn_t k);

/**
 * Multiplies a fixed prime elliptic point using a precomputation table and
 * the double-table comb method.
 *
 * @param[out] r			- the result.
 * @param[in] t				- the precomputation table.
 * @param[in] k				- the integer.
 */
void ep2_mul_fix_combd(ep2_t r, ep2_t *t, bn_t k);

/**
 * Multiplies a fixed prime elliptic point using a precomputation table and
 * the w-(T)NAF method.
 *
 * @param[out] r			- the result.
 * @param[in] t				- the precomputation table.
 * @param[in] k				- the integer.
 */
void ep2_mul_fix_lwnaf(ep2_t r, ep2_t *t, bn_t k);

/**
 * Multiplies and adds two prime elliptic curve points simultaneously using
 * scalar multiplication and point addition.
 *
 * @param[out] r			- the result.
 * @param[in] p				- the first point to multiply.
 * @param[in] k				- the first integer.
 * @param[in] q				- the second point to multiply.
 * @param[in] m				- the second integer,
 */
void ep2_mul_sim_basic(ep2_t r, ep2_t p, bn_t k, ep2_t q, bn_t m);

/**
 * Multiplies and adds two prime elliptic curve points simultaneously using
 * shamir's trick.
 *
 * @param[out] r			- the result.
 * @param[in] p				- the first point to multiply.
 * @param[in] k				- the first integer.
 * @param[in] q				- the second point to multiply.
 * @param[in] m				- the second integer,
 */
void ep2_mul_sim_trick(ep2_t r, ep2_t p, bn_t k, ep2_t q, bn_t m);

/**
 * Multiplies and adds two prime elliptic curve points simultaneously using
 * interleaving of NAFs.
 *
 * @param[out] r			- the result.
 * @param[in] p				- the first point to multiply.
 * @param[in] k				- the first integer.
 * @param[in] q				- the second point to multiply.
 * @param[in] m				- the second integer,
 */
void ep2_mul_sim_inter(ep2_t r, ep2_t p, bn_t k, ep2_t q, bn_t m);

/**
 * Multiplies and adds two prime elliptic curve points simultaneously using
 * Solinas' Joint Sparse Form.
 *
 * @param[out] r			- the result.
 * @param[in] p				- the first point to multiply.
 * @param[in] k				- the first integer.
 * @param[in] q				- the second point to multiply.
 * @param[in] m				- the second integer,
 */
void ep2_mul_sim_joint(ep2_t r, ep2_t p, bn_t k, ep2_t q, bn_t m);

/**
 * Multiplies and adds the generator and a prime elliptic curve point
 * simultaneously. Computes R = kG + lQ.
 *
 * @param[out] r			- the result.
 * @param[in] k				- the first integer.
 * @param[in] q				- the second point to multiply.
 * @param[in] m				- the second integer,
 */
void ep2_mul_sim_gen(ep2_t r, bn_t k, ep2_t q, bn_t m);

/**
 * Multiplies a prime elliptic point by a small integer.
 *
 * @param[out] r			- the result.
 * @param[in] p				- the point to multiply.
 * @param[in] k				- the integer.
 */
void ep2_mul_dig(ep2_t r, ep2_t p, dig_t k);

/**
 * Converts a point to affine coordinates.
 *
 * @param[out] r			- the result.
 * @param[in] p				- the point to convert.
 */
void ep2_norm(ep2_t r, ep2_t p);

/**
 * Converts multiple points to affine coordinates.
 *
 * @param[out] r			- the result.
 * @param[in] t				- the points to convert.
 * @param[in] n				- the number of points.
 */
void ep2_norm_sim(ep2_t *r, ep2_t *t, int n);

/**
 * Maps a byte array to a point in an elliptic curve over a quadratic extension.
 *
 * @param[out] p			- the result.
 * @param[in] msg			- the byte array to map.
 * @param[in] len			- the array length in bytes.
 * @param[in] performHash	- whether to hash internally
 */
void ep2_map(ep2_t p, const uint8_t *msg, int len, int performHash);

/**
 * Computes a power of the Gailbraith-Lin-Scott homomorphism of a point
 * represented in affine coordinates on a twisted elliptic curve over a
 * quadratic exension. That is, Psi^i(P) = Twist(P)(Frob^i(unTwist(P)).
 * On the trace-zero group of a quadratic twist, consists of a power of the
 * Frobenius map of a point represented in affine coordinates in an elliptic
 * curve over a quadratic exension. Computes Frob^i(P) = (p^i)P.
 *
 * @param[out] r			- the result in affine coordinates.
 * @param[in] p				- a point in affine coordinates.
 * @param[in] i				- the power of the Frobenius map.
 */
void ep2_frb(ep2_t r, ep2_t p, int i);

/**
 * Compresses a point in an elliptic curve over a quadratic extension.
 *
 * @param[out] r			- the result.
 * @param[in] p				- the point to compress.
 */
void ep2_pck(ep2_t r, ep2_t p);

/**
 * Decompresses a point in an elliptic curve over a quadratic extension.
 *
 * @param[out] r			- the result.
 * @param[in] p				- the point to decompress.
 * @return if the decompression was successful
 */
int ep2_upk(ep2_t r, ep2_t p);

#endif /* !RELIC_EPX_H */
