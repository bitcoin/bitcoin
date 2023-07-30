// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <index/coinstatsindex.h>
#include <test/util/setup_common.h>
#include <util/time.h>
#include <validation.h>

#include <boost/test/unit_test.hpp>

#include <chrono>


BOOST_AUTO_TEST_SUITE(coinstatsindex_tests)

BOOST_FIXTURE_TEST_CASE(coinstatsindex_initial_sync, TestChain100Setup)
{
    CoinStatsIndex coin_stats_index{1 << 20, true};

    CCoinsStats coin_stats{CoinStatsHashType::MUHASH};
    const CBlockIndex* block_index;
    {
        LOCK(cs_main);
        block_index = ChainActive().Tip();
    }

    // CoinStatsIndex should not be found before it is started.
    BOOST_CHECK(!coin_stats_index.LookUpStats(block_index, coin_stats));

    // BlockUntilSyncedToCurrentChain should return false before CoinStatsIndex
    // is started.
    BOOST_CHECK(!coin_stats_index.BlockUntilSyncedToCurrentChain());

    BOOST_REQUIRE(coin_stats_index.Start());

    // Allow the CoinStatsIndex to catch up with the block index that is syncing
    // in a background thread.
    const auto timeout = GetTime<std::chrono::seconds>() + 120s;
    while (!coin_stats_index.BlockUntilSyncedToCurrentChain()) {
        BOOST_REQUIRE(timeout > GetTime<std::chrono::milliseconds>());
        UninterruptibleSleep(100ms);
    }

    // Check that CoinStatsIndex works for genesis block.
    const CBlockIndex* genesis_block_index;
    {
        LOCK(cs_main);
        genesis_block_index = ChainActive().Genesis();
    }
    BOOST_CHECK(coin_stats_index.LookUpStats(genesis_block_index, coin_stats));

    // Check that CoinStatsIndex updates with new blocks.
    coin_stats_index.LookUpStats(block_index, coin_stats);

    const CScript script_pub_key{CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG};
    std::vector<CMutableTransaction> noTxns;
    CreateAndProcessBlock(noTxns, script_pub_key);

    // Let the CoinStatsIndex to catch up again.
    BOOST_CHECK(coin_stats_index.BlockUntilSyncedToCurrentChain());

    CCoinsStats new_coin_stats{CoinStatsHashType::MUHASH};
    const CBlockIndex* new_block_index;
    {
        LOCK(cs_main);
        new_block_index = ChainActive().Tip();
    }
    coin_stats_index.LookUpStats(new_block_index, new_coin_stats);

    BOOST_CHECK(block_index != new_block_index);

    // Shutdown sequence (c.f. Shutdown() in init.cpp)
    coin_stats_index.Stop();

    // Rest of shutdown sequence and destructors happen in ~TestingSetup()
}

BOOST_AUTO_TEST_SUITE_END()
