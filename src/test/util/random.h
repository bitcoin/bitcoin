// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_UTIL_RANDOM_H
#define BITCOIN_TEST_UTIL_RANDOM_H

#include <consensus/amount.h>
#include <random.h>
#include <uint256.h>

#include <cstdint>

/**
 * This global and the helpers that use it are not thread-safe.
 *
 * If thread-safety is needed, a per-thread instance could be
 * used in the multi-threaded test.
 */
extern FastRandomContext g_rng;

enum class SeedRand {
    ZEROS, //!< Seed with a compile time constant of zeros
    SEED,  //!< Use (and report) random seed from environment, or a (truly) random one.
};

/** Seed the RNG for testing. This affects all randomness, except GetStrongRandBytes(). */
void SeedRandomForTest(SeedRand seed = SeedRand::SEED);

template <RandomNumberGenerator Rng>
inline CAmount RandMoney(Rng&& rng)
{
    return CAmount{rng.randrange(MAX_MONEY + 1)};
}

#endif // BITCOIN_TEST_UTIL_RANDOM_H
