// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addresstype.h>
#include <blockfilter.h>
#include <chain.h>
#include <chainparams.h>
#include <coins.h>
#include <common/args.h>
#include <consensus/validation.h>
#include <index/base.h>
#include <index/blockfilterindex.h>
#include <index/coinstatsindex.h>
#include <index/txindex.h>
#include <index/txospenderindex.h>
#include <interfaces/chain.h>
#include <kernel/types.h>
#include <key.h>
#include <node/context.h>
#include <primitives/block.h>
#include <script/script.h>
#include <sync.h>
#include <test/util/index.h>
#include <test/util/mining.h>
#include <test/util/setup_common.h>
#include <test/util/time.h>
#include <test/util/validation.h>
#include <tinyformat.h>
#include <util/byte_units.h>
#include <util/check.h>
#include <util/fs.h>
#include <validation.h>

#include <boost/test/unit_test.hpp>

#include <chrono>
#include <functional>
#include <future>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

using kernel::ChainstateRole;

using IndexFactory = std::function<std::unique_ptr<BaseIndex>(node::NodeContext&)>;

static const std::vector<std::pair<std::string, IndexFactory>> INDEX_FACTORIES{
    {"coinstatsindex", [](node::NodeContext& node) -> std::unique_ptr<BaseIndex> {
        return std::make_unique<CoinStatsIndex>(interfaces::MakeChain(node), /*n_cache_size=*/1_MiB); }},
    {"txindex", [](node::NodeContext& node) -> std::unique_ptr<BaseIndex> {
        return std::make_unique<TxIndex>(interfaces::MakeChain(node), /*n_cache_size=*/1_MiB); }},
    {"txospenderindex", [](node::NodeContext& node) -> std::unique_ptr<BaseIndex> {
        return std::make_unique<TxoSpenderIndex>(interfaces::MakeChain(node), /*n_cache_size=*/1_MiB); }},
    {"blockfilterindex", [](node::NodeContext& node) -> std::unique_ptr<BaseIndex> {
        return std::make_unique<BlockFilterIndex>(interfaces::MakeChain(node), BlockFilterType::BASIC, /*n_cache_size=*/1_MiB); }},
};

// Tests of generic BaseIndex functionality that is independent of which
// concrete index is being used.
BOOST_AUTO_TEST_SUITE(baseindex_tests)

// Test that the index commits up to, but never ahead of, the chainstate's last
// flushed block. Committing ahead of the flush would corrupt the index on an
// unclean shutdown: on the next startup the index would be ahead of the chainstate
// and, when reverting to catch back up, would need undo data for blocks that were
// never flushed to disk.
//
// History of the end-of-sync commit behavior this exercises:
//   - Originally the index committed at the chain tip at the end of a background
//     sync unconditionally. That could commit ahead of the flushed chainstate and
//     cause the corruption described above.
//   - PR #34897 (commit 3679f1ecf5e "index: Don't commit ahead of the flushed
//     chainstate") fixed the corruption by skipping the end-of-sync commit whenever
//     the index best block was ahead of the flushed best block. Safe, but it can
//     leave the index committed behind the flush point, i.e. stale on disk.
//   - Current code commits at the flushed best block instead: up to the flush,
//     never ahead. That avoids both the corruption (not ahead) and the staleness
//     (not needlessly behind).
BOOST_FIXTURE_TEST_CASE(baseindex_no_commit_ahead_of_flush, TestChain100Setup)
{
    Chainstate& chainstate = Assert(m_node.chainman)->ActiveChainstate();
    // Chainstate's last-flushed best block height, which (per the behavior history
    // above) is the height each index is expected to commit at the end of sync.
    // It is declared outside the loop because all the indexes share the same node
    // chainstate: a flush performed while testing one index is still in effect when
    // testing the next. So an index's expected commit height depends on flushes done
    // by earlier iterations, not just its own -- only the first index (before any
    // flush) is expected to commit nothing (flushed_height == 0).
    int flushed_height{0};
    for (const auto& [index_name, make_index] : INDEX_FACTORIES) {
        BOOST_TEST_INFO_SCOPE(index_name);
        const int tip_height{WITH_LOCK(cs_main, return m_node.chainman->ActiveChain().Tip()->nHeight)};
        auto sync_index = [&](bool do_flush, int expected_sync_height, int expected_commit_height) {
            auto index{make_index(m_node)};
            BOOST_REQUIRE(index->Init());
            IndexTester{*index}.Sync();
            if (do_flush) {
                chainstate.ForceFlushStateToDisk();
                m_node.chain->context()->validation_signals->SyncWithValidationInterfaceQueue();
            }
            BOOST_CHECK_EQUAL(index->GetSummary().best_block_height, expected_sync_height);
            index->Stop();
            // Reload index to see which block data was actually committed.
            BOOST_REQUIRE(index->Init());
            BOOST_CHECK_EQUAL(index->GetSummary().best_block_height, expected_commit_height);
            // Deliver the reload's queued notifications before destroying the
            // index, as the node does before stopping indexes; otherwise the
            // reload's queued SYNCED notification would fire on the freed
            // index. Note: this is removed in a later change adding a
            // WaitForCurrentCallback() call to
            // NotificationsHandlerImpl::disconnect() to ensure no notifications
            // notifications can be received after disconnect() returns.
            m_node.chain->context()->validation_signals->SyncWithValidationInterfaceQueue();
            index->Stop();
        };

        // Part 1: Sync, then "crash" (stop without flushing this round). Models a
        // node that started up, had its index catch up, but never flushed before
        // going down. At the end of sync the index commits up to the chainstate's
        // last flushed block -- never ahead of it. For the first index that is
        // genesis (flushed_height == 0, so nothing is committed); for later indexes
        // it is the height flushed by a previous iteration.
        sync_index(false, tip_height, flushed_height);

        // Part 2: Restart cleanly. Sync, force a chainstate flush at the tip, and
        // drain the validation queue so the index's ChainStateFlushed callback runs.
        // Now the last flushed block == tip and the index can commit at the tip.
        sync_index(true, tip_height, tip_height);
        flushed_height = tip_height;

        // Part 3: Connect a new block on the chain without flushing (the last
        // flushed block stays at flushed_height). For a real node this would happen
        // in parallel with Sync(). Here we do it before Sync() to make the race
        // state deterministic. The index syncs the new block but still only commits
        // up to flushed_height, not ahead of the flush.
        CreateAndProcessBlock({}, CScript() << OP_TRUE);
        sync_index(false, tip_height + 1, flushed_height);
    }
}

// Test shutdown between BlockConnected and ChainStateFlushed notifications,
// make sure index is not corrupted and is able to reload.
BOOST_FIXTURE_TEST_CASE(index_unclean_shutdown, TestChain100Setup)
{
    Chainstate& chainstate = Assert(m_node.chainman)->ActiveChainstate();
    const CChainParams& params = Params();
    for (const auto& [index_name, make_index] : INDEX_FACTORIES) {
        BOOST_TEST_INFO_SCOPE(index_name);
        {
            auto index{make_index(m_node)};
            BOOST_REQUIRE(index->Init());
            IndexTester{*index}.Sync();
            std::shared_ptr<const CBlock> new_block;
            CBlockIndex* new_block_index = nullptr;
            {
                const CScript script_pub_key{CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG};
                const CBlock block = this->CreateBlock({}, script_pub_key);

                new_block = std::make_shared<CBlock>(block);

                LOCK(cs_main);
                BlockValidationState state;
                BOOST_CHECK(CheckBlock(block, state, params.GetConsensus()));
                BOOST_CHECK(m_node.chainman->AcceptBlock(new_block, state, &new_block_index, true, nullptr, nullptr, true));
                CCoinsViewCache view(&chainstate.CoinsTip());
                BOOST_CHECK(chainstate.ConnectBlock(block, state, new_block_index, view));
            }
            // Send block connected notification, then stop the index without
            // sending a chainstate flushed notification. Prior to #24138, this
            // would cause the index to be corrupted and fail to reload.
            m_node.validation_signals->BlockConnected(ChainstateRole{}, new_block, new_block_index);
            m_node.validation_signals->SyncWithValidationInterfaceQueue();
            index->Stop();
        }

        {
            auto index{make_index(m_node)};
            BOOST_REQUIRE(index->Init());
            // Make sure the index can be loaded.
            IndexTester{*index}.Sync();
            index->Stop();
        }
    }
}

class IndexReorgCrash : public BaseIndex
{
private:
    FakeNodeClock& m_clock;
    std::unique_ptr<BaseIndex::DB> m_db;
    std::shared_future<void> m_blocker;
    int m_blocking_height;

public:
    explicit IndexReorgCrash(std::unique_ptr<interfaces::Chain> chain, std::shared_future<void> blocker, int blocking_height, FakeNodeClock& clock)
        : BaseIndex(std::move(chain), "test index", "testidx"), m_clock(clock), m_blocker(blocker), m_blocking_height(blocking_height)
    {
        const fs::path path = gArgs.GetDataDirNet() / "index";
        fs::create_directories(path);
        m_db = std::make_unique<BaseIndex::DB>(path / "db", /*n_cache_size=*/0, /*f_memory=*/true, /*f_wipe=*/false);
    }

    bool AllowPrune() const override { return false; }
    BaseIndex::DB& GetDB() const override { return *m_db; }

    bool CustomAppend(const interfaces::BlockInfo& block) override
    {
        // Simulate a delay so new blocks can get connected during the initial sync
        if (block.height == m_blocking_height) m_blocker.wait();

        // Move mock time forward so the best index gets updated only when we are not at the blocking height
        if (block.height == m_blocking_height - 1 || block.height > m_blocking_height) {
            m_clock += 31s;
        }

        return true;
    }
};

BOOST_FIXTURE_TEST_CASE(index_reorg_crash, TestChain100Setup)
{
    std::promise<void> promise;
    std::shared_future<void> blocker(promise.get_future());
    int blocking_height = WITH_LOCK(cs_main, return m_node.chainman->ActiveChain().Tip()->nHeight);

    IndexReorgCrash index{interfaces::MakeChain(m_node), blocker, blocking_height, m_clock};
    BOOST_REQUIRE(index.Init());
    // This test drives the background sync directly (StartBackgroundSync without an
    // immediate WaitForBackgroundSync) instead of IndexTester::Sync(): it needs the
    // sync running concurrently so it can trigger a reorg while the index is still
    // mid-sync, whereas IndexTester::Sync() blocks until the sync has finished.
    BOOST_REQUIRE(index.StartBackgroundSync());

    auto func_wait_until = [&](int height, std::chrono::milliseconds timeout) {
        auto deadline = std::chrono::steady_clock::now() + timeout;
        while (index.GetSummary().best_block_height < height) {
            if (std::chrono::steady_clock::now() > deadline) {
                BOOST_FAIL(strprintf("Timeout waiting for index height %d (current: %d)", height, index.GetSummary().best_block_height));
                return;
            }
            std::this_thread::sleep_for(100ms);
        }
    };

    // Wait until the index is one block before the fork point
    func_wait_until(blocking_height - 1, /*timeout=*/5s);

    // Create a fork to trigger the reorg
    std::vector<std::shared_ptr<CBlock>> fork;
    const CBlockIndex* prev_tip = WITH_LOCK(cs_main, return m_node.chainman->ActiveChain().Tip()->pprev);
    BOOST_REQUIRE(BuildChain(m_node, prev_tip, GetScriptForDestination(PKHash(GenerateRandomKey().GetPubKey())), 3, fork));

    for (const auto& block : fork) {
        BOOST_REQUIRE(m_node.chainman->ProcessNewBlock(block, /*force_processing=*/true, /*min_pow_checked=*/true, nullptr));
    }

    // Unblock the index thread so it can process the reorg
    promise.set_value();
    // Wait for the index to reach the new tip
    func_wait_until(blocking_height + 2, 5s);
    index.Stop();
}

BOOST_AUTO_TEST_SUITE_END()
