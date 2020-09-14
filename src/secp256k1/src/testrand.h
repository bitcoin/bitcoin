/**********************************************************************
 * Copyright (c) 2013, 2014 Pieter Wuille                             *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef SECP256K1_TESTRAND_H
#define SECP256K1_TESTRAND_H

#if defined HAVE_CONFIG_H
#include "libsecp256k1-config.h"
#endif

/* A non-cryptographic RNG used only for test infrastructure. */

/** Seed the pseudorandom number generator for testing. */
SECP256K1_INLINE static void secp256k1_rand_seed(const unsigned char *seed16);

/** Generate a pseudorandom number in the range [0..2**32-1]. */
static uint32_t secp256k1_rand32(void);

/** Generate a pseudorandom number in the range [0..2**bits-1]. Bits must be 1 or
 *  more. */
static uint32_t secp256k1_rand_bits(int bits);

/** Generate a pseudorandom number in the range [0..range-1]. */
static uint32_t secp256k1_rand_int(uint32_t range);

/** Generate a pseudorandom 32-byte array. */
static void secp256k1_rand256(unsigned char *b32);

/** Generate a pseudorandom 32-byte array with long sequences of zero and one bits. */
static void secp256k1_rand256_test(unsigned char *b32);

/** Generate pseudorandom bytes with long sequences of zero and one bits. */
static void secp256k1_rand_bytes_test(unsigned char *bytes, size_t len);

/** Flip a single random bit in a byte array */
static void secp256k1_rand_flip(unsigned char *b, size_t len);

#endif /* SECP256K1_TESTRAND_H */
