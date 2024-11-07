// Copyright (c) 2017-2022 The Bitcoin Core developers
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
#include <test/util/index.h>
#include <test/util/setup_common.h>
#include <validation.h>

#include <boost/test/unit_test.hpp>

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
    std::unique_ptr<CBlockTemplate> pblocktemplate = BlockAssembler{m_node.chainman->ActiveChainstate(), m_node.mempool.get(), options}.CreateNewBlock(scriptPubKey);
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
        CBlockHeader header = block->GetBlockHeader();

        BlockValidationState state;
        if (!Assert(m_node.chainman)->ProcessNewBlockHeaders({{header}}, true, state, &pindex)) {
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

    BOOST_REQUIRE(filter_index.StartBackgroundSync());

    // Allow filter index to catch up with the block index.
    IndexWaitSynced(filter_index, *Assert(m_node.shutdown_signal));

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

BOOST_AUTO_TEST_SUITE_END()
