// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <arith_uint256.h>
#include <chain.h>
#include <kernel/cs_main.h>
#include <node/staletiptracker.h>
#include <test/util/blocktree.h>
#include <test/util/setup_common.h>
#include <util/check.h>

#include <boost/test/unit_test.hpp>

#include <algorithm>
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

BOOST_AUTO_TEST_CASE(staletip_tracker_rejects_too_long_branch)
{
    LOCK(::cs_main);

    BlockTree tree;
    CBlockIndex* active{tree.AddChain(/*height=*/10)};

    StaleTipTracker tracker{STALETIP_RECENT_WINDOW, /*max_headers=*/2};
    CBlockIndex* fork{Assert(active->GetAncestor(active->nHeight - 3))};
    CBlockIndex* stale{tree.Add(fork, /*active=*/false)};
    stale = tree.Add(stale, /*active=*/false);
    BOOST_CHECK(tracker.AddStaleTip(tree.active_chain, *stale));
    BOOST_REQUIRE_EQUAL(tracker.GetStaleTips(tree.active_chain).size(), 1U);

    CBlockIndex* too_long{tree.Add(stale, /*active=*/false)};
    BOOST_CHECK(too_long->nChainWork <= active->nChainWork);
    BOOST_CHECK(!tracker.AddStaleTip(tree.active_chain, *too_long));

    // With no headers allowed, even a one-block branch is too long.
    StaleTipTracker no_headers{STALETIP_RECENT_WINDOW, /*max_headers=*/0};
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

    StaleTipTracker tracker{/*recent_window=*/5};
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

BOOST_AUTO_TEST_SUITE_END()
