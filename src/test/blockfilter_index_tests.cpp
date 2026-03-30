// Copyright (c) 2017-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addresstype.h>
#include <blockfilter.h>
#include <chainparams.h>
#include <consensus/merkle.h>
#include <consensus/validation.h>
#include <index/blockfilterindex.h>
#include <interfaces/chain.h>
#include <node/miner.h>
#include <pow.h>
#include <test/util/blockfilter.h>
#include <test/util/common.h>
#include <test/util/setup_common.h>
#include <util/fs_helpers.h>
#include <validation.h>

#include <boost/test/unit_test.hpp>
#include <future>
#include <latch>

using node::BlockAssembler;
using node::BlockManager;
using node::CBlockTemplate;

BOOST_AUTO_TEST_SUITE(blockfilter_index_tests)

struct BuildChainTestingSetup : public TestChain100Setup {
    CBlock CreateBlock(const CBlockIndex* prev, const std::vector<CMutableTransaction>& txns, const CScript& scriptPubKey);
    bool BuildChain(const CBlockIndex* pindex, const CScript& coinbase_script_pub_key, size_t length, std::vector<std::shared_ptr<CBlock>>& chain);
};

static bool CheckFilterLookups(BlockFilterIndex& filter_index, const CBlockIndex* block_index,
                               uint256& last_header, const BlockManager& blockman)
{
    BlockFilter expected_filter;
    if (!ComputeFilter(filter_index.GetFilterType(), *block_index, expected_filter, blockman)) {
        BOOST_ERROR("ComputeFilter failed on block " << block_index->nHeight);
        return false;
    }

    BlockFilter filter;
    uint256 filter_header;
    std::vector<BlockFilter> filters;
    std::vector<uint256> filter_hashes;

    BOOST_CHECK(filter_index.LookupFilter(block_index, filter));
    BOOST_CHECK(filter_index.LookupFilterHeader(block_index, filter_header));
    BOOST_CHECK(filter_index.LookupFilterRange(block_index->nHeight, block_index, filters));
    BOOST_CHECK(filter_index.LookupFilterHashRange(block_index->nHeight, block_index,
                                                   filter_hashes));

    BOOST_CHECK_EQUAL(filters.size(), 1U);
    BOOST_CHECK_EQUAL(filter_hashes.size(), 1U);

    BOOST_CHECK_EQUAL(filter.GetHash(), expected_filter.GetHash());
    BOOST_CHECK_EQUAL(filter_header, expected_filter.ComputeHeader(last_header));
    BOOST_CHECK_EQUAL(filters[0].GetHash(), expected_filter.GetHash());
    BOOST_CHECK_EQUAL(filter_hashes[0], expected_filter.GetHash());

    filters.clear();
    filter_hashes.clear();
    last_header = filter_header;
    return true;
}

CBlock BuildChainTestingSetup::CreateBlock(const CBlockIndex* prev,
    const std::vector<CMutableTransaction>& txns,
    const CScript& scriptPubKey)
{
    BlockAssembler::Options options;
    options.coinbase_output_script = scriptPubKey;
    options.include_dummy_extranonce = true;
    std::unique_ptr<CBlockTemplate> pblocktemplate = BlockAssembler{m_node.chainman->ActiveChainstate(), m_node.mempool.get(), options}.CreateNewBlock();
    CBlock& block = pblocktemplate->block;
    block.hashPrevBlock = prev->GetBlockHash();
    block.nTime = prev->nTime + 1;

    // Replace mempool-selected txns with just coinbase plus passed-in txns:
    block.vtx.resize(1);
    for (const CMutableTransaction& tx : txns) {
        block.vtx.push_back(MakeTransactionRef(tx));
    }
    {
        CMutableTransaction tx_coinbase{*block.vtx.at(0)};
        tx_coinbase.nLockTime = static_cast<uint32_t>(prev->nHeight);
        tx_coinbase.vin.at(0).scriptSig = CScript{} << prev->nHeight + 1;
        block.vtx.at(0) = MakeTransactionRef(std::move(tx_coinbase));
        block.hashMerkleRoot = BlockMerkleRoot(block);
    }

    while (!CheckProofOfWork(block.GetHash(), block.nBits, m_node.chainman->GetConsensus())) ++block.nNonce;

    return block;
}

bool BuildChainTestingSetup::BuildChain(const CBlockIndex* pindex,
    const CScript& coinbase_script_pub_key,
    size_t length,
    std::vector<std::shared_ptr<CBlock>>& chain)
{
    std::vector<CMutableTransaction> no_txns;

    chain.resize(length);
    for (auto& block : chain) {
        block = std::make_shared<CBlock>(CreateBlock(pindex, no_txns, coinbase_script_pub_key));

        BlockValidationState state;
        if (!Assert(m_node.chainman)->ProcessNewBlockHeaders({{*block}}, true, state, &pindex)) {
            return false;
        }
    }

    return true;
}

BOOST_FIXTURE_TEST_CASE(blockfilter_index_initial_sync, BuildChainTestingSetup)
{
    BlockFilterIndex filter_index(interfaces::MakeChain(m_node), BlockFilterType::BASIC, 1 << 20, true);
    BOOST_REQUIRE(filter_index.Init());

    uint256 last_header;

    // Filter should not be found in the index before it is started.
    {
        LOCK(cs_main);

        BlockFilter filter;
        uint256 filter_header;
        std::vector<BlockFilter> filters;
        std::vector<uint256> filter_hashes;

        for (const CBlockIndex* block_index = m_node.chainman->ActiveChain().Genesis();
             block_index != nullptr;
             block_index = m_node.chainman->ActiveChain().Next(block_index)) {
            BOOST_CHECK(!filter_index.LookupFilter(block_index, filter));
            BOOST_CHECK(!filter_index.LookupFilterHeader(block_index, filter_header));
            BOOST_CHECK(!filter_index.LookupFilterRange(block_index->nHeight, block_index, filters));
            BOOST_CHECK(!filter_index.LookupFilterHashRange(block_index->nHeight, block_index,
                                                            filter_hashes));
        }
    }

    // BlockUntilSyncedToCurrentChain should return false before index is started.
    BOOST_CHECK(!filter_index.BlockUntilSyncedToCurrentChain());

    filter_index.Sync();

    // Check that filter index has all blocks that were in the chain before it started.
    {
        LOCK(cs_main);
        const CBlockIndex* block_index;
        for (block_index = m_node.chainman->ActiveChain().Genesis();
             block_index != nullptr;
             block_index = m_node.chainman->ActiveChain().Next(block_index)) {
            CheckFilterLookups(filter_index, block_index, last_header, m_node.chainman->m_blockman);
        }
    }

    // Create two forks.
    const CBlockIndex* tip;
    {
        LOCK(cs_main);
        tip = m_node.chainman->ActiveChain().Tip();
    }
    CKey coinbase_key_A = GenerateRandomKey();
    CKey coinbase_key_B = GenerateRandomKey();
    CScript coinbase_script_pub_key_A = GetScriptForDestination(PKHash(coinbase_key_A.GetPubKey()));
    CScript coinbase_script_pub_key_B = GetScriptForDestination(PKHash(coinbase_key_B.GetPubKey()));
    std::vector<std::shared_ptr<CBlock>> chainA, chainB;
    BOOST_REQUIRE(BuildChain(tip, coinbase_script_pub_key_A, 10, chainA));
    BOOST_REQUIRE(BuildChain(tip, coinbase_script_pub_key_B, 10, chainB));

    // Check that new blocks on chain A get indexed.
    uint256 chainA_last_header = last_header;
    for (size_t i = 0; i < 2; i++) {
        const auto& block = chainA[i];
        BOOST_REQUIRE(Assert(m_node.chainman)->ProcessNewBlock(block, true, true, nullptr));
    }
    for (size_t i = 0; i < 2; i++) {
        const auto& block = chainA[i];
        const CBlockIndex* block_index;
        {
            LOCK(cs_main);
            block_index = m_node.chainman->m_blockman.LookupBlockIndex(block->GetHash());
        }

        BOOST_CHECK(filter_index.BlockUntilSyncedToCurrentChain());
        CheckFilterLookups(filter_index, block_index, chainA_last_header, m_node.chainman->m_blockman);
    }

    // Reorg to chain B.
    uint256 chainB_last_header = last_header;
    for (size_t i = 0; i < 3; i++) {
        const auto& block = chainB[i];
        BOOST_REQUIRE(Assert(m_node.chainman)->ProcessNewBlock(block, true, true, nullptr));
    }
    for (size_t i = 0; i < 3; i++) {
        const auto& block = chainB[i];
        const CBlockIndex* block_index;
        {
            LOCK(cs_main);
            block_index = m_node.chainman->m_blockman.LookupBlockIndex(block->GetHash());
        }

        BOOST_CHECK(filter_index.BlockUntilSyncedToCurrentChain());
        CheckFilterLookups(filter_index, block_index, chainB_last_header, m_node.chainman->m_blockman);
    }

    // Check that filters for stale blocks on A can be retrieved.
    chainA_last_header = last_header;
    for (size_t i = 0; i < 2; i++) {
        const auto& block = chainA[i];
        const CBlockIndex* block_index;
        {
            LOCK(cs_main);
            block_index = m_node.chainman->m_blockman.LookupBlockIndex(block->GetHash());
        }

        BOOST_CHECK(filter_index.BlockUntilSyncedToCurrentChain());
        CheckFilterLookups(filter_index, block_index, chainA_last_header, m_node.chainman->m_blockman);
    }

    // Reorg back to chain A.
     for (size_t i = 2; i < 4; i++) {
         const auto& block = chainA[i];
         BOOST_REQUIRE(Assert(m_node.chainman)->ProcessNewBlock(block, true, true, nullptr));
     }

     // Check that chain A and B blocks can be retrieved.
     chainA_last_header = last_header;
     chainB_last_header = last_header;
     for (size_t i = 0; i < 3; i++) {
         const CBlockIndex* block_index;

         {
             LOCK(cs_main);
             block_index = m_node.chainman->m_blockman.LookupBlockIndex(chainA[i]->GetHash());
         }
         BOOST_CHECK(filter_index.BlockUntilSyncedToCurrentChain());
         CheckFilterLookups(filter_index, block_index, chainA_last_header, m_node.chainman->m_blockman);

         {
             LOCK(cs_main);
             block_index = m_node.chainman->m_blockman.LookupBlockIndex(chainB[i]->GetHash());
         }
         BOOST_CHECK(filter_index.BlockUntilSyncedToCurrentChain());
         CheckFilterLookups(filter_index, block_index, chainB_last_header, m_node.chainman->m_blockman);
     }

    // Test lookups for a range of filters/hashes.
    std::vector<BlockFilter> filters;
    std::vector<uint256> filter_hashes;

    {
        LOCK(cs_main);
        tip = m_node.chainman->ActiveChain().Tip();
    }
    BOOST_CHECK(filter_index.LookupFilterRange(0, tip, filters));
    BOOST_CHECK(filter_index.LookupFilterHashRange(0, tip, filter_hashes));

    assert(tip->nHeight >= 0);
    BOOST_CHECK_EQUAL(filters.size(), tip->nHeight + 1U);
    BOOST_CHECK_EQUAL(filter_hashes.size(), tip->nHeight + 1U);

    filters.clear();
    filter_hashes.clear();

    filter_index.Interrupt();
    filter_index.Stop();
}

BOOST_FIXTURE_TEST_CASE(blockfilter_index_init_destroy, BasicTestingSetup)
{
    BlockFilterIndex* filter_index;

    filter_index = GetBlockFilterIndex(BlockFilterType::BASIC);
    BOOST_CHECK(filter_index == nullptr);

    BOOST_CHECK(InitBlockFilterIndex([&]{ return interfaces::MakeChain(m_node); }, BlockFilterType::BASIC, 1 << 20, true, false));

    filter_index = GetBlockFilterIndex(BlockFilterType::BASIC);
    BOOST_CHECK(filter_index != nullptr);
    BOOST_CHECK(filter_index->GetFilterType() == BlockFilterType::BASIC);

    // Initialize returns false if index already exists.
    BOOST_CHECK(!InitBlockFilterIndex([&]{ return interfaces::MakeChain(m_node); }, BlockFilterType::BASIC, 1 << 20, true, false));

    int iter_count = 0;
    ForEachBlockFilterIndex([&iter_count](BlockFilterIndex& _index) { iter_count++; });
    BOOST_CHECK_EQUAL(iter_count, 1);

    BOOST_CHECK(DestroyBlockFilterIndex(BlockFilterType::BASIC));

    // Destroy returns false because index was already destroyed.
    BOOST_CHECK(!DestroyBlockFilterIndex(BlockFilterType::BASIC));

    filter_index = GetBlockFilterIndex(BlockFilterType::BASIC);
    BOOST_CHECK(filter_index == nullptr);

    // Reinitialize index.
    BOOST_CHECK(InitBlockFilterIndex([&]{ return interfaces::MakeChain(m_node); }, BlockFilterType::BASIC, 1 << 20, true, false));

    DestroyAllBlockFilterIndexes();

    filter_index = GetBlockFilterIndex(BlockFilterType::BASIC);
    BOOST_CHECK(filter_index == nullptr);
}

class IndexReorgCrash : public BaseIndex
{
private:
    std::unique_ptr<BaseIndex::DB> m_db;
    std::shared_future<void> m_blocker;
    int m_blocking_height;

public:
    explicit IndexReorgCrash(std::unique_ptr<interfaces::Chain> chain, std::shared_future<void> blocker,
                             int blocking_height) : BaseIndex(std::move(chain), "test index"), m_blocker(blocker),
                                                    m_blocking_height(blocking_height)
    {
        const fs::path path = gArgs.GetDataDirNet() / "index";
        fs::create_directories(path);
        m_db = std::make_unique<BaseIndex::DB>(path / "db", /*n_cache_size=*/0, /*f_memory=*/true, /*f_wipe=*/false);
    }

    bool AllowPrune() const override { return false; }
    BaseIndex::DB& GetDB() const override { return *m_db; }

    bool CustomAppend(CDBBatch& db_batch, const interfaces::BlockInfo& block) override
    {
        // Simulate a delay so new blocks can get connected during the initial sync
        if (block.height == m_blocking_height) m_blocker.wait();

        // Move mock time forward so the best index gets updated only when we are not at the blocking height
        if (block.height == m_blocking_height - 1 || block.height > m_blocking_height) {
            SetMockTime(GetTime<std::chrono::seconds>() + 31s);
        }

        return true;
    }
};

template <typename F>
static bool WaitUntil(F fn, const std::chrono::milliseconds timeout)
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (!fn()) {
        if (std::chrono::steady_clock::now() > deadline) return false;
        std::this_thread::sleep_for(100ms);
    }
    return true;
}

static void WaitForHeight(const BaseIndex* index, const int height, const std::chrono::milliseconds timeout)
{
    if (!WaitUntil([index, height]() { return index->GetSummary().best_block_height == height; }, timeout)) {
        BOOST_FAIL(strprintf("Timeout waiting for index height %d (current: %d)", height, index->GetSummary().best_block_height));
    }
}

BOOST_FIXTURE_TEST_CASE(index_reorg_crash, BuildChainTestingSetup)
{
    // Enable mock time
    SetMockTime(GetTime<std::chrono::minutes>());

    std::promise<void> promise;
    std::shared_future<void> blocker(promise.get_future());
    int blocking_height = WITH_LOCK(cs_main, return m_node.chainman->ActiveChain().Tip()->nHeight);

    IndexReorgCrash index(interfaces::MakeChain(m_node), blocker, blocking_height);
    index.SetProcessingBatchSize(1);
    BOOST_REQUIRE(index.Init());
    BOOST_REQUIRE(index.StartBackgroundSync());

    // Wait until the index is one block before the fork point
    WaitForHeight(&index, blocking_height - 1, /*timeout=*/5s);

    // Create a fork to trigger the reorg
    std::vector<std::shared_ptr<CBlock>> fork;
    const CBlockIndex* prev_tip = WITH_LOCK(cs_main, return m_node.chainman->ActiveChain().Tip()->pprev);
    BOOST_REQUIRE(BuildChain(prev_tip, GetScriptForDestination(PKHash(GenerateRandomKey().GetPubKey())), 3, fork));

    for (const auto& block : fork) {
        BOOST_REQUIRE(m_node.chainman->ProcessNewBlock(block, /*force_processing=*/true, /*min_pow_checked=*/true, nullptr));
    }

    // Unblock the index thread so it can process the reorg
    promise.set_value();
    // Wait for the index to reach the new tip
    WaitForHeight(&index, blocking_height + 2, 5s);
    index.Stop();
}

// Ensure the initial sync batch window behaves as expected.
// Tests sync from the genesis block and from a higher block to mimic a restart.
// Note: Test runs in /tmp by default, which is usually cached, so timings are not
// a meaningful benchmark (use -testdatadir to run it elsewhere).
BOOST_FIXTURE_TEST_CASE(initial_sync_batch_window, BuildChainTestingSetup)
{
    constexpr int MINE_BLOCKS = 173;
    constexpr int BATCH_SIZE = 30;

    int expected_tip = 100; // pre-mined blocks
    for (int round = 0; round < 2; round++) { // two rounds to test sync from genesis and from a higher block
        mineBlocks(MINE_BLOCKS); // Generate blocks
        const int tip_height = WITH_LOCK(::cs_main, return m_node.chainman->ActiveChain().Height());
        BOOST_REQUIRE(tip_height == MINE_BLOCKS + expected_tip);
        expected_tip = tip_height;

        BlockFilterIndex filter_index(interfaces::MakeChain(m_node), BlockFilterType::BASIC, 1 << 20, /*f_memory=*/false);
        filter_index.SetProcessingBatchSize(BATCH_SIZE);
        BOOST_REQUIRE(filter_index.Init());
        BOOST_CHECK(!filter_index.BlockUntilSyncedToCurrentChain());

        // Ensure we can sync up to the tip
        filter_index.Sync();
        const auto& summary{filter_index.GetSummary()};
        BOOST_CHECK(summary.synced);
        BOOST_CHECK_EQUAL(summary.best_block_height, expected_tip);

        {
            // Verify all blocks up to the tip exist, always start from genesis to verify nothing was overwritten
            LOCK(::cs_main);
            const auto& chain = m_node.chainman->ActiveChain();
            uint256 last_header;
            for (auto pblock = chain.Genesis(); pblock; pblock = chain.Next(pblock)) {
                CheckFilterLookups(filter_index, pblock, last_header, m_node.chainman->m_blockman);
            }
        }

        filter_index.Interrupt();
        filter_index.Stop();
    }
}

// A BaseIndex subclass that simulates delays during 'CustomAppend'.
// This allows testing more complex scenarios, such as reorgs during initial sync.
class IndexBlockSim : public BaseIndex
{
    std::unique_ptr<BaseIndex::DB> m_db;
    std::latch m_reached_block{1};
    std::latch m_continue_sync{1};
    int m_blocking_height;
    // Guards against dup notifications when the blocking height is reprocessed during reorgs
    std::atomic<bool> m_has_notified{false};

public:
    IndexBlockSim(const fs::path& parent_dir, std::unique_ptr<interfaces::Chain> chain, bool f_memory, const int blocking_height) :
        BaseIndex(std::move(chain), "sim_index"), m_blocking_height(blocking_height)
    {
        const fs::path path = parent_dir / "index" / fs::PathFromString(GetName());
        TryCreateDirectories(path);
        m_db = std::make_unique<BaseIndex::DB>(path / "db", /*n_cache_size=*/0, f_memory, /*f_wipe=*/false);
    }

    void wait_at_blocking_point() const { m_reached_block.wait(); }
    void allow_continue() { m_continue_sync.count_down(); }

protected:
    bool CustomAppend(CDBBatch& batch, const interfaces::BlockInfo& block) override {
        if (block.height == m_blocking_height && !m_has_notified.exchange(true)) {
            m_reached_block.count_down(); // notify waiters
            m_continue_sync.wait(); // pause at the blocking height
        }
        return BaseIndex::CustomAppend(batch, block);
    }

    bool AllowPrune() const override { return false; }
    BaseIndex::DB& GetDB() const override { return *m_db; }
};

// Tests that indexes can complete its initial sync even if a reorg occurs mid-sync.
// The index is paused at a specific block while a fork is introduced a few blocks before the tip.
// Once unblocked, the index should continue syncing and correctly reach the new chain tip.
BOOST_FIXTURE_TEST_CASE(initial_sync_reorg, BuildChainTestingSetup)
{
    const auto& chainman = m_node.chainman;

    // Create a filter index that will block during initial sync
    const int start_tip_height = WITH_LOCK(::cs_main, return chainman->ActiveChain().Height());
    const int blocking_height = start_tip_height - 1;
    IndexBlockSim index(m_args.GetDataDirNet(), interfaces::MakeChain(m_node), /*f_memory=*/true, blocking_height);
    index.SetProcessingBatchSize(1);
    BOOST_REQUIRE(index.Init());
    BOOST_REQUIRE(index.StartBackgroundSync());
    // Wait for the index to reach the blocking point
    index.wait_at_blocking_point();

    // Create a fork starting 3 blocks before the tip
    const CBlockIndex* fork_point = WITH_LOCK(cs_main, return chainman->ActiveChain()[chainman->ActiveChain().Height() - 3]);
    constexpr int fork_length = 5;
    std::vector<std::shared_ptr<CBlock>> fork_chain;
    BOOST_REQUIRE(BuildChain(fork_point, CScript() << OP_TRUE, fork_length, fork_chain));
    for (const auto& block : fork_chain) {
        BOOST_REQUIRE(chainman->ProcessNewBlock(block, /*force_processing=*/true, /*min_pow_checked=*/true, nullptr));
    }
    // Ensure we have fully processed the fork
    m_node.validation_signals->SyncWithValidationInterfaceQueue();
    const CBlockIndex* tip_block = WITH_LOCK(cs_main, return chainman->ActiveChain().Tip());
    BOOST_CHECK(start_tip_height != tip_block->nHeight);

    // Unblock the index and let it finish syncing
    index.allow_continue();
    index.Stop(); // Wait for background thread to stop

    // Verify index is fully synced with the current tip
    const auto& summary = index.GetSummary();
    BOOST_CHECK(index.GetSummary().synced);
    BOOST_CHECK_EQUAL(summary.best_block_height, tip_block->nHeight);
    BOOST_CHECK_EQUAL(summary.best_block_hash, tip_block->GetBlockHash());
}

// Verifies that the index persists its sync progress when interrupted during initial sync.
// The index should resume from the last processed batch rather than restarting from genesis.
BOOST_FIXTURE_TEST_CASE(shutdown_during_initial_sync, BuildChainTestingSetup)
{
    // The index will be interrupted at block 45. Due to the batch size of 10,
    // the last synced block will be block 39 after interruption (end of batch range [30-39]).
    constexpr int SHUTDOWN_HEIGHT = 45;
    constexpr int BATCH_SIZE = 10;
    constexpr int EXPECTED_LAST_SYNCED_BLOCK = 39;
    const auto& dir = m_args.GetDataDirNet();

    {
        // Create a filter index that will block during initial sync
        IndexBlockSim index(dir, interfaces::MakeChain(m_node), /*f_memory=*/false, /*blocking_height=*/SHUTDOWN_HEIGHT);
        index.SetProcessingBatchSize(BATCH_SIZE);
        BOOST_REQUIRE(index.Init());
        BOOST_REQUIRE(index.StartBackgroundSync());

        // Wait for the index to reach the blocking point
        index.wait_at_blocking_point();

        // Now mimic shutdown by interrupting the index and unblock sync
        index.Interrupt();
        index.allow_continue();
        index.Stop(); // Wait for background thread to stop

        // Check index ended with height at end of the last completed batch
        BOOST_CHECK_EQUAL(index.GetSummary().best_block_height, EXPECTED_LAST_SYNCED_BLOCK);
    }

    {
        // Check the index will resume from the last locator
        IndexBlockSim index(dir, interfaces::MakeChain(m_node), /*f_memory=*/false, /*blocking_height=*/SHUTDOWN_HEIGHT);
        BOOST_REQUIRE(index.Init());
        BOOST_CHECK_EQUAL(index.GetSummary().best_block_height, EXPECTED_LAST_SYNCED_BLOCK);
    }
}

BOOST_AUTO_TEST_SUITE_END()
