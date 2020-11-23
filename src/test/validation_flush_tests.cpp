// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
#include <sync.h>
#include <test/util/setup_common.h>
#include <txmempool.h>
#include <validation.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(validation_flush_tests, BasicTestingSetup)

//! Test utilities for detecting when we need to flush the coins cache based
//! on estimated memory usage.
//!
//! @sa CChainState::GetCoinsCacheSizeState()
//!
BOOST_AUTO_TEST_CASE(getcoinscachesizestate)
{
    BlockManager blockman{};
    CChainState chainstate{blockman};
    chainstate.InitCoinsDB(/*cache_size_bytes*/ 1 << 10, /*in_memory*/ true, /*should_wipe*/ false);
    WITH_LOCK(::cs_main, chainstate.InitCoinsCache());
    CTxMemPool tx_pool{};

    constexpr bool is_64_bit = sizeof(void*) == 8;

    LOCK(::cs_main);
    auto& view = chainstate.CoinsTip();

    //! Create and add a Coin with DynamicMemoryUsage of 80 bytes to the given view.
    auto add_coin = [](CCoinsViewCache& coins_view) -> COutPoint {
        Coin newcoin;
        uint256 txid = InsecureRand256();
        COutPoint outp{txid, 0};
        newcoin.nHeight = 1;
        newcoin.out.nValue = InsecureRand32();
        newcoin.out.scriptPubKey.assign((uint32_t)56, 1);
        coins_view.AddCoin(outp, std::move(newcoin), false);

        return outp;
    };

    // The number of bytes consumed by coin's heap data, i.e. CScript
    // (prevector<28, unsigned char>) when assigned 56 bytes of data per above.
    //
    // See also: Coin::DynamicMemoryUsage().
    constexpr int COIN_SIZE = is_64_bit ? 80 : 64;

    auto print_view_mem_usage = [](CCoinsViewCache& view) {
        BOOST_TEST_MESSAGE("CCoinsViewCache memory usage: " << view.DynamicMemoryUsage());
    };

    constexpr size_t MAX_COINS_CACHE_BYTES = 1024;

    // Without any coins in the cache, we shouldn't need to flush.
    BOOST_CHECK_EQUAL(
        chainstate.GetCoinsCacheSizeState(tx_pool, MAX_COINS_CACHE_BYTES, /*max_mempool_size_bytes*/ 0),
        CoinsCacheSizeState::OK);

    // If the initial memory allocations of cacheCoins don't match these common
    // cases, we can't really continue to make assertions about memory usage.
    // End the test early.
    if (view.DynamicMemoryUsage() != 32 && view.DynamicMemoryUsage() != 16) {
        // Add a bunch of coins to see that we at least flip over to CRITICAL.

        for (int i{0}; i < 1000; ++i) {
            COutPoint res = add_coin(view);
            BOOST_CHECK_EQUAL(view.AccessCoin(res).DynamicMemoryUsage(), COIN_SIZE);
        }

        BOOST_CHECK_EQUAL(
            chainstate.GetCoinsCacheSizeState(tx_pool, MAX_COINS_CACHE_BYTES, /*max_mempool_size_bytes*/ 0),
            CoinsCacheSizeState::CRITICAL);

        BOOST_TEST_MESSAGE("Exiting cache flush tests early due to unsupported arch");
        return;
    }

    print_view_mem_usage(view);
    BOOST_CHECK_EQUAL(view.DynamicMemoryUsage(), is_64_bit ? 32 : 16);

    // We should be able to add COINS_UNTIL_CRITICAL coins to the cache before going CRITICAL.
    // This is contingent not only on the dynamic memory usage of the Coins
    // that we're adding (COIN_SIZE bytes per), but also on how much memory the
    // cacheCoins (unordered_map) preallocates.
    constexpr int COINS_UNTIL_CRITICAL{3};

    for (int i{0}; i < COINS_UNTIL_CRITICAL; ++i) {
        COutPoint res = add_coin(view);
        print_view_mem_usage(view);
        BOOST_CHECK_EQUAL(view.AccessCoin(res).DynamicMemoryUsage(), COIN_SIZE);
        BOOST_CHECK_EQUAL(
            chainstate.GetCoinsCacheSizeState(tx_pool, MAX_COINS_CACHE_BYTES, /*max_mempool_size_bytes*/ 0),
            CoinsCacheSizeState::OK);
    }

    // Adding some additional coins will push us over the edge to CRITICAL.
    for (int i{0}; i < 4; ++i) {
        add_coin(view);
        print_view_mem_usage(view);
        if (chainstate.GetCoinsCacheSizeState(tx_pool, MAX_COINS_CACHE_BYTES, /*max_mempool_size_bytes*/ 0) ==
            CoinsCacheSizeState::CRITICAL) {
            break;
        }
    }

    BOOST_CHECK_EQUAL(
        chainstate.GetCoinsCacheSizeState(tx_pool, MAX_COINS_CACHE_BYTES, /*max_mempool_size_bytes*/ 0),
        CoinsCacheSizeState::CRITICAL);

    // Passing non-zero max mempool usage should allow us more headroom.
    BOOST_CHECK_EQUAL(
        chainstate.GetCoinsCacheSizeState(tx_pool, MAX_COINS_CACHE_BYTES, /*max_mempool_size_bytes*/ 1 << 10),
        CoinsCacheSizeState::OK);

    for (int i{0}; i < 3; ++i) {
        add_coin(view);
        print_view_mem_usage(view);
        BOOST_CHECK_EQUAL(
            chainstate.GetCoinsCacheSizeState(tx_pool, MAX_COINS_CACHE_BYTES, /*max_mempool_size_bytes*/ 1 << 10),
            CoinsCacheSizeState::OK);
    }

    // Adding another coin with the additional mempool room will put us >90%
    // but not yet critical.
    add_coin(view);
    print_view_mem_usage(view);

    // Only perform these checks on 64 bit hosts; I haven't done the math for 32.
    if (is_64_bit) {
        float usage_percentage = (float)view.DynamicMemoryUsage() / (MAX_COINS_CACHE_BYTES + (1 << 10));
        BOOST_TEST_MESSAGE("CoinsTip usage percentage: " << usage_percentage);
        BOOST_CHECK(usage_percentage >= 0.9);
        BOOST_CHECK(usage_percentage < 1);
        BOOST_CHECK_EQUAL(
            chainstate.GetCoinsCacheSizeState(tx_pool, MAX_COINS_CACHE_BYTES, 1 << 10),
            CoinsCacheSizeState::LARGE);
    }

    // Using the default max_* values permits way more coins to be added.
    for (int i{0}; i < 1000; ++i) {
        add_coin(view);
        BOOST_CHECK_EQUAL(
            chainstate.GetCoinsCacheSizeState(tx_pool),
            CoinsCacheSizeState::OK);
    }

    // Flushing the view doesn't take us back to OK because cacheCoins has
    // preallocated memory that doesn't get reclaimed even after flush.

    BOOST_CHECK_EQUAL(
        chainstate.GetCoinsCacheSizeState(tx_pool, MAX_COINS_CACHE_BYTES, 0),
        CoinsCacheSizeState::CRITICAL);

    view.SetBestBlock(InsecureRand256());
    BOOST_CHECK(view.Flush());
    print_view_mem_usage(view);

    BOOST_CHECK_EQUAL(
        chainstate.GetCoinsCacheSizeState(tx_pool, MAX_COINS_CACHE_BYTES, 0),
        CoinsCacheSizeState::CRITICAL);
}

BOOST_AUTO_TEST_SUITE_END()
