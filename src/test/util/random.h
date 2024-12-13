// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_UTIL_RANDOM_H
#define BITCOIN_TEST_UTIL_RANDOM_H

#include <consensus/amount.h>
#include <random.h>
#include <uint256.h>

#include <atomic>
#include <cstdint>

enum class SeedRand {
    /**
     * Seed with a compile time constant of zeros.
     */
    ZEROS,
    /**
     * Seed with a fixed value that never changes over the lifetime of this
     * process. The seed is read from the RANDOM_CTX_SEED environment variable
     * if set, otherwise generated randomly once, saved, and reused.
     */
    FIXED_SEED,
};

/** Seed the global RNG state for testing and log the seed value. This affects all randomness, except GetStrongRandBytes(). */
void SeedRandomStateForTest(SeedRand seed);

extern std::atomic<bool> g_seeded_g_prng_zero;
extern std::atomic<bool> g_used_g_prng;

template <RandomNumberGenerator Rng>
inline CAmount RandMoney(Rng&& rng)
{
    return CAmount{rng.randrange(MAX_MONEY + 1)};
}

#endif // BITCOIN_TEST_UTIL_RANDOM_H
