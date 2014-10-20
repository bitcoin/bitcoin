// Copyright (c) 2013 Pieter Wuille
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef _SECP256K1_UTIL_H_
#define _SECP256K1_UTIL_H_

#if defined HAVE_CONFIG_H
#include "libsecp256k1-config.h"
#endif

#include <stdint.h>
#include <stdio.h>

#define TEST_FAILURE(msg) do { \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, msg); \
    abort(); \
} while(0)

#ifndef HAVE_BUILTIN_EXPECT
#define EXPECT(x,c) __builtin_expect((x),(c))
#else
#define EXPECT(x,c) (x)
#endif

#define CHECK(cond) do { \
    if (EXPECT(!(cond), 0)) { \
        TEST_FAILURE("test condition failed: " #cond); \
    } \
} while(0)

// Like assert(), but safe to use on expressions with side effects.
#ifndef NDEBUG
#define DEBUG_CHECK CHECK
#else
#define DEBUG_CHECK(cond) do { (cond); } while(0)
#endif

// Like DEBUG_CHECK(), but when VERIFY is defined instead of NDEBUG not defined.
#ifdef VERIFY
#define VERIFY_CHECK CHECK
#else
#define VERIFY_CHECK(cond) do { (cond); } while(0)
#endif

/** Seed the pseudorandom number generator. */
static inline void secp256k1_rand_seed(uint64_t v);

/** Generate a pseudorandom 32-bit number. */
static uint32_t secp256k1_rand32(void);

/** Generate a pseudorandom 32-byte array. */
static void secp256k1_rand256(unsigned char *b32);

/** Generate a pseudorandom 32-byte array with long sequences of zero and one bits. */
static void secp256k1_rand256_test(unsigned char *b32);

#endif
