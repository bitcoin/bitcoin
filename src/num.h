// Copyright (c) 2013 Pieter Wuille
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef _SECP256K1_NUM_
#define _SECP256K1_NUM_

#if defined(USE_NUM_GMP)
#include "num_gmp.h"
#elif defined(USE_NUM_OPENSSL)
#include "num_openssl.h"
#else
#error "Please select num implementation"
#endif

/** Initialize a number. */
void static secp256k1_num_init(secp256k1_num_t *r);

/** Free a number. */
void static secp256k1_num_free(secp256k1_num_t *r);

/** Copy a number. */
void static secp256k1_num_copy(secp256k1_num_t *r, const secp256k1_num_t *a);

/** Convert a number's absolute value to a binary big-endian string.
 *  There must be enough place. */
void static secp256k1_num_get_bin(unsigned char *r, unsigned int rlen, const secp256k1_num_t *a);

/** Set a number to the value of a binary big-endian string. */
void static secp256k1_num_set_bin(secp256k1_num_t *r, const unsigned char *a, unsigned int alen);

/** Set a number equal to a (signed) integer. */
void static secp256k1_num_set_int(secp256k1_num_t *r, int a);

/** Compute a modular inverse. The input must be less than the modulus. */
void static secp256k1_num_mod_inverse(secp256k1_num_t *r, const secp256k1_num_t *a, const secp256k1_num_t *m);

/** Multiply two numbers modulo another. */
void static secp256k1_num_mod_mul(secp256k1_num_t *r, const secp256k1_num_t *a, const secp256k1_num_t *b, const secp256k1_num_t *m);

/** Compare the absolute value of two numbers. */
int  static secp256k1_num_cmp(const secp256k1_num_t *a, const secp256k1_num_t *b);

/** Add two (signed) numbers. */
void static secp256k1_num_add(secp256k1_num_t *r, const secp256k1_num_t *a, const secp256k1_num_t *b);

/** Subtract two (signed) numbers. */
void static secp256k1_num_sub(secp256k1_num_t *r, const secp256k1_num_t *a, const secp256k1_num_t *b);

/** Multiply two (signed) numbers. */
void static secp256k1_num_mul(secp256k1_num_t *r, const secp256k1_num_t *a, const secp256k1_num_t *b);

/** Divide two (signed) numbers. */
void static secp256k1_num_div(secp256k1_num_t *r, const secp256k1_num_t *a, const secp256k1_num_t *b);

/** Replace a number by its remainder modulo m. M's sign is ignored. The result is a number between 0 and m-1,
    even if r was negative. */
void static secp256k1_num_mod(secp256k1_num_t *r, const secp256k1_num_t *m);

/** Calculate the number of bits in (the absolute value of) a number. */
int  static secp256k1_num_bits(const secp256k1_num_t *a);

/** Right-shift the passed number by bits bits, and return those bits. */
int  static secp256k1_num_shift(secp256k1_num_t *r, int bits);

/** Check whether a number is zero. */
int  static secp256k1_num_is_zero(const secp256k1_num_t *a);

/** Check whether a number is odd. */
int  static secp256k1_num_is_odd(const secp256k1_num_t *a);

/** Check whether a number is strictly negative. */
int  static secp256k1_num_is_neg(const secp256k1_num_t *a);

/** Check whether a particular bit is set in a number. */
int  static secp256k1_num_get_bit(const secp256k1_num_t *a, int pos);

/** Increase a number by 1. */
void static secp256k1_num_inc(secp256k1_num_t *r);

/** Set a number equal to the value of a hex string (unsigned). */
void static secp256k1_num_set_hex(secp256k1_num_t *r, const char *a, int alen);

/** Convert (the absolute value of) a number to a hexadecimal string. */
void static secp256k1_num_get_hex(char *r, int rlen, const secp256k1_num_t *a);

/** Split a number into a low and high part. */
void static secp256k1_num_split(secp256k1_num_t *rl, secp256k1_num_t *rh, const secp256k1_num_t *a, int bits);

/** Change a number's sign. */
void static secp256k1_num_negate(secp256k1_num_t *r);

#endif
