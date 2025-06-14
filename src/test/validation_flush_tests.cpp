// Copyright (c) 2019-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
#include <sync.h>
#include <test/util/coins.h>
#include <test/util/random.h>
#include <test/util/setup_common.h>
#include <validation.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(validation_flush_tests, TestingSetup)

//! Test utilities for detecting when we need to flush the coins cache based
//! on estimated memory usage.
//!
//! @sa Chainstate::GetCoinsCacheSizeState()
//!
BOOST_AUTO_TEST_CASE(getcoinscachesizestate)
{
    Chainstate& chainstate{m_node.chainman->ActiveChainstate()};

    constexpr bool is_64_bit = sizeof(void*) == 8;

    LOCK(::cs_main);
    auto& view = chainstate.CoinsTip();

    // The number of bytes consumed by coin's heap data, i.e.
    // CScript (prevector<36, unsigned char>) when assigned 56 bytes of data per above.
    // See also: Coin::DynamicMemoryUsage().
    constexpr unsigned int COIN_SIZE = is_64_bit ? 80 : 64;

    auto print_view_mem_usage = [](CCoinsViewCache& view) {
        BOOST_TEST_MESSAGE("CCoinsViewCache memory usage: " << view.DynamicMemoryUsage());
    };

    // PoolResource defaults to 256 KiB that will be allocated, so we'll take that and make it a bit larger.
    constexpr size_t MAX_COINS_CACHE_BYTES = 262144 + 512;

    // Without any coins in the cache, we shouldn't need to flush.
    BOOST_TEST(
        chainstate.GetCoinsCacheSizeState(MAX_COINS_CACHE_BYTES, /*max_mempool_size_bytes=*/ 0) != CoinsCacheSizeState::CRITICAL);

    // If the initial memory allocations of cacheCoins don't match these common
    // cases, we can't really continue to make assertions about memory usage.
    // End the test early.
    if (view.DynamicMemoryUsage() != 32 && view.DynamicMemoryUsage() != 16) {
        // Add a bunch of coins to see that we at least flip over to CRITICAL.

        for (int i{0}; i < 1000; ++i) {
            const COutPoint res = AddTestCoin(m_rng, view);
            BOOST_CHECK_EQUAL(view.AccessCoin(res).DynamicMemoryUsage(), COIN_SIZE);
        }

        BOOST_CHECK_EQUAL(
            chainstate.GetCoinsCacheSizeState(MAX_COINS_CACHE_BYTES, /*max_mempool_size_bytes=*/0),
            CoinsCacheSizeState::CRITICAL);

        BOOST_TEST_MESSAGE("Exiting cache flush tests early due to unsupported arch");
        return;
    }

    print_view_mem_usage(view);
    BOOST_CHECK_EQUAL(view.DynamicMemoryUsage(), is_64_bit ? 32U : 16U);

    // We should be able to add COINS_UNTIL_CRITICAL coins to the cache before going CRITICAL.
    // This is contingent not only on the dynamic memory usage of the Coins
    // that we're adding (COIN_SIZE bytes per), but also on how much memory the
    // cacheCoins (unordered_map) preallocates.
    constexpr int COINS_UNTIL_CRITICAL{3};

    // no coin added, so we have plenty of space left.
    BOOST_CHECK_EQUAL(
        chainstate.GetCoinsCacheSizeState(MAX_COINS_CACHE_BYTES, /*max_mempool_size_bytes*/ 0),
        CoinsCacheSizeState::OK);

    for (int i{0}; i < COINS_UNTIL_CRITICAL; ++i) {
        const COutPoint res = AddTestCoin(m_rng, view);
        print_view_mem_usage(view);
        BOOST_CHECK_EQUAL(view.AccessCoin(res).DynamicMemoryUsage(), COIN_SIZE);

        // adding first coin causes the MemoryResource to allocate one 256 KiB chunk of memory,
        // pushing us immediately over to LARGE
        BOOST_CHECK_EQUAL(
            chainstate.GetCoinsCacheSizeState(MAX_COINS_CACHE_BYTES, /*max_mempool_size_bytes=*/ 0),
            CoinsCacheSizeState::LARGE);
    }

    // Adding some additional coins will push us over the edge to CRITICAL.
    for (int i{0}; i < 4; ++i) {
        AddTestCoin(m_rng, view);
        print_view_mem_usage(view);
        if (chainstate.GetCoinsCacheSizeState(MAX_COINS_CACHE_BYTES, /*max_mempool_size_bytes=*/0) ==
            CoinsCacheSizeState::CRITICAL) {
            break;
        }
    }

    BOOST_CHECK_EQUAL(
        chainstate.GetCoinsCacheSizeState(MAX_COINS_CACHE_BYTES, /*max_mempool_size_bytes=*/0),
        CoinsCacheSizeState::CRITICAL);

    // Passing non-zero max mempool usage (512 KiB) should allow us more headroom.
    BOOST_CHECK_EQUAL(
        chainstate.GetCoinsCacheSizeState(MAX_COINS_CACHE_BYTES, /*max_mempool_size_bytes=*/ 1 << 19),
        CoinsCacheSizeState::OK);

    for (int i{0}; i < 3; ++i) {
        AddTestCoin(m_rng, view);
        print_view_mem_usage(view);
        BOOST_CHECK_EQUAL(
            chainstate.GetCoinsCacheSizeState(MAX_COINS_CACHE_BYTES, /*max_mempool_size_bytes=*/ 1 << 19),
            CoinsCacheSizeState::OK);
    }

    // Adding another coin with the additional mempool room will put us >90%
    // but not yet critical.
    AddTestCoin(m_rng, view);
    print_view_mem_usage(view);

    // Only perform these checks on 64 bit hosts; I haven't done the math for 32.
    if (is_64_bit) {
        float usage_percentage = (float)view.DynamicMemoryUsage() / (MAX_COINS_CACHE_BYTES + (1 << 10));
        BOOST_TEST_MESSAGE("CoinsTip usage percentage: " << usage_percentage);
        BOOST_CHECK(usage_percentage >= 0.9);
        BOOST_CHECK(usage_percentage < 1);
        BOOST_CHECK_EQUAL(
            chainstate.GetCoinsCacheSizeState(MAX_COINS_CACHE_BYTES, /*max_mempool_size_bytes*/ 1 << 10), // 1024
            CoinsCacheSizeState::LARGE);
    }

    // Using the default max_* values permits way more coins to be added.
    for (int i{0}; i < 1000; ++i) {
        AddTestCoin(m_rng, view);
        BOOST_CHECK_EQUAL(
            chainstate.GetCoinsCacheSizeState(),
            CoinsCacheSizeState::OK);
    }

    // Flushing the view does take us back to OK because ReallocateCache() is called

    BOOST_CHECK_EQUAL(
        chainstate.GetCoinsCacheSizeState(MAX_COINS_CACHE_BYTES, 0),
        CoinsCacheSizeState::CRITICAL);

    view.SetBestBlock(m_rng.rand256());
    BOOST_CHECK(view.Flush());
    print_view_mem_usage(view);

    BOOST_CHECK_EQUAL(
        chainstate.GetCoinsCacheSizeState(MAX_COINS_CACHE_BYTES, 0),
        CoinsCacheSizeState::OK);
}

BOOST_AUTO_TEST_SUITE_END()
