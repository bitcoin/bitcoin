/***********************************************************************
 * Copyright (c) 2013, 2014 Pieter Wuille                              *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_TESTRAND_H
#define SECP256K1_TESTRAND_H

/* A non-cryptographic RNG used only for test infrastructure. */

/** Seed the pseudorandom number generator for testing. */
SECP256K1_INLINE static void secp256k1_testrand_seed(const unsigned char *seed16);

/** Generate a pseudorandom number in the range [0..2**32-1]. */
SECP256K1_INLINE static uint32_t secp256k1_testrand32(void);

/** Generate a pseudorandom number in the range [0..2**64-1]. */
SECP256K1_INLINE static uint64_t secp256k1_testrand64(void);

/** Generate a pseudorandom number in the range [0..2**bits-1]. Bits must be 1 or
 *  more. */
SECP256K1_INLINE static uint64_t secp256k1_testrand_bits(int bits);

/** Generate a pseudorandom number in the range [0..range-1]. */
static uint32_t secp256k1_testrand_int(uint32_t range);

/** Generate a pseudorandom 32-byte array. */
static void secp256k1_testrand256(unsigned char *b32);

/** Generate a pseudorandom 32-byte array with long sequences of zero and one bits. */
static void secp256k1_testrand256_test(unsigned char *b32);

/** Generate pseudorandom bytes with long sequences of zero and one bits. */
static void secp256k1_testrand_bytes_test(unsigned char *bytes, size_t len);

/** Flip a single random bit in a byte array */
static void secp256k1_testrand_flip(unsigned char *b, size_t len);

/** Initialize the test RNG using (hex encoded) array up to 16 bytes, or randomly if hexseed is NULL. */
static void secp256k1_testrand_init(const char* hexseed);

/** Print final test information. */
static void secp256k1_testrand_finish(void);

#endif /* SECP256K1_TESTRAND_H */
