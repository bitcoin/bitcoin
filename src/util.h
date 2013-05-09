// Copyright (c) 2013 Pieter Wuille
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef _SECP256K1_UTIL_H_
#define _SECP256K1_UTIL_H_

/** Generate a pseudorandom 32-bit number. */
static uint32_t secp256k1_rand32(void);

/** Generate a pseudorandom 32-byte array. */
static void secp256k1_rand256(unsigned char *b32);

/** Generate a pseudorandom 32-byte array with long sequences of zero and one bits. */
static void secp256k1_rand256_test(unsigned char *b32);

#include "impl/util.h"

#endif
