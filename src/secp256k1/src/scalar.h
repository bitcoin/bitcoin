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

/** Access bits from a scalar. */
static int secp256k1_scalar_get_bits(const secp256k1_scalar_t *a, int offset, int count);

/** Set a scalar from a big endian byte array. */
static void secp256k1_scalar_set_b32(secp256k1_scalar_t *r, const unsigned char *bin, int *overflow);

/** Convert a scalar to a byte array. */
static void secp256k1_scalar_get_b32(unsigned char *bin, const secp256k1_scalar_t* a);

/** Add two scalars together (modulo the group order). */
static void secp256k1_scalar_add(secp256k1_scalar_t *r, const secp256k1_scalar_t *a, const secp256k1_scalar_t *b);

/** Multiply two scalars (modulo the group order). */
static void secp256k1_scalar_mul(secp256k1_scalar_t *r, const secp256k1_scalar_t *a, const secp256k1_scalar_t *b);

/** Compute the square of a scalar (modulo the group order). */
static void secp256k1_scalar_sqr(secp256k1_scalar_t *r, const secp256k1_scalar_t *a);

/** Compute the inverse of a scalar (modulo the group order). */
static void secp256k1_scalar_inverse(secp256k1_scalar_t *r, const secp256k1_scalar_t *a);

/** Compute the complement of a scalar (modulo the group order). */
static void secp256k1_scalar_negate(secp256k1_scalar_t *r, const secp256k1_scalar_t *a);

/** Check whether a scalar equals zero. */
static int secp256k1_scalar_is_zero(const secp256k1_scalar_t *a);

/** Check whether a scalar equals one. */
static int secp256k1_scalar_is_one(const secp256k1_scalar_t *a);

/** Check whether a scalar is higher than the group order divided by 2. */
static int secp256k1_scalar_is_high(const secp256k1_scalar_t *a);

/** Convert a scalar to a number. */
static void secp256k1_scalar_get_num(secp256k1_num_t *r, const secp256k1_scalar_t *a);

#endif
