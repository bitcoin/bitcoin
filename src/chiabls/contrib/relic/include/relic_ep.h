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
 * @defgroup ep Elliptic curves over prime fields
 */

/**
 * @file
 *
 * Interface of the module for arithmetic on prime elliptic curves.
 *
 * @ingroup ep
 */

#ifndef RELIC_EP_H
#define RELIC_EP_H

#include "relic_fp.h"
#include "relic_bn.h"
#include "relic_types.h"
#include "relic_label.h"

/*============================================================================*/
/* Constant definitions                                                       */
/*============================================================================*/

/**
 * Prime elliptic curve identifiers.
 */
enum {
	/** SECG P-160 prime curve. */
	SECG_P160 = 1,
	/** SECG K-160 prime curve. */
	SECG_K160,
	/** NIST P-192 prime curve. */
	NIST_P192,
	/** SECG K-192 prime curve. */
	SECG_K192,
	/** Curve22103 prime curve. */
	CURVE_22103,
	/** NIST P-224 prime curve. */
	NIST_P224,
	/** SECG K-224 prime curve. */
	SECG_K224,
	/** Curve4417 prime curve. */
	CURVE_4417,
	/** Curve1147 prime curve. */
	CURVE_1174,
	/** Curve25519 prime curve. */
	CURVE_25519,
	/** NIST P-256 prime curve. */
	NIST_P256,
	/** Brainpool P256r1 curve. */
	BSI_P256,
	/** SECG K-256 prime curve. */
	SECG_K256,
	/** Curve67254 prime curve. */
	CURVE_67254,
	/** Curve383187 prime curve. */
	CURVE_383187,
	/** NIST P-384 prime curve. */
	NIST_P384,
	/** Curve 511187 prime curve. */
	CURVE_511187,
	/** NIST P-521 prime curve. */
	NIST_P521,
	/** Barreto-Naehrig curve with positive x */
	BN_P158,
	/** Barreto-Naehrig curve with negative x (found by Nogami et al.). */
	BN_P254,
	/** Barreto-Naehrig curve with negative x. */
	BN_P256,
	/** Barreto-Lynn-Scott curve with embedding degree 12. */
	B12_P381,
	/** Barreto-Naehrig curve with negative x. */
	BN_P382,
	/** Barreto-Lynn-Scott curve with embedding degree 12. */
	B12_P455,
	/** Barreto-Lynn-Scott curve with embedding degree 24. */
	B24_P477,
	/** Kachisa-Schafer-Scott with negative x. */
	KSS_P508,
	/** Barreto-Naehrig curve with positive x. */
	BN_P638,
	/** Barreto-Lynn-Scott curve with embedding degree 12. */
	B12_P638,
	/** 1536-bit supersingular curve. */
	SS_P1536,
};

/*============================================================================*/
/* Constant definitions                                                       */
/*============================================================================*/

/**
 * Denotes a divisive twist.
 */
#define EP_DTYPE		1

/**
 * Denotes a multiplicative twist.
 */
#define EP_MTYPE		2

/**
 * Size of a precomputation table using the binary method.
 */
#define RELIC_EP_TABLE_BASIC		(FP_BITS + 1)

/**
 * Size of a precomputation table using Yao's windowing method.
 */
#define RELIC_EP_TABLE_YAOWI      (FP_BITS / EP_DEPTH + 1)

/**
 * Size of a precomputation table using the NAF windowing method.
 */
#define RELIC_EP_TABLE_NAFWI      (FP_BITS / EP_DEPTH + 1)

/**
 * Size of a precomputation table using the single-table comb method.
 */
#define RELIC_EP_TABLE_COMBS      (1 << EP_DEPTH)

/**
 * Size of a precomputation table using the double-table comb method.
 */
#define RELIC_EP_TABLE_COMBD		(1 << (EP_DEPTH + 1))

/**
 * Size of a precomputation table using the w-(T)NAF method.
 */
#define RELIC_EP_TABLE_LWNAF		(1 << (EP_DEPTH - 2))

/**
 * Size of a precomputation table using the chosen algorithm.
 */
#if EP_FIX == BASIC
#define RELIC_EP_TABLE			RELIC_EP_TABLE_BASIC
#elif EP_FIX == YAOWI
#define RELIC_EP_TABLE			RELIC_EP_TABLE_YAOWI
#elif EP_FIX == NAFWI
#define RELIC_EP_TABLE			RELIC_EP_TABLE_NAFWI
#elif EP_FIX == COMBS
#define RELIC_EP_TABLE			RELIC_EP_TABLE_COMBS
#elif EP_FIX == COMBD
#define RELIC_EP_TABLE			RELIC_EP_TABLE_COMBD
#elif EP_FIX == LWNAF
#define RELIC_EP_TABLE			RELIC_EP_TABLE_LWNAF
#endif

/**
 * Maximum size of a precomputation table.
 */
#ifdef STRIP
#define RELIC_EP_TABLE_MAX RELIC_EP_TABLE
#else
#define RELIC_EP_TABLE_MAX MAX(RELIC_EP_TABLE_BASIC, RELIC_EP_TABLE_COMBD)
#endif

/*============================================================================*/
/* Type definitions                                                           */
/*============================================================================*/

/**
 * Represents an elliptic curve point over a prime field.
 */
typedef struct {
#if ALLOC == STATIC
	/** The first coordinate. */
	fp_t x;
	/** The second coordinate. */
	fp_t y;
	/** The third coordinate (projective representation). */
	fp_t z;
#elif ALLOC == DYNAMIC || ALLOC == STACK || ALLOC == AUTO
	/** The first coordinate. */
	fp_st x;
	/** The second coordinate. */
	fp_st y;
	/** The third coordinate (projective representation). */
	fp_st z;
#endif
	/** Flag to indicate that this point is normalized. */
	int norm;
} ep_st;

/**
 * Pointer to an elliptic curve point.
 */
#if ALLOC == AUTO
typedef ep_st ep_t[1];
#else
typedef ep_st *ep_t;
#endif

/*============================================================================*/
/* Macro definitions                                                          */
/*============================================================================*/

/**
 * Initializes a point on a prime elliptic curve with a null value.
 *
 * @param[out] A			- the point to initialize.
 */
#if ALLOC == AUTO
#define ep_null(A)				/* empty */
#else
#define ep_null(A)		A = NULL;
#endif

/**
 * Calls a function to allocate a point on a prime elliptic curve.
 *
 * @param[out] A			- the new point.
 * @throw ERR_NO_MEMORY		- if there is no available memory.
 */
#if ALLOC == DYNAMIC
#define ep_new(A)															\
	A = (ep_t)calloc(1, sizeof(ep_st));										\
	if (A == NULL) {														\
		THROW(ERR_NO_MEMORY);												\
	}																		\

#elif ALLOC == STATIC
#define ep_new(A)															\
	A = (ep_t)alloca(sizeof(ep_st));										\
	if (A == NULL) {														\
		THROW(ERR_NO_MEMORY);												\
	}																		\
	fp_null((A)->x);														\
	fp_null((A)->y);														\
	fp_null((A)->z);														\
	fp_new((A)->x);															\
	fp_new((A)->y);															\
	fp_new((A)->z);															\

#elif ALLOC == AUTO
#define ep_new(A)				/* empty */

#elif ALLOC == STACK
#define ep_new(A)															\
	A = (ep_t)alloca(sizeof(ep_st));										\

#endif

/**
 * Calls a function to clean and free a point on a prime elliptic curve.
 *
 * @param[out] A			- the point to free.
 */
#if ALLOC == DYNAMIC
#define ep_free(A)															\
	if (A != NULL) {														\
		free(A);															\
		A = NULL;															\
	}

#elif ALLOC == STATIC
#define ep_free(A)															\
	if (A != NULL) {														\
		fp_free((A)->x);													\
		fp_free((A)->y);													\
		fp_free((A)->z);													\
		A = NULL;															\
	}																		\

#elif ALLOC == AUTO
#define ep_free(A)				/* empty */

#elif ALLOC == STACK
#define ep_free(A)															\
	A = NULL;																\

#endif

/**
 * Negates a prime elliptic curve point. Computes R = -P.
 *
 * @param[out] R			- the result.
 * @param[in] P				- the point to negate.
 */
#if EP_ADD == BASIC
#define ep_neg(R, P)		ep_neg_basic(R, P)
#elif EP_ADD == PROJC
#define ep_neg(R, P)		ep_neg_projc(R, P)
#endif

/**
 * Adds two prime elliptic curve points. Computes R = P + Q.
 *
 * @param[out] R			- the result.
 * @param[in] P				- the first point to add.
 * @param[in] Q				- the second point to add.
 */
#if EP_ADD == BASIC
#define ep_add(R, P, Q)		ep_add_basic(R, P, Q);
#elif EP_ADD == PROJC
#define ep_add(R, P, Q)		ep_add_projc(R, P, Q);
#endif

/**
 * Subtracts a prime elliptic curve point from another. Computes R = P - Q.
 *
 * @param[out] R			- the result.
 * @param[in] P				- the first point.
 * @param[in] Q				- the second point.
 */
#if EP_ADD == BASIC
#define ep_sub(R, P, Q)		ep_sub_basic(R, P, Q)
#elif EP_ADD == PROJC
#define ep_sub(R, P, Q)		ep_sub_projc(R, P, Q)
#endif

/**
 * Doubles a prime elliptic curve point. Computes R = 2P.
 *
 * @param[out] R			- the result.
 * @param[in] P				- the point to double.
 */
#if EP_ADD == BASIC
#define ep_dbl(R, P)		ep_dbl_basic(R, P);
#elif EP_ADD == PROJC
#define ep_dbl(R, P)		ep_dbl_projc(R, P);
#endif

/**
 * Multiplies a prime elliptic curve point by an integer. Computes R = kP.
 *
 * @param[out] R			- the result.
 * @param[in] P				- the point to multiply.
 * @param[in] K				- the integer.
 */
#if EP_MUL == BASIC
#define ep_mul(R, P, K)		ep_mul_basic(R, P, K)
#elif EP_MUL == SLIDE
#define ep_mul(R, P, K)		ep_mul_slide(R, P, K)
#elif EP_MUL == MONTY
#define ep_mul(R, P, K)		ep_mul_monty(R, P, K)
#elif EP_MUL == LWNAF
#define ep_mul(R, P, K)		ep_mul_lwnaf(R, P, K)
#endif

/**
 * Builds a precomputation table for multiplying a fixed prime elliptic point.
 *
 * @param[out] T			- the precomputation table.
 * @param[in] P				- the point to multiply.
 */
#if EP_FIX == BASIC
#define ep_mul_pre(T, P)		ep_mul_pre_basic(T, P)
#elif EP_FIX == YAOWI
#define ep_mul_pre(T, P)		ep_mul_pre_yaowi(T, P)
#elif EP_FIX == NAFWI
#define ep_mul_pre(T, P)		ep_mul_pre_nafwi(T, P)
#elif EP_FIX == COMBS
#define ep_mul_pre(T, P)		ep_mul_pre_combs(T, P)
#elif EP_FIX == COMBD
#define ep_mul_pre(T, P)		ep_mul_pre_combd(T, P)
#elif EP_FIX == LWNAF
#define ep_mul_pre(T, P)		ep_mul_pre_lwnaf(T, P)
#endif

/**
 * Multiplies a fixed prime elliptic point using a precomputation table.
 * Computes R = kP.
 *
 * @param[out] R			- the result.
 * @param[in] T				- the precomputation table.
 * @param[in] K				- the integer.
 */
#if EP_FIX == BASIC
#define ep_mul_fix(R, T, K)		ep_mul_fix_basic(R, T, K)
#elif EP_FIX == YAOWI
#define ep_mul_fix(R, T, K)		ep_mul_fix_yaowi(R, T, K)
#elif EP_FIX == NAFWI
#define ep_mul_fix(R, T, K)		ep_mul_fix_nafwi(R, T, K)
#elif EP_FIX == COMBS
#define ep_mul_fix(R, T, K)		ep_mul_fix_combs(R, T, K)
#elif EP_FIX == COMBD
#define ep_mul_fix(R, T, K)		ep_mul_fix_combd(R, T, K)
#elif EP_FIX == LWNAF
#define ep_mul_fix(R, T, K)		ep_mul_fix_lwnaf(R, T, K)
#endif

/**
 * Multiplies and adds two prime elliptic curve points simultaneously. Computes
 * R = kP + mQ.
 *
 * @param[out] R			- the result.
 * @param[in] P				- the first point to multiply.
 * @param[in] K				- the first integer.
 * @param[in] Q				- the second point to multiply.
 * @param[in] M				- the second integer,
 */
#if EP_SIM == BASIC
#define ep_mul_sim(R, P, K, Q, M)	ep_mul_sim_basic(R, P, K, Q, M)
#elif EP_SIM == TRICK
#define ep_mul_sim(R, P, K, Q, M)	ep_mul_sim_trick(R, P, K, Q, M)
#elif EP_SIM == INTER
#define ep_mul_sim(R, P, K, Q, M)	ep_mul_sim_inter(R, P, K, Q, M)
#elif EP_SIM == JOINT
#define ep_mul_sim(R, P, K, Q, M)	ep_mul_sim_joint(R, P, K, Q, M)
#endif

/*============================================================================*/
/* Function prototypes                                                        */
/*============================================================================*/

/**
 * Initializes the prime elliptic curve arithmetic module.
 */
void ep_curve_init(void);

/**
 * Finalizes the prime elliptic curve arithmetic module.
 */
void ep_curve_clean(void);

/**
 * Returns the 'a' coefficient of the currently configured prime elliptic curve.
 *
 * @return the 'a' coefficient of the elliptic curve.
 */
dig_t *ep_curve_get_a(void);

/**
 * Returns the 'b' coefficient of the currently configured prime elliptic curve.
 *
 * @return the 'b' coefficient of the elliptic curve.
 */
dig_t *ep_curve_get_b(void);

/**
 * Returns the efficient endormorphism associated with the prime curve.
 */
dig_t *ep_curve_get_beta(void);

/**
 * Returns the parameter V1 of the prime curve.
 */
void ep_curve_get_v1(bn_t v[]);

/**
 * Returns the parameter V2 of the prime curve.
 */
void ep_curve_get_v2(bn_t v[]);

/**
 * Returns a optimization identifier based on the 'a' coefficient of the curve.
 *
 * @return the optimization identifier.
 */
int ep_curve_opt_a(void);

/**
 * Returns a optimization identifier based on the 'b' coefficient of the curve.
 *
 * @return the optimization identifier.
 */
int ep_curve_opt_b(void);

/**
 * Tests if the configured prime elliptic curve is a Koblitz curve.
 *
 * @return 1 if the prime elliptic curve is a Koblitz curve, 0 otherwise.
 */
int ep_curve_is_endom(void);

/**
 * Tests if the configured prime elliptic curve is supersingular.
 *
 * @return 1 if the prime elliptic curve is supersingular, 0 otherwise.
 */
int ep_curve_is_super(void);

/**
 * Returns the generator of the group of points in the prime elliptic curve.
 *
 * @param[out] g			- the returned generator.
 */
void ep_curve_get_gen(ep_t g);

/**
 * Returns the precomputation table for the generator.
 *
 * @return the table.
 */
const ep_t *ep_curve_get_tab(void);

/**
 * Returns the order of the group of points in the prime elliptic curve.
 *
 * @param[out] r			- the returned order.
 */
void ep_curve_get_ord(bn_t n);

/**
 * Returns the cofactor of the binary elliptic curve.
 *
 * @param[out] n			- the returned cofactor.
 */
void ep_curve_get_cof(bn_t h);

/**
 * Configures a prime elliptic curve without endomorphisms by its coefficients
 * and generator.
 *
 * @param[in] a			- the 'a' coefficient of the curve.
 * @param[in] b			- the 'b' coefficient of the curve.
 * @param[in] g			- the generator.
 * @param[in] r			- the order of the group of points.
 * @param[in] h			- the cofactor of the group order.
 */
void ep_curve_set_plain(const fp_t a, const fp_t b, const ep_t g, const bn_t r,
		const bn_t h);

/**
 * Configures a supersingular prime elliptic curve by its coefficients and
 * generator.
 *
 * @param[in] a			- the 'a' coefficient of the curve.
 * @param[in] b			- the 'b' coefficient of the curve.
 * @param[in] g			- the generator.
 * @param[in] r			- the order of the group of points.
 * @param[in] h			- the cofactor of the group order.
 */
void ep_curve_set_super(const fp_t a, const fp_t b, const ep_t g, const bn_t r,
		const bn_t h);

/**
 * Configures a prime elliptic curve with endomorphisms by its coefficients and
 * generator.
 *
 * @param[in] b			- the 'b' coefficient of the curve.
 * @param[in] g			- the generator.
 * @param[in] r			- the order of the group of points.
 * @param[in] beta		- the constant associated with the endomorphism.
 * @param[in] l			- the exponent corresponding to the endomorphism.
 * @param[in] h			- the cofactor of the group order.
 */
void ep_curve_set_endom(const fp_t b, const ep_t g, const bn_t r, const bn_t h,
		const fp_t beta, const bn_t l);

/**
 * Configures a prime elliptic curve by its parameter identifier.
 *
 * @param				- the parameter identifier.
 */
void ep_param_set(int param);

/**
 * Configures some set of curve parameters for the current security level.
 */
int ep_param_set_any(void);

/**
 * Configures some set of ordinary curve parameters for the current security
 * level.
 *
 * @return STS_OK if there is a curve at this security level, STS_ERR otherwise.
 */
int ep_param_set_any_plain(void);

/**
 * Configures some set of Koblitz curve parameters for the current security
 * level.
 *
 * @return STS_OK if there is a curve at this security level, STS_ERR otherwise.
 */
int ep_param_set_any_endom(void);

/**
 * Configures some set of supersingular curve parameters for the current
 * security level.
 *
 * @return STS_OK if there is a curve at this security level, STS_ERR otherwise.
 */
int ep_param_set_any_super(void);

/**
 * Configures some set of pairing-friendly curve parameters for the current
 * security level.
 *
 * @return STS_OK if there is a curve at this security level, STS_ERR otherwise.
 */
int ep_param_set_any_pairf(void);

/**
 * Returns the parameter identifier of the currently configured prime elliptic
 * curve.
 *
 * @return the parameter identifier.
 */
int ep_param_get(void);

/**
 * Prints the current configured prime elliptic curve.
 */
void ep_param_print(void);

/**
 * Returns the current security level.
 */
int ep_param_level(void);

/**
 * Returns the embedding degree of the currently configured elliptic curve.
 */
int ep_param_embed(void);

/**
 * Tests if a point on a prime elliptic curve is at the infinity.
 *
 * @param[in] p				- the point to test.
 * @return 1 if the point is at infinity, 0 otherise.
 */
int ep_is_infty(const ep_t p);

/**
 * Assigns a prime elliptic curve point to a point at the infinity.
 *
 * @param[out] p			- the point to assign.
 */
void ep_set_infty(ep_t p);

/**
 * Copies the second argument to the first argument.
 *
 * @param[out] q			- the result.
 * @param[in] p				- the prime elliptic curve point to copy.
 */
void ep_copy(ep_t r, const ep_t p);

/**
 * Compares two prime elliptic curve points.
 *
 * @param[in] p				- the first prime elliptic curve point.
 * @param[in] q				- the second prime elliptic curve point.
 * @return CMP_EQ if p == q and CMP_NE if p != q.
 */
int ep_cmp(const ep_t p, const ep_t q);

/**
 * Assigns a random value to a prime elliptic curve point.
 *
 * @param[out] p			- the prime elliptic curve point to assign.
 */
void ep_rand(ep_t p);

/**
 * Computes the right-hand side of the elliptic curve equation at a certain
 * elliptic curve point.
 *
 * @param[out] rhs			- the result.
 * @param[in] p				- the point.
 */
void ep_rhs(fp_t rhs, const ep_t p);

/**
 * Tests if a point is in the curve.
 *
 * @param[in] p				- the point to test.
 */
int ep_is_valid(const ep_t p);

/**
 * Builds a precomputation table for multiplying a random prime elliptic point.
 *
 * @param[out] t			- the precomputation table.
 * @param[in] p				- the point to multiply.
 * @param[in] w				- the window width.
 */
void ep_tab(ep_t *t, const ep_t p, int w);

/**
 * Prints a prime elliptic curve point.
 *
 * @param[in] p				- the prime elliptic curve point to print.
 */
void ep_print(const ep_t p);

/**
 * Returns the number of bytes necessary to store a prime elliptic curve point
 * with optional point compression.
 *
 * @param[in] a				- the prime field element.
 * @param[in] pack			- the flag to indicate compression.
 * @return the number of bytes.
 */
int ep_size_bin(const ep_t a, int pack);

/**
 * Reads a prime elliptic curve point from a byte vector in big-endian format.
 *
 * @param[out] a			- the result.
 * @param[in] bin			- the byte vector.
 * @param[in] len			- the buffer capacity.
 * @throw ERR_NO_VALID		- if the encoded point is invalid.
 * @throw ERR_NO_BUFFER		- if the buffer capacity is invalid.
 */
void ep_read_bin(ep_t a, const uint8_t *bin, int len);

/**
 * Writes a prime elliptic curve point to a byte vector in big-endian format
 * with optional point compression.
 *
 * @param[out] bin			- the byte vector.
 * @param[in] len			- the buffer capacity.
 * @param[in] a				- the prime elliptic curve point to write.
 * @param[in] pack			- the flag to indicate point compression.
 * @throw ERR_NO_BUFFER		- if the buffer capacity is invalid.
 */
void ep_write_bin(uint8_t *bin, int len, const ep_t a, int pack);

/**
 * Negates a prime elliptic curve point represented by affine coordinates.
 *
 * @param[out] r			- the result.
 * @param[in] p				- the point to negate.
 */
void ep_neg_basic(ep_t r, const ep_t p);

/**
 * Negates a prime elliptic curve point represented by projective coordinates.
 *
 * @param[out] r			- the result.
 * @param[in] p				- the point to negate.
 */
void ep_neg_projc(ep_t r, const ep_t p);

/**
 * Adds two prime elliptic curve points represented in affine coordinates.
 *
 * @param[out] r			- the result.
 * @param[in] p				- the first point to add.
 * @param[in] q				- the second point to add.
 */
void ep_add_basic(ep_t r, const ep_t p, const ep_t q);

/**
 * Adds two prime elliptic curve points represented in affine coordinates and
 * returns the computed slope.
 *
 * @param[out] r			- the result.
 * @param[out] s			- the slope.
 * @param[in] p				- the first point to add.
 * @param[in] q				- the second point to add.
 */
void ep_add_slp_basic(ep_t r, fp_t s, const ep_t p, const ep_t q);

/**
 * Adds two prime elliptic curve points represented in projective coordinates.
 *
 * @param[out] r			- the result.
 * @param[in] p				- the first point to add.
 * @param[in] q				- the second point to add.
 */
void ep_add_projc(ep_t r, const ep_t p, const ep_t q);

/**
 * Subtracts a prime elliptic curve point from another, both points represented
 * by affine coordinates..
 *
 * @param[out] r			- the result.
 * @param[in] p				- the first point.
 * @param[in] q				- the second point.
 */
void ep_sub_basic(ep_t r, const ep_t p, const ep_t q);

/**
 * Subtracts a prime elliptic curve point from another, both points represented
 * by projective coordinates..
 *
 * @param[out] r			- the result.
 * @param[in] p				- the first point.
 * @param[in] q				- the second point.
 */
void ep_sub_projc(ep_t r, const ep_t p, const ep_t q);

/**
 * Doubles a prime elliptic curve point represented in affine coordinates.
 *
 * @param[out] r			- the result.
 * @param[in] p				- the point to double.
 */
void ep_dbl_basic(ep_t r, const ep_t p);

/**
 * Doubles a prime elliptic curve point represented in affine coordinates and
 * returns the computed slope.
 *
 * @param[out] r			- the result.
 * @param[out] s			- the slope.
 * @param[in] p				- the point to double.
 */
void ep_dbl_slp_basic(ep_t r, fp_t s, const ep_t p);

/**
 * Doubles a prime elliptic curve point represented in projective coordinates.
 *
 * @param[out] r			- the result.
 * @param[in] p				- the point to double.
 */
void ep_dbl_projc(ep_t r, const ep_t p);

/**
 * Multiplies a prime elliptic point by an integer using the binary method.
 *
 * @param[out] r			- the result.
 * @param[in] p				- the point to multiply.
 * @param[in] k				- the integer.
 */
void ep_mul_basic(ep_t r, const ep_t p, const bn_t k);

/**
 * Multiplies a prime elliptic point by an integer using the sliding window
 * method.
 *
 * @param[out] r			- the result.
 * @param[in] p				- the point to multiply.
 * @param[in] k				- the integer.
 */
void ep_mul_slide(ep_t r, const ep_t p, const bn_t k);

/**
 * Multiplies a prime elliptic point by an integer using the constant-time
 * Montgomery laddering point multiplication method.
 *
 * @param[out] r			- the result.
 * @param[in] p				- the point to multiply.
 * @param[in] k				- the integer.
 */
void ep_mul_monty(ep_t r, const ep_t p, const bn_t k);

/**
 * Multiplies a prime elliptic point by an integer using the w-NAF method.
 *
 * @param[out] r			- the result.
 * @param[in] p				- the point to multiply.
 * @param[in] k				- the integer.
 */
void ep_mul_lwnaf(ep_t r, const ep_t p, const bn_t k);

/**
 * Multiplies a prime elliptic point by an integer using a regular method.
 *
 * @param[out] r			- the result.
 * @param[in] p				- the point to multiply.
 * @param[in] k				- the integer.
 */
void ep_mul_lwreg(ep_t r, const ep_t p, const bn_t k);

/**
 * Multiplies the generator of a prime elliptic curve by an integer.
 *
 * @param[out] r			- the result.
 * @param[in] k				- the integer.
 */
void ep_mul_gen(ep_t r, const bn_t k);

/**
 * Multiplies a prime elliptic point by a small integer.
 *
 * @param[out] r			- the result.
 * @param[in] p				- the point to multiply.
 * @param[in] k				- the integer.
 */
void ep_mul_dig(ep_t r, const ep_t p, dig_t k);

/**
 * Builds a precomputation table for multiplying a fixed prime elliptic point
 * using the binary method.
 *
 * @param[out] t			- the precomputation table.
 * @param[in] p				- the point to multiply.
 */
void ep_mul_pre_basic(ep_t *t, const ep_t p);

/**
 * Builds a precomputation table for multiplying a fixed prime elliptic point
 * using Yao's windowing method.
 *
 * @param[out] t			- the precomputation table.
 * @param[in] p				- the point to multiply.
 */
void ep_mul_pre_yaowi(ep_t *t, const ep_t p);

/**
 * Builds a precomputation table for multiplying a fixed prime elliptic point
 * using the NAF windowing method.
 *
 * @param[out] t			- the precomputation table.
 * @param[in] p				- the point to multiply.
 */
void ep_mul_pre_nafwi(ep_t *t, const ep_t p);

/**
 * Builds a precomputation table for multiplying a fixed prime elliptic point
 * using the single-table comb method.
 *
 * @param[out] t			- the precomputation table.
 * @param[in] p				- the point to multiply.
 */
void ep_mul_pre_combs(ep_t *t, const ep_t p);

/**
 * Builds a precomputation table for multiplying a fixed prime elliptic point
 * using the double-table comb method.
 *
 * @param[out] t			- the precomputation table.
 * @param[in] p				- the point to multiply.
 */
void ep_mul_pre_combd(ep_t *t, const ep_t p);

/**
 * Builds a precomputation table for multiplying a fixed prime elliptic point
 * using the w-(T)NAF method.
 *
 * @param[out] t			- the precomputation table.
 * @param[in] p				- the point to multiply.
 */
void ep_mul_pre_lwnaf(ep_t *t, const ep_t p);

/**
 * Multiplies a fixed prime elliptic point using a precomputation table and
 * the binary method.
 *
 * @param[out] r			- the result.
 * @param[in] t				- the precomputation table.
 * @param[in] k				- the integer.
 */
void ep_mul_fix_basic(ep_t r, const ep_t *t, const bn_t k);

/**
 * Multiplies a fixed prime elliptic point using a precomputation table and
 * Yao's windowing method
 *
 * @param[out] r			- the result.
 * @param[in] t				- the precomputation table.
 * @param[in] k				- the integer.
 */
void ep_mul_fix_yaowi(ep_t r, const ep_t *t, const bn_t k);

/**
 * Multiplies a fixed prime elliptic point using a precomputation table and
 * the w-(T)NAF method.
 *
 * @param[out] r			- the result.
 * @param[in] t				- the precomputation table.
 * @param[in] k				- the integer.
 */
void ep_mul_fix_nafwi(ep_t r, const ep_t *t, const bn_t k);

/**
 * Multiplies a fixed prime elliptic point using a precomputation table and
 * the single-table comb method.
 *
 * @param[out] r			- the result.
 * @param[in] t				- the precomputation table.
 * @param[in] k				- the integer.
 */
void ep_mul_fix_combs(ep_t r, const ep_t *t, const bn_t k);

/**
 * Multiplies a fixed prime elliptic point using a precomputation table and
 * the double-table comb method.
 *
 * @param[out] r			- the result.
 * @param[in] t				- the precomputation table.
 * @param[in] k				- the integer.
 */
void ep_mul_fix_combd(ep_t r, const ep_t *t, const bn_t k);

/**
 * Multiplies a fixed prime elliptic point using a precomputation table and
 * the w-(T)NAF method.
 *
 * @param[out] r			- the result.
 * @param[in] t				- the precomputation table.
 * @param[in] k				- the integer.
 */
void ep_mul_fix_lwnaf(ep_t r, const ep_t *t, const bn_t k);

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
void ep_mul_sim_basic(ep_t r, const ep_t p, const bn_t k, const ep_t q,
		const bn_t m);

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
void ep_mul_sim_trick(ep_t r, const ep_t p, const bn_t k, const ep_t q,
		const bn_t m);

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
void ep_mul_sim_inter(ep_t r, const ep_t p, const bn_t k, const ep_t q,
		const bn_t m);

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
void ep_mul_sim_joint(ep_t r, const ep_t p, const bn_t k, const ep_t q,
		const bn_t m);

/**
 * Multiplies and adds the generator and a prime elliptic curve point
 * simultaneously. Computes R = kG + mQ.
 *
 * @param[out] r			- the result.
 * @param[in] k				- the first integer.
 * @param[in] q				- the second point to multiply.
 * @param[in] m				- the second integer.
 */
void ep_mul_sim_gen(ep_t r, const bn_t k, const ep_t q, const bn_t m);

/**
 * Converts a point to affine coordinates.
 *
 * @param[out] r			- the result.
 * @param[in] p				- the point to convert.
 */
void ep_norm(ep_t r, const ep_t p);

/**
 * Converts multiple points to affine coordinates.
 *
 * @param[out] r			- the result.
 * @param[in] t				- the points to convert.
 * @param[in] n				- the number of points.
 */
void ep_norm_sim(ep_t *r, const ep_t *t, int n);

/**
 * Maps a byte array to a point in a prime elliptic curve. The
 * algorithm implemented is the Fouque-Tibouchi algorithm from the
 * paper "Indifferentiable Hashing to Barreto-Naehrig curves" for
 * the BLS12-381 curve.
 *
 * @param[out] p			- the result.
 * @param[in] msg			- the byte array to map.
 * @param[in] len			- the array length in bytes.
 */
void ep_map(ep_t p, const uint8_t *msg, int len);

/**
 * Compresses a point.
 *
 * @param[out] r			- the result.
 * @param[in] p				- the point to compress.
 */
void ep_pck(ep_t r, const ep_t p);

/**
 * Decompresses a point.
 *
 * @param[out] r			- the result.
 * @param[in] p				- the point to decompress.
 * @return a boolean value indicating if the decompression was successful.
 */
int ep_upk(ep_t r, const ep_t p);

#endif /* !RELIC_EP_H */
