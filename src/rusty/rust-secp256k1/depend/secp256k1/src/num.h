/**********************************************************************
 * Copyright (c) 2013, 2014 Pieter Wuille                             *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef SECP256K1_NUM_H
#define SECP256K1_NUM_H

#ifndef USE_NUM_NONE

#if defined HAVE_CONFIG_H
#include "libsecp256k1-config.h"
#endif

#if defined(USE_NUM_GMP)
#include "num_gmp.h"
#else
#error "Please select num implementation"
#endif

/** Copy a number. */
static void secp256k1_num_copy(secp256k1_num *r, const secp256k1_num *a);

/** Convert a number's absolute value to a binary big-endian string.
 *  There must be enough place. */
static void secp256k1_num_get_bin(unsigned char *r, unsigned int rlen, const secp256k1_num *a);

/** Set a number to the value of a binary big-endian string. */
static void secp256k1_num_set_bin(secp256k1_num *r, const unsigned char *a, unsigned int alen);

/** Compute a modular inverse. The input must be less than the modulus. */
static void secp256k1_num_mod_inverse(secp256k1_num *r, const secp256k1_num *a, const secp256k1_num *m);

/** Compute the jacobi symbol (a|b). b must be positive and odd. */
static int secp256k1_num_jacobi(const secp256k1_num *a, const secp256k1_num *b);

/** Compare the absolute value of two numbers. */
static int secp256k1_num_cmp(const secp256k1_num *a, const secp256k1_num *b);

/** Test whether two number are equal (including sign). */
static int secp256k1_num_eq(const secp256k1_num *a, const secp256k1_num *b);

/** Add two (signed) numbers. */
static void secp256k1_num_add(secp256k1_num *r, const secp256k1_num *a, const secp256k1_num *b);

/** Subtract two (signed) numbers. */
static void secp256k1_num_sub(secp256k1_num *r, const secp256k1_num *a, const secp256k1_num *b);

/** Multiply two (signed) numbers. */
static void secp256k1_num_mul(secp256k1_num *r, const secp256k1_num *a, const secp256k1_num *b);

/** Replace a number by its remainder modulo m. M's sign is ignored. The result is a number between 0 and m-1,
    even if r was negative. */
static void secp256k1_num_mod(secp256k1_num *r, const secp256k1_num *m);

/** Right-shift the passed number by bits bits. */
static void secp256k1_num_shift(secp256k1_num *r, int bits);

/** Check whether a number is zero. */
static int secp256k1_num_is_zero(const secp256k1_num *a);

/** Check whether a number is one. */
static int secp256k1_num_is_one(const secp256k1_num *a);

/** Check whether a number is strictly negative. */
static int secp256k1_num_is_neg(const secp256k1_num *a);

/** Change a number's sign. */
static void secp256k1_num_negate(secp256k1_num *r);

#endif

#endif /* SECP256K1_NUM_H */
