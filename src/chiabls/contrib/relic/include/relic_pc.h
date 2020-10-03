/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (C) 2007-2013 RELIC Authors
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
 * @defgroup pc Pairing-based cryptography
 */

/**
 * @file
 *
 * Abstractions of pairing computation useful to protocol implementors.
 *
 * @ingroup pc
 */

#ifndef RELIC_PC_H
#define RELIC_PC_H

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
#define G1_LOWER			ep_
#define G1_UPPER			EP
#define G2_LOWER			ep2_
#define G2_UPPER			EP
#define GT_LOWER			fp12_
#define PC_LOWER			pp_
#else
#define G1_LOWER			ep_
#define G1_UPPER			EP
#define G2_LOWER			ep_
#define G2_UPPER			EP
#define GT_LOWER			fp2_
#define PC_LOWER			pp_
#endif
/** @} */

/**
 * Prefix for constant mappings.
 */
#define PC_UPPER			PP_

/**
 * Represents the size in bytes of the order of G_1 and G_2.
 */
#define PC_BYTES			FP_BYTES

/**
 * Represents a G_1 precomputable table.
 */
#define RELIC_G1_TABLE			CAT(CAT(RELIC_, G1_UPPER), _TABLE_MAX)

/**
 * Represents a G_2 precomputable table.
 */
#define RELIC_G2_TABLE			CAT(CAT(RELIC_, G2_UPPER), _TABLE_MAX)

/*============================================================================*/
/* Type definitions                                                           */
/*============================================================================*/

/**
 * Represents a G_1 element.
 */
typedef CAT(G1_LOWER, t) g1_t;

/**
 * Represents a G_1 element with automatic allocation.
 */
typedef CAT(G1_LOWER, st) g1_st;

/**
 * Represents a G_1 element.
 */
typedef CAT(G2_LOWER, t) g2_t;

/**
 * Represents a G_2 element with automatic allocation.
 */
typedef CAT(G2_LOWER, st) g2_st;

/**
 * Represents a G_T element.
 */
typedef CAT(GT_LOWER, t) gt_t;

/*============================================================================*/
/* Macro definitions                                                          */
/*============================================================================*/

/**
 * Initializes a G_1 element with a null value.
 *
 * @param[out] A			- the element to initialize.
 */
#define g1_null(A)			CAT(G1_LOWER, null)(A)

/**
 * Initializes a G_2 element with a null value.
 *
 * @param[out] A			- the element to initialize.
 */
#define g2_null(A)			CAT(G2_LOWER, null)(A)

/**
 * Initializes a G_T element with a null value.
 *
 * @param[out] A			- the element to initialize.
 */
#define gt_null(A)			CAT(GT_LOWER, null)(A)

/**
 * Calls a function to allocate a G_1 element.
 *
 * @param[out] A			- the new element.
 * @throw ERR_NO_MEMORY		- if there is no available memory.
 */
#define g1_new(A)			CAT(G1_LOWER, new)(A)

/**
 * Calls a function to allocate a G_2 element.
 *
 * @param[out] A			- the new element.
 * @throw ERR_NO_MEMORY		- if there is no available memory.
 */
#define g2_new(A)			CAT(G2_LOWER, new)(A)

/**
 * Calls a function to allocate a G_T element.
 *
 * @param[out] A			- the new element.
 * @throw ERR_NO_MEMORY		- if there is no available memory.
 */
#define gt_new(A)			CAT(GT_LOWER, new)(A)

/**
 * Calls a function to clean and free a G_1 element.
 *
 * @param[out] A			- the element to clean and free.
 */
#define g1_free(A)			CAT(G1_LOWER, free)(A)

/**
 * Calls a function to clean and free a G_2 element.
 *
 * @param[out] A			- the element to clean and free.
 */
#define g2_free(A)			CAT(G2_LOWER, free)(A)

/**
 * Calls a function to clean and free a G_T element.
 *
 * @param[out] A			- the element to clean and free.
 */
#define gt_free(A)			CAT(GT_LOWER, free)(A)

/**
 * Returns the generator of the group G_1.
 *
 * @param[out] G			- the returned generator.
 */
#define g1_get_gen(G)		CAT(G1_LOWER, curve_get_gen)(G)

/**
 * Returns the generator of the group G_2.
 *
 * @param[out] G			- the returned generator.
 */
#define g2_get_gen(G)		CAT(G2_LOWER, curve_get_gen)(G)

/**
 * Returns the order of the group G_1.
 *
 * @param[out] N			0 the returned order.
 */
#define g1_get_ord(N)		CAT(G1_LOWER, curve_get_ord)(N)

/**
 * Returns the order of the group G_2.
 *
 * @param[out] N			0 the returned order.
 */
#define g2_get_ord(N)		CAT(G2_LOWER, curve_get_ord)(N)

/**
 * Returns the order of the group G_T.
 *
 * @param[out] N			0 the returned order.
 */
#define gt_get_ord(N)		CAT(G1_LOWER, curve_get_ord)(N)

/**
 * Configures some set of curve parameters for the current security level.
 */
#define pc_param_set_any()	ep_param_set_any_pairf()

/**
 * Returns the type of the configured pairing.
 *
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
#define pc_param_print()	CAT(G1_LOWER, param_print)()

/**
 * Returns the current security level.
 */
#define pc_param_level()	CAT(G1_LOWER, param_level)()

/**
 * Tests if a G_1 element is the unity.
 *
 * @param[in] P				- the element to test.
 * @return 1 if the element it the unity, 0 otherwise.
 */
#define g1_is_infty(P)		CAT(G1_LOWER, is_infty)(P)

/**
 * Tests if a G_2 element is the unity.
 *
 * @param[in] P				- the element to test.
 * @return 1 if the element it the unity, 0 otherwise.
 */
#define g2_is_infty(P)		CAT(G2_LOWER, is_infty)(P)

/**
 * Tests if a G_T element is the unity.
 *
 * @param[in] A				- the element to test.
 * @return 1 if the element it the unity, 0 otherwise.
 */
#define gt_is_unity(A)		(CAT(GT_LOWER, cmp_dig)(A, 1) == CMP_EQ)

/**
 * Assigns a G_1 element to the unity.
 *
 * @param[out] P			- the element to assign.
 */
#define g1_set_infty(P)		CAT(G1_LOWER, set_infty)(P)

/**
 * Assigns a G_2 element to the unity.
 *
 * @param[out] P			- the element to assign.
 */
#define g2_set_infty(P)		CAT(G2_LOWER, set_infty)(P)

/**
 * Assigns a G_T element to zero.
 *
 * @param[out] A			- the element to assign.
 */
#define gt_zero(A)			CAT(GT_LOWER, zero)(A)

/**
 * Assigns a G_T element to the unity.
 *
 * @param[out] A			- the element to assign.
 */
#define gt_set_unity(A)		CAT(GT_LOWER, set_dig)(A, 1)

/**
 * Copies the second argument to the first argument.
 *
 * @param[out] R			- the result.
 * @param[in] P				- the element to copy.
 */
#define g1_copy(R, P)		CAT(G1_LOWER, copy)(R, P)

/**
 * Copies the second argument to the first argument.
 *
 * @param[out] R			- the result.
 * @param[in] P				- the element to copy.
 */
#define g2_copy(R, P)		CAT(G2_LOWER, copy)(R, P)

/**
 * Copies the second argument to the first argument.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the element to copy.
 */
#define gt_copy(C, A)		CAT(GT_LOWER, copy)(C, A)

/**
 * Compares two elements from G_1.
 *
 * @param[in] P				- the first element.
 * @param[in] Q				- the second element.
 * @return CMP_EQ if P == Q and CMP_NE if P != Q.
 */
#define g1_cmp(P, Q)		CAT(G1_LOWER, cmp)(P, Q)

/**
 * Compares two elements from G_2.
 *
 * @param[in] P				- the first element.
 * @param[in] Q				- the second element.
 * @return CMP_EQ if P == Q and CMP_NE if P != Q.
 */
#define g2_cmp(P, Q)		CAT(G2_LOWER, cmp)(P, Q)

/**
 * Compares two elements from G_T.
 *
 * @param[in] A				- the first element.
 * @param[in] B				- the second element.
 * @return CMP_EQ if A == B and CMP_NE if P != Q.
 */
#define gt_cmp(A, B)		CAT(GT_LOWER, cmp)(A, B)

/**
 * Compares a G_T element with a digit.
 *
 * @param[in] A				- the G_T element.
 * @param[in] D				- the digit.
 * @return CMP_EQ if A == D and CMP_NE if A != D.
 */
#define gt_cmp_dig(A, D)	CAT(GT_LOWER, cmp_dig)(A, D)

/**
 * Assigns a random value to a G_1 element.
 *
 * @param[out] P			- the element to assign.
 */
#define g1_rand(P)			CAT(G1_LOWER, rand)(P)

/**
 * Assigns a random value to a G_2 element.
 *
 * @param[out] P			- the element to assign.
 */
#define g2_rand(P)			CAT(G2_LOWER, rand)(P)

/**
 * Tests if G_1 element is valid.
 *
 * @param[out] P			- the element to assign.
 */
#define g1_is_valid(P)		CAT(G1_LOWER, is_valid)(P)

/**
 * Tests if G_2 element is valid.
 *
 * @param[out] P			- the element to assign.
 */
#define g2_is_valid(P)		CAT(G2_LOWER, is_valid)(P)

/**
 * Prints a G_1 element.
 *
 * @param[in] P				- the element to print.
 */
#define g1_print(P)			CAT(G1_LOWER, print)(P)

/**
 * Prints a G_2 element.
 *
 * @param[in] P				- the element to print.
 */
#define g2_print(P)			CAT(G2_LOWER, print)(P)

/**
 * Prints a G_T element.
 *
 * @param[in] A				- the element to print.
 */
#define gt_print(A)			CAT(GT_LOWER, print)(A)

/**
 * Returns the number of bytes necessary to store a G_1 element.
 *
 * @param[in] P				- the element of G_1.
 * @param[in] C 			- the flag to indicate point compression.
 */
#define g1_size_bin(P, C)	CAT(G1_LOWER, size_bin)(P, C)

/**
 * Returns the number of bytes necessary to store a G_2 element.
 *
 * @param[in] P				- the element of G_2.
 * @param[in] C 			- the flag to indicate point compression.
 */
#define g2_size_bin(P, C)	CAT(G2_LOWER, size_bin)(P, C)

/**
 * Returns the number of bytes necessary to store a G_T element.
 *
 * @param[in] A				- the element of G_T.
 * @param[in] C 			- the flag to indicate compression.
 */
#define gt_size_bin(A, C)	CAT(GT_LOWER, size_bin)(A, C)

/**
 * Reads a G_1 element from a byte vector in big-endian format.
 *
 * @param[out] P			- the result.
 * @param[in] B				- the byte vector.
 * @param[in] L				- the buffer capacity.
 * @throw ERR_NO_BUFFER		- if the buffer capacity is not sufficient.
 */
#define g1_read_bin(P, B, L) 	CAT(G1_LOWER, read_bin)(P, B, L)

/**
 * Reads a G_2 element from a byte vector in big-endian format.
 *
 * @param[out] P			- the result.
 * @param[in] B				- the byte vector.
 * @param[in] L				- the buffer capacity.
 * @throw ERR_NO_BUFFER		- if the buffer capacity is not sufficient.
 */
#define g2_read_bin(P, B, L) 	CAT(G2_LOWER, read_bin)(P, B, L)

/**
 * Reads a G_T element from a byte vector in big-endian format.
 *
 * @param[out] A			- the result.
 * @param[in] B				- the byte vector.
 * @param[in] L				- the buffer capacity.
 * @throw ERR_NO_BUFFER		- if the buffer capacity is not sufficient.
 */
#define gt_read_bin(A, B, L) 	CAT(GT_LOWER, read_bin)(A, B, L)

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
#define g1_write_bin(B, L, P, C)	CAT(G1_LOWER, write_bin)(B, L, P, C)

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
#define g2_write_bin(B, L, P, C)	CAT(G2_LOWER, write_bin)(B, L, P, C)

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
#define gt_write_bin(B, L, A, C)	CAT(GT_LOWER, write_bin)(B, L, A, C)

/**
 * Negates a element from G_1. Computes R = -P.
 *
 * @param[out] R			- the result.
 * @param[in] P				- the element to negate.
 */
#define g1_neg(R, P)		CAT(G1_LOWER, neg)(R, P)

/**
 * Negates a element from G_2. Computes R = -P.
 *
 * @param[out] R			- the result.
 * @param[in] P				- the element to negate.
 */
#define g2_neg(R, P)		CAT(G2_LOWER, neg)(R, P)

/**
 * Inverts a element from G_T. Computes C = 1/A.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the element to invert.
 */
#define gt_inv(C, A)		CAT(GT_LOWER, inv_uni)(C, A)

/**
 * Adds two elliptic elements from G_1. Computes R = P + Q.
 *
 * @param[out] R			- the result.
 * @param[in] P				- the first element to add.
 * @param[in] Q				- the second element to add.
 */
#define g1_add(R, P, Q)		CAT(G1_LOWER, add)(R, P, Q)

/**
 * Adds two elliptic elements from G_2. Computes R = P + Q.
 *
 * @param[out] R			- the result.
 * @param[in] P				- the first element to add.
 * @param[in] Q				- the second element to add.
 */
#define g2_add(R, P, Q)		CAT(G2_LOWER, add)(R, P, Q)

/**
 * Multiplies two elliptic elements from G_T. Computes C = A * B.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the first element to multiply.
 * @param[in] B				- the second element to multiply.
 */
#define gt_mul(C, A, B)		CAT(GT_LOWER, mul)(C, A, B)

/**
 * Subtracts a G_1 element from another. Computes R = P - Q.
 *
 * @param[out] R			- the result.
 * @param[in] P				- the first element.
 * @param[in] Q				- the second element.
 */
#define g1_sub(R, P, Q)		CAT(G1_LOWER, sub)(R, P, Q)

/**
 * Subtracts a G_2 element from another. Computes R = P - Q.
 *
 * @param[out] R			- the result.
 * @param[in] P				- the first element.
 * @param[in] Q				- the second element.
 */
#define g2_sub(R, P, Q)		CAT(G2_LOWER, sub)(R, P, Q)

/**
 * Doubles a G_1 element. Computes R = 2P.
 *
 * @param[out] R			- the result.
 * @param[in] P				- the element to double.
 */
#define g1_dbl(R, P)		CAT(G1_LOWER, dbl)(R, P)

/**
 * Doubles a G_2 element. Computes R = 2P.
 *
 * @param[out] R			- the result.
 * @param[in] P				- the element to double.
 */
#define g2_dbl(R, P)		CAT(G2_LOWER, dbl)(R, P)

/**
 * Squares a G_T element. Computes C = A^2.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the element to square.
 */
#define gt_sqr(C, A)		CAT(GT_LOWER, sqr)(C, A)

/**
 * Normalizes an element of G_1.
 *
 * @param[out] R			- the result.
 * @param[in] P				- the element to normalize.
 */
#define g1_norm(R, P)		CAT(G1_LOWER, norm)(R, P)

/**
 * Normalizes an element of G_2.
 *
 * @param[out] R			- the result.
 * @param[in] P				- the element to normalize.
 */
#define g2_norm(R, P)		CAT(G2_LOWER, norm)(R, P)

/**
 * Multiplies an element from G_1 by an integer. Computes R = kP.
 *
 * @param[out] R			- the result.
 * @param[in] P				- the element to multiply.
 * @param[in] K				- the integer.
 */
#define g1_mul(R, P, K)		CAT(G1_LOWER, mul)(R, P, K)

/**
 * Multiplies an element from G_1 by a small integer. Computes R = kP.
 *
 * @param[out] R			- the result.
 * @param[in] P				- the element to multiply.
 * @param[in] K				- the small integer.
 */
#define g1_mul_dig(R, P, K)		CAT(G1_LOWER, mul_dig)(R, P, K)

/**
 * Multiplies an element from G_2 by an integer. Computes R = kP.
 *
 * @param[out] R			- the result.
 * @param[in] P				- the element to multiply.
 * @param[in] K				- the integer.
 */
#define g2_mul(R, P, K)		CAT(G2_LOWER, mul)(R, P, K)

/**
 * Multiplies an element from G_2 by a small integer. Computes R = kP.
 *
 * @param[out] R			- the result.
 * @param[in] P				- the element to multiply.
 * @param[in] K				- the small integer.
 */
#define g2_mul_dig(R, P, K)		CAT(G2_LOWER, mul_dig)(R, P, K)

/**
 * Multiplies the generator of G_1 by an integer.
 *
 * @param[out] R			- the result.
 * @param[in] K				- the integer.
 */
#define g1_mul_gen(R, K)	CAT(G1_LOWER, mul_gen)(R, K)

/**
 * Multiplies the generator of G_2 by an integer.
 *
 * @param[out] R			- the result.
 * @param[in] K				- the integer.
 */
#define g2_mul_gen(R, K)	CAT(G2_LOWER, mul_gen)(R, K)

/**
 * Builds a precomputation table for multiplying an element from G_1.
 *
 * @param[out] T			- the precomputation table.
 * @param[in] P				- the element to multiply.
 */
#define g1_mul_pre(T, P)	CAT(G1_LOWER, mul_pre)(T, P)

/**
 * Builds a precomputation table for multiplying an element from G_2.
 *
 * @param[out] T			- the precomputation table.
 * @param[in] P				- the element to multiply.
 */
#define g2_mul_pre(T, P)	CAT(G2_LOWER, mul_pre)(T, P)

/**
 * Multiplies an element from G_1 using a precomputation table.
 * Computes R = kP.
 *
 * @param[out] R			- the result.
 * @param[in] T				- the precomputation table.
 * @param[in] K				- the integer.
 */
#define g1_mul_fix(R, T, K)	CAT(G1_LOWER, mul_fix)(R, T, K)

/**
 * Multiplies an element from G_2 using a precomputation table.
 * Computes R = kP.
 *
 * @param[out] R			- the result.
 * @param[in] T				- the precomputation table.
 * @param[in] K				- the integer.
 */
#define g2_mul_fix(R, T, K)	CAT(G2_LOWER, mul_fix)(R, T, K)

/**
 * Multiplies simultaneously two elements from G_1. Computes R = kP + lQ.
 *
 * @param[out] R			- the result.
 * @param[out] P			- the first G_1 element to multiply.
 * @param[out] K			- the first integer scalar.
 * @param[out] L			- the second G_1 element to multiply.
 * @param[out] Q			- the second integer scalar.
 */
#define g1_mul_sim(R, P, K, Q, L)	CAT(G1_LOWER, mul_sim)(R, P, K, Q, L)

/**
 * Multiplies simultaneously two elements from G_2. Computes R = kP + lQ.
 *
 * @param[out] R			- the result.
 * @param[out] P			- the first G_2 element to multiply.
 * @param[out] K			- the first integer scalar.
 * @param[out] L			- the second G_2 element to multiply.
 * @param[out] Q			- the second integer scalar.
 */
#define g2_mul_sim(R, P, K, Q, L)	CAT(G2_LOWER, mul_sim)(R, P, K, Q, L)

/**
 * Multiplies simultaneously two elements from G_1, where one of the is the
 * generator. Computes R = kG + lQ.
 *
 * @param[out] R			- the result.
 * @param[out] K			- the first integer scalar.
 * @param[out] L			- the second G_1 element to multiply.
 * @param[out] Q			- the second integer scalar.
 */
#define g1_mul_sim_gen(R, K, Q, L)	CAT(G1_LOWER, mul_sim_gen)(R, K, Q, L)

/**
 * Multiplies simultaneously two elements from G_1, where one of the is the
 * generator. Computes R = kG + lQ.
 *
 * @param[out] R			- the result.
 * @param[out] K			- the first integer scalar.
 * @param[out] L			- the second G_1 element to multiply.
 * @param[out] Q			- the second integer scalar.
 */
#define g2_mul_sim_gen(R, K, Q, L)	CAT(G2_LOWER, mul_sim_gen)(R, K, Q, L)

/**
 * Maps a byte array to an element in G_1.
 *
 * @param[out] P			- the result.
 * @param[in] M				- the byte array to map.
 * @param[in] L				- the array length in bytes.
 */
#define g1_map(P, M, L);	CAT(G1_LOWER, map)(P, M, L)

/**
 * Maps a byte array to an element in G_2.
 *
 * @param[out] P			- the result.
 * @param[in] M				- the byte array to map.
 * @param[in] L				- the array length in bytes.
 * @param[in] H				- whether to hash internally.
 */
#define g2_map(P, M, L, H);	CAT(G2_LOWER, map)(P, M, L, H)

/**
 * Computes the bilinear pairing of a G_1 element and a G_2 element. Computes
 * R = e(P, Q).
 *
 * @param[out] R			- the result.
 * @param[in] P				- the first element.
 * @param[in] Q				- the second element.
 */
#if FP_PRIME < 1536
#define pc_map(R, P, Q);		CAT(PC_LOWER, map_k12)(R, P, Q)
#else
#define pc_map(R, P, Q);		CAT(PC_LOWER, map_k2)(R, P, Q)
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
#define pc_map_sim(R, P, Q, M);	CAT(PC_LOWER, map_sim_k12)(R, P, Q, M)
#else
#define pc_map_sim(R, P, Q, M);	CAT(PC_LOWER, map_sim_k2)(R, P, Q, M)
#endif

/**
 * Computes the final exponentiation of the pairing.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the field element to exponentiate.
 */
#if FP_PRIME < 1536
#define pc_exp(C, A);			CAT(PC_LOWER, exp_k12)(C, A)
#else
#define pc_exp(C, A);			CAT(PC_LOWER, exp_k2)(C, A)
#endif

/*============================================================================*/
/* Function prototypes                                                        */
/*============================================================================*/

/**
 * Assigns a random value to a G_T element.
 *
 * @param[out] a			- the element to assign.
 */
void gt_rand(gt_t a);

/**
 * Powers an element from G_T.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the element to exponentiate.
 * @param[in] b				- the integer exponent.
 */
void gt_exp(gt_t c, gt_t a, bn_t b);

 /**
  * Returns the generator of the group G_T.
  *
  * @param[out] G			- the returned generator.
  */
void gt_get_gen(gt_t a);

#endif /* !RELIC_PC_H */
