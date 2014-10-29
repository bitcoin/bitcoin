// Copyright (c) 2014 Pieter Wuille
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef _SECP256K1_SCALAR_
#define _SECP256K1_SCALAR_

#include "num.h"

/** A scalar modulo the group order of the secp256k1 curve. */
typedef struct {
    secp256k1_num_t n;
} secp256k1_scalar_t;

/** Initialize a scalar. */
void static secp256k1_scalar_init(secp256k1_scalar_t *r);

/** Clear a scalar to prevent the leak of sensitive data. */
void static secp256k1_scalar_clear(secp256k1_scalar_t *r);

/** Free a scalar. */
void static secp256k1_scalar_free(secp256k1_scalar_t *r);

/** Access bits from a scalar. */
int static secp256k1_scalar_get_bits(const secp256k1_scalar_t *a, int offset, int count);

/** Set a scalar from a big endian byte array. */
void static secp256k1_scalar_set_bin(secp256k1_scalar_t *r, const unsigned char *bin, int len, int *overflow);

/** Convert a scalar to a byte array. */
void static secp256k1_scalar_get_bin(unsigned char *bin, int len, const secp256k1_scalar_t* a);

/** Add two scalars together (modulo the group order). */
void static secp256k1_scalar_add(secp256k1_scalar_t *r, const secp256k1_scalar_t *a, const secp256k1_scalar_t *b);

/** Multiply two scalars (modulo the group order). */
void static secp256k1_scalar_mul(secp256k1_scalar_t *r, const secp256k1_scalar_t *a, const secp256k1_scalar_t *b);

/** Compute the inverse of a scalar (modulo the group order). */
void static secp256k1_scalar_inverse(secp256k1_scalar_t *r, const secp256k1_scalar_t *a);

/** Compute the complement of a scalar (modulo the group order). */
void static secp256k1_scalar_negate(secp256k1_scalar_t *r, const secp256k1_scalar_t *a);

/** Check whether a scalar equals zero. */
int static secp256k1_scalar_is_zero(const secp256k1_scalar_t *a);

/** Check whether a scalar is higher than the group order divided by 2. */
int static secp256k1_scalar_is_high(const secp256k1_scalar_t *a);

/** Convert a scalar to a number. */
void static secp256k1_scalar_get_num(secp256k1_num_t *r, const secp256k1_scalar_t *a);

#endif
