// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <flatfile.h>
#include <node/blockstorage.h>
#include <test/util/logging.h>
#include <test/util/setup_common.h>
#include <test/util/time.h>
#include <txdb.h>
#include <util/fs.h>
#include <validation.h>
#include <validationinterface.h>

#include <boost/test/unit_test.hpp>

#include <cstdlib>

using kernel::ChainstateRole;

// Taken from validation.cpp
static constexpr auto DATABASE_WRITE_INTERVAL_MIN{50min};
static constexpr auto DATABASE_WRITE_INTERVAL_MAX{70min};

static void InjectFileOpenFailure(const fs::path& file)
{
    fs::rename(file, file + ".bak");
    Assert(fs::create_directory(file));
}

static void RestoreFile(const fs::path& file)
{
    Assert(fs::remove(file));
    fs::rename(file + ".bak", file);
}

struct ManualPruneSetup : public TestChain100Setup {
    ManualPruneSetup()
        : TestChain100Setup{ChainType::REGTEST, {.extra_args = {"-prune=1", "-fastprune"}}} {}
};

BOOST_AUTO_TEST_SUITE(chainstate_write_tests)

BOOST_FIXTURE_TEST_CASE(chainstate_write_interval, TestingSetup)
{
    struct TestSubscriber final : CValidationInterface {
        bool m_did_flush{false};
        void ChainStateFlushed(const ChainstateRole&, const CBlockLocator&) override
        {
            m_did_flush = true;
        }
    };

    const auto sub{std::make_shared<TestSubscriber>()};
    m_node.validation_signals->RegisterSharedValidationInterface(sub);
    auto& chainstate{Assert(m_node.chainman)->ActiveChainstate()};
    BlockValidationState state_dummy{};
    FakeNodeClock clock{};

    // The first periodic flush sets m_next_write and does not flush
    chainstate.FlushStateToDisk(state_dummy, FlushStateMode::PERIODIC);
    m_node.validation_signals->SyncWithValidationInterfaceQueue();
    BOOST_CHECK(!sub->m_did_flush);

    // The periodic flush interval is between 50 and 70 minutes (inclusive)
    clock += DATABASE_WRITE_INTERVAL_MIN - 1min;
    chainstate.FlushStateToDisk(state_dummy, FlushStateMode::PERIODIC);
    m_node.validation_signals->SyncWithValidationInterfaceQueue();
    BOOST_CHECK(!sub->m_did_flush);

    clock += DATABASE_WRITE_INTERVAL_MAX;
    chainstate.FlushStateToDisk(state_dummy, FlushStateMode::PERIODIC);
    m_node.validation_signals->SyncWithValidationInterfaceQueue();
    BOOST_CHECK(sub->m_did_flush);
}

// Test that we do PERIODIC flushes inside ActivateBestChain.
// This is necessary for reindex-chainstate to be able to periodically flush
// before reaching chain tip.
BOOST_FIXTURE_TEST_CASE(write_during_multiblock_activation, TestChain100Setup)
{
    struct TestSubscriber final : CValidationInterface
    {
        const CBlockIndex* m_tip{nullptr};
        const CBlockIndex* m_flushed_at_block{nullptr};
        void ChainStateFlushed(const ChainstateRole&, const CBlockLocator&) override
        {
            m_flushed_at_block = m_tip;
        }
        void UpdatedBlockTip(const CBlockIndex* block_index, const CBlockIndex*, bool) override {
            m_tip = block_index;
        }
    };

    auto& chainstate{Assert(m_node.chainman)->ActiveChainstate()};
    BlockValidationState state_dummy{};

    // Pop two blocks from the tip
    const CBlockIndex* tip{chainstate.m_chain.Tip()};
    CBlockIndex* second_from_tip{tip->pprev};

    {
        LOCK2(m_node.chainman->GetMutex(), chainstate.MempoolMutex());
        chainstate.DisconnectTip(state_dummy, nullptr);
        chainstate.DisconnectTip(state_dummy, nullptr);
    }

    BOOST_CHECK_EQUAL(second_from_tip->pprev, chainstate.m_chain.Tip());

    // Set m_next_write to current time
    chainstate.FlushStateToDisk(state_dummy, FlushStateMode::FORCE_FLUSH);
    BOOST_CHECK_EQUAL(WITH_LOCK(::cs_main, return chainstate.GetLastFlushedBlock()), second_from_tip->pprev);
    m_node.validation_signals->SyncWithValidationInterfaceQueue();
    // The periodic flush interval is between 50 and 70 minutes (inclusive)
    // The next call to a PERIODIC write will flush
    m_clock += DATABASE_WRITE_INTERVAL_MAX;

    const auto sub{std::make_shared<TestSubscriber>()};
    m_node.validation_signals->RegisterSharedValidationInterface(sub);

    // ActivateBestChain back to tip
    chainstate.ActivateBestChain(state_dummy, nullptr);
    BOOST_CHECK_EQUAL(tip, chainstate.m_chain.Tip());
    // Check that we flushed inside ActivateBestChain while we were at the
    // second block from tip, since FlushStateToDisk is called with PERIODIC
    // inside the outer loop.
    m_node.validation_signals->SyncWithValidationInterfaceQueue();
    BOOST_CHECK_EQUAL(sub->m_flushed_at_block, second_from_tip);
    BOOST_CHECK_EQUAL(WITH_LOCK(::cs_main, return chainstate.GetLastFlushedBlock()), second_from_tip);
}

BOOST_FIXTURE_TEST_CASE(chainstate_flush_failure_boundary, TestChain100Setup)
{
    auto& chainstate{Assert(m_node.chainman)->ActiveChainstate()};
    BlockValidationState state{};

    BOOST_REQUIRE(chainstate.FlushStateToDisk(state, FlushStateMode::FORCE_FLUSH) && state.IsValid());
    auto* old_flushed{WITH_LOCK(::cs_main, return chainstate.GetLastFlushedBlock())};

    mineBlocks(1);
    auto* new_tip{WITH_LOCK(::cs_main, return chainstate.m_chain.Tip())};
    BOOST_REQUIRE_NE(old_flushed, new_tip);
    const auto blk_file{WITH_LOCK(::cs_main, return chainstate.m_blockman.GetBlockPosFilename(new_tip->GetBlockPos()))};
    InjectFileOpenFailure(blk_file);

    ASSERT_DEBUG_LOG("Flushing block file to disk failed");
    bool flushed{chainstate.FlushStateToDisk(state, FlushStateMode::FORCE_FLUSH)};
    BOOST_CHECK_EQUAL(m_node.exit_status.load(), EXIT_FAILURE);
    BOOST_CHECK(!flushed && state.IsError());
    BOOST_CHECK_EQUAL(WITH_LOCK(::cs_main, return chainstate.GetLastFlushedBlock()), old_flushed);
    BOOST_CHECK(WITH_LOCK(::cs_main, return chainstate.CoinsDB().GetBestBlock()) == old_flushed->GetBlockHash());
    node::CBlockFileInfo file_info;
    BOOST_REQUIRE(WITH_LOCK(::cs_main, return chainstate.m_blockman.m_block_tree_db->ReadBlockFileInfo(new_tip->nFile, file_info)));
    BOOST_CHECK_EQUAL(file_info.nHeightLast, old_flushed->nHeight);

    RestoreFile(blk_file);
    state = {};
    BOOST_CHECK(chainstate.FlushStateToDisk(state, FlushStateMode::FORCE_FLUSH));
    BOOST_CHECK(state.IsValid());
    BOOST_CHECK_EQUAL(WITH_LOCK(::cs_main, return chainstate.GetLastFlushedBlock()), new_tip);
    BOOST_CHECK(WITH_LOCK(::cs_main, return chainstate.CoinsDB().GetBestBlock()) == new_tip->GetBlockHash());
    BOOST_REQUIRE(WITH_LOCK(::cs_main, return chainstate.m_blockman.m_block_tree_db->ReadBlockFileInfo(new_tip->nFile, file_info)));
    BOOST_CHECK_EQUAL(file_info.nHeightLast, new_tip->nHeight);
}

BOOST_FIXTURE_TEST_CASE(prune_flush_failure, ManualPruneSetup)
{
    auto& chainstate{Assert(m_node.chainman)->ActiveChainstate()};
    auto& blockman{chainstate.m_blockman};
    BlockValidationState state{};
    BOOST_REQUIRE(blockman.IsPruneMode());

    while (WITH_LOCK(::cs_main, return chainstate.m_chain.Tip()->nFile) == 0) mineBlocks(1);
    const int prune_height{WITH_LOCK(::cs_main, return static_cast<int>(blockman.GetBlockFileInfo(0)->nHeightLast))};
    const int tip_height{WITH_LOCK(::cs_main, return chainstate.m_chain.Height())};
    mineBlocks(prune_height + static_cast<int>(MIN_BLOCKS_TO_KEEP) - tip_height);

    BOOST_REQUIRE(chainstate.FlushStateToDisk(state, FlushStateMode::FORCE_FLUSH));
    const auto* flushed_tip{WITH_LOCK(::cs_main, return chainstate.m_chain.Tip())};
    const auto pruned_size{WITH_LOCK(::cs_main, return blockman.GetBlockFileInfo(0)->nSize)};
    const auto pruned_blk_file{blockman.GetBlockPosFilename(FlatFilePos{0, 0})};
    const fs::path pruned_rev_file{pruned_blk_file.parent_path() / "rev00000.dat"};

    mineBlocks(1);
    const auto tip_blk_file{WITH_LOCK(::cs_main, return blockman.GetBlockPosFilename(chainstate.m_chain.Tip()->GetBlockPos()))};
    InjectFileOpenFailure(tip_blk_file);

    ASSERT_DEBUG_LOG("Flushing block file to disk failed");
    BOOST_CHECK(!chainstate.FlushStateToDisk(state, FlushStateMode::NONE, prune_height));
    BOOST_CHECK(state.IsError());
    {
        LOCK(::cs_main);
        BOOST_CHECK_EQUAL(blockman.GetBlockFileInfo(0)->nSize, 0U);
        BOOST_CHECK_EQUAL(blockman.GetBlockFileInfo(0)->nUndoSize, 0U);
        for (int height{0}; height <= prune_height; ++height) {
            BOOST_CHECK(!(chainstate.m_chain[height]->nStatus & BLOCK_HAVE_DATA));
        }
    }
    bool have_pruned{false};
    BOOST_REQUIRE(WITH_LOCK(::cs_main, return blockman.m_block_tree_db->ReadFlag("prunedblockfiles", have_pruned)));
    BOOST_CHECK(have_pruned);
    node::CBlockFileInfo file_info;
    BOOST_REQUIRE(WITH_LOCK(::cs_main, return blockman.m_block_tree_db->ReadBlockFileInfo(0, file_info)));
    BOOST_CHECK_EQUAL(file_info.nSize, pruned_size);
    BOOST_CHECK(WITH_LOCK(::cs_main, return chainstate.CoinsDB().GetBestBlock()) == flushed_tip->GetBlockHash());
    BOOST_CHECK(fs::exists(pruned_blk_file));
    BOOST_CHECK(fs::exists(pruned_rev_file));

    RestoreFile(tip_blk_file);
    state = {};
    BOOST_CHECK(chainstate.FlushStateToDisk(state, FlushStateMode::FORCE_FLUSH));
    BOOST_REQUIRE(WITH_LOCK(::cs_main, return blockman.m_block_tree_db->ReadBlockFileInfo(0, file_info)));
    BOOST_CHECK_EQUAL(file_info.nSize, 0U);
    // TODO: delete pruned files after a retried flush
    BOOST_CHECK(fs::exists(pruned_blk_file));
    BOOST_CHECK(fs::exists(pruned_rev_file));
}

BOOST_AUTO_TEST_SUITE_END()
