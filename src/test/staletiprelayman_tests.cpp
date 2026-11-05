// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <arith_uint256.h>
#include <chain.h>
#include <kernel/cs_main.h>
#include <net.h>
#include <node/staletiprelayman.h>
#include <staletipmessage.h>
#include <sync.h>
#include <test/util/blocktree.h>
#include <test/util/setup_common.h>
#include <uint256.h>
#include <util/chaintype.h>
#include <util/check.h>

#include <boost/test/unit_test.hpp>

#include <cstddef>
#include <initializer_list>
#include <optional>
#include <vector>

BOOST_FIXTURE_TEST_SUITE(staletiprelayman_tests, BasicTestingSetup)

namespace {

/** A peer's view of the block tree, as PeerManager reports it to the
 *  StaleTipRelayManager. */
struct TestPeer {
    NodeId id;
    const CBlockIndex* best_known{nullptr};

    bool HasHeader(const CBlockIndex& block) const
    {
        return best_known != nullptr && best_known->GetAncestor(block.nHeight) == &block;
    }
};

std::vector<StaleTipMessage> Announce(node::StaleTipRelayManager& manager, const CChain& chain, const TestPeer& peer,
                                      bool can_serve = false) EXCLUSIVE_LOCKS_REQUIRED(::cs_main)
{
    return manager.GetStaleTipsToAnnounce(
        peer.id, chain,
        [&](const CBlockIndex& block) { return peer.HasHeader(block); },
        [&](const node::StaleFork&) { return can_serve; });
}

std::vector<uint256> Tips(const std::vector<StaleTipMessage>& announcements)
{
    std::vector<uint256> tips;
    for (const auto& announcement : announcements) tips.push_back(announcement.DecompressHeaders().back().GetHash());
    return tips;
}

std::vector<uint256> Hashes(std::initializer_list<const CBlockIndex*> blocks)
{
    std::vector<uint256> hashes;
    for (const CBlockIndex* block : blocks) hashes.push_back(block->GetBlockHash());
    return hashes;
}

} // namespace

BOOST_AUTO_TEST_CASE(announcement_policy)
{
    LOCK(::cs_main);

    BlockTree tree;
    CBlockIndex* active{tree.AddChain(/*height=*/4, {.have_data = true})};
    CBlockIndex* fork_a{Assert(active->pprev)};
    CBlockIndex* fork_b{Assert(fork_a->pprev)};
    CBlockIndex* tip_a{tree.Add(fork_a, /*active=*/false)};
    CBlockIndex* tip_b{tree.Add(tree.Add(fork_b, /*active=*/false), /*active=*/false)};
    const CChain& chain{tree.active_chain};

    node::StaleTipRelayManager manager{{}};
    const TestPeer source{.id = 0, .best_known = active};
    const TestPeer peer{.id = 1, .best_known = active};
    TestPeer below_fork{.id = 2, .best_known = fork_b};
    const TestPeer has_tip{.id = 3, .best_known = tip_a};
    for (NodeId id{0}; id < 4; ++id) manager.ConnectedPeer(id);

    BOOST_CHECK(manager.ReceivedStaleTip(source.id, chain, *tip_a));
    BOOST_CHECK(manager.AcceptedStaleTip(chain, *tip_b));
    BOOST_CHECK(!manager.AcceptedStaleTip(chain, *tip_b));

    // Tips are announced in the order they were tracked, but not back to the
    // peer that announced them, and not twice.
    const auto announcements{Announce(manager, chain, peer)};
    BOOST_CHECK(Tips(announcements) == Hashes({tip_a, tip_b}));
    BOOST_CHECK(!announcements.front().have_block);
    BOOST_CHECK(announcements.front().fork_point == fork_a->GetBlockHash());
    BOOST_CHECK(Tips(Announce(manager, chain, source)) == Hashes({tip_b}));
    BOOST_CHECK(Announce(manager, chain, peer).empty());

    // A tip is only announced to a peer that has its fork point.
    BOOST_CHECK(Tips(Announce(manager, chain, below_fork)) == Hashes({tip_b}));
    below_fork.best_known = active;
    BOOST_CHECK(Tips(Announce(manager, chain, below_fork)) == Hashes({tip_a}));

    // A tip is not announced to a peer that already has it.
    BOOST_CHECK(Tips(Announce(manager, chain, has_tip)) == Hashes({tip_b}));

    // A tip that is reorged onto the active chain and back stays tracked, so
    // it is not announced again.
    tree.active_chain.SetTip(*tip_a);
    BOOST_CHECK(Announce(manager, chain, peer).empty());
    tree.active_chain.SetTip(*active);
    BOOST_CHECK(Announce(manager, chain, peer).empty());

    // Only the last MAX_STALETIPS_FROM_PEER tips a peer announced are
    // remembered, so an older one may be announced back to it.
    for (size_t i{0}; i < node::MAX_STALETIPS_FROM_PEER; ++i) {
        manager.ReceivedStaleTip(source.id, chain, *tree.Add(fork_a, /*active=*/false));
    }
    BOOST_CHECK(Tips(Announce(manager, chain, source)) == Hashes({tip_a}));

    for (NodeId id{0}; id < 4; ++id) manager.DisconnectedPeer(id);
    manager.CheckIsEmpty();
}

BOOST_AUTO_TEST_CASE(evicted_tip_not_readmitted)
{
    LOCK(::cs_main);

    BlockTree tree;
    CBlockIndex* active{tree.AddChain(/*height=*/4, {.have_data = true})};
    CBlockIndex* fork{Assert(active->GetAncestor(2))};

    node::StaleTipRelayManager manager{{}};
    const TestPeer peer{.id = 0, .best_known = active};
    manager.ConnectedPeer(peer.id);
    manager.ConnectedPeer(1);

    std::vector<CBlockIndex*> tips;
    for (size_t i{0}; i < node::MAX_ADVERTISED_STALETIPS; ++i) {
        tips.push_back(tree.Add(fork, /*active=*/false));
        BOOST_CHECK(manager.AcceptedStaleTip(tree.active_chain, *tips.back()));
    }
    BOOST_CHECK_EQUAL(Announce(manager, tree.active_chain, peer).size(), node::MAX_ADVERTISED_STALETIPS);

    // A higher-work tip evicts the oldest of the equal-work tips.
    CBlockIndex* higher{tree.Add(tree.Add(fork, /*active=*/false), /*active=*/false)};
    BOOST_CHECK(manager.AcceptedStaleTip(tree.active_chain, *higher));
    BOOST_CHECK(Tips(Announce(manager, tree.active_chain, peer)) == Hashes({higher}));

    // Another peer announcing the evicted tip cannot readmit it, so it is not
    // announced again.
    BOOST_CHECK(!manager.ReceivedStaleTip(1, tree.active_chain, *tips.front()));
    BOOST_CHECK(Announce(manager, tree.active_chain, peer).empty());
}

BOOST_AUTO_TEST_CASE(signet_announces_with_block)
{
    LOCK(::cs_main);

    BlockTree tree;
    CBlockIndex* active{tree.AddChain(/*height=*/3, {.have_data = true})};
    CBlockIndex* tip{tree.Add(Assert(active->pprev), /*active=*/false, {.have_data = true})};

    BOOST_CHECK(!node::StaleTipRelayManager{{}}.PrefersBlocks());
    node::StaleTipRelayManager manager{{.chain_type = ChainType::SIGNET}};
    BOOST_CHECK(manager.PrefersBlocks());
    const TestPeer peer{.id = 0, .best_known = active};
    manager.ConnectedPeer(peer.id);
    BOOST_CHECK(manager.AcceptedStaleTip(tree.active_chain, *tip));

    // A signet tip is only announced once its blocks can be served.
    BOOST_CHECK(Announce(manager, tree.active_chain, peer, /*can_serve=*/false).empty());
    const auto announcements{Announce(manager, tree.active_chain, peer, /*can_serve=*/true)};
    BOOST_REQUIRE_EQUAL(announcements.size(), 1U);
    BOOST_CHECK(announcements.front().have_block);
    manager.DisconnectedPeer(peer.id);
}

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
