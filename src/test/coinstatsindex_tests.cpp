// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <index/coinstatsindex.h>
#include <test/util/setup_common.h>
#include <validation.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(coinstatsindex_tests)

BOOST_FIXTURE_TEST_CASE(coinstatsindex_initial_sync, TestChain100Setup)
{
    CoinStatsIndex coin_stats_index(1 << 20, true);

    CCoinsStats coin_stats;
    const CBlockIndex* block_index;
    {
        LOCK(cs_main);
        block_index = ::ChainActive().Tip();
    }

    // CoinStatsIndex should not be found before it is started.
    BOOST_CHECK(!coin_stats_index.LookupStats(block_index, coin_stats));

    // BlockUntilSyncedToCurrentChain should return false before CoinStatsIndex
    // is started.
    BOOST_CHECK(!coin_stats_index.BlockUntilSyncedToCurrentChain());

    coin_stats_index.Start();

    // Allow the CoinStatsIndex to catch up with the block index that is syncing
    // in a background thread.
    constexpr int64_t timeout_ms = 120 * 1000;
    int64_t time_start = GetTimeMillis();
    while (!coin_stats_index.BlockUntilSyncedToCurrentChain()) {
        BOOST_REQUIRE(time_start + timeout_ms > GetTimeMillis());
        UninterruptibleSleep(std::chrono::milliseconds{100});
    }

    // Check that CoinStatsIndex works for genesis block.
    const CBlockIndex* genesis_block_index;
    {
        LOCK(cs_main);
        genesis_block_index = ::ChainActive().Genesis();
    }
    BOOST_CHECK(coin_stats_index.LookupStats(genesis_block_index, coin_stats));

    // Check that CoinStatsIndex updates with new blocks.
    coin_stats_index.LookupStats(block_index, coin_stats);

    CScript scriptPubKey = CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;
    std::vector<CMutableTransaction> noTxns;
    CreateAndProcessBlock(noTxns, scriptPubKey);

    // Let the CoinStatsIndex to catch up again.
    BOOST_CHECK(coin_stats_index.BlockUntilSyncedToCurrentChain());

    CCoinsStats new_coin_stats;
    const CBlockIndex* new_block_index;
    {
        LOCK(cs_main);
        new_block_index = ::ChainActive().Tip();
    }
    coin_stats_index.LookupStats(new_block_index, new_coin_stats);

    BOOST_CHECK(block_index != new_block_index);
    BOOST_CHECK(coin_stats.hashSerialized != new_coin_stats.hashSerialized);

    // Shutdown sequence (c.f. Shutdown() in init.cpp)
    coin_stats_index.Stop();

    // Rest of shutdown sequence and destructors happen in ~TestingSetup()
}

BOOST_AUTO_TEST_SUITE_END()
