// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_UTIL_RANDOM_H
#define BITCOIN_TEST_UTIL_RANDOM_H

#include <consensus/amount.h>
#include <random.h>
#include <uint256.h>

#include <cstdint>

enum class SeedRand {
    ZEROS, //!< Seed with a compile time constant of zeros
    SEED,  //!< Use (and report) random seed from environment, or a (truly) random one.
};

/** Seed the global RNG state for testing and log the seed value. This affects all randomness, except GetStrongRandBytes(). */
void SeedRandomStateForTest(SeedRand seed);

template <RandomNumberGenerator Rng>
inline CAmount RandMoney(Rng&& rng)
{
    return CAmount{rng.randrange(MAX_MONEY + 1)};
}

#endif // BITCOIN_TEST_UTIL_RANDOM_H
