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

    LOCK(::cs_main);
    auto& view = chainstate.CoinsTip();

    // An empty coins cache still allocates one PoolResource 'chunk', which is 256 KiB, and there
    // is some overhead. As a sanity check, an empty coins cache should be only slightly larger.
    BOOST_CHECK_LT(view.DynamicMemoryUsage(), 256 * 1024 * 100 / 98);

    // PoolResource defaults to 256 KiB that will be allocated, so we need to test with a size
    // significantly larger that size so that fragmentation effects are minimal.
    // But this size can still be much smaller than the default coins cache size, 450 MB.
    constexpr size_t MAX_COINS_CACHE_BYTES{8'000'000};

    // The coins cache can also use the mempool.
    constexpr size_t MAX_MEMPOOL_CACHE_BYTES{4'000'000};

    // Check GetCoinsCacheSizeState() with an empty coins cache.
    const CoinsCacheSizeState empty_cache{chainstate.GetCoinsCacheSizeState(MAX_COINS_CACHE_BYTES, /*max_mempool_size_bytes=*/0)};

    // Without any coins in the cache, we shouldn't need to flush.
    BOOST_CHECK_NE(empty_cache, CoinsCacheSizeState::CRITICAL);

    // And we shouldn't be within 10% of full.
    BOOST_CHECK_NE(empty_cache, CoinsCacheSizeState::LARGE);

    // There should be plenty of space.
    BOOST_CHECK_EQUAL(empty_cache, CoinsCacheSizeState::OK);

    // Add coins until the cache is just under 90% full; the returned state should
    // be OK until 90%. The loop counter prevents a runaway test.
    size_t i;
    for (i = 0; i < 80'000; ++i) {
        AddTestCoin(m_rng, view);

        // Fill to about 89% (below 90%) of the cache size limit.
        if (view.DynamicMemoryUsage() >= MAX_COINS_CACHE_BYTES * 89 / 100) break;

        BOOST_REQUIRE_EQUAL(
            chainstate.GetCoinsCacheSizeState(MAX_COINS_CACHE_BYTES, /*max_mempool_size_bytes=*/0),
            CoinsCacheSizeState::OK);
    }
    BOOST_CHECK_LT(i, 80'000);

    // This loop transitions the cache state from OK (< 90%) to LARGE (between 90% and 100%).
    for (i = 0; i < 80'000; ++i) {
        AddTestCoin(m_rng, view);

        // Fill to about 95% of the cache size limit.
        if (view.DynamicMemoryUsage() >= MAX_COINS_CACHE_BYTES * 95 / 100) break;
    }
    BOOST_CHECK_LT(i, 80'000);
    BOOST_CHECK_EQUAL(
        chainstate.GetCoinsCacheSizeState(MAX_COINS_CACHE_BYTES, /*max_mempool_size_bytes=*/0),
        CoinsCacheSizeState::LARGE);

    // This loop moves the cache state from LARGE (90% to 100%) to CRITICAL (over 100%).
    for (i = 0; i < 80'000; ++i) {
        AddTestCoin(m_rng, view);

        // Fill to just beyojnd the cache size limit.
        if (view.DynamicMemoryUsage() > MAX_COINS_CACHE_BYTES) break;
    }
    BOOST_CHECK_LT(i, 80'000);
    BOOST_CHECK_EQUAL(
        chainstate.GetCoinsCacheSizeState(MAX_COINS_CACHE_BYTES, /*max_mempool_size_bytes=*/0),
        CoinsCacheSizeState::CRITICAL);

    // Repeat (continuing with the existing cache) but with a non-zero max mempool;
    // this allows more headroom.
    BOOST_CHECK_EQUAL(
        chainstate.GetCoinsCacheSizeState(MAX_COINS_CACHE_BYTES, /*max_mempool_size_bytes=*/MAX_MEMPOOL_CACHE_BYTES),
        CoinsCacheSizeState::OK);

    for (i = 0; i < 80'000; ++i) {
        AddTestCoin(m_rng, view);
        // Fill to about 89% (below 90%) of the cache size limit.
        if (view.DynamicMemoryUsage() >= (MAX_COINS_CACHE_BYTES + MAX_MEMPOOL_CACHE_BYTES) * 89 / 100) break;

        BOOST_REQUIRE_EQUAL(
            chainstate.GetCoinsCacheSizeState(MAX_COINS_CACHE_BYTES, /*max_mempool_size_bytes=*/MAX_MEMPOOL_CACHE_BYTES),
            CoinsCacheSizeState::OK);
    }
    BOOST_CHECK_LT(i, 80'000);

    // This loop transitions the cache state from OK (< 90%) to LARGE (between 90% and 100%).
    for (i = 0; i < 80'000; ++i) {
        AddTestCoin(m_rng, view);

        // Fill to about 95% of the cache size limit..
        if (view.DynamicMemoryUsage() >= (MAX_COINS_CACHE_BYTES + MAX_MEMPOOL_CACHE_BYTES) * 95 / 100) break;
    }
    BOOST_CHECK_LT(i, 80'000);
    BOOST_CHECK_EQUAL(
        chainstate.GetCoinsCacheSizeState(MAX_COINS_CACHE_BYTES, /*max_mempool_size_bytes=*/MAX_MEMPOOL_CACHE_BYTES),
        CoinsCacheSizeState::LARGE);

    // This loop moves the cache state from LARGE (90% to 100%) to CRITICAL (over 100%).
    for (i = 0; i < 80'000; ++i) {
        AddTestCoin(m_rng, view);

        // Fill to just beyojnd the cache size limit.
        if (view.DynamicMemoryUsage() > MAX_COINS_CACHE_BYTES + MAX_MEMPOOL_CACHE_BYTES) break;
    }
    BOOST_CHECK_LT(i, 80'000);
    BOOST_CHECK_EQUAL(
        chainstate.GetCoinsCacheSizeState(MAX_COINS_CACHE_BYTES, /*max_mempool_size_bytes=*/MAX_MEMPOOL_CACHE_BYTES),
        CoinsCacheSizeState::CRITICAL);


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

    BOOST_CHECK_EQUAL(
        chainstate.GetCoinsCacheSizeState(MAX_COINS_CACHE_BYTES, 0),
        CoinsCacheSizeState::OK);
}

BOOST_AUTO_TEST_SUITE_END()
