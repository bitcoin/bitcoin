// Copyright (c) 2017-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addresstype.h>
#include <blockfilter.h>
#include <chain.h>
#include <index/blockfilterindex.h>
#include <interfaces/chain.h>
#include <key.h>
#include <node/blockstorage.h>
#include <primitives/block.h>
#include <script/script.h>
#include <sync.h>
#include <test/util/blockfilter.h>
#include <test/util/common.h>
#include <test/util/index.h>
#include <test/util/mining.h>
#include <test/util/setup_common.h>
#include <uint256.h>
#include <util/check.h>
#include <validation.h>

#include <boost/test/unit_test.hpp>

#include <compare>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>

using node::BlockManager;

BOOST_AUTO_TEST_SUITE(blockfilter_index_tests)

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

    BOOST_CHECK(filter_index.LookupFilter({block_index->GetBlockHash(), block_index->nHeight}, filter));
    BOOST_CHECK(filter_index.LookupFilterHeader({block_index->GetBlockHash(), block_index->nHeight}, filter_header));
    BOOST_CHECK(filter_index.LookupFilterRange(block_index->nHeight, {block_index->GetBlockHash(), block_index->nHeight}, filters));
    BOOST_CHECK(filter_index.LookupFilterHashRange(block_index->nHeight, {block_index->GetBlockHash(), block_index->nHeight},
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

BOOST_FIXTURE_TEST_CASE(blockfilter_index_initial_sync, TestChain100Setup)
{
    BlockFilterIndex filter_index(interfaces::MakeChain(m_node), BlockFilterType::BASIC, 1_MiB, true);
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
             block_index = m_node.chainman->ActiveChain().Next(*block_index)) {
            BOOST_CHECK(!filter_index.LookupFilter({block_index->GetBlockHash(), block_index->nHeight}, filter));
            BOOST_CHECK(!filter_index.LookupFilterHeader({block_index->GetBlockHash(), block_index->nHeight}, filter_header));
            BOOST_CHECK(!filter_index.LookupFilterRange(block_index->nHeight, {block_index->GetBlockHash(), block_index->nHeight}, filters));
            BOOST_CHECK(!filter_index.LookupFilterHashRange(block_index->nHeight, {block_index->GetBlockHash(), block_index->nHeight},
                                                            filter_hashes));
        }
    }

    // BlockUntilSyncedToCurrentChain should return false before index is started.
    BOOST_CHECK(!filter_index.BlockUntilSyncedToCurrentChain());
    IndexTester{filter_index}.Sync();

    // Check that filter index has all blocks that were in the chain before it started.
    {
        LOCK(cs_main);
        const CBlockIndex* block_index;
        for (block_index = m_node.chainman->ActiveChain().Genesis();
             block_index != nullptr;
             block_index = m_node.chainman->ActiveChain().Next(*block_index)) {
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
    BOOST_REQUIRE(BuildChain(m_node, tip, coinbase_script_pub_key_A, 10, chainA));
    BOOST_REQUIRE(BuildChain(m_node, tip, coinbase_script_pub_key_B, 10, chainB));

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
    BOOST_CHECK(filter_index.LookupFilterRange(0, {tip->GetBlockHash(), tip->nHeight}, filters));
    BOOST_CHECK(filter_index.LookupFilterHashRange(0, {tip->GetBlockHash(), tip->nHeight}, filter_hashes));

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

    BOOST_CHECK(InitBlockFilterIndex([&]{ return interfaces::MakeChain(m_node); }, BlockFilterType::BASIC, 1_MiB, true, false));

    filter_index = GetBlockFilterIndex(BlockFilterType::BASIC);
    BOOST_CHECK(filter_index != nullptr);
    BOOST_CHECK(filter_index->GetFilterType() == BlockFilterType::BASIC);

    // Initialize returns false if index already exists.
    BOOST_CHECK(!InitBlockFilterIndex([&]{ return interfaces::MakeChain(m_node); }, BlockFilterType::BASIC, 1_MiB, true, false));

    int iter_count = 0;
    ForEachBlockFilterIndex([&iter_count](BlockFilterIndex& _index) { iter_count++; });
    BOOST_CHECK_EQUAL(iter_count, 1);

    BOOST_CHECK(DestroyBlockFilterIndex(BlockFilterType::BASIC));

    // Destroy returns false because index was already destroyed.
    BOOST_CHECK(!DestroyBlockFilterIndex(BlockFilterType::BASIC));

    filter_index = GetBlockFilterIndex(BlockFilterType::BASIC);
    BOOST_CHECK(filter_index == nullptr);

    // Reinitialize index.
    BOOST_CHECK(InitBlockFilterIndex([&]{ return interfaces::MakeChain(m_node); }, BlockFilterType::BASIC, 1_MiB, true, false));

    DestroyAllBlockFilterIndexes();

    filter_index = GetBlockFilterIndex(BlockFilterType::BASIC);
    BOOST_CHECK(filter_index == nullptr);
}

BOOST_AUTO_TEST_SUITE_END()
