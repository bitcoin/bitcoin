// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/random.h>

#include <logging.h>
#include <random.h>
#include <uint256.h>

#include <cstdlib>
#include <string>

FastRandomContext g_insecure_rand_ctx;

extern void MakeRandDeterministicDANGEROUS(const uint256& seed) noexcept;

void SeedRandomForTest(SeedRand seedtype)
{
    static const std::string RANDOM_CTX_SEED{"RANDOM_CTX_SEED"};

    // Do this once, on the first call, regardless of seedtype, because once
    // MakeRandDeterministicDANGEROUS is called, the output of GetRandHash is
    // no longer truly random. It should be enough to get the seed once for the
    // process.
    static const uint256 seed = []() {
        // If RANDOM_CTX_SEED is set, use that as seed.
        const char* num = std::getenv(RANDOM_CTX_SEED.c_str());
        if (num) return uint256S(num);
        // Otherwise use a (truly) random value.
        return GetRandHash();
    }();

    uint256 use_seed;
    if (seedtype == SeedRand::SEED) {
        LogPrintf("%s: Setting random seed for current tests to %s=%s\n", __func__, RANDOM_CTX_SEED, seed.GetHex());
        use_seed = seed;
    } else {
        use_seed = uint256::ZERO;
    }
    MakeRandDeterministicDANGEROUS(use_seed);
    g_insecure_rand_ctx = FastRandomContext(GetRandHash());
}
