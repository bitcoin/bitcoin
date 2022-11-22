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
 * @defgroup ec Elliptic curve cryptography
 */

/**
 * @file
 *
 * Abstractions of elliptic curve arithmetic useful to protocol implementors.
 *
 * @ingroup ec
 */

#ifndef RLC_EC_H
#define RLC_EC_H

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
#define RLC_EC_LOWER			ep_
#elif EC_CUR == CHAR2
#define RLC_EC_LOWER			eb_
#elif EC_CUR == EDDIE
#define RLC_EC_LOWER      		ed_
#endif

/**
 * Prefix for mappings of constant definitions.
 */
#if EC_CUR == PRIME
#define RLC_EC_UPPER			EP_
#elif EC_CUR == CHAR2
#define RLC_EC_UPPER			EB_
#elif EC_CUR == EDDIE
#define RLC_EC_UPPER      		ED_
#endif

/**
 * Size of a precomputation table using the chosen algorithm.
 */
#if EC_CUR == PRIME
#define RLC_EC_TABLE			RLC_EP_TABLE
#elif EC_CUR == CHAR2
#define RLC_EC_TABLE			RLC_EB_TABLE
#elif EC_CUR == EDDIE
#define RLC_EC_TABLE			RLC_ED_TABLE
#endif

/**
 * Size of a field element in words.
 */
#if EC_CUR == PRIME
#define RLC_FC_DIGS				RLC_FP_DIGS
#elif EC_CUR == CHAR2
#define RLC_FC_DIGS				RLC_FB_DIGS
#elif EC_CUR == EDDIE
#define RLC_FC_DIGS				RLC_FP_DIGS
#endif

/**
 * Size of a field element in bits.
 */
#if EC_CUR == PRIME
#define RLC_FC_BITS				RLC_FP_BITS
#elif EC_CUR == CHAR2
#define RLC_FC_BITS				RLC_FB_BITS
#elif EC_CUR == EDDIE
#define RLC_FC_BITS				RLC_FP_BITS
#endif

/**
 * Size of a field element in bytes.
 */
#if EC_CUR == PRIME
#define RLC_FC_BYTES			RLC_FP_BYTES
#elif EC_CUR == CHAR2
#define RLC_FC_BYTES			RLC_FB_BYTES
#elif EC_CUR == EDDIE
#define RLC_FC_BYTES			RLC_FP_BYTES
#endif

/*============================================================================*/
/* Type definitions                                                           */
/*============================================================================*/

/**
 * Represents an elliptic curve point.
 */
typedef RLC_CAT(RLC_EC_LOWER, t) ec_t;

/*============================================================================*/
/* Macro definitions                                                          */
/*============================================================================*/

/**
 * Initializes a point on an elliptic curve with a null value.
 *
 * @param[out] A				- the point to initialize.
 */
#define ec_null(A)				RLC_CAT(RLC_EC_LOWER, null)(A)

/**
 * Calls a function to allocate a point on an elliptic curve.
 *
 * @param[out] A				- the new point.
 * @throw ERR_NO_MEMORY			- if there is no available memory.
 */
#define ec_new(A)				RLC_CAT(RLC_EC_LOWER, new)(A)

/**
 * Calls a function to clean and free a point on an elliptic curve.
 *
 * @param[out] A				- the point to clean and free.
 */
#define ec_free(A)				RLC_CAT(RLC_EC_LOWER, free)(A)

/**
 * Returns the generator of the group of points in the elliptic curve.
 *
 * @param[out] G				- the returned generator.
 */
#define ec_curve_get_gen(G)		RLC_CAT(RLC_EC_LOWER, curve_get_gen)(G)

/**
 * Returns the precomputation table for the generator.
 *
 * @return the table.
 */
#define ec_curve_get_tab()		RLC_CAT(RLC_EC_LOWER, curve_get_tab)()

/**
 * Returns the order of the group of points in the elliptic curve.
 *
 * @param[out]	N			- the returned order.
 */
#define ec_curve_get_ord(N)		RLC_CAT(RLC_EC_LOWER, curve_get_ord)(N)

/**
 * Returns the cofactor of the group of points in the elliptic curve.
 *
 * @param[out]	H			- the returned order.
 */
#define ec_curve_get_cof(H) 	RLC_CAT(RLC_EC_LOWER, curve_get_cof)(H)

/**
 * Configures some set of curve parameters for the current security level.
 */
#if EC_CUR == PRIME
#if defined(EC_ENDOM)
#define ec_param_set_any()		ep_param_set_any_endom()
#else
#define ec_param_set_any()		ep_param_set_any()
#endif
#elif EC_CUR == CHAR2
#if defined(EC_ENDOM)
#define ec_param_set_any()		eb_param_set_any_kbltz()
#else
#define ec_param_set_any()		eb_param_set_any()
#endif
#elif EC_CUR == EDDIE
#define ec_param_set_any()  	ed_param_set_any()
#endif

/**
 * Prints the current configured elliptic curve.
 */
#define ec_param_print()		RLC_CAT(RLC_EC_LOWER, param_print)()

 /**
  * Returns the current configured elliptic curve.
  */
#define ec_param_get()			RLC_CAT(RLC_EC_LOWER, param_get)()

/**
 * Returns the current security level.
 */
#define ec_param_level()		RLC_CAT(RLC_EC_LOWER, param_level)()

/**
 * Tests if a point on an elliptic curve is at the infinity.
 *
 * @param[in] P					- the point to test.
 * @return 1 if the point is at infinity, 0 otherwise.
 */
#define ec_is_infty(P)			RLC_CAT(RLC_EC_LOWER, is_infty)(P)

/**
 * Assigns an elliptic curve point to the point at infinity.
 *
 * @param[out] P				- the point to assign.
 */
#define ec_set_infty(P)			RLC_CAT(RLC_EC_LOWER, set_infty)(P)

/**
 * Copies the second argument to the first argument.
 *
 * @param[out] R				- the result.
 * @param[in] P					- the elliptic curve point to copy.
 */
#define ec_copy(R, P)			RLC_CAT(RLC_EC_LOWER, copy)(R, P)

/**
 * Compares two elliptic curve points.
 *
 * @param[in] P					- the first elliptic curve point.
 * @param[in] Q					- the second elliptic curve point.
 * @return RLC_EQ if P == Q and RLC_NE if P != Q.
 */
#define ec_cmp(P, Q)			RLC_CAT(RLC_EC_LOWER, cmp)(P, Q)

/**
 * Assigns a random value to an elliptic curve point.
 *
 * @param[out] P				- the elliptic curve point to assign.
 */
#define ec_rand(P)				RLC_CAT(RLC_EC_LOWER, rand)(P)

/**
 * Randomizes coordinates of an elliptic curve point.
 *
 * @param[out] R			- the blinded elliptic curve point.
 * @param[in] P				- the elliptic curve point to blind.
 */
#define ec_blind(R, P)				RLC_CAT(RLC_EC_LOWER, blind)(R, P)

/** Tests if a point is on the curve.
 *
 * @param[in] P					- the point to test.
 */
#define ec_on_curve(P)			RLC_CAT(RLC_EC_LOWER, on_curve)(P)

/**
 * Returns the number of bytes necessary to store an elliptic curve point with
 * optional point compression.
 *
 * @param[in] A					- the elliptic curve point.
 * @param[in] P					- the flag to indicate compression.
 */
#define ec_size_bin(A, P)		RLC_CAT(RLC_EC_LOWER, size_bin)(A, P)

/**
 * Reads an elliptic curve point from a byte vector in big-endian format.
 *
 * @param[out] A				- the result.
 * @param[in] B					- the byte vector.
 * @param[in] L					- the buffer capacity.
 */
#define ec_read_bin(A, B, L)	RLC_CAT(RLC_EC_LOWER, read_bin)(A, B, L)

/**
 * Writes an elliptic curve point to a byte vector in big-endian format with
 * optional point compression.
 *
 * @param[out] B				- the byte vector.
 * @param[in] L					- the buffer capacity.
 * @param[in] A					- the prime elliptic curve point to write.
 * @param[in] P					- the flag to indicate point compression.
 */
#define ec_write_bin(B, L, A, P)	RLC_CAT(RLC_EC_LOWER, write_bin)(B, L, A, P)

/**
 * Prints an elliptic curve point.
 *
 * @param[in] P					- the elliptic curve point to print.
 */
#define ec_print(P)				RLC_CAT(RLC_EC_LOWER, print)(P)

/**
 * Negates an elliptic curve point. Computes R = -P.
 *
 * @param[out] R				- the result.
 * @param[in] P					- the point to negate.
 */
#define ec_neg(R, P)			RLC_CAT(RLC_EC_LOWER, neg)(R, P)

/**
 * Adds two elliptic curve points. Computes R = P + Q.
 *
 * @param[out] R				- the result.
 * @param[in] P					- the first point to add.
 * @param[in] Q					- the second point to add.
 */
#define ec_add(R, P, Q)			RLC_CAT(RLC_EC_LOWER, add)(R, P, Q)

/**
 * Subtracts an elliptic curve point from another. Computes R = P - Q.
 *
 * @param[out] R				- the result.
 * @param[in] P					- the first point.
 * @param[in] Q					- the second point.
 */
#define ec_sub(R, P, Q)			RLC_CAT(RLC_EC_LOWER, sub)(R, P, Q)

/**
 * Doubles an elliptic curve point. Computes R = 2P.
 *
 * @param[out] R				- the result.
 * @param[in] P					- the point to double.
 */
#define ec_dbl(R, P)			RLC_CAT(RLC_EC_LOWER, dbl)(R, P)

/**
 * Multiplies an elliptic curve point by an integer. Computes R = [k]P.
 *
 * @param[out] R				- the result.
 * @param[in] P					- the point to multiply.
 * @param[in] K					- the integer.
 */
#define ec_mul(R, P, K)			RLC_CAT(RLC_EC_LOWER, mul)(R, P, K)

/**
 * Multiplies the generator of a prime elliptic curve by an integer.
 *
 * @param[out] R				- the result.
 * @param[in] K					- the integer.
 */
#define ec_mul_gen(R, K)		RLC_CAT(RLC_EC_LOWER, mul_gen)(R, K)

/**
 * Multiplies an elliptic curve point by a small integer. Computes R = [k]P.
 *
 * @param[out] R				- the result.
 * @param[in] P					- the point to multiply.
 * @param[in] K					- the integer.
 */
#define ec_mul_dig(R, P, K)		RLC_CAT(RLC_EC_LOWER, mul_dig)(R, P, K)

/**
 * Builds a precomputation table for multiplying a fixed elliptic curve
 * point.
 *
 * @param[out] T				- the precomputation table.
 * @param[in] P					- the point to multiply.
 */
#define ec_mul_pre(T, P)		RLC_CAT(RLC_EC_LOWER, mul_pre)(T, P)
/**
 * Multiplies an elliptic point using a precomputation table.
 * Computes R = [k]P.
 *
 * @param[out] R				- the result.
 * @param[in] T					- the precomputation table.
 * @param[in] K					- the integer.
 */
#define ec_mul_fix(R, T, K)		RLC_CAT(RLC_EC_LOWER, mul_fix)(R, T, K)

/**
 * Multiplies and adds two elliptic curve points simultaneously. Computes
 * R = [k]P + [l]Q.
 *
 * @param[out] R				- the result.
 * @param[in] P					- the first point to multiply.
 * @param[in] K					- the first integer.
 * @param[in] Q					- the second point to multiply.
 * @param[in] L					- the second integer,
 */
#define ec_mul_sim(R, P, K, Q, L)	RLC_CAT(RLC_EC_LOWER, mul_sim)(R, P, K, Q, L)

/**
 * Multiplies and adds two elliptic curve points simultaneously. Computes
 * R = [k]G + [l]Q.
 *
 * @param[out] R				- the result.
 * @param[in] K					- the first integer.
 * @param[in] Q					- the second point to multiply.
 * @param[in] L					- the second integer,
 */
#define ec_mul_sim_gen(R, K, Q, L)	RLC_CAT(RLC_EC_LOWER, mul_sim_gen)(R, K, Q, L)

/**
 * Converts a point to affine coordinates.
 *
 * @param[out] R				- the result.
 * @param[in] P					- the point to convert.
 */
#define ec_norm(R, P)			RLC_CAT(RLC_EC_LOWER, norm)(R, P)

/**
 * Maps a byte array to a point in an elliptic curve.
 *
 * @param[out] P				- the result.
 * @param[in] M					- the byte array to map.
 * @param[in] L					- the array length in bytes.
 */
#define ec_map(P, M, L)			RLC_CAT(RLC_EC_LOWER, map)(P, M, L)

/**
 * Compresses a point.
 *
 * @param[out] R				- the result.
 * @param[in] P					- the point to compress.
 */
#define ec_pck(R, P)			RLC_CAT(RLC_EC_LOWER, pck)(R, P)

/**
 * Decompresses a point.
 *
 * @param[out] R				- the result.
 * @param[in] P					- the point to decompress.
 */
#define ec_upk(R, P)			RLC_CAT(RLC_EC_LOWER, upk)(R, P)

/**
 * Returns the x-coordinate of an elliptic curve point represented as a
 * multiple precision integer.
 *
 * @param[out] X				- the x-coordinate.
 * @param[in] P					- the point to read.
 */
#if EC_CUR == PRIME || EC_CUR == EDDIE
#define ec_get_x(X, P)			fp_prime_back(X, P->x)
#else
#define ec_get_x(X, P)			bn_read_raw(X, P->x, RLC_FC_DIGS)
#endif

/**
* Returns the y-coordinate of an elliptic curve point represented as a
* multiple precision integer.
*
* @param[out] Y					- the y-coordinate.
* @param[in] P					- the point to read.
*/
#if EC_CUR == PRIME || EC_CUR == EDDIE
#define ec_get_y(Y, P)			fp_prime_back(Y, (P)->y)
#else
#define ec_get_y(Y, P)			bn_read_raw(Y, (P)->y, RLC_FC_DIGS)
#endif

#endif /* !RLC_EC_H */
