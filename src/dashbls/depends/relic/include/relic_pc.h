/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2010 RELIC Authors
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
 * @defgroup pc Pairing-based cryptography
 */

/**
 * @file
 *
 * Abstractions of pairing computation useful to protocol implementors.
 *
 * @ingroup pc
 */

#ifndef RLC_PC_H
#define RLC_PC_H

#include "relic_fbx.h"
#include "relic_ep.h"
#include "relic_eb.h"
#include "relic_pp.h"
#include "relic_bn.h"
#include "relic_util.h"
#include "relic_conf.h"
#include "relic_types.h"

/*============================================================================*/
/* Constant definitions                                                       */
/*============================================================================*/

/**
 * Prefix for function mappings.
 */
/** @{ */
#if FP_PRIME < 1536

#define RLC_G1_LOWER			ep_
#define RLC_G1_UPPER			EP

#if FP_PRIME == 509
#define RLC_G2_LOWER			ep4_
#else
#define RLC_G2_LOWER			ep2_
#endif

#define RLC_G2_UPPER			EP

#if FP_PRIME == 509
#define RLC_GT_LOWER			fp24_
#else
#define RLC_GT_LOWER			fp12_
#endif

#define RLC_PC_LOWER			pp_

#else
#define RLC_G1_LOWER			ep_
#define RLC_G1_UPPER			EP
#define RLC_G2_LOWER			ep_
#define RLC_G2_UPPER			EP
#define RLC_GT_LOWER			fp2_
#define RLC_PC_LOWER			pp_
#endif
/** @} */

/**
 * Represents the size in bytes of the order of G_1 and G_2.
 */
#define RLC_PC_BYTES			RLC_FP_BYTES

/**
 * Represents a G_1 precomputed table.
 */
#define RLC_G1_TABLE			RLC_CAT(RLC_CAT(RLC_, RLC_G1_UPPER), _TABLE)

/**
 * Represents a G_2 precomputed table.
 */
#define RLC_G2_TABLE			RLC_CAT(RLC_CAT(RLC_, RLC_G2_UPPER), _TABLE)

/*============================================================================*/
/* Type definitions                                                           */
/*============================================================================*/

/**
 * Represents a G_1 element.
 */
typedef RLC_CAT(RLC_G1_LOWER, t) g1_t;

/**
 * Represents a G_1 element with automatic allocation.
 */
typedef RLC_CAT(RLC_G1_LOWER, st) g1_st;

/**
 * Represents a G_2 element.
 */
typedef RLC_CAT(RLC_G2_LOWER, t) g2_t;

/**
 * Represents a G_2 element with automatic allocation.
 */
typedef RLC_CAT(RLC_G2_LOWER, st) g2_st;

/**
 * Represents a G_T element.
 */
typedef RLC_CAT(RLC_GT_LOWER, t) gt_t;

/*============================================================================*/
/* Macro definitions                                                          */
/*============================================================================*/

/**
 * Initializes a G_1 element with a null value.
 *
 * @param[out] A			- the element to initialize.
 */
#define g1_null(A)			RLC_CAT(RLC_G1_LOWER, null)(A)

/**
 * Initializes a G_2 element with a null value.
 *
 * @param[out] A			- the element to initialize.
 */
#define g2_null(A)			RLC_CAT(RLC_G2_LOWER, null)(A)

/**
 * Initializes a G_T element with a null value.
 *
 * @param[out] A			- the element to initialize.
 */
#define gt_null(A)			RLC_CAT(RLC_GT_LOWER, null)(A)

/**
 * Calls a function to allocate a G_1 element.
 *
 * @param[out] A			- the new element.
 * @throw ERR_NO_MEMORY		- if there is no available memory.
 */
#define g1_new(A)			RLC_CAT(RLC_G1_LOWER, new)(A)

/**
 * Calls a function to allocate a G_2 element.
 *
 * @param[out] A			- the new element.
 * @throw ERR_NO_MEMORY		- if there is no available memory.
 */
#define g2_new(A)			RLC_CAT(RLC_G2_LOWER, new)(A)

/**
 * Calls a function to allocate a G_T element.
 *
 * @param[out] A			- the new element.
 * @throw ERR_NO_MEMORY		- if there is no available memory.
 */
#define gt_new(A)			RLC_CAT(RLC_GT_LOWER, new)(A)

/**
 * Calls a function to clean and free a G_1 element.
 *
 * @param[out] A			- the element to clean and free.
 */
#define g1_free(A)			RLC_CAT(RLC_G1_LOWER, free)(A)

/**
 * Calls a function to clean and free a G_2 element.
 *
 * @param[out] A			- the element to clean and free.
 */
#define g2_free(A)			RLC_CAT(RLC_G2_LOWER, free)(A)

/**
 * Calls a function to clean and free a G_T element.
 *
 * @param[out] A			- the element to clean and free.
 */
#define gt_free(A)			RLC_CAT(RLC_GT_LOWER, free)(A)



/**
 * Returns the generator of the group G_1.
 *
 * @param[out] G			- the returned generator.
 */
#define g1_get_gen(G)		RLC_CAT(RLC_G1_LOWER, curve_get_gen)(G)

/**
 * Returns the generator of the group G_2.
 *
 * @param[out] G			- the returned generator.
 */
#define g2_get_gen(G)		RLC_CAT(RLC_G2_LOWER, curve_get_gen)(G)

/**
 * Returns the order of the groups G_1, G_2 and G_T.
 *
 * @param[out] N			0 the returned order.
 */
#define pc_get_ord(N)		RLC_CAT(RLC_G1_LOWER, curve_get_ord)(N)

/**
 * Old macros to preserve interface.
 * @{
 */
#define g1_get_ord			pc_get_ord
#define g2_get_ord			pc_get_ord
#define gt_get_ord			pc_get_ord

/**
 * Configures some set of curve parameters for the current security level.
 */
#define pc_param_set_any()	ep_param_set_any_pairf()

/**
 * Returns the type of the configured pairing.
 * @{
 */
#if FP_PRIME < 1536
#define pc_map_is_type1()	(0)
#define pc_map_is_type3()	(1)
#else
#define pc_map_is_type1()	(1)
#define pc_map_is_type3()	(0)
#endif
/**
 * @}
 */

/**
 * Prints the current configured binary elliptic curve.
 */
#define pc_param_print()	RLC_CAT(RLC_G1_LOWER, param_print)()

/*
 * Returns the current security level.
 */
#define pc_param_level()	RLC_CAT(RLC_G1_LOWER, param_level)()

/**
 * Tests if a G_1 element is on the curve.
 *
 * @param[in] P				- the element to test.
 * @return 1 if the element it the unity, 0 otherwise.
 */
#define g1_on_curve(P)		RLC_CAT(RLC_G1_LOWER, on_curve)(P)

/**
 * Tests if a G_2 element is on the curve.
 *
 * @param[in] P				- the element to test.
 * @return 1 if the element it the unity, 0 otherwise.
 */
#define g2_on_curve(P)		RLC_CAT(RLC_G2_LOWER, on_curve)(P)

/**
 * Tests if a G_1 element is the point at infinity.
 *
 * @param[in] P				- the element to test.
 * @return 1 if the element it the unity, 0 otherwise.
 */
#define g1_is_infty(P)		RLC_CAT(RLC_G1_LOWER, is_infty)(P)

/**
 * Tests if a G_2 element is the point at infinity.
 *
 * @param[in] P				- the element to test.
 * @return 1 if the element it the unity, 0 otherwise.
 */
#define g2_is_infty(P)		RLC_CAT(RLC_G2_LOWER, is_infty)(P)

/**
 * Tests if a G_T element is the identity.
 *
 * @param[in] A				- the element to test.
 * @return 1 if the element it the unity, 0 otherwise.
 */
#define gt_is_unity(A)		(RLC_CAT(RLC_GT_LOWER, cmp_dig)(A, 1) == RLC_EQ)

/**
 * Assigns a G_1 element to the unity.
 *
 * @param[out] P			- the element to assign.
 */
#define g1_set_infty(P)		RLC_CAT(RLC_G1_LOWER, set_infty)(P)

/**
 * Assigns a G_2 element to the unity.
 *
 * @param[out] P			- the element to assign.
 */
#define g2_set_infty(P)		RLC_CAT(RLC_G2_LOWER, set_infty)(P)

/**
 * Assigns a G_T element to zero.
 *
 * @param[out] A			- the element to assign.
 */
#define gt_zero(A)			RLC_CAT(RLC_GT_LOWER, zero)(A)

/**
 * Assigns a G_T element to the unity.
 *
 * @param[out] A			- the element to assign.
 */
#define gt_set_unity(A)		RLC_CAT(RLC_GT_LOWER, set_dig)(A, 1)

/**
 * Copies the second argument to the first argument.
 *
 * @param[out] R			- the result.
 * @param[in] P				- the element to copy.
 */
#define g1_copy(R, P)		RLC_CAT(RLC_G1_LOWER, copy)(R, P)

/**
 * Copies the second argument to the first argument.
 *
 * @param[out] R			- the result.
 * @param[in] P				- the element to copy.
 */
#define g2_copy(R, P)		RLC_CAT(RLC_G2_LOWER, copy)(R, P)

/**
 * Copies the second argument to the first argument.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the element to copy.
 */
#define gt_copy(C, A)		RLC_CAT(RLC_GT_LOWER, copy)(C, A)

/**
 * Compares two elements from G_1.
 *
 * @param[in] P				- the first element.
 * @param[in] Q				- the second element.
 * @return RLC_EQ if P == Q and RLC_NE if P != Q.
 */
#define g1_cmp(P, Q)		RLC_CAT(RLC_G1_LOWER, cmp)(P, Q)

/**
 * Compares two elements from G_2.
 *
 * @param[in] P				- the first element.
 * @param[in] Q				- the second element.
 * @return RLC_EQ if P == Q and RLC_NE if P != Q.
 */
#define g2_cmp(P, Q)		RLC_CAT(RLC_G2_LOWER, cmp)(P, Q)

/**
 * Compares two elements from G_T.
 *
 * @param[in] A				- the first element.
 * @param[in] B				- the second element.
 * @return RLC_EQ if A == B and RLC_NE if P != Q.
 */
#define gt_cmp(A, B)		RLC_CAT(RLC_GT_LOWER, cmp)(A, B)

/**
 * Compares a G_T element with a digit.
 *
 * @param[in] A				- the G_T element.
 * @param[in] D				- the digit.
 * @return RLC_EQ if A == D and RLC_NE if A != D.
 */
#define gt_cmp_dig(A, D)	RLC_CAT(RLC_GT_LOWER, cmp_dig)(A, D)

/**
 * Assigns a random value to a G_1 element.
 *
 * @param[out] P			- the element to assign.
 */
#define g1_rand(P)			RLC_CAT(RLC_G1_LOWER, rand)(P)

/**
 * Assigns a random value to a G_2 element.
 *
 * @param[out] P			- the element to assign.
 */
#define g2_rand(P)			RLC_CAT(RLC_G2_LOWER, rand)(P)

/**
 * Randomizes coordinates of a G_1 element.
 *
 * @param[out] R			- the blinded G_1 element.
 * @param[in] P				- the G_1 element to blind.
 */
 #define g1_blind(R, P)		RLC_CAT(RLC_G1_LOWER, blind)(R, P)

 /**
  * Randomizes coordinates of a G_2 element.
  *
  * @param[out] R			- the blinded G_2 element.
  * @param[in] P			- the G_2 element to blind.
  */
#define g2_blind(R, P)		RLC_CAT(RLC_G2_LOWER, blind)(R, P)

/**
 * Prints a G_1 element.
 *
 * @param[in] P				- the element to print.
 */
#define g1_print(P)			RLC_CAT(RLC_G1_LOWER, print)(P)

/**
 * Prints a G_2 element.
 *
 * @param[in] P				- the element to print.
 */
#define g2_print(P)			RLC_CAT(RLC_G2_LOWER, print)(P)

/**
 * Prints a G_T element.
 *
 * @param[in] A				- the element to print.
 */
#define gt_print(A)			RLC_CAT(RLC_GT_LOWER, print)(A)

/**
 * Returns the number of bytes necessary to store a G_1 element.
 *
 * @param[in] P				- the element of G_1.
 * @param[in] C 			- the flag to indicate point compression.
 */
#define g1_size_bin(P, C)	RLC_CAT(RLC_G1_LOWER, size_bin)(P, C)

/**
 * Returns the number of bytes necessary to store a G_2 element.
 *
 * @param[in] P				- the element of G_2.
 * @param[in] C 			- the flag to indicate point compression.
 */
#define g2_size_bin(P, C)	RLC_CAT(RLC_G2_LOWER, size_bin)(P, C)

/**
 * Returns the number of bytes necessary to store a G_T element.
 *
 * @param[in] A				- the element of G_T.
 * @param[in] C 			- the flag to indicate compression.
 */
#define gt_size_bin(A, C)	RLC_CAT(RLC_GT_LOWER, size_bin)(A, C)

/**
 * Reads a G_1 element from a byte vector in big-endian format.
 *
 * @param[out] P			- the result.
 * @param[in] B				- the byte vector.
 * @param[in] L				- the buffer capacity.
 * @throw ERR_NO_BUFFER		- if the buffer capacity is not sufficient.
 */
#define g1_read_bin(P, B, L) 	RLC_CAT(RLC_G1_LOWER, read_bin)(P, B, L)

/**
 * Reads a G_2 element from a byte vector in big-endian format.
 *
 * @param[out] P			- the result.
 * @param[in] B				- the byte vector.
 * @param[in] L				- the buffer capacity.
 * @throw ERR_NO_BUFFER		- if the buffer capacity is not sufficient.
 */
#define g2_read_bin(P, B, L) 	RLC_CAT(RLC_G2_LOWER, read_bin)(P, B, L)

/**
 * Reads a G_T element from a byte vector in big-endian format.
 *
 * @param[out] A			- the result.
 * @param[in] B				- the byte vector.
 * @param[in] L				- the buffer capacity.
 * @throw ERR_NO_BUFFER		- if the buffer capacity is not sufficient.
 */
#define gt_read_bin(A, B, L) 	RLC_CAT(RLC_GT_LOWER, read_bin)(A, B, L)

/**
 * Writes an optionally compressed G_1 element to a byte vector in big-endian
 * format.
 *
 * @param[out] B			- the byte vector.
 * @param[in] L				- the buffer capacity.
 * @param[in] P				- the G_1 element to write.
 * @param[in] C 			- the flag to indicate point compression.
 * @throw ERR_NO_BUFFER		- if the buffer capacity is not enough.
 */
#define g1_write_bin(B, L, P, C)	RLC_CAT(RLC_G1_LOWER, write_bin)(B, L, P, C)

/**
 * Writes an optionally compressed G_2 element to a byte vector in big-endian
 * format.
 *
 * @param[out] B			- the byte vector.
 * @param[in] L				- the buffer capacity.
 * @param[in] P				- the G_2 element to write.
 * @param[in] C 			- the flag to indicate point compression.
 * @throw ERR_NO_BUFFER		- if the buffer capacity is not enough.
 */
#define g2_write_bin(B, L, P, C)	RLC_CAT(RLC_G2_LOWER, write_bin)(B, L, P, C)

/**
 * Writes an optionally compresseds G_T element to a byte vector in big-endian
 * format.
 *
 * @param[out] B			- the byte vector.
 * @param[in] L				- the buffer capacity.
 * @param[in] A				- the G_T element to write.
 * @param[in] C 			- the flag to indicate point compression.
 * @throw ERR_NO_BUFFER		- if the buffer capacity is not sufficient.
 */
#define gt_write_bin(B, L, A, C)	RLC_CAT(RLC_GT_LOWER, write_bin)(B, L, A, C)

/**
 * Negates a element from G_1. Computes R = -P.
 *
 * @param[out] R			- the result.
 * @param[in] P				- the element to negate.
 */
#define g1_neg(R, P)		RLC_CAT(RLC_G1_LOWER, neg)(R, P)

/**
 * Negates a element from G_2. Computes R = -P.
 *
 * @param[out] R			- the result.
 * @param[in] P				- the element to negate.
 */
#define g2_neg(R, P)		RLC_CAT(RLC_G2_LOWER, neg)(R, P)

/**
 * Inverts a element from G_T. Computes C = 1/A.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the element to invert.
 */
#define gt_inv(C, A)		RLC_CAT(RLC_GT_LOWER, inv_cyc)(C, A)

/**
 * Adds two elliptic elements from G_1. Computes R = P + Q.
 *
 * @param[out] R			- the result.
 * @param[in] P				- the first element to add.
 * @param[in] Q				- the second element to add.
 */
#define g1_add(R, P, Q)		RLC_CAT(RLC_G1_LOWER, add)(R, P, Q)

/**
 * Adds two elliptic elements from G_2. Computes R = P + Q.
 *
 * @param[out] R			- the result.
 * @param[in] P				- the first element to add.
 * @param[in] Q				- the second element to add.
 */
#define g2_add(R, P, Q)		RLC_CAT(RLC_G2_LOWER, add)(R, P, Q)

/**
 * Multiplies two elliptic elements from G_T. Computes C = A * B.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the first element to multiply.
 * @param[in] B				- the second element to multiply.
 */
#define gt_mul(C, A, B)		RLC_CAT(RLC_GT_LOWER, mul)(C, A, B)

/**
 * Subtracts a G_1 element from another. Computes R = P - Q.
 *
 * @param[out] R			- the result.
 * @param[in] P				- the first element.
 * @param[in] Q				- the second element.
 */
#define g1_sub(R, P, Q)		RLC_CAT(RLC_G1_LOWER, sub)(R, P, Q)

/**
 * Subtracts a G_2 element from another. Computes R = P - Q.
 *
 * @param[out] R			- the result.
 * @param[in] P				- the first element.
 * @param[in] Q				- the second element.
 */
#define g2_sub(R, P, Q)		RLC_CAT(RLC_G2_LOWER, sub)(R, P, Q)

/**
 * Doubles a G_1 element. Computes R = 2P.
 *
 * @param[out] R			- the result.
 * @param[in] P				- the element to double.
 */
#define g1_dbl(R, P)		RLC_CAT(RLC_G1_LOWER, dbl)(R, P)

/**
 * Doubles a G_2 element. Computes R = 2P.
 *
 * @param[out] R			- the result.
 * @param[in] P				- the element to double.
 */
#define g2_dbl(R, P)		RLC_CAT(RLC_G2_LOWER, dbl)(R, P)

/**
 * Squares a G_T element. Computes C = A^2.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the element to square.
 */
#define gt_sqr(C, A)		RLC_CAT(RLC_GT_LOWER, sqr)(C, A)

/**
 * Normalizes an element of G_1.
 *
 * @param[out] R			- the result.
 * @param[in] P				- the element to normalize.
 */
#define g1_norm(R, P)		RLC_CAT(RLC_G1_LOWER, norm)(R, P)

/**
 * Normalizes a vector of G_1 elements.
 *
 * @param[out] R			- the result.
 * @param[in] P				- the elements to normalize.
 * @param[in] N				- the number of elements to normalize.
 */
#define g1_norm_sim(R, P, N)	RLC_CAT(RLC_G1_LOWER, norm_sim)(R, P, N)

/**
 * Normalizes an element of G_2.
 *
 * @param[out] R			- the result.
 * @param[in] P				- the element to normalize.
 */
#define g2_norm(R, P)		RLC_CAT(RLC_G2_LOWER, norm)(R, P)

/**
 * Normalizes a vector of G_2 elements.
 *
 * @param[out] R			- the result.
 * @param[in] P				- the elements to normalize.
 * @param[in] N				- the number of elements to normalize.
 */
#define g2_norm_sim(R, P, N)	RLC_CAT(RLC_G2_LOWER, norm_sim)(R, P, N)

/**
 * Multiplies an element from G_1 by a secret scalar. Computes R = [k]P.
 *
 * @param[out] R				- the result.
 * @param[in] P					- the element to multiply.
 * @param[in] K					- the secret scalar.
 */
#define g1_mul_key(R, P, K)		RLC_CAT(RLC_G1_LOWER, mul_lwreg)(R, P, K)

/**
 * Multiplies an element from G_1 by a small integer. Computes R = [k]P.
 *
 * @param[out] R			- the result.
 * @param[in] P				- the element to multiply.
 * @param[in] K				- the small integer.
 */
#define g1_mul_dig(R, P, K)		RLC_CAT(RLC_G1_LOWER, mul_dig)(R, P, K)

/**
 * Multiplies an element from G_2 by a small integer. Computes R = [k]P.
 *
 * @param[out] R			- the result.
 * @param[in] P				- the element to multiply.
 * @param[in] K				- the small integer.
 */
#define g2_mul_dig(R, P, K)		RLC_CAT(RLC_G2_LOWER, mul_dig)(R, P, K)

/**
 * Builds a precomputation table for multiplying an element from G_1.
 *
 * @param[out] T			- the precomputation table.
 * @param[in] P				- the element to multiply.
 */
#define g1_mul_pre(T, P)	RLC_CAT(RLC_G1_LOWER, mul_pre)(T, P)

/**
 * Builds a precomputation table for multiplying an element from G_2.
 *
 * @param[out] T			- the precomputation table.
 * @param[in] P				- the element to multiply.
 */
#define g2_mul_pre(T, P)	RLC_CAT(RLC_G2_LOWER, mul_pre)(T, P)

/**
 * Multiplies an element from G_1 using a precomputation table.
 * Computes R = [k]P.
 *
 * @param[out] R			- the result.
 * @param[in] T				- the precomputation table.
 * @param[in] K				- the integer.
 */
#define g1_mul_fix(R, T, K)	RLC_CAT(RLC_G1_LOWER, mul_fix)(R, T, K)

/**
 * Multiplies an element from G_2 using a precomputation table.
 * Computes R = [k]P.
 *
 * @param[out] R			- the result.
 * @param[in] T				- the precomputation table.
 * @param[in] K				- the integer.
 */
#define g2_mul_fix(R, T, K)	RLC_CAT(RLC_G2_LOWER, mul_fix)(R, T, K)

/**
 * Multiplies simultaneously two elements from G_1. Computes R = [k]P + [l]Q.
 *
 * @param[out] R			- the result.
 * @param[out] P			- the first G_1 element to multiply.
 * @param[out] K			- the first integer scalar.
 * @param[out] L			- the second G_1 element to multiply.
 * @param[out] Q			- the second integer scalar.
 */
#define g1_mul_sim(R, P, K, Q, L)	RLC_CAT(RLC_G1_LOWER, mul_sim)(R, P, K, Q, L)

/**
 * Multiplies simultaneously elements from G_1. Computes R = \Sum_i=0..n k_iP_i.
 *
 * @param[out] R			- the result.
 * @param[out] P			- the G_1 elements to multiply.
 * @param[out] K			- the integer scalars.
 * @param[out] N			- the number of elements to multiply.
 */
#define g1_mul_sim_lot(R, P, K, N)	RLC_CAT(RLC_G1_LOWER, mul_sim_lot)(R, P, K, N)

/**
 * Multiplies elements from G_1 by small scalars. Computes R = \sum k_iP_i.
 *
 * @param[out] R			- the result.
 * @param[in] P				- the elements to multiply.
 * @param[in] K				- the small scalars.
 * @param[in] L				- the number of points to multiply.
 */
#define g1_mul_sim_dig(R, P, K, L)	RLC_CAT(RLC_G1_LOWER, mul_sim_dig)(R, P, K, L)

/**
 * Multiplies simultaneously two elements from G_2. Computes R = [k]P + [l]Q.
 *
 * @param[out] R			- the result.
 * @param[out] P			- the first G_2 element to multiply.
 * @param[out] K			- the first integer scalar.
 * @param[out] L			- the second G_2 element to multiply.
 * @param[out] Q			- the second integer scalar.
 */
#define g2_mul_sim(R, P, K, Q, L)	RLC_CAT(RLC_G2_LOWER, mul_sim)(R, P, K, Q, L)

/**
 * Multiplies simultaneously elements from G_2. Computes R = \Sum_i=0..n k_iP_i.
 *
 * @param[out] R			- the result.
 * @param[out] P			- the G_2 elements to multiply.
 * @param[out] K			- the integer scalars.
 * @param[out] N			- the number of elements to multiply.
 */
#define g2_mul_sim_lot(R, P, K, N)	RLC_CAT(RLC_G2_LOWER, mul_sim_lot)(R, P, K, N)

/**
 * Multiplies elements from G_2 by small scalars. Computes R = \sum k_iP_i.
 *
 * @param[out] R			- the result.
 * @param[in] P				- the elements to multiply.
 * @param[in] K				- the small scalars.
 * @param[in] L				- the number of points to multiply.
 */
#define g2_mul_sim_dig(R, P, K, L)	RLC_CAT(RLC_G2_LOWER, mul_sim_dig)(R, P, K, L)

/**
 * Multiplies simultaneously two elements from G_1, where one of the is the
 * generator. Computes R = [k]G + [l]Q.
 *
 * @param[out] R			- the result.
 * @param[out] K			- the first integer scalar.
 * @param[out] L			- the second G_1 element to multiply.
 * @param[out] Q			- the second integer scalar.
 */
#define g1_mul_sim_gen(R, K, Q, L)	RLC_CAT(RLC_G1_LOWER, mul_sim_gen)(R, K, Q, L)

/**
 * Multiplies simultaneously two elements from G_1, where one of the is the
 * generator. Computes R = [k]G + [l]Q.
 *
 * @param[out] R			- the result.
 * @param[out] K			- the first integer scalar.
 * @param[out] L			- the second G_1 element to multiply.
 * @param[out] Q			- the second integer scalar.
 */
#define g2_mul_sim_gen(R, K, Q, L)	RLC_CAT(RLC_G2_LOWER, mul_sim_gen)(R, K, Q, L)

/**
 * Exponetiates a G_2 element using the i-th power Frobenius.
 * Computes C = [p^i]A.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the element to exponentiate.
 * @param[in] I				- the power of the Frobenius map.
 */
#define g2_frb(C, A, I)		RLC_CAT(RLC_G2_LOWER, frb)(C, A, I)

/**
 * Exponetiates a G_T element using the i-th power Frobenius.
 * Computes C = A^(p^i).
 *
 * @param[out] C			- the result.
 * @param[in] A				- the element to exponentiate.
 * @param[in] I				- the power of the Frobenius map.
 */
#define gt_frb(C, A, I)		RLC_CAT(RLC_GT_LOWER, frb)(C, A, I)

/**
 * Maps a byte array to an element in G_1.
 *
 * @param[out] P			- the result.
 * @param[in] M				- the byte array to map.
 * @param[in] L				- the array length in bytes.
 */
#define g1_map(P, M, L);	RLC_CAT(RLC_G1_LOWER, map)(P, M, L)

/**
 * Maps a byte array to an element in G_2.
 *
 * @param[out] P			- the result.
 * @param[in] M				- the byte array to map.
 * @param[in] L				- the array length in bytes.
 */
#define g2_map(P, M, L);	RLC_CAT(RLC_G2_LOWER, map)(P, M, L)

/**
 * Computes the bilinear pairing of a G_1 element and a G_2 element. Computes
 * R = e(P, Q).
 *
 * @param[out] R			- the result.
 * @param[in] P				- the first element.
 * @param[in] Q				- the second element.
 */
#if FP_PRIME < 1536

#if FP_PRIME == 509
#define pc_map(R, P, Q);		RLC_CAT(RLC_PC_LOWER, map_k24)(R, P, Q)
#else
#define pc_map(R, P, Q);		RLC_CAT(RLC_PC_LOWER, map_k12)(R, P, Q)
#endif

#else

#define pc_map(R, P, Q);		RLC_CAT(RLC_PC_LOWER, map_k2)(R, P, Q)

#endif

/**
 * Computes the multi-pairing of G_1 elements and G_2 elements. Computes
 * R = \prod e(P_i, Q_i).
 *
 * @param[out] R			- the result.
 * @param[in] P				- the first pairing arguments.
 * @param[in] Q				- the second pairing arguments.
 * @param[in] M 			- the number of pairing arguments.
 */
#if FP_PRIME < 1536

#if FP_PRIME == 509
#define pc_map_sim(R, P, Q, M);	RLC_CAT(RLC_PC_LOWER, map_sim_k24)(R, P, Q, M)
#else
#define pc_map_sim(R, P, Q, M);	RLC_CAT(RLC_PC_LOWER, map_sim_k12)(R, P, Q, M)
#endif

#else
#define pc_map_sim(R, P, Q, M);	RLC_CAT(RLC_PC_LOWER, map_sim_k2)(R, P, Q, M)
#endif

/**
 * Computes the final exponentiation of the pairing.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the field element to exponentiate.
 */
#if FP_PRIME < 1536

#if FP_PRIME == 509
#define pc_exp(C, A);			RLC_CAT(RLC_PC_LOWER, exp_k24)(C, A)
#else
#define pc_exp(C, A);			RLC_CAT(RLC_PC_LOWER, exp_k12)(C, A)
#endif

#else
#define pc_exp(C, A);			RLC_CAT(RLC_PC_LOWER, exp_k2)(C, A)
#endif

/*============================================================================*/
/* Function prototypes                                                        */
/*============================================================================*/

/**
 * Initializes the cryptographic protocol module.
 */
void pc_core_init(void);

/**
 * Computes constants internal to the cryptographic protocol module.
 */
void pc_core_calc(void);

/**
 * Finalizes the cryptographic protocol module.
 */
void pc_core_clean(void);


/**
 * Assigns a random value to an element from G_T.
 *
 * @param[out] a			- the element to assign.
 */
void gt_rand(gt_t a);

/**
 * Multiplies an element from G_1 by an integer. Computes R = [k]P.
 *
 * @param[out] r			- the result.
 * @param[in] p				- the element to multiply.
 * @param[in] k				- the integer.
 */
void g1_mul(g1_t r, g1_t p, bn_t k);

/**
 * Multiplies an element from G_2 by an integer. Computes R = [k]P.
 *
 * @param[out] r			- the result.
 * @param[in] p				- the element to multiply.
 * @param[in] k				- the integer.
 */
void g2_mul(g2_t r, g2_t p, bn_t k);

/**
 * Multiplies the generator of G_1 by an integer.
 *
 * @param[out] r			- the result.
 * @param[in] k				- the integer.
 */
void g1_mul_gen(g1_t r, bn_t k);

/**
 * Multiplies the generator of G_2 by an integer.
 *
 * @param[out] r			- the result.
 * @param[in] k				- the integer.
 */
void g2_mul_gen(g2_t r, bn_t k);

/**
 * Exponentiates an element from G_T by an integer. Computes c = a^b.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the element to exponentiate.
 * @param[in] b				- the integer exponent.
 */
void gt_exp(gt_t c, gt_t a, bn_t b);

/**
 * Exponentiates an element from G_T by a small integer. Computes c = a^b.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the element to exponentiate.
 * @param[in] b				- the integer exponent.
 */
void gt_exp_dig(gt_t c, gt_t a, dig_t b);

/**
 * Exponentiates two element from G_T by integers simultaneously. Computes
 * e = a^b * c^d.
 *
 * @param[out] e			- the result.
 * @param[in] a				- the first element to exponentiate.
 * @param[in] b				- the first integer exponent.
 * @param[in] a				- the second element to exponentiate.
 * @param[in] b				- the second integer exponent.
 */
void gt_exp_sim(gt_t e, gt_t a, bn_t b, gt_t c, bn_t d);

/**
 * Exponentiates a generator from G_T by an integer. Computes c = a^b.
 *
 * @param[out] c			- the result.
 * @param[in] b				- the integer exponent.
 */
void gt_exp_gen(gt_t c, bn_t b);

 /**
  * Returns the generator for the group G_T.
  *
  * @param[out] g			- the returned generator.
  */
void gt_get_gen(gt_t g);

/**
 * Checks if an element from G_1 is valid (has the right order).
 *
 * @param[in] a             - the element to check.
 */
int g1_is_valid(g1_t a);

/**
 * Checks if an element form G_2 is valid (has the right order).
 *
 * @param[in] a             - the element to check.
 */
int g2_is_valid(g2_t a);

/**
 * Checks if an element form G_T is valid (has the right order).
 *
 * @param[in] a             - the element to check.
 */
int gt_is_valid(gt_t a);

#endif /* !RLC_PC_H */
