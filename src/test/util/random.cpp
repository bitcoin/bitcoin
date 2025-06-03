// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/random.h>

#include <logging.h>
#include <random.h>
#include <uint256.h>
#include <util/check.h>

#include <cstdlib>
#include <iostream>

std::atomic<bool> g_seeded_g_prng_zero{false};

extern void MakeRandDeterministicDANGEROUS(const uint256& seed) noexcept;

void SeedRandomStateForTest(SeedRand seedtype)
{
    constexpr auto RANDOM_CTX_SEED{"RANDOM_CTX_SEED"};

    // Do this once, on the first call, regardless of seedtype, because once
    // MakeRandDeterministicDANGEROUS is called, the output of GetRandHash is
    // no longer truly random. It should be enough to get the seed once for the
    // process.
    static const auto g_ctx_seed = []() -> std::optional<uint256> {
        if (EnableFuzzDeterminism()) return {};
        // If RANDOM_CTX_SEED is set, use that as seed.
        if (const char* num{std::getenv(RANDOM_CTX_SEED)}) {
            if (auto num_parsed{uint256::FromUserHex(num)}) {
                return *num_parsed;
            } else {
                std::cerr << RANDOM_CTX_SEED << " must consist of up to " << uint256::size() * 2 << " hex digits (\"0x\" prefix allowed), it was set to: '" << num << "'.\n";
                std::abort();
            }
        }
        // Otherwise use a (truly) random value.
        return GetRandHash();
    }();

    g_seeded_g_prng_zero = seedtype == SeedRand::ZEROS;
    if (EnableFuzzDeterminism()) {
        Assert(g_seeded_g_prng_zero); // Only SeedRandomStateForTest(SeedRand::ZEROS) is allowed in fuzz tests
        Assert(!g_used_g_prng);       // The global PRNG must not have been used before SeedRandomStateForTest(SeedRand::ZEROS)
    }
    const uint256& seed{seedtype == SeedRand::FIXED_SEED ? g_ctx_seed.value() : uint256::ZERO};
    LogInfo("Setting random seed for current tests to %s=%s\n", RANDOM_CTX_SEED, seed.GetHex());
    MakeRandDeterministicDANGEROUS(seed);
}
