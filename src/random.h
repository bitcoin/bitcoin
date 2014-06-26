// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RANDOM_H
#define BITCOIN_RANDOM_H

#include "uint256.h"

#include <stdint.h>

/**
 * Seed OpenSSL PRNG with additional entropy data
 */
void RandAddSeed();
void RandAddSeedPerfmon();

/**
 * Functions to gather random data via the OpenSSL PRNG
 */
bool GetRandBytes(unsigned char *buf, int num);
uint64_t GetRand(uint64_t nMax);
int GetRandInt(int nMax);
uint256 GetRandHash();

/**
 * Seed insecure_rand using the random pool.
 * @param Deterministic Use a determinstic seed
 */
void seed_insecure_rand(bool fDeterministic = false);

/**
 * MWC RNG of George Marsaglia
 * This is intended to be fast. It has a period of 2^59.3, though the
 * least significant 16 bits only have a period of about 2^30.1.
 *
 * @return random value
 */
extern uint32_t insecure_rand_Rz;
extern uint32_t insecure_rand_Rw;
static inline uint32_t insecure_rand(void)
{
    insecure_rand_Rz = 36969 * (insecure_rand_Rz & 65535) + (insecure_rand_Rz >> 16);
    insecure_rand_Rw = 18000 * (insecure_rand_Rw & 65535) + (insecure_rand_Rw >> 16);
    return (insecure_rand_Rw << 16) + insecure_rand_Rz;
}

#endif // BITCOIN_RANDOM_H
