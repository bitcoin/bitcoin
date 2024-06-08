// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_UTIL_XOROSHIRO128PLUSPLUS_H
#define BITCOIN_TEST_UTIL_XOROSHIRO128PLUSPLUS_H

#include <bit>
#include <cstdint>
#include <limits>

/** xoroshiro128++ PRNG. Extremely fast, not appropriate for cryptographic purposes.
 *
 * Memory footprint is 128bit, period is 2^128 - 1.
 * This class is not thread-safe.
 *
 * Reference implementation available at https://prng.di.unimi.it/xoroshiro128plusplus.c
 * See https://prng.di.unimi.it/
 */
class XoRoShiRo128PlusPlus
{
    uint64_t m_s0;
    uint64_t m_s1;

    [[nodiscard]] constexpr static uint64_t SplitMix64(uint64_t& seedval) noexcept
    {
        uint64_t z = (seedval += 0x9e3779b97f4a7c15);
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
        z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
        return z ^ (z >> 31);
    }

public:
    using result_type = uint64_t;

    constexpr explicit XoRoShiRo128PlusPlus(uint64_t seedval) noexcept
        : m_s0(SplitMix64(seedval)), m_s1(SplitMix64(seedval)) {}

    // no copy - that is dangerous, we don't want accidentally copy the RNG and then have two streams
    // with exactly the same results.
    XoRoShiRo128PlusPlus(const XoRoShiRo128PlusPlus&) = delete;
    XoRoShiRo128PlusPlus& operator=(const XoRoShiRo128PlusPlus&) = delete;

    // allow moves
    XoRoShiRo128PlusPlus(XoRoShiRo128PlusPlus&&) = default;
    XoRoShiRo128PlusPlus& operator=(XoRoShiRo128PlusPlus&&) = default;

    constexpr result_type operator()() noexcept
    {
        uint64_t s0 = m_s0, s1 = m_s1;
        const uint64_t result = std::rotl(s0 + s1, 17) + s0;
        s1 ^= s0;
        m_s0 = std::rotl(s0, 49) ^ s1 ^ (s1 << 21);
        m_s1 = std::rotl(s1, 28);
        return result;
    }

    static constexpr result_type min() noexcept { return std::numeric_limits<result_type>::min(); }
    static constexpr result_type max() noexcept { return std::numeric_limits<result_type>::max(); }
    static constexpr double entropy() noexcept { return 0.0; }
};

#endif // BITCOIN_TEST_UTIL_XOROSHIRO128PLUSPLUS_H
