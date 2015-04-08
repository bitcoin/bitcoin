/**********************************************************************
 * Copyright (c) 2014 Pieter Wuille                                   *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef _SECP256K1_SCALAR_
#define _SECP256K1_SCALAR_

#include "num.h"

#if defined HAVE_CONFIG_H
#include "libsecp256k1-config.h"
#endif

#if defined(USE_SCALAR_4X64)
#include "scalar_4x64.h"
#elif defined(USE_SCALAR_8X32)
#include "scalar_8x32.h"
#else
#error "Please select scalar implementation"
#endif

/** Clear a scalar to prevent the leak of sensitive data. */
static void secp256k1_scalar_clear(secp256k1_scalar_t *r);

/** Access bits from a scalar. All requested bits must belong to the same 32-bit limb. */
static unsigned int secp256k1_scalar_get_bits(const secp256k1_scalar_t *a, unsigned int offset, unsigned int count);

/** Access bits from a scalar. Not constant time. */
static unsigned int secp256k1_scalar_get_bits_var(const secp256k1_scalar_t *a, unsigned int offset, unsigned int count);

/** Set a scalar from a big endian byte array. */
static void secp256k1_scalar_set_b32(secp256k1_scalar_t *r, const unsigned char *bin, int *overflow);

/** Set a scalar to an unsigned integer. */
static void secp256k1_scalar_set_int(secp256k1_scalar_t *r, unsigned int v);

/** Convert a scalar to a byte array. */
static void secp256k1_scalar_get_b32(unsigned char *bin, const secp256k1_scalar_t* a);

/** Add two scalars together (modulo the group order). Returns whether it overflowed. */
static int secp256k1_scalar_add(secp256k1_scalar_t *r, const secp256k1_scalar_t *a, const secp256k1_scalar_t *b);

/** Add a power of two to a scalar. The result is not allowed to overflow. */
static void secp256k1_scalar_add_bit(secp256k1_scalar_t *r, unsigned int bit);

/** Multiply two scalars (modulo the group order). */
static void secp256k1_scalar_mul(secp256k1_scalar_t *r, const secp256k1_scalar_t *a, const secp256k1_scalar_t *b);

/** Compute the square of a scalar (modulo the group order). */
static void secp256k1_scalar_sqr(secp256k1_scalar_t *r, const secp256k1_scalar_t *a);

/** Compute the inverse of a scalar (modulo the group order). */
static void secp256k1_scalar_inverse(secp256k1_scalar_t *r, const secp256k1_scalar_t *a);

/** Compute the inverse of a scalar (modulo the group order), without constant-time guarantee. */
static void secp256k1_scalar_inverse_var(secp256k1_scalar_t *r, const secp256k1_scalar_t *a);

/** Compute the complement of a scalar (modulo the group order). */
static void secp256k1_scalar_negate(secp256k1_scalar_t *r, const secp256k1_scalar_t *a);

/** Check whether a scalar equals zero. */
static int secp256k1_scalar_is_zero(const secp256k1_scalar_t *a);

/** Check whether a scalar equals one. */
static int secp256k1_scalar_is_one(const secp256k1_scalar_t *a);

/** Check whether a scalar is higher than the group order divided by 2. */
static int secp256k1_scalar_is_high(const secp256k1_scalar_t *a);

#ifndef USE_NUM_NONE
/** Convert a scalar to a number. */
static void secp256k1_scalar_get_num(secp256k1_num_t *r, const secp256k1_scalar_t *a);

/** Get the order of the group as a number. */
static void secp256k1_scalar_order_get_num(secp256k1_num_t *r);
#endif

/** Compare two scalars. */
static int secp256k1_scalar_eq(const secp256k1_scalar_t *a, const secp256k1_scalar_t *b);

#ifdef USE_ENDOMORPHISM
/** Find r1 and r2 such that r1+r2*2^128 = a. */
static void secp256k1_scalar_split_128(secp256k1_scalar_t *r1, secp256k1_scalar_t *r2, const secp256k1_scalar_t *a);
/** Find r1 and r2 such that r1+r2*lambda = a, and r1 and r2 are maximum 128 bits long (see secp256k1_gej_mul_lambda). */
static void secp256k1_scalar_split_lambda_var(secp256k1_scalar_t *r1, secp256k1_scalar_t *r2, const secp256k1_scalar_t *a);
#endif

/** Multiply a and b (without taking the modulus!), divide by 2**shift, and round to the nearest integer. Shift must be at least 256. */
static void secp256k1_scalar_mul_shift_var(secp256k1_scalar_t *r, const secp256k1_scalar_t *a, const secp256k1_scalar_t *b, unsigned int shift);

#endif
