// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <arith_uint256.h>
#include <chain.h>
#include <kernel/cs_main.h>
#include <node/staletiprelayman.h>
#include <staletipmessage.h>
#include <test/util/blocktree.h>
#include <test/util/setup_common.h>
#include <util/check.h>

#include <boost/test/unit_test.hpp>

#include <optional>

BOOST_FIXTURE_TEST_SUITE(staletiprelayman_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(staletip_make_message)
{
    using node::MakeStaleTipMessage;

    LOCK(::cs_main);

    BlockTree tree;
    CBlockIndex* fork_point{tree.Add(nullptr, /*active=*/true)};
    CBlockIndex* first{tree.Add(fork_point, /*active=*/false)};
    CBlockIndex* tip{tree.Add(first, /*active=*/false)};

    const StaleTipMessage message{MakeStaleTipMessage({.fork_point = fork_point, .tip = tip}, /*have_block=*/true)};

    BOOST_CHECK_EQUAL(message.fork_point.ToString(), fork_point->GetBlockHash().ToString());
    BOOST_REQUIRE_EQUAL(message.headers.size(), 2U);
    BOOST_CHECK_EQUAL(message.headers.front().merkle_root.ToString(), first->hashMerkleRoot.ToString());
    BOOST_CHECK_EQUAL(message.headers.back().nonce, tip->nNonce);
    BOOST_CHECK(message.have_block);

    const auto decompressed{message.DecompressHeaders()};
    BOOST_REQUIRE_EQUAL(decompressed.size(), 2U);
    BOOST_CHECK_EQUAL(decompressed.front().GetHash().ToString(), first->GetBlockHash().ToString());
    BOOST_CHECK_EQUAL(decompressed.back().GetHash().ToString(), tip->GetBlockHash().ToString());
}

BOOST_AUTO_TEST_CASE(validate_staletip_announcement)
{
    using node::StaleTipAnnouncementError;

    LOCK(::cs_main);

    constexpr int recent_window{5};
    BlockTree tree;
    CBlockIndex* active{tree.Add(nullptr, /*active=*/true)};
    while (active->nHeight < recent_window + 10) active = tree.Add(active, /*active=*/true);
    // The lowest fork point whose one-block branch is still within the
    // recency window.
    const CBlockIndex& fork_point{*Assert(active->GetAncestor(active->nHeight - recent_window - 1))};
    const arith_uint256 claimed_work{2};
    const arith_uint256 threshold{fork_point.nChainWork + claimed_work};
    const auto rejection{[&](const CBlockIndex& fork, size_t num_headers, const arith_uint256& work, const CBlockIndex* tip,
                             const arith_uint256& work_threshold) -> std::optional<StaleTipAnnouncementError> {
        const auto valid{node::ValidateStaleTipAnnouncement(fork, num_headers, work, tip, work_threshold, recent_window)};
        if (valid) return std::nullopt;
        return valid.error();
    }};

    BOOST_CHECK(!rejection(fork_point, 1, claimed_work, active, threshold));
    BOOST_CHECK(rejection(*fork_point.pprev, 1, claimed_work, active, threshold) == StaleTipAnnouncementError::OUTSIDE_RECENT_WINDOW);
    // A longer branch from a lower fork point can still reach into the window.
    BOOST_CHECK(!rejection(*fork_point.pprev, 2, claimed_work, active, threshold - arith_uint256{1}));

    // The branch must meet the work threshold, which still applies without an
    // active tip.
    BOOST_CHECK(rejection(fork_point, 1, claimed_work - arith_uint256{1}, active, threshold) == StaleTipAnnouncementError::LOW_WORK);
    BOOST_CHECK(rejection(*fork_point.pprev, 1, claimed_work, nullptr, threshold) == StaleTipAnnouncementError::LOW_WORK);
}

BOOST_AUTO_TEST_SUITE_END()
