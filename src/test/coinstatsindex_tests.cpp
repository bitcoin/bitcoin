// Copyright (c) 2020-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <index/coinstatsindex.h>
#include <interfaces/chain.h>
#include <kernel/coinstats.h>
#include <test/util/index.h>
#include <test/util/setup_common.h>
#include <test/util/validation.h>
#include <validation.h>

#include <gtest/gtest.h>

class CoinstatsindexTests : public TestChain100Setup, public testing::Test {};

TEST_F(CoinstatsindexTests, InitialSync)
{
    CoinStatsIndex coin_stats_index{interfaces::MakeChain(m_node), 1 << 20, true};
    ASSERT_TRUE(coin_stats_index.Init());

    const CBlockIndex* block_index;
    {
        LOCK(cs_main);
        block_index = m_node.chainman->ActiveChain().Tip();
    }

    // CoinStatsIndex should not be found before it is started.
    EXPECT_EQ(coin_stats_index.LookUpStats(*block_index), std::nullopt);

    // BlockUntilSyncedToCurrentChain should return false before CoinStatsIndex
    // is started.
    EXPECT_FALSE(coin_stats_index.BlockUntilSyncedToCurrentChain());

    EXPECT_TRUE(coin_stats_index.StartBackgroundSync());

    IndexWaitSynced(coin_stats_index, *Assert(m_node.shutdown_signal));

    // Check that CoinStatsIndex works for genesis block.
    const CBlockIndex* genesis_block_index;
    {
        LOCK(cs_main);
        genesis_block_index = m_node.chainman->ActiveChain().Genesis();
    }
    EXPECT_NE(coin_stats_index.LookUpStats(*genesis_block_index), std::nullopt);

    // Check that CoinStatsIndex updates with new blocks.
    EXPECT_NE(coin_stats_index.LookUpStats(*block_index), std::nullopt);

    const CScript script_pub_key{CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG};
    std::vector<CMutableTransaction> noTxns;
    CreateAndProcessBlock(noTxns, script_pub_key);

    // Let the CoinStatsIndex to catch up again.
    EXPECT_TRUE(coin_stats_index.BlockUntilSyncedToCurrentChain());

    const CBlockIndex* new_block_index;
    {
        LOCK(cs_main);
        new_block_index = m_node.chainman->ActiveChain().Tip();
    }
    EXPECT_NE(coin_stats_index.LookUpStats(*new_block_index), std::nullopt);

    EXPECT_NE(block_index, new_block_index);

    // It is not safe to stop and destroy the index until it finishes handling
    // the last BlockConnected notification. The BlockUntilSyncedToCurrentChain()
    // call above is sufficient to ensure this, but the
    // SyncWithValidationInterfaceQueue() call below is also needed to ensure
    // TSAN always sees the test thread waiting for the notification thread, and
    // avoid potential false positive reports.
    m_node.validation_signals->SyncWithValidationInterfaceQueue();

    // Shutdown sequence (c.f. Shutdown() in init.cpp)
    coin_stats_index.Stop();
}

// Test shutdown between BlockConnected and ChainStateFlushed notifications,
// make sure index is not corrupted and is able to reload.
TEST_F(CoinstatsindexTests, UncleanShutdown)
{
    Chainstate& chainstate = Assert(m_node.chainman)->ActiveChainstate();
    const CChainParams& params = Params();
    {
        CoinStatsIndex index{interfaces::MakeChain(m_node), 1 << 20};
        ASSERT_TRUE(index.Init());
        ASSERT_TRUE(index.StartBackgroundSync());
        IndexWaitSynced(index, *Assert(m_node.shutdown_signal));
        std::shared_ptr<const CBlock> new_block;
        CBlockIndex* new_block_index = nullptr;
        {
            const CScript script_pub_key{CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG};
            const CBlock block = this->CreateBlock({}, script_pub_key, chainstate);

            new_block = std::make_shared<CBlock>(block);

            LOCK(cs_main);
            BlockValidationState state;
            EXPECT_TRUE(CheckBlock(block, state, params.GetConsensus()));
            EXPECT_TRUE(m_node.chainman->AcceptBlock(new_block, state, &new_block_index, true, nullptr, nullptr, true));
            CCoinsViewCache view(&chainstate.CoinsTip());
            EXPECT_TRUE(chainstate.ConnectBlock(block, state, new_block_index, view));
        }
        // Send block connected notification, then stop the index without
        // sending a chainstate flushed notification. Prior to #24138, this
        // would cause the index to be corrupted and fail to reload.
        ValidationInterfaceTest::BlockConnected(ChainstateRole::NORMAL, index, new_block, new_block_index);
        index.Stop();
    }

    {
        CoinStatsIndex index{interfaces::MakeChain(m_node), 1 << 20};
        EXPECT_TRUE(index.Init());
        // Make sure the index can be loaded.
        ASSERT_TRUE(index.StartBackgroundSync());
        index.Stop();
    }
}