// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
#include <chainparams.h>
#include <consensus/amount.h>
#include <consensus/validation.h>
#include <node/kernel_notifications.h>
#include <random.h>
#include <rpc/blockchain.h>
#include <script/script.h>
#include <sync.h>
#include <test/util/chainstate.h>
#include <test/util/common.h>
#include <test/util/coins.h>
#include <test/util/logging.h>
#include <test/util/random.h>
#include <test/util/setup_common.h>
#include <uint256.h>
#include <util/byte_units.h>
#include <util/check.h>
#include <validation.h>

#include <vector>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(validation_chainstate_tests, ChainTestingSetup)

//! Test resizing coins-related Chainstate caches during runtime.
//!
BOOST_AUTO_TEST_CASE(validation_chainstate_resize_caches)
{
    ChainstateManager& manager = *Assert(m_node.chainman);
    CTxMemPool& mempool = *Assert(m_node.mempool);
    Chainstate& c1 = WITH_LOCK(cs_main, return manager.InitializeChainstate(&mempool));
    c1.InitCoinsDB(
        /*cache_size_bytes=*/8_MiB, /*in_memory=*/true, /*should_wipe=*/false);
    WITH_LOCK(::cs_main, c1.InitCoinsCache(8_MiB));
    BOOST_REQUIRE(c1.LoadGenesisBlock()); // Need at least one block loaded to be able to flush caches

    // Add a coin to the in-memory cache, upsize once, then downsize.
    {
        LOCK(::cs_main);
        const auto outpoint = AddTestCoin(m_rng, c1.CoinsTip());

        // Set a meaningless bestblock value in the coinsview cache - otherwise we won't
        // flush during ResizecoinsCaches() and will subsequently hit an assertion.
        c1.CoinsTip().SetBestBlock(m_rng.rand256());

        BOOST_CHECK(c1.CoinsTip().HaveCoinInCache(outpoint));

        c1.ResizeCoinsCaches(
            16_MiB, // upsizing the coinsview cache
            4_MiB // downsizing the coinsdb cache
        );

        // View should still have the coin cached, since we haven't destructed the cache on upsize.
        BOOST_CHECK(c1.CoinsTip().HaveCoinInCache(outpoint));

        c1.ResizeCoinsCaches(
            4_MiB, // downsizing the coinsview cache
            8_MiB // upsizing the coinsdb cache
        );

        // The view cache should be empty since we had to destruct to downsize.
        BOOST_CHECK(!c1.CoinsTip().HaveCoinInCache(outpoint));
    }
}

BOOST_FIXTURE_TEST_CASE(connect_tip_does_not_cache_inputs_on_failed_connect, TestChain100Setup)
{
    Chainstate& chainstate{Assert(m_node.chainman)->ActiveChainstate()};

    COutPoint outpoint;
    {
        LOCK(cs_main);
        outpoint = AddTestCoin(m_rng, chainstate.CoinsTip());
        chainstate.CoinsTip().Flush(/*reallocate_cache=*/false);
    }

    CMutableTransaction tx;
    tx.vin.emplace_back(outpoint);
    tx.vout.emplace_back(MAX_MONEY, CScript{} << OP_TRUE);

    const auto tip{WITH_LOCK(cs_main, return chainstate.m_chain.Tip()->GetBlockHash())};
    const CBlock block{CreateBlock({tx}, CScript{} << OP_TRUE, chainstate)};
    BOOST_CHECK(Assert(m_node.chainman)->ProcessNewBlock(std::make_shared<CBlock>(block), true, true, nullptr));

    LOCK(cs_main);
    BOOST_CHECK_EQUAL(tip, chainstate.m_chain.Tip()->GetBlockHash()); // block rejected
    BOOST_CHECK(!chainstate.CoinsTip().HaveCoinInCache(outpoint));    // input not cached
}

//! Test UpdateTip behavior for both active and background chainstates.
//!
//! When run on the background chainstate, UpdateTip should do a subset
//! of what it does for the active chainstate.
BOOST_FIXTURE_TEST_CASE(chainstate_update_tip, TestChain100Setup)
{
    ChainstateManager& chainman = *Assert(m_node.chainman);
    const auto get_notify_tip{[&]() {
        LOCK(m_node.notifications->m_tip_block_mutex);
        BOOST_REQUIRE(m_node.notifications->TipBlock());
        return *m_node.notifications->TipBlock();
    }};
    uint256 curr_tip = get_notify_tip();

    // Mine 10 more blocks, putting at us height 110 where a valid assumeutxo value can
    // be found.
    mineBlocks(10);

    // After adding some blocks to the tip, best block should have changed.
    BOOST_CHECK(get_notify_tip() != curr_tip);

    // Grab block 1 from disk; we'll add it to the background chain later.
    std::shared_ptr<CBlock> pblockone = std::make_shared<CBlock>();
    {
        LOCK(::cs_main);
        chainman.m_blockman.ReadBlock(*pblockone, *chainman.ActiveChain()[1]);
    }

    BOOST_REQUIRE(CreateAndActivateUTXOSnapshot(
        this, NoMalleation, /*reset_chainstate=*/ true));

    // Ensure our active chain is the snapshot chainstate.
    BOOST_CHECK(WITH_LOCK(::cs_main, return chainman.CurrentChainstate().m_from_snapshot_blockhash));

    curr_tip = get_notify_tip();

    // Mine a new block on top of the activated snapshot chainstate.
    mineBlocks(1);  // Defined in TestChain100Setup.

    // After adding some blocks to the snapshot tip, best block should have changed.
    BOOST_CHECK(get_notify_tip() != curr_tip);

    curr_tip = get_notify_tip();

    Chainstate& background_cs{*Assert(WITH_LOCK(::cs_main, return chainman.HistoricalChainstate()))};

    // Append the first block to the background chain.
    BlockValidationState state;
    CBlockIndex* pindex = nullptr;
    const CChainParams& chainparams = Params();
    bool newblock = false;

    // TODO: much of this is inlined from ProcessNewBlock(); just reuse PNB()
    // once it is changed to support multiple chainstates.
    {
        LOCK(::cs_main);
        bool checked = CheckBlock(*pblockone, state, chainparams.GetConsensus());
        BOOST_CHECK(checked);
        bool accepted = chainman.AcceptBlock(
            pblockone, state, &pindex, true, nullptr, &newblock, true);
        BOOST_CHECK(accepted);
    }

    // UpdateTip is called here
    bool block_added = background_cs.ActivateBestChain(state, pblockone);

    // Ensure tip is as expected
    BOOST_CHECK_EQUAL(background_cs.m_chain.Tip()->GetBlockHash(), pblockone->GetHash());

    // get_notify_tip() should be unchanged after adding a block to the background
    // validation chain.
    BOOST_CHECK(block_added);
    BOOST_CHECK_EQUAL(curr_tip, get_notify_tip());
}

static void check_activate_best_chain_fatal_error(const std::string& expected_log, node::NodeContext& node, Chainstate& chainstate)
{
    ASSERT_DEBUG_LOG(expected_log);
    BlockValidationState state;
    BOOST_CHECK(!chainstate.ActivateBestChain(state));
    BOOST_CHECK_EQUAL(node.exit_status.load(), EXIT_FAILURE); // Ensure fatal error
    node.exit_status = EXIT_SUCCESS;
}

// Verify ActivateBestChain triggers a fatal error when a block read fails
// during block connection (ConnectTip path).
BOOST_FIXTURE_TEST_CASE(activate_best_chain_connect_io_failure, TestChain100Setup)
{
    auto& chainman = m_node.chainman;
    auto& chainstate = chainman->ActiveChainstate();
    const auto blocks_dir = m_args.GetBlocksDirPath();

    // Disconnect block without changing validity so ActivateBestChain
    // has a block to reconnect.
    {
        LOCK2(chainman->GetMutex(), chainstate.MempoolMutex());
        BlockValidationState state;
        BOOST_CHECK(chainstate.DisconnectTip(state, /*disconnectpool=*/nullptr));
    }

    // Inaccessible block file must trigger a fatal error.
    const auto blk_file = blocks_dir / "blk00000.dat";
    SimulateFileSystemError(m_path_root, blk_file, [&]() {
        check_activate_best_chain_fatal_error("Failed to read block", m_node, chainstate);
    });

    // Inaccessible blocks directory must also trigger a fatal error.
    const auto parent_dir = blocks_dir.parent_path();
    SimulateFileSystemError(m_path_root, parent_dir, [&]() {
        check_activate_best_chain_fatal_error("Failed to read block", m_node, chainstate);
    });

    // Sanity check: the block reconnects once the filesystem is restored.
    BlockValidationState state;
    BOOST_CHECK(chainstate.ActivateBestChain(state));
}

// Verify ActivateBestChain triggers a fatal error when a block read fails
// during a reorg (DisconnectTip path).
BOOST_FIXTURE_TEST_CASE(activate_best_chain_disconnect_io_failure, TestChain100Setup)
{
    auto& chainman = m_node.chainman;
    auto& chainstate = chainman->ActiveChainstate();
    const auto blocks_dir = m_args.GetBlocksDirPath();

    // Set up a scenario where ActivateBestChain must reorg:
    //
    //   Original chain: ... -> 100 -> 101 -> 102 -> 103   (more work)
    //   Fork:           ... -> 100 -> F101 -> F102        (currently active)
    //
    // The fork is active but has less work than the original chain, so
    // ActivateBestChain will disconnect the fork blocks via DisconnectTip
    // before reconnecting the original chain.

    // Extend the chain by 3 blocks (heights 101–103).
    for (int i = 0; i < 3; i++) CreateAndProcessBlock({}, CScript() << OP_TRUE, &chainstate);

    // Invalidate at height 101 to create room for a shorter fork.
    CBlockIndex* b101_index = WITH_LOCK(chainman->GetMutex(), return chainman->ActiveChain()[101]);
    BlockValidationState state;
    chainstate.InvalidateBlock(state, b101_index);

    // Build a 2-block fork from height 100. The original chain is invalid,
    // so these become the active tip.
    for (int i = 0; i < 2; i++) CreateAndProcessBlock({}, CScript() << OP_TRUE << OP_TRUE, &chainstate);

    // Now restore the original chain's validity. It has more work (103 > 102) but ActivateBestChain
    // hasn't been called yet, so we're still on the fork. Similar to ReconsiderBlock but without the ABC call.
    {
        LOCK(chainman->GetMutex());
        chainstate.ResetBlockFailureFlags(b101_index);
        chainman->RecalculateBestHeader();
    }

    // Inaccessible block file during reorg must trigger fatal error.
    const auto blk_file = blocks_dir / "blk00000.dat";
    SimulateFileSystemError(m_path_root, blk_file, [&]() {
        check_activate_best_chain_fatal_error("Failed to disconnect block", m_node, chainstate);
    });

    // Inaccessible undo file during reorg must trigger fatal error.
    const auto rev_file = blocks_dir / "rev00000.dat";
    SimulateFileSystemError(m_path_root, rev_file, [&]() {
        check_activate_best_chain_fatal_error("Failed to disconnect block", m_node, chainstate);
    });

    // Inaccessible blocks directory during reorg must also trigger fatal error.
    const auto parent_dir = blocks_dir.parent_path();
    SimulateFileSystemError(m_path_root, parent_dir, [&]() {
        check_activate_best_chain_fatal_error("Failed to disconnect block", m_node, chainstate);
    });

    // Sanity check: the reorg completes once the filesystem is restored.
    state = BlockValidationState();
    BOOST_CHECK(chainstate.ActivateBestChain(state));
    BOOST_CHECK_EQUAL(103, WITH_LOCK(chainman->GetMutex(), return chainman->ActiveHeight()));
}

BOOST_AUTO_TEST_SUITE_END()
