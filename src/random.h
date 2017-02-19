// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RANDOM_H
#define BITCOIN_RANDOM_H

#include "uint256.h"

#include <stdint.h>

/* Seed OpenSSL PRNG with additional entropy data */
void RandAddSeed();

/**
 * Functions to gather random data via the OpenSSL PRNG
 */
void GetRandBytes(unsigned char* buf, int num);
uint64_t GetRand(uint64_t nMax);
int GetRandInt(int nMax);
uint256 GetRandHash();

/**
 * Function to gather random data from multiple sources, failing whenever any
 * of those source fail to provide a result.
 */
void GetStrongRandBytes(unsigned char* buf, int num);

/**
 * Fast randomness source. This is seeded once with secure random data, but
 * is completely deterministic and insecure after that.
 * This class is not thread-safe.
 */
class FastRandomContext {
public:
    explicit FastRandomContext(bool fDeterministic=false);

    uint32_t rand32() {
        Rz = 36969 * (Rz & 65535) + (Rz >> 16);
        Rw = 18000 * (Rw & 65535) + (Rw >> 16);
        return (Rw << 16) + Rz;
    }

    uint32_t Rz;
    uint32_t Rw;
};

#endif // BITCOIN_RANDOM_H
