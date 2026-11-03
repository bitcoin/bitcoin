// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <consensus/validation.h>
#include <index/coinstatsindex.h>
#include <interfaces/chain.h>
#include <node/blockstorage.h>
#include <script/script.h>
#include <test/util/index.h>
#include <test/util/setup_common.h>
#include <util/check.h>
#include <util/system.h>
#include <validation.h>
#include <validationinterface.h>

#include <boost/test/unit_test.hpp>

#include <memory>

namespace {
//! A minimal index that, like Dash's address and spent indexes, keeps one entry per block and erases the entries of
//! rewound blocks in CustomRewind() block by block. The rewind is exposed, and can stop after a given block, so a test
//! can leave the on-disk state that an unclean shutdown in the middle of a reorg would leave, without having to time
//! an actual crash.
class ErasingIndex final : public BaseIndex
{
    const std::unique_ptr<BaseIndex::DB> m_db;
    const CBlockIndex* m_stop_after{nullptr};

public:
    explicit ErasingIndex(std::unique_ptr<interfaces::Chain> chain)
        : BaseIndex(std::move(chain), "erasing test index"),
          m_db{std::make_unique<BaseIndex::DB>(gArgs.GetDataDirNet() / "indexes" / "erasingtest", /*n_cache_size=*/1 << 20)}
    {
    }
    ~ErasingIndex() override
    {
        Interrupt();
        Stop();
    }

    bool Indexed(const CBlockIndex* block) const { return m_db->Exists(std::make_pair(uint8_t{'e'}, block->GetBlockHash())); }
    //! Rewind, or with stop_after set, stop as a crash would once that block's batch is written
    bool RewindTo(const CBlockIndex* current_tip, const CBlockIndex* new_tip, const CBlockIndex* stop_after = nullptr)
    {
        m_stop_after = stop_after;
        const bool ok{CustomRewind({current_tip->GetBlockHash(), current_tip->nHeight}, {new_tip->GetBlockHash(), new_tip->nHeight})};
        m_stop_after = nullptr;
        return ok;
    }
    uint256 CommittedTip() const
    {
        const auto locator{m_db->ReadBestBlock()};
        BOOST_REQUIRE(!locator.IsNull());
        return locator.vHave.front();
    }

protected:
    bool CustomAppend(const interfaces::BlockInfo& block) override { return m_db->Write(std::make_pair(uint8_t{'e'}, block.hash), true); }
    bool CustomRewind(const interfaces::BlockKey& current_tip, const interfaces::BlockKey& new_tip) override
    {
        const CBlockIndex* current_tip_index{WITH_LOCK(::cs_main, return m_chainstate->m_blockman.LookupBlockIndex(current_tip.hash))};
        const CBlockIndex* new_tip_index{WITH_LOCK(::cs_main, return m_chainstate->m_blockman.LookupBlockIndex(new_tip.hash))};
        return RewindBlockByBlock(current_tip_index, new_tip_index, [&](const CBlockIndex* pindex, CDBBatch& batch) {
            if (m_stop_after && pindex == m_stop_after->pprev) return false;
            batch.Erase(std::make_pair(uint8_t{'e'}, pindex->GetBlockHash()));
            return true;
        });
    }
    BaseIndex::DB& GetDB() const override { return *m_db; }
    bool AllowPrune() const override { return true; }
};

const CBlockIndex* AtHeight(ChainstateManager& chainman, int height)
{
    return WITH_LOCK(::cs_main, return chainman.ActiveChain()[height]);
}

//! Make `height` the chainstate's last flushed block while the tip stays above it: flush with the blocks above
//! disconnected, then connect them again without a flush.
void LowerLastFlushedBlock(ChainstateManager& chainman, int height)
{
    Chainstate& chainstate{chainman.ActiveChainstate()};
    CBlockIndex* first_disconnected{WITH_LOCK(::cs_main, return chainman.ActiveChain()[height + 1])};
    BlockValidationState state;
    BOOST_REQUIRE(chainstate.InvalidateBlock(state, first_disconnected));
    chainstate.ForceFlushStateToDisk();
    WITH_LOCK(::cs_main, chainstate.ResetBlockFailureFlags(first_disconnected));
    BOOST_REQUIRE(chainstate.ActivateBestChain(state));
    BOOST_REQUIRE_EQUAL(WITH_LOCK(::cs_main, return chainstate.GetLastFlushedBlock()->nHeight), height);
}
} // namespace

// Tests of generic BaseIndex functionality that is independent of which
// concrete index is being used. CoinStatsIndex is used here merely as a
// convenient instantiation of BaseIndex.
BOOST_AUTO_TEST_SUITE(baseindex_tests)

// Test that the index does not commit ahead of the chainstate's last
// flushed block. If it did, a subsequent unclean shutdown would corrupt
// the index, because during reverting it would require blocks that were
// never flushed to disk.
BOOST_FIXTURE_TEST_CASE(baseindex_no_commit_ahead_of_flush, TestChain100Setup)
{
    Chainstate& chainstate = Assert(m_node.chainman)->ActiveChainstate();
    auto sync_index = [&](bool do_flush, int expected_sync_height, int expected_commit_height) {
        {
            CoinStatsIndex index{interfaces::MakeChain(m_node), /*n_cache_size=*/1 << 20};
            BOOST_REQUIRE(index.Start());
            IndexWaitSynced(index);
            if (do_flush) {
                chainstate.ForceFlushStateToDisk();
                SyncWithValidationInterfaceQueue();
            }
            BOOST_CHECK_EQUAL(index.GetSummary().best_block_height, expected_sync_height);
            index.Stop();
        }
        // Reload index to see which block data was actually committed. Init() and Sync() are not
        // callable on their own in Dash, so hold back the sync thread while the loaded best block is read.
        node::g_indexes_ready_to_sync = false;
        {
            CoinStatsIndex index{interfaces::MakeChain(m_node), /*n_cache_size=*/1 << 20};
            BOOST_REQUIRE(index.Start());
            BOOST_CHECK_EQUAL(index.GetSummary().best_block_height, expected_commit_height);
            index.Interrupt();
            index.Stop();
        }
        node::g_indexes_ready_to_sync = true;
    };

    // Part 1: Sync, then "crash" (stop without flushing). Models a node that
    // started up, had its index catch up, but never flushed before going down.
    // The end-of-sync Commit() runs at chain tip (height 100) but
    // m_last_flushed_block is null, so it is skipped.
    sync_index(false, 100, 0);

    // Part 2: Restart cleanly. Sync, force a chainstate flush, and drain the
    // validation queue so the index's ChainStateFlushed callback runs.
    // Now m_last_flushed_block == tip == 100 and the index can commit.
    sync_index(true, 100, 100);

    // Part 3: Connect a new block on the chain without flushing
    // (m_last_flushed_block stays at 100). For a real node this would happen
    // in parallel with Sync(). Here we do it before Sync() to make the race
    // state deterministic.
    CreateAndProcessBlock({}, CScript() << OP_TRUE);
    sync_index(false, 101, 100);
}

// A rewind persists its erasures right away. If the node then stops before the rewound tip is flushed and the
// rewound blocks are back on the active chain, the next start must index them again instead of resuming above them.
BOOST_FIXTURE_TEST_CASE(restart_after_rewind_reindexes_erased_blocks, TestChain100Setup)
{
    ChainstateManager& chainman{*Assert(m_node.chainman)};
    {
        ErasingIndex index{interfaces::MakeChain(m_node)};
        BOOST_REQUIRE(index.Start());
        IndexWaitSynced(index);
        chainman.ActiveChainstate().ForceFlushStateToDisk();
        SyncWithValidationInterfaceQueue();
        index.Stop();
    }
    // The rewind target (97) is now above the last flush (95), so Commit() cannot persist it
    LowerLastFlushedBlock(chainman, 95);
    const CBlockIndex* tip{AtHeight(chainman, 100)};

    node::g_indexes_ready_to_sync = false;
    {
        ErasingIndex index{interfaces::MakeChain(m_node)};
        BOOST_REQUIRE(index.Start());
        BOOST_CHECK(index.CommittedTip() == tip->GetBlockHash());
        BOOST_REQUIRE(index.RewindTo(tip, AtHeight(chainman, 97)));
        BOOST_CHECK(!index.Indexed(tip));
        BOOST_CHECK(index.CommittedTip() == AtHeight(chainman, 97)->GetBlockHash());
        index.Interrupt();
        index.Stop();
    }
    node::g_indexes_ready_to_sync = true;

    // The rewound blocks are still the active chain, as after a restart in which their branch wins again
    ErasingIndex index{interfaces::MakeChain(m_node)};
    BOOST_REQUIRE(index.Start());
    IndexWaitSynced(index);
    for (int height{98}; height <= 100; ++height) {
        BOOST_CHECK_MESSAGE(index.Indexed(AtHeight(chainman, height)), "block " << height << " was not indexed again");
    }
    index.Stop();
}

// Each block's erasures are written in one batch with a locator at its parent. A node that stops in the middle of a
// rewind then resumes from the last block whose entries are still there, and indexes the erased ones again.
BOOST_FIXTURE_TEST_CASE(interrupted_rewind_keeps_locator_below_erased_entries, TestChain100Setup)
{
    ChainstateManager& chainman{*Assert(m_node.chainman)};
    {
        ErasingIndex index{interfaces::MakeChain(m_node)};
        BOOST_REQUIRE(index.Start());
        IndexWaitSynced(index);
        chainman.ActiveChainstate().ForceFlushStateToDisk();
        SyncWithValidationInterfaceQueue();
        index.Stop();
    }
    const CBlockIndex* tip{AtHeight(chainman, 100)};

    node::g_indexes_ready_to_sync = false;
    {
        ErasingIndex index{interfaces::MakeChain(m_node)};
        BOOST_REQUIRE(index.Start());
        BOOST_CHECK(index.CommittedTip() == tip->GetBlockHash());
        BOOST_CHECK(!index.RewindTo(tip, AtHeight(chainman, 97), /*stop_after=*/tip));
        BOOST_CHECK(!index.Indexed(tip));
        BOOST_CHECK(index.Indexed(AtHeight(chainman, 99)));
        BOOST_CHECK(index.CommittedTip() == AtHeight(chainman, 99)->GetBlockHash());
        index.Interrupt();
        index.Stop();
    }
    node::g_indexes_ready_to_sync = true;

    ErasingIndex index{interfaces::MakeChain(m_node)};
    BOOST_REQUIRE(index.Start());
    IndexWaitSynced(index);
    BOOST_CHECK_MESSAGE(index.Indexed(tip), "block 100 was not indexed again");
    index.Stop();
}

BOOST_AUTO_TEST_SUITE_END()
