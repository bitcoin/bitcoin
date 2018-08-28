// Copyright (c) 2017-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blockfilter.h>
#include <chainparams.h>
#include <index/blockfilterindex.h>
#include <test/test_bitcoin.h>
#include <script/standard.h>
#include <validation.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(blockfilter_index_tests)

static bool ComputeFilter(BlockFilterType filter_type, const CBlockIndex* block_index,
                          BlockFilter& filter)
{
    CBlock block;
    if (!ReadBlockFromDisk(block, block_index->GetBlockPos(), Params().GetConsensus())) {
        return false;
    }

    CBlockUndo block_undo;
    if (block_index->nHeight > 0 && !UndoReadFromDisk(block_undo, block_index)) {
        return false;
    }

    filter = BlockFilter(filter_type, block, block_undo);
    return true;
}

static bool CheckFilterLookups(BlockFilterIndex& filter_index, const CBlockIndex* block_index,
                               uint256& last_header)
{
    BlockFilter expected_filter;
    if (!ComputeFilter(filter_index.GetFilterType(), block_index, expected_filter)) {
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

    BOOST_CHECK_EQUAL(filters.size(), 1);
    BOOST_CHECK_EQUAL(filter_hashes.size(), 1);

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
    BlockFilterIndex filter_index(BlockFilterType::BASIC, 1 << 20, true);

    uint256 last_header;

    // Filter should not be found in the index before it is started.
    {
        LOCK(cs_main);

        BlockFilter filter;
        uint256 filter_header;
        std::vector<BlockFilter> filters;
        std::vector<uint256> filter_hashes;

        for (const CBlockIndex* block_index = chainActive.Genesis();
             block_index != nullptr;
             block_index = chainActive.Next(block_index)) {
            BOOST_CHECK(!filter_index.LookupFilter(block_index, filter));
            BOOST_CHECK(!filter_index.LookupFilterHeader(block_index, filter_header));
            BOOST_CHECK(!filter_index.LookupFilterRange(block_index->nHeight, block_index, filters));
            BOOST_CHECK(!filter_index.LookupFilterHashRange(block_index->nHeight, block_index,
                                                            filter_hashes));
        }
    }

    // BlockUntilSyncedToCurrentChain should return false before index is started.
    BOOST_CHECK(!filter_index.BlockUntilSyncedToCurrentChain());

    filter_index.Start();

    // Allow filter index to catch up with the block index.
    constexpr int64_t timeout_ms = 10 * 1000;
    int64_t time_start = GetTimeMillis();
    while (!filter_index.BlockUntilSyncedToCurrentChain()) {
        BOOST_REQUIRE(time_start + timeout_ms > GetTimeMillis());
        MilliSleep(100);
    }

    // Check that filter index has all blocks that were in the chain before it started.
    {
        LOCK(cs_main);
        const CBlockIndex* block_index;
        for (block_index = chainActive.Genesis();
             block_index != nullptr;
             block_index = chainActive.Next(block_index)) {
            CheckFilterLookups(filter_index, block_index, last_header);
        }
    }

    // Check that new blocks get indexed.
    for (int i = 0; i < 10; i++) {
        CScript coinbase_script_pub_key = GetScriptForDestination(coinbaseKey.GetPubKey().GetID());
        std::vector<CMutableTransaction> no_txns;
        const CBlock& block = CreateAndProcessBlock(no_txns, coinbase_script_pub_key);
        const CBlockIndex* block_index;
        {
            LOCK(cs_main);
            block_index = LookupBlockIndex(block.GetHash());
        }

        BOOST_CHECK(filter_index.BlockUntilSyncedToCurrentChain());
        CheckFilterLookups(filter_index, block_index, last_header);
    }

    // Test lookups for a range of filters/hashes.
    std::vector<BlockFilter> filters;
    std::vector<uint256> filter_hashes;

    const CBlockIndex* block_index = chainActive.Tip();
    BOOST_CHECK(filter_index.LookupFilterRange(0, block_index, filters));
    BOOST_CHECK(filter_index.LookupFilterHashRange(0, block_index, filter_hashes));

    BOOST_CHECK_EQUAL(filters.size(), chainActive.Height() + 1);
    BOOST_CHECK_EQUAL(filter_hashes.size(), chainActive.Height() + 1);

    filters.clear();
    filter_hashes.clear();

    filter_index.Interrupt();
    filter_index.Stop();
}

BOOST_AUTO_TEST_SUITE_END()
