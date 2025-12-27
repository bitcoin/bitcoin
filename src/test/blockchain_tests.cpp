// Copyright (c) 2017-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <node/blockstorage.h>
#include <rpc/blockchain.h>
#include <sync.h>
#include <test/util/setup_common.h>
#include <util/string.h>

#include <boost/test/unit_test.hpp>
#include <array>

#include <algorithm>
#include <cstdlib>

using util::ToString;

/* Equality between doubles is imprecise. Comparison should be done
 * with a small threshold of tolerance, rather than exact equality.
 */
static bool DoubleEquals(double a, double b, double epsilon)
{
    return std::abs(a - b) < epsilon;
}

static CBlockIndex* CreateBlockIndexWithNbits(uint32_t nbits)
{
    CBlockIndex* block_index = new CBlockIndex();
    block_index->nHeight = 46367;
    block_index->nTime = 1269211443;
    block_index->nBits = nbits;
    return block_index;
}

static void RejectDifficultyMismatch(double difficulty, double expected_difficulty)
{
    BOOST_CHECK_MESSAGE(
        DoubleEquals(difficulty, expected_difficulty, 0.00001),
        "Difficulty was " + ToString(difficulty) + " but was expected to be " + ToString(expected_difficulty));
}

/* Given a BlockIndex with the provided nbits,
 * verify that the expected difficulty results.
 */
static void TestDifficulty(uint32_t nbits, double expected_difficulty)
{
    CBlockIndex* block_index = CreateBlockIndexWithNbits(nbits);
    double difficulty = GetDifficulty(*block_index);
    delete block_index;

    RejectDifficultyMismatch(difficulty, expected_difficulty);
}

BOOST_FIXTURE_TEST_SUITE(blockchain_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(get_difficulty_for_very_low_target)
{
    TestDifficulty(0x1f111111, 0.000001);
}

BOOST_AUTO_TEST_CASE(get_difficulty_for_low_target)
{
    TestDifficulty(0x1ef88f6f, 0.000016);
}

BOOST_AUTO_TEST_CASE(get_difficulty_for_mid_target)
{
    TestDifficulty(0x1df88f6f, 0.004023);
}

BOOST_AUTO_TEST_CASE(get_difficulty_for_high_target)
{
    TestDifficulty(0x1cf88f6f, 1.029916);
}

BOOST_AUTO_TEST_CASE(get_difficulty_for_very_high_target)
{
    TestDifficulty(0x12345678, 5913134931067755359633408.0);
}

//! Prune chain from height down to genesis block and check that
//! GetPruneHeight returns the correct value
static void CheckGetPruneHeight(node::BlockManager& blockman, CChain& chain, int height) EXCLUSIVE_LOCKS_REQUIRED(::cs_main)
{
    AssertLockHeld(::cs_main);

    // Emulate pruning all blocks from `height` down to the genesis block
    // by unsetting the `BLOCK_HAVE_DATA` flag from `nStatus`
    for (CBlockIndex* it{chain[height]}; it != nullptr && it->nHeight > 0; it = it->pprev) {
        it->nStatus &= ~BLOCK_HAVE_DATA;
    }

    const auto prune_height{GetPruneHeight(blockman, chain)};
    BOOST_REQUIRE(prune_height.has_value());
    BOOST_CHECK_EQUAL(*prune_height, height);
}

BOOST_FIXTURE_TEST_CASE(get_prune_height, TestChain100Setup)
{
    LOCK(::cs_main);
    auto& chain = m_node.chainman->ActiveChain();
    auto& blockman = m_node.chainman->m_blockman;

    // Fresh chain of 100 blocks without any pruned blocks, so std::nullopt should be returned
    BOOST_CHECK(!GetPruneHeight(blockman, chain).has_value());

    // Start pruning
    CheckGetPruneHeight(blockman, chain, 1);
    CheckGetPruneHeight(blockman, chain, 99);
    CheckGetPruneHeight(blockman, chain, 100);
}

BOOST_AUTO_TEST_CASE(num_chain_tx_max)
{
    CBlockIndex block_index{};
    block_index.m_chain_tx_count = std::numeric_limits<uint64_t>::max();
    BOOST_CHECK_EQUAL(block_index.m_chain_tx_count, std::numeric_limits<uint64_t>::max());
}

BOOST_AUTO_TEST_CASE(cblockindex_comparator_equivalence)
{
    // Old implementation to prove refactor correctness
    auto old_comparator{[](const CBlockIndex* pa, const CBlockIndex* pb) {
        // First sort by most total work, ...
        if (pa->nChainWork > pb->nChainWork)
            return false;
        if (pa->nChainWork < pb->nChainWork)
            return true;

        // ... then by earliest activatable time, ...
        if (pa->nSequenceId < pb->nSequenceId)
            return false;
        if (pa->nSequenceId > pb->nSequenceId)
            return true;

        // Use pointer address as tie breaker (should only happen with blocks
        // loaded from disk, as those share the same id: 0 for blocks on the
        // best chain, 1 for all others).
        if (pa < pb)
            return false;
        if (pa > pb)
            return true;

        // Identical blocks.
        return false;
    }};

    for (int test{0}; test < 100; ++test) {
        const node::CBlockIndexWorkComparator comparator;
        std::array<CBlockIndex, 2> ab; // defined pointer ordering
        CBlockIndex *a{&ab[0]}, *b{&ab[1]};

        // Generate random chain work and sequence IDs
        a->nChainWork = UintToArith256(m_rng.rand256());
        a->nSequenceId = int32_t(m_rng.rand32());
        // Add some duplicates to test self-equality and pointer tie break
        if (!m_rng.randrange(10)) {
            b = a;
        } else {
            b->nChainWork = m_rng.randbool() ? a->nChainWork : UintToArith256(m_rng.rand256());
            b->nSequenceId = m_rng.randbool() ? a->nSequenceId : int32_t(m_rng.rand32());
        }

        BOOST_CHECK_EQUAL(old_comparator(a, b), comparator(a, b));
        BOOST_CHECK_EQUAL(old_comparator(b, a), comparator(b, a));
    }
}

BOOST_AUTO_TEST_CASE(cblockindex_comparator_sorting_simple)
{
    constexpr std::array<std::pair<uint64_t, int32_t>, 4> original{{
        {1, 20},
        {1, 10},
        {2, 20},
        {2, 10},
    }};

    std::array<CBlockIndex, original.size()> pointer_storage{};
    std::array<CBlockIndex*, original.size()> indexes{};
    for (size_t i{0}; i < pointer_storage.size(); ++i) {
        pointer_storage[i].nChainWork = arith_uint256{original[i].first};
        pointer_storage[i].nSequenceId = original[i].second;
        indexes[i] = &pointer_storage[i];
    }

    std::ranges::shuffle(indexes, m_rng);
    std::ranges::sort(indexes, node::CBlockIndexWorkComparator{});

    for (size_t i{0}; i < indexes.size(); ++i) {
        BOOST_CHECK_EQUAL(indexes[i]->nChainWork, original[i].first);
        BOOST_CHECK_EQUAL(indexes[i]->nSequenceId, original[i].second);
    }
}

BOOST_FIXTURE_TEST_CASE(invalidate_block, TestChain100Setup)
{
    const CChain& active{*WITH_LOCK(Assert(m_node.chainman)->GetMutex(), return &Assert(m_node.chainman)->ActiveChain())};

    // Check BlockStatus when doing InvalidateBlock()
    BlockValidationState state;
    auto* orig_tip = active.Tip();
    int height_to_invalidate = orig_tip->nHeight - 10;
    auto* tip_to_invalidate = active[height_to_invalidate];
    m_node.chainman->ActiveChainstate().InvalidateBlock(state, tip_to_invalidate);

    // tip_to_invalidate just got invalidated, so it's BLOCK_FAILED_VALID
    WITH_LOCK(::cs_main, assert(tip_to_invalidate->nStatus & BLOCK_FAILED_VALID));
    WITH_LOCK(::cs_main, assert((tip_to_invalidate->nStatus & BLOCK_FAILED_CHILD) == 0));

    // check all ancestors of the invalidated block are validated up to BLOCK_VALID_TRANSACTIONS and are not invalid
    auto pindex = tip_to_invalidate->pprev;
    while (pindex) {
        WITH_LOCK(::cs_main, assert(pindex->IsValid(BLOCK_VALID_TRANSACTIONS)));
        WITH_LOCK(::cs_main, assert((pindex->nStatus & BLOCK_FAILED_MASK) == 0));
        pindex = pindex->pprev;
    }

    // check all descendants of the invalidated block are BLOCK_FAILED_CHILD
    pindex = orig_tip;
    while (pindex && pindex != tip_to_invalidate) {
        WITH_LOCK(::cs_main, assert((pindex->nStatus & BLOCK_FAILED_VALID) == 0));
        WITH_LOCK(::cs_main, assert(pindex->nStatus & BLOCK_FAILED_CHILD));
        pindex = pindex->pprev;
    }

    // don't mark already invalidated block (orig_tip is BLOCK_FAILED_CHILD) with BLOCK_FAILED_VALID again
    m_node.chainman->ActiveChainstate().InvalidateBlock(state, orig_tip);
    WITH_LOCK(::cs_main, assert(orig_tip->nStatus & BLOCK_FAILED_CHILD));
    WITH_LOCK(::cs_main, assert((orig_tip->nStatus & BLOCK_FAILED_VALID) == 0));
}

BOOST_AUTO_TEST_SUITE_END()
