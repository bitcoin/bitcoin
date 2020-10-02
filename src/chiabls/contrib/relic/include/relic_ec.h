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
 * @defgroup ec Elliptic curve cryptography
 */

/**
 * @file
 *
 * Abstractions of elliptic curve arithmetic useful to protocol implementors.
 *
 * @ingroup ec
 */

#ifndef RELIC_EC_H
#define RELIC_EC_H

#include "relic_ep.h"
#include "relic_eb.h"
#include "relic_ed.h"
#include "relic_bn.h"
#include "relic_util.h"
#include "relic_conf.h"
#include "relic_types.h"

/*============================================================================*/
/* Constant definitions                                                       */
/*============================================================================*/

/**
 * Prefix for mappings of functions.
 */
#if EC_CUR == PRIME
#define EC_LOWER			ep_
#elif EC_CUR == CHAR2
#define EC_LOWER			eb_
#elif EC_CUR == EDWARD
#define EC_LOWER      		ed_
#endif

/**
 * Prefix for mappings of constant definitions.
 */
#if EC_CUR == PRIME
#define EC_UPPER			EP_
#elif EC_CUR == CHAR2
#define EC_UPPER			EB_
#elif EC_CUR == EDWARD
#define EC_UPPER      		ED_
#endif

/**
 * Size of a precomputation table using the chosen algorithm.
 */
#if EC_CUR == PRIME
#define RELIC_EC_TABLE			RELIC_EP_TABLE
#elif EC_CUR == CHAR2
#define RELIC_EC_TABLE			RELIC_EB_TABLE
#elif EC_CUR == EDWARD
#define RELIC_EC_TABLE			RELIC_ED_TABLE
#endif

/**
 * Size of a field element in words.
 */
#if EC_CUR == PRIME
#define FC_DIGS				FP_DIGS
#elif EC_CUR == CHAR2
#define FC_DIGS				FB_DIGS
#elif EC_CUR == EDWARD
#define FC_DIGS				FP_DIGS
#endif

/**
 * Size of a field element in bits.
 */
#if EC_CUR == PRIME
#define FC_BITS				FP_BITS
#elif EC_CUR == CHAR2
#define FC_BITS				FB_BITS
#elif EC_CUR == EDWARD
#define FC_BITS				FP_BITS
#endif

/**
 * Size of a field element in bytes.
 */
#if EC_CUR == PRIME
#define FC_BYTES			FP_BYTES
#elif EC_CUR == CHAR2
#define FC_BYTES			FB_BYTES
#elif EC_CUR == EDWARD
#define FC_BYTES 			FP_BYTES
#endif

/*============================================================================*/
/* Type definitions                                                           */
/*============================================================================*/

/**
 * Represents an elliptic curve point.
 */
typedef CAT(EC_LOWER, t) ec_t;

/*============================================================================*/
/* Macro definitions                                                          */
/*============================================================================*/

/**
 * Initializes a point on an elliptic curve with a null value.
 *
 * @param[out] A			- the point to initialize.
 */
#define ec_null(A)			CAT(EC_LOWER, null)(A)

/**
 * Calls a function to allocate a point on an elliptic curve.
 *
 * @param[out] A			- the new point.
 * @throw ERR_NO_MEMORY		- if there is no available memory.
 */
#define ec_new(A)			CAT(EC_LOWER, new)(A)

/**
 * Calls a function to clean and free a point on an elliptic curve.
 *
 * @param[out] A			- the point to clean and free.
 */
#define ec_free(A)			CAT(EC_LOWER, free)(A)

/**
 * Returns the generator of the group of points in the elliptic curve.
 *
 * @param[out] G			- the returned generator.
 */
#define ec_curve_get_gen(G)	CAT(EC_LOWER, curve_get_gen)(G)

/**
 * Returns the precomputation table for the generator.
 *
 * @return the table.
 */
#define ec_curve_get_tab()	CAT(EC_LOWER, curve_get_tab)()

/**
 * Returns the order of the group of points in the elliptic curve.
 *
 * @param[out]	N			- the returned order.
 */
#define ec_curve_get_ord(N)	CAT(EC_LOWER, curve_get_ord)(N)

/**
 * Returns the cofactor of the group of points in the elliptic curve.
 *
 * @param[out]	H			- the returned order.
 */
#define ec_curve_get_cof(H) CAT(EC_LOWER, curve_get_cof)(H)

/**
 * Configures some set of curve parameters for the current security level.
 */
#if EC_CUR == PRIME
#if defined(EC_ENDOM)
#define ec_param_set_any()	ep_param_set_any_endom()
#else
#define ec_param_set_any()	ep_param_set_any()
#endif
#elif EC_CUR == CHAR2
#if defined(EC_ENDOM)
#define ec_param_set_any()	eb_param_set_any_kbltz()
#else
#define ec_param_set_any()	eb_param_set_any()
#endif
#elif EC_CUR == EDWARD
#define ec_param_set_any()  ed_param_set_any()
#endif

/**
 * Prints the current configured elliptic curve.
 */
#define ec_param_print()	CAT(EC_LOWER, param_print)()

 /**
  * Returns the current configured elliptic curve.
  */
#define ec_param_get()		CAT(EC_LOWER, param_get)()

/**
 * Returns the current security level.
 */
#define ec_param_level()	CAT(EC_LOWER, param_level)()

/**
 * Tests if a point on a elliptic curve is at the infinity.
 *
 * @param[in] P				- the point to test.
 * @return 1 if the point is at infinity, 0 otherwise.
 */
#define ec_is_infty(P)		CAT(EC_LOWER, is_infty)(P)

/**
 * Assigns a elliptic curve point to a point at the infinity.
 *
 * @param[out] P			- the point to assign.
 */
#define ec_set_infty(P)		CAT(EC_LOWER, set_infty)(P)

/**
 * Copies the second argument to the first argument.
 *
 * @param[out] R			- the result.
 * @param[in] P				- the elliptic curve point to copy.
 */
#define ec_copy(R, P)		CAT(EC_LOWER, copy)(R, P)

/**
 * Compares two elliptic curve points.
 *
 * @param[in] P				- the first elliptic curve point.
 * @param[in] Q				- the second elliptic curve point.
 * @return CMP_EQ if P == Q and CMP_NE if P != Q.
 */
#define ec_cmp(P, Q)		CAT(EC_LOWER, cmp)(P, Q)

/**
 * Assigns a random value to a elliptic curve point.
 *
 * @param[out] P			- the elliptic curve point to assign.
 */
#define ec_rand(P)			CAT(EC_LOWER, rand)(P)

/** Tests if a point is in the curve.
 *
 * @param[in] P			- the point to test.
 */
#define ec_is_valid(P)		CAT(EC_LOWER, is_valid)(P)

/**
 * Returns the number of bytes necessary to store an elliptic curve point with
 * optional point compression.
 *
 * @param[in] A				- the elliptic curve point.
 * @param[in] P				- the flag to indicate compression.
 */
#define ec_size_bin(A, P)	CAT(EC_LOWER, size_bin)(A, P)

/**
 * Reads an elliptic curve point from a byte vector in big-endian format.
 *
 * @param[out] A			- the result.
 * @param[in] B				- the byte vector.
 * @param[in] L				- the buffer capacity.
 */
#define ec_read_bin(A, B, L)	CAT(EC_LOWER, read_bin)(A, B, L)

/**
 * Writes an elliptic curve point to a byte vector in big-endian format with
 * optional point compression.
 *
 * @param[out] B			- the byte vector.
 * @param[in] L				- the buffer capacity.
 * @param[in] A				- the prime elliptic curve point to write.
 * @param[in] P				- the flag to indicate point compression.
 */
#define ec_write_bin(B, L, A, P)	CAT(EC_LOWER, write_bin)(B, L, A, P)

/**
 * Prints a elliptic curve point.
 *
 * @param[in] P				- the elliptic curve point to print.
 */
#define ec_print(P)			CAT(EC_LOWER, print)(P)

/**
 * Negates an elliptic curve point. Computes R = -P.
 *
 * @param[out] R			- the result.
 * @param[in] P				- the point to negate.
 */
#define ec_neg(R, P)		CAT(EC_LOWER, neg)(R, P)

/**
 * Adds two elliptic curve points. Computes R = P + Q.
 *
 * @param[out] R			- the result.
 * @param[in] P				- the first point to add.
 * @param[in] Q				- the second point to add.
 */
#define ec_add(R, P, Q)		CAT(EC_LOWER, add)(R, P, Q)

/**
 * Subtracts an elliptic curve point from another. Computes R = P - Q.
 *
 * @param[out] R			- the result.
 * @param[in] P				- the first point.
 * @param[in] Q				- the second point.
 */
#define ec_sub(R, P, Q)		CAT(EC_LOWER, sub)(R, P, Q)

/**
 * Doubles an elliptic curve point. Computes R = 2P.
 *
 * @param[out] R			- the result.
 * @param[in] P				- the point to double.
 */
#define ec_dbl(R, P)		CAT(EC_LOWER, dbl)(R, P)

/**
 * Multiplies an elliptic curve point by an integer. Computes R = kP.
 *
 * @param[out] R			- the result.
 * @param[in] P				- the point to multiply.
 * @param[in] K				- the integer.
 */
#define ec_mul(R, P, K)		CAT(EC_LOWER, mul)(R, P, K)

/**
 * Multiplies the generator of a prime elliptic curve by an integer.
 *
 * @param[out] R			- the result.
 * @param[in] K				- the integer.
 */
#define ec_mul_gen(R, K)	CAT(EC_LOWER, mul_gen)(R, K)

/**
 * Multiplies an elliptic curve point by a small integer. Computes R = kP.
 *
 * @param[out] R			- the result.
 * @param[in] P				- the point to multiply.
 * @param[in] K				- the integer.
 */
#define ec_mul_dig(R, P, K)		CAT(EC_LOWER, mul_dig)(R, P, K)

/**
 * Builds a precomputation table for multiplying a fixed elliptic curve
 * point.
 *
 * @param[out] T			- the precomputation table.
 * @param[in] P				- the point to multiply.
 */
#define ec_mul_pre(T, P)	CAT(EC_LOWER, mul_pre)(T, P)
/**
 * Multiplies a elliptic point using a precomputation table.
 * Computes R = kP.
 *
 * @param[out] R			- the result.
 * @param[in] T				- the precomputation table.
 * @param[in] K				- the integer.
 */
#define ec_mul_fix(R, T, K)	CAT(EC_LOWER, mul_fix)(R, T, K)

/**
 * Multiplies and adds two elliptic curve points simultaneously. Computes
 * R = kP + lQ.
 *
 * @param[out] R			- the result.
 * @param[in] P				- the first point to multiply.
 * @param[in] K				- the first integer.
 * @param[in] Q				- the second point to multiply.
 * @param[in] L				- the second integer,
 */
#define ec_mul_sim(R, P, K, Q, L)	CAT(EC_LOWER, mul_sim)(R, P, K, Q, L)

/**
 * Multiplies and adds two elliptic curve points simultaneously. Computes
 * R = kG + lQ.
 *
 * @param[out] R			- the result.
 * @param[in] K				- the first integer.
 * @param[in] Q				- the second point to multiply.
 * @param[in] L				- the second integer,
 */
#define ec_mul_sim_gen(R, K, Q, L)	CAT(EC_LOWER, mul_sim_gen)(R, K, Q, L)

/**
 * Converts a point to affine coordinates.
 *
 * @param[out] R			- the result.
 * @param[in] P				- the point to convert.
 */
#define ec_norm(R, P)		CAT(EC_LOWER, norm)(R, P)

/**
 * Maps a byte array to a point in an elliptic curve.
 *
 * @param[out] P			- the result.
 * @param[in] M				- the byte array to map.
 * @param[in] L				- the array length in bytes.
 */
#define ec_map(P, M, L)		CAT(EC_LOWER, map)(P, M, L)

/**
 * Compresses a point.
 *
 * @param[out] R			- the result.
 * @param[in] P				- the point to compress.
 */
#define ec_pck(R, P)		CAT(EC_LOWER, pck)(R, P)

/**
 * Decompresses a point.
 *
 * @param[out] R			- the result.
 * @param[in] P				- the point to decompress.
 */
#define ec_upk(R, P)		CAT(EC_LOWER, upk)(R, P)

/**
 * Returns the x-coordinate of an elliptic curve point represented as a
 * multiple precision integer.
 *
 * @param[out] X			- the x-coordinate.
 * @param[in] P				- the point to read.
 */
#if EC_CUR == PRIME
#define ec_get_x(X, P)		fp_prime_back(X, P->x)
#else
#define ec_get_x(X, P)		bn_read_raw(X, P->x, FC_DIGS)
#endif

/**
* Returns the y-coordinate of an elliptic curve point represented as a
* multiple precision integer.
*
* @param[out] Y				- the y-coordinate.
* @param[in] P				- the point to read.
*/
#if EC_CUR == PRIME
#define ec_get_y(Y, P)		fp_prime_back(Y, (P)->y)
#else
#define ec_get_y(Y, P)		bn_read_raw(Y, (P)->y, FC_DIGS)
#endif

#endif /* !RELIC_EC_H */
