// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RANDOM_H
#define BITCOIN_RANDOM_H

#include "uint256.h"

#include <stdint.h>

enum RandEventSource
{
    RAND_EVENT_NETWORK = 1,
    RAND_EVENT_PROCESSING = 2,
    RAND_EVENT_RPC = 3,
};

/** Add data from a random event.
 *  Only use this for regularly-occurring events.
 */
void RandEvent(RandEventSource source, const unsigned char* data, size_t size);

/** Add entropy to the pool directly. Use this for seeding or on-demand entropy. */
void RandSeed(const unsigned char* data, size_t);

/** Reset the PRNG to a known state. Never use this outside of testing. */
void RandSeedDeterministic();

/** Add periodic entropy from various system-dependent sources to the pool. */
void RandSeedSystem(bool slow = false);

/** Request various types of random data */
void GetRandBytes(unsigned char* buf, int num);
void GetStrongRandBytes(unsigned char* buf, int num);
uint64_t GetRand(uint64_t nMax);
int GetRandInt(int nMax);
uint32_t GetInsecureRand();
uint256 GetRandHash();

#endif // BITCOIN_RANDOM_H
