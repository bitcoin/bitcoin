// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
#include <random.h>
#include <uint256.h>
#include <consensus/validation.h>
#include <sync.h>
#include <test/util/setup_common.h>
#include <validation.h>

#include <vector>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(validation_chainstate_tests, TestingSetup)

//! Test resizing coins-related CChainState caches during runtime.
//!
BOOST_AUTO_TEST_CASE(validation_chainstate_resize_caches)
{
    ChainstateManager manager;
    WITH_LOCK(::cs_main, manager.m_blockman.m_block_tree_db = std::make_unique<CBlockTreeDB>(1 << 20, true));
    CTxMemPool mempool;

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

    CChainState& c1 = WITH_LOCK(cs_main, return manager.InitializeChainstate(&mempool));
    c1.InitCoinsDB(
        /* cache_size_bytes */ 1 << 23, /* in_memory */ true, /* should_wipe */ false);
    WITH_LOCK(::cs_main, c1.InitCoinsCache(1 << 23));

    // Add a coin to the in-memory cache, upsize once, then downsize.
    {
        LOCK(::cs_main);
        auto outpoint = add_coin(c1.CoinsTip());

        // Set a meaningless bestblock value in the coinsview cache - otherwise we won't
        // flush during ResizecoinsCaches() and will subsequently hit an assertion.
        c1.CoinsTip().SetBestBlock(InsecureRand256());

        BOOST_CHECK(c1.CoinsTip().HaveCoinInCache(outpoint));

        c1.ResizeCoinsCaches(
            1 << 24,  // upsizing the coinsview cache
            1 << 22  // downsizing the coinsdb cache
        );

        // View should still have the coin cached, since we haven't destructed the cache on upsize.
        BOOST_CHECK(c1.CoinsTip().HaveCoinInCache(outpoint));

        c1.ResizeCoinsCaches(
            1 << 22,  // downsizing the coinsview cache
            1 << 23  // upsizing the coinsdb cache
        );

        // The view cache should be empty since we had to destruct to downsize.
        BOOST_CHECK(!c1.CoinsTip().HaveCoinInCache(outpoint));
    }

    // Avoid triggering the address sanitizer.
    WITH_LOCK(::cs_main, manager.Unload());
}

BOOST_AUTO_TEST_SUITE_END()
