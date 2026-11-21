// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <arith_uint256.h>
#include <chain.h>
#include <kernel/cs_main.h>
#include <node/staletiptracker.h>
#include <test/util/blocktree.h>
#include <test/util/setup_common.h>
#include <uint256.h>
#include <util/check.h>
#include <validation.h>

#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <utility>
#include <vector>

using node::MAX_ADVERTISED_STALETIPS;
using node::STALETIP_RECENT_WINDOW;
using node::StaleFork;
using node::StaleTipTracker;

BOOST_FIXTURE_TEST_SUITE(staletiptracker_tests, BasicTestingSetup)

namespace {

/** Number of stale blocks between the fork point and the stale tip. */
int ForkLength(const StaleFork& fork)
{
    return fork.tip->nHeight - fork.fork_point->nHeight;
}

} // namespace

BOOST_AUTO_TEST_CASE(staletip_tracker_basic)
{
    LOCK(::cs_main);

    BlockTree tree;
    CBlockIndex* tip{tree.AddChain(/*height=*/4)};

    CBlockIndex* stale{tree.Add(Assert(tip->pprev), /*active=*/false)};
    StaleTipTracker tracker;

    BOOST_CHECK(tracker.AddStaleTip(tree.active_chain, *stale));
    const auto forks{tracker.GetStaleTips(tree.active_chain)};
    BOOST_REQUIRE_EQUAL(forks.size(), 1U);
    BOOST_CHECK_EQUAL(forks[0].tip, stale);
    BOOST_CHECK_EQUAL(forks[0].fork_point, tip->pprev);
    BOOST_CHECK_EQUAL(ForkLength(forks[0]), 1);
}

BOOST_AUTO_TEST_CASE(staletip_tracker_returns_tips_in_discovery_order)
{
    LOCK(::cs_main);

    BlockTree tree;
    CBlockIndex* active{tree.AddChain(/*height=*/4)};

    StaleTipTracker tracker;
    CBlockIndex* first{tree.Add(Assert(active->GetAncestor(active->nHeight - 3)), /*active=*/false)};
    CBlockIndex* second{tree.Add(Assert(active->GetAncestor(active->nHeight - 2)), /*active=*/false)};
    BOOST_CHECK(tracker.AddStaleTip(tree.active_chain, *first));
    BOOST_CHECK(tracker.AddStaleTip(tree.active_chain, *second));

    // The extension replaces the first tip, and was added last, so it is
    // returned last.
    CBlockIndex* first_child{tree.Add(first, /*active=*/false)};
    BOOST_CHECK(tracker.AddStaleTip(tree.active_chain, *first_child));

    const auto forks{tracker.GetStaleTips(tree.active_chain)};
    BOOST_REQUIRE_EQUAL(forks.size(), 2U);
    BOOST_CHECK_EQUAL(forks[0].tip, second);
    BOOST_CHECK_EQUAL(forks[1].tip, first_child);
}

BOOST_AUTO_TEST_CASE(staletip_tracker_serve_policy)
{
    LOCK(::cs_main);

    BlockTree tree;
    CBlockIndex* tip{tree.AddChain(/*height=*/4, {.have_data = true})};

    CBlockIndex* stale_parent{tree.Add(Assert(Assert(tip->pprev)->pprev), /*active=*/false)};
    CBlockIndex* stale{tree.Add(stale_parent, /*active=*/false)};
    StaleTipTracker tips;

    // A tracked header-only branch is not servable.
    BOOST_CHECK(tips.AddStaleTip(tree.active_chain, *stale_parent));
    BOOST_CHECK(!tips.CanServeStaleBranchBlock(tree.active_chain, *stale));
    BOOST_CHECK(!tips.CanServeStaleBranchBlock(tree.active_chain, *stale_parent));

    stale_parent->RaiseValidity(BLOCK_VALID_TRANSACTIONS);
    stale_parent->nStatus |= BLOCK_HAVE_DATA;
    BOOST_CHECK(tips.CanServeStaleBranchBlock(tree.active_chain, *stale_parent));
    BOOST_CHECK(tips.AddStaleTip(tree.active_chain, *stale));
    BOOST_CHECK(!tips.IsTracked(*stale_parent));
    BOOST_CHECK(tips.CanServeStaleBranchBlock(tree.active_chain, *stale_parent));
    BOOST_CHECK(!tips.CanServeStaleBranchBlock(tree.active_chain, *stale));

    stale->RaiseValidity(BLOCK_VALID_TRANSACTIONS);
    stale->nStatus |= BLOCK_HAVE_DATA;
    BOOST_CHECK(tips.IsTracked(*stale));
    BOOST_CHECK(tips.CanServeStaleBranchBlock(tree.active_chain, *stale));
    BOOST_CHECK(tips.CanServeStaleBranchBlock(tree.active_chain, *stale_parent));

    // A block with data that is not on a tracked stale branch is not servable.
    CBlockIndex* untracked{tree.Add(Assert(tip->pprev), /*active=*/false, {.have_data = true})};
    BOOST_CHECK(!tips.CanServeStaleBranchBlock(tree.active_chain, *untracked));

    // A tracked tip that is no longer eligible (here: reorged onto the
    // active chain) is not servable.
    tree.active_chain.SetTip(*stale);
    BOOST_CHECK(!tips.CanServeStaleBranchBlock(tree.active_chain, *stale));
    BOOST_CHECK(!tips.CanServeStaleBranchBlock(tree.active_chain, *stale_parent));
}

BOOST_AUTO_TEST_CASE(staletip_tracker_rejects_too_long_branch)
{
    LOCK(::cs_main);

    BlockTree tree;
    CBlockIndex* active{tree.AddChain(/*height=*/10)};

    StaleTipTracker tracker{ChainType::MAIN, STALETIP_RECENT_WINDOW, /*max_headers=*/2};
    CBlockIndex* fork{Assert(active->GetAncestor(active->nHeight - 3))};
    CBlockIndex* stale{tree.Add(fork, /*active=*/false)};
    stale = tree.Add(stale, /*active=*/false);
    BOOST_CHECK(tracker.AddStaleTip(tree.active_chain, *stale));
    BOOST_REQUIRE_EQUAL(tracker.GetStaleTips(tree.active_chain).size(), 1U);

    CBlockIndex* too_long{tree.Add(stale, /*active=*/false)};
    BOOST_CHECK(too_long->nChainWork <= active->nChainWork);
    BOOST_CHECK(!tracker.AddStaleTip(tree.active_chain, *too_long));

    // With no headers allowed, even a one-block branch is too long.
    StaleTipTracker no_headers{ChainType::MAIN, STALETIP_RECENT_WINDOW, /*max_headers=*/0};
    BOOST_CHECK(!no_headers.AddStaleTip(tree.active_chain, *tree.Add(active->pprev, /*active=*/false)));
}

BOOST_AUTO_TEST_CASE(staletip_tracker_extending_tip_replaces_old_tip)
{
    LOCK(::cs_main);

    BlockTree tree;
    CBlockIndex* active{tree.AddChain(/*height=*/3)};

    CBlockIndex* stale{tree.Add(Assert(active->GetAncestor(active->nHeight - 2)), /*active=*/false)};
    StaleTipTracker tracker;
    BOOST_CHECK(tracker.AddStaleTip(tree.active_chain, *stale));
    BOOST_CHECK(!tracker.AddStaleTip(tree.active_chain, *stale));

    CBlockIndex* stale_child{tree.Add(stale, /*active=*/false)};
    BOOST_CHECK(tracker.AddStaleTip(tree.active_chain, *stale_child));
    // An ancestor of a tracked tip is not tracked separately.
    BOOST_CHECK(!tracker.AddStaleTip(tree.active_chain, *stale));

    const auto forks{tracker.GetStaleTips(tree.active_chain)};
    BOOST_REQUIRE_EQUAL(forks.size(), 1U);
    BOOST_CHECK_EQUAL(forks[0].tip, stale_child);
    BOOST_CHECK_EQUAL(ForkLength(forks[0]), 2);
}

BOOST_AUTO_TEST_CASE(staletip_tracker_expiry)
{
    LOCK(::cs_main);

    BlockTree tree;
    CBlockIndex* active{tree.AddChain(/*height=*/10)};

    StaleTipTracker tracker{ChainType::MAIN, /*recent_window=*/5};
    CBlockIndex* below_window{tree.Add(Assert(active->GetAncestor(active->nHeight - 7)), /*active=*/false)};
    BOOST_CHECK(!tracker.AddStaleTip(tree.active_chain, *below_window));

    // A tip exactly at the edge of the window is tracked, until the active
    // chain grows past it.
    CBlockIndex* at_edge{tree.Add(Assert(active->GetAncestor(active->nHeight - 6)), /*active=*/false)};
    BOOST_CHECK_EQUAL(active->nHeight - at_edge->nHeight, 5);
    BOOST_CHECK(tracker.AddStaleTip(tree.active_chain, *at_edge));
    BOOST_CHECK_EQUAL(tracker.GetStaleTips(tree.active_chain).size(), 1U);
    active = tree.Add(active, /*active=*/true);
    BOOST_CHECK(tracker.GetStaleTips(tree.active_chain).empty());

    // A tracked tip expires once one of its ancestors is found invalid.
    CBlockIndex* invalid_child{tree.Add(active->pprev, /*active=*/false)};
    BOOST_CHECK(tracker.AddStaleTip(tree.active_chain, *invalid_child));
    invalid_child->nStatus |= BLOCK_FAILED_VALID;
    BOOST_CHECK(tracker.GetStaleTips(tree.active_chain).empty());
    BOOST_CHECK(!tracker.AddStaleTip(tree.active_chain, *invalid_child));
}

BOOST_AUTO_TEST_CASE(staletip_tracker_keeps_temporarily_ineligible_tip)
{
    LOCK(::cs_main);

    BlockTree tree;
    CBlockIndex* active{tree.AddChain(/*height=*/4)};

    StaleTipTracker tracker;
    CBlockIndex* stale{tree.Add(Assert(active->GetAncestor(active->nHeight - 2)), /*active=*/false)};
    BOOST_CHECK(tracker.AddStaleTip(tree.active_chain, *stale));

    // While the tip is part of the active chain it is not returned, and
    // adding another tip does not drop it.
    tree.active_chain.SetTip(*stale);
    BOOST_CHECK(tracker.GetStaleTips(tree.active_chain).empty());
    BOOST_CHECK(tracker.IsTracked(*stale));
    BOOST_CHECK(!tracker.IsTracked(*active));
    CBlockIndex* other{tree.Add(Assert(active->GetAncestor(active->nHeight - 3)), /*active=*/false)};
    BOOST_CHECK(tracker.AddStaleTip(tree.active_chain, *other));

    // Once the active chain switches back, it is returned again.
    tree.active_chain.SetTip(*active);
    const auto forks{tracker.GetStaleTips(tree.active_chain)};
    BOOST_REQUIRE_EQUAL(forks.size(), 2U);
    BOOST_CHECK_EQUAL(forks[0].tip, stale);
    BOOST_CHECK_EQUAL(forks[1].tip, other);
}

/** An active chain of 21 blocks and an empty stale-tip tracker. */
struct StaleTipTrackerSetup : public BasicTestingSetup {
    BlockTree tree;
    StaleTipTracker tracker;
    CBlockIndex* active{nullptr};

    StaleTipTrackerSetup()
    {
        LOCK(::cs_main);
        active = tree.AddChain(/*height=*/20);
    }

    /** Fill the tracker with stale tips that fork off `fork`. */
    std::vector<CBlockIndex*> Fill(CBlockIndex* fork) EXCLUSIVE_LOCKS_REQUIRED(::cs_main)
    {
        std::vector<CBlockIndex*> stale_tips;
        for (size_t i{0}; i < MAX_ADVERTISED_STALETIPS; ++i) {
            stale_tips.push_back(tree.Add(fork, /*active=*/false));
            BOOST_CHECK(tracker.AddStaleTip(tree.active_chain, *stale_tips.back()));
        }
        return stale_tips;
    }

    /** Whether `block` is returned by GetStaleTips(), so tracked and eligible. */
    bool IsReturned(const CBlockIndex* block) EXCLUSIVE_LOCKS_REQUIRED(::cs_main)
    {
        return std::ranges::any_of(tracker.GetStaleTips(tree.active_chain), [&](const StaleFork& fork) { return fork.tip == block; });
    }
};

BOOST_FIXTURE_TEST_CASE(staletip_tracker_evicts_least_work_tip, StaleTipTrackerSetup)
{
    LOCK(::cs_main);

    std::vector<CBlockIndex*> stale_tips;
    for (size_t i{0}; i < MAX_ADVERTISED_STALETIPS; ++i) {
        const int fork_depth{static_cast<int>(MAX_ADVERTISED_STALETIPS - i)};
        CBlockIndex* fork{Assert(active->GetAncestor(active->nHeight - fork_depth))};
        stale_tips.push_back(tree.Add(fork, /*active=*/false));
        BOOST_CHECK(tracker.AddStaleTip(tree.active_chain, *stale_tips.back()));
        BOOST_CHECK_EQUAL(tracker.GetStaleTips(tree.active_chain).size(), i + 1);
    }

    CBlockIndex* replacement_fork{Assert(active->GetAncestor(active->nHeight - 5))};
    CBlockIndex* replacement{tree.Add(replacement_fork, /*active=*/false)};
    BOOST_CHECK(tracker.AddStaleTip(tree.active_chain, *replacement));

    BOOST_CHECK_EQUAL(tracker.GetStaleTips(tree.active_chain).size(), MAX_ADVERTISED_STALETIPS);
    BOOST_CHECK(!IsReturned(stale_tips.front()));
    for (size_t i{1}; i < stale_tips.size(); ++i) BOOST_CHECK(IsReturned(stale_tips[i]));
    BOOST_CHECK(IsReturned(replacement));
}

BOOST_FIXTURE_TEST_CASE(staletip_tracker_evicts_first_added_of_equal_work_tips, StaleTipTrackerSetup)
{
    LOCK(::cs_main);

    CBlockIndex* fork{Assert(active->GetAncestor(active->nHeight - 2))};
    const auto stale_tips{Fill(fork)};

    // A higher-work tip on a branch of its own evicts the first added of the
    // equal-work tips.
    CBlockIndex* candidate{tree.Add(tree.Add(fork, /*active=*/false), /*active=*/false)};
    BOOST_CHECK(candidate->nChainWork > stale_tips.front()->nChainWork);
    BOOST_CHECK(tracker.AddStaleTip(tree.active_chain, *candidate));

    BOOST_CHECK_EQUAL(tracker.GetStaleTips(tree.active_chain).size(), MAX_ADVERTISED_STALETIPS);
    BOOST_CHECK(!IsReturned(stale_tips.front()));
    for (size_t i{1}; i < stale_tips.size(); ++i) BOOST_CHECK(IsReturned(stale_tips[i]));
    BOOST_CHECK(IsReturned(candidate));
}

BOOST_FIXTURE_TEST_CASE(staletip_tracker_prunes_expired_tips, StaleTipTrackerSetup)
{
    LOCK(::cs_main);

    CBlockIndex* fork{active->pprev};
    const auto stale_tips{Fill(fork)};

    // A lower-work tip cannot evict any of the tracked tips...
    CBlockIndex* candidate{tree.Add(fork->pprev, /*active=*/false)};
    BOOST_CHECK(!tracker.AddStaleTip(tree.active_chain, *candidate));
    BOOST_CHECK(!IsReturned(candidate));

    // ...but takes the slot of one that has since been found invalid, even
    // though the invalid tip has more work.
    stale_tips.front()->nStatus |= BLOCK_FAILED_VALID;
    BOOST_CHECK(tracker.AddStaleTip(tree.active_chain, *candidate));

    BOOST_CHECK_EQUAL(tracker.GetStaleTips(tree.active_chain).size(), MAX_ADVERTISED_STALETIPS);
    BOOST_CHECK(IsReturned(candidate));
}

BOOST_FIXTURE_TEST_CASE(staletip_tracker_full_keeps_equal_work_tips, StaleTipTrackerSetup)
{
    LOCK(::cs_main);

    CBlockIndex* fork{active->pprev};
    const auto stale_tips{Fill(fork)};

    CBlockIndex* candidate{tree.Add(fork, /*active=*/false)};
    BOOST_CHECK(!tracker.AddStaleTip(tree.active_chain, *candidate));

    BOOST_CHECK_EQUAL(tracker.GetStaleTips(tree.active_chain).size(), MAX_ADVERTISED_STALETIPS);
    for (const CBlockIndex* stale_tip : stale_tips) BOOST_CHECK(IsReturned(stale_tip));
    BOOST_CHECK(!IsReturned(candidate));
}

BOOST_FIXTURE_TEST_CASE(staletip_tracker_evicts_by_work_not_height, StaleTipTrackerSetup)
{
    LOCK(::cs_main);

    CBlockIndex* fork{active->pprev};
    const auto stale_tips{Fill(fork)};
    // Give one tracked tip less work than a candidate at a lower height.
    CBlockIndex* low_work{stale_tips.back()};
    low_work->nChainWork = fork->nChainWork - arith_uint256{1};

    CBlockIndex* candidate{tree.Add(fork->pprev, /*active=*/false)};
    BOOST_CHECK(candidate->nHeight < low_work->nHeight);
    BOOST_CHECK(candidate->nChainWork > low_work->nChainWork);
    BOOST_CHECK(tracker.AddStaleTip(tree.active_chain, *candidate));

    BOOST_CHECK_EQUAL(tracker.GetStaleTips(tree.active_chain).size(), MAX_ADVERTISED_STALETIPS);
    BOOST_CHECK(!IsReturned(low_work));
    BOOST_CHECK(IsReturned(candidate));
}

BOOST_AUTO_TEST_CASE(staletip_tracker_signet_requires_validated_block)
{
    LOCK(::cs_main);

    BlockTree tree;
    CBlockIndex* active{tree.AddChain(/*height=*/3, {.have_data = true})};

    CBlockIndex* headers_only{tree.Add(active->pprev, /*active=*/false)};
    StaleTipTracker tracker{ChainType::SIGNET};
    BOOST_CHECK(!tracker.AddStaleTip(tree.active_chain, *headers_only));

    headers_only->RaiseValidity(BLOCK_VALID_TRANSACTIONS);
    headers_only->nStatus |= BLOCK_HAVE_DATA;
    BOOST_CHECK(tracker.AddStaleTip(tree.active_chain, *headers_only));
    // The block was validated, so pruning its data keeps the tip eligible.
    headers_only->nStatus &= ~BLOCK_HAVE_DATA;
    BOOST_CHECK_EQUAL(tracker.GetStaleTips(tree.active_chain).size(), 1U);
}

BOOST_AUTO_TEST_CASE(staletip_tracker_testnet_min_difficulty)
{
    LOCK(::cs_main);

    BlockTree tree;
    CBlockIndex* active{tree.AddChain(/*height=*/3, {.have_data = true})};

    CBlockIndex* fork{active->pprev};
    CBlockIndex* low_difficulty{tree.Add(fork, /*active=*/false, {.have_data = true, .bits = 0x207fffff})};
    StaleTipTracker testnet_tips{ChainType::TESTNET};
    BOOST_CHECK(!testnet_tips.AddStaleTip(tree.active_chain, *low_difficulty));
    StaleTipTracker testnet4_tips{ChainType::TESTNET4};
    BOOST_CHECK(!testnet4_tips.AddStaleTip(tree.active_chain, *low_difficulty));

    // A target just above the limit is rejected, one just below it is not.
    CBlockIndex* above_limit{tree.Add(fork, /*active=*/false, {.have_data = true, .bits = 0x1a100000})};
    BOOST_CHECK(!testnet_tips.AddStaleTip(tree.active_chain, *above_limit));
    CBlockIndex* below_limit{tree.Add(fork, /*active=*/false, {.have_data = true, .bits = 0x1a0fffff})};
    BOOST_CHECK(testnet_tips.AddStaleTip(tree.active_chain, *below_limit));
}

BOOST_AUTO_TEST_CASE(staletip_tracker_signet_variant_policy)
{
    LOCK(::cs_main);

    BlockTree tree;
    CBlockIndex* active{tree.AddChain(/*height=*/3, {.have_data = true})};

    CBlockIndex* fork{Assert(Assert(active->pprev)->pprev)};
    const uint256 shared_merkle_root{uint8_t{42}};
    CBlockIndex* first_variant{tree.Add(fork, /*active=*/false, {.have_data = true, .merkle_root = shared_merkle_root})};
    CBlockIndex* second_variant{tree.Add(fork, /*active=*/false, {.have_data = true, .merkle_root = shared_merkle_root})};

    StaleTipTracker tips{ChainType::SIGNET};
    BOOST_CHECK(tips.AddStaleTip(tree.active_chain, *first_variant));
    BOOST_CHECK(!tips.AddStaleTip(tree.active_chain, *second_variant));

    CBlockIndex* extended_second{tree.Add(second_variant, /*active=*/false, {.have_data = true})};
    BOOST_CHECK(tips.AddStaleTip(tree.active_chain, *extended_second));
    const auto forks{tips.GetStaleTips(tree.active_chain)};
    BOOST_REQUIRE_EQUAL(forks.size(), 1U);
    BOOST_CHECK_EQUAL(forks.front().tip, extended_second);
    // The first variant is now a variant of a block on a tracked branch.
    BOOST_CHECK(!tips.AddStaleTip(tree.active_chain, *first_variant));

    CBlockIndex* active_variant{tree.Add(active->pprev, /*active=*/false, {.have_data = true, .merkle_root = active->hashMerkleRoot})};
    BOOST_CHECK(!tips.AddStaleTip(tree.active_chain, *active_variant));

    // Other networks track every variant.
    StaleTipTracker main_tips;
    BOOST_CHECK(main_tips.AddStaleTip(tree.active_chain, *first_variant));
    BOOST_CHECK(main_tips.AddStaleTip(tree.active_chain, *second_variant));
    BOOST_CHECK(main_tips.AddStaleTip(tree.active_chain, *active_variant));
}

BOOST_AUTO_TEST_CASE(staletip_tracker_signet_variant_becomes_active)
{
    LOCK(::cs_main);

    BlockTree tree;
    CBlockIndex* active{tree.AddChain(/*height=*/3, {.have_data = true})};

    CBlockIndex* fork{active->pprev};
    const uint256 shared_merkle_root{uint8_t{42}};
    CBlockIndex* stale{tree.Add(fork, /*active=*/false, {.have_data = true, .merkle_root = shared_merkle_root})};

    StaleTipTracker tips{ChainType::SIGNET};
    BOOST_CHECK(tips.AddStaleTip(tree.active_chain, *stale));
    BOOST_CHECK_EQUAL(tips.GetStaleTips(tree.active_chain).size(), 1U);

    // Reorg onto a branch whose block at the tracked tip's height is a
    // variant of it. The tip is no longer eligible, since the active chain's
    // variant counts as seen first.
    CBlockIndex* variant{tree.Add(fork, /*active=*/false, {.have_data = true, .merkle_root = shared_merkle_root})};
    tree.Add(variant, /*active=*/true, {.have_data = true});
    BOOST_CHECK(!tips.AddStaleTip(tree.active_chain, *stale));
    BOOST_CHECK(tips.GetStaleTips(tree.active_chain).empty());

    // It stays tracked, and is eligible again once the variant is reorged out.
    tree.active_chain.SetTip(*active);
    BOOST_CHECK_EQUAL(tips.GetStaleTips(tree.active_chain).size(), 1U);
}

/** A BlockManager whose block index holds an active chain of five blocks,
 *  for tests of StaleTipTracker::SeedFromBlockIndex(). */
struct SeedFromBlockIndexSetup : public ChainTestingSetup {
    node::BlockManager& blockman{m_node.chainman->m_blockman};
    CBlockIndex* best_header{nullptr};
    CChain active_chain;
    CBlockIndex* active{nullptr};
    uint32_t next_nonce{1};

    SeedFromBlockIndexSetup()
    {
        LOCK(::cs_main);
        for (int i{0}; i < 5; ++i) {
            active = Add(active);
            active_chain.SetTip(*active);
        }
    }

    CBlockIndex* Add(const CBlockIndex* prev, BlockTreeAddOptions opts = {}) EXCLUSIVE_LOCKS_REQUIRED(::cs_main)
    {
        CBlockIndex* block{blockman.AddToBlockIndex(MakeHeader(prev, next_nonce++, opts), best_header)};
        if (opts.have_data) {
            block->RaiseValidity(BLOCK_VALID_TRANSACTIONS);
            block->nStatus |= BLOCK_HAVE_DATA;
        }
        return block;
    }

    /** Seed `tracker` and return the tracked tips. */
    std::vector<const CBlockIndex*> Seed(StaleTipTracker& tracker) EXCLUSIVE_LOCKS_REQUIRED(::cs_main)
    {
        tracker.SeedFromBlockIndex(blockman, active_chain);
        std::vector<const CBlockIndex*> tips;
        for (const StaleFork& fork : tracker.GetStaleTips(active_chain)) tips.push_back(fork.tip);
        return tips;
    }
};

BOOST_FIXTURE_TEST_CASE(staletip_seed_orders_tips_deterministically, SeedFromBlockIndexSetup)
{
    LOCK(::cs_main);

    const CBlockIndex* lower_tip{Add(active->GetAncestor(active->nHeight - 2))};
    const CBlockIndex* same_work_a{Add(active->pprev)};
    const CBlockIndex* same_work_b{Add(active->pprev)};
    if (same_work_b->GetBlockHash() < same_work_a->GetBlockHash()) std::swap(same_work_a, same_work_b);

    // More work first, then lower hash first among tips with equal work.
    StaleTipTracker tracker;
    const std::vector<const CBlockIndex*> expected{same_work_a, same_work_b, lower_tip};
    BOOST_CHECK(Seed(tracker) == expected);
}

BOOST_FIXTURE_TEST_CASE(staletip_seed_tracks_branch_tips_only, SeedFromBlockIndexSetup)
{
    LOCK(::cs_main);

    CBlockIndex* branch{Add(active->GetAncestor(active->nHeight - 3))};
    branch = Add(branch);
    branch = Add(branch);

    StaleTipTracker tracker;
    BOOST_CHECK(Seed(tracker) == std::vector<const CBlockIndex*>{branch});
}

BOOST_FIXTURE_TEST_CASE(staletip_seed_signet_tracks_last_validated_block, SeedFromBlockIndexSetup)
{
    LOCK(::cs_main);

    // On signet, a branch whose last block has not been downloaded yet is
    // tracked up to its last validated block.
    const CBlockIndex* validated{Add(active->GetAncestor(active->nHeight - 2), {.have_data = true})};
    const CBlockIndex* headers_only{Add(validated)};

    StaleTipTracker signet_tracker{ChainType::SIGNET};
    BOOST_CHECK(Seed(signet_tracker) == std::vector<const CBlockIndex*>{validated});

    StaleTipTracker main_tracker;
    BOOST_CHECK(Seed(main_tracker) == std::vector<const CBlockIndex*>{headers_only});
}

BOOST_FIXTURE_TEST_CASE(staletip_seed_signet_skips_active_tip_variant, SeedFromBlockIndexSetup)
{
    LOCK(::cs_main);

    const CBlockIndex* active_variant{Add(active->pprev, {.have_data = true, .merkle_root = active->hashMerkleRoot})};

    StaleTipTracker signet_tracker{ChainType::SIGNET};
    BOOST_CHECK(Seed(signet_tracker).empty());

    // Header variants are only deduplicated on signet.
    StaleTipTracker main_tracker;
    BOOST_CHECK(Seed(main_tracker) == std::vector<const CBlockIndex*>{active_variant});
}

BOOST_FIXTURE_TEST_CASE(staletip_seed_keeps_most_work_tips, SeedFromBlockIndexSetup)
{
    LOCK(::cs_main);

    // Added to the block index first, but has less work than the tips that
    // fill the tracker.
    const CBlockIndex* least_work{Add(active->GetAncestor(1))};
    std::vector<const CBlockIndex*> most_work;
    for (size_t i{0}; i < MAX_ADVERTISED_STALETIPS; ++i) most_work.push_back(Add(active->pprev));

    StaleTipTracker tracker;
    const std::vector<const CBlockIndex*> tips{Seed(tracker)};
    BOOST_CHECK(std::ranges::is_permutation(tips, most_work));
    BOOST_CHECK(std::ranges::find(tips, least_work) == tips.end());
}

BOOST_AUTO_TEST_SUITE_END()
