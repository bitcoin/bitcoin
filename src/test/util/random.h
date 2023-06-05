// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_UTIL_RANDOM_H
#define BITCOIN_TEST_UTIL_RANDOM_H

#include <consensus/amount.h>
#include <random.h>
#include <test/util/setup_common.h>
#include <uint256.h>

#include <cstdint>

static inline uint32_t InsecureRand32()
{
    return g_insecure_rand_ctx.rand32();
}

static inline uint256 InsecureRand256()
{
    return g_insecure_rand_ctx.rand256();
}

static inline uint64_t InsecureRandBits(int bits)
{
    return g_insecure_rand_ctx.randbits(bits);
}

static inline uint64_t InsecureRandRange(uint64_t range)
{
    return g_insecure_rand_ctx.randrange(range);
}

static inline bool InsecureRandBool()
{
    return g_insecure_rand_ctx.randbool();
}

static inline CAmount InsecureRandMoneyAmount()
{
    return static_cast<CAmount>(InsecureRandRange(MAX_MONEY + 1));
}

#endif // BITCOIN_TEST_UTIL_RANDOM_H
