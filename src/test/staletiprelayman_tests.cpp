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

#include <chrono>
#include <cstddef>
#include <initializer_list>
#include <optional>
#include <vector>

using namespace std::chrono_literals;

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
                                      std::optional<int> max_serve_depth = std::nullopt) EXCLUSIVE_LOCKS_REQUIRED(::cs_main)
{
    return manager.GetStaleTipsToAnnounce(
        peer.id, chain,
        [&](const CBlockIndex& block) { return !max_serve_depth || chain.Height() - block.nHeight <= *max_serve_depth; },
        [&](const CBlockIndex& block) { return peer.HasHeader(block); });
}

using Blocks = std::vector<const CBlockIndex*>;

bool AnyBlock(const CBlockIndex&) { return true; }

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

    BOOST_CHECK(manager.ReceivedStaleTip(source.id, chain, *tip_a, /*have_block=*/false));
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
        manager.ReceivedStaleTip(source.id, chain, *tree.Add(fork_a, /*active=*/false), /*have_block=*/false);
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
    BOOST_CHECK(!manager.ReceivedStaleTip(1, tree.active_chain, *tips.front(), /*have_block=*/false));
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

    // A signet tip is only announced while its block can be served, so not
    // after its validated block was pruned.
    tip->nStatus &= ~BLOCK_HAVE_DATA;
    BOOST_CHECK(Announce(manager, tree.active_chain, peer).empty());
    tip->nStatus |= BLOCK_HAVE_DATA;
    const auto announcements{Announce(manager, tree.active_chain, peer)};
    BOOST_REQUIRE_EQUAL(announcements.size(), 1U);
    BOOST_CHECK(announcements.front().have_block);
    manager.DisconnectedPeer(peer.id);

    node::StaleTipRelayManager downloading{{.chain_type = ChainType::SIGNET, .fetch_blocks = true}};
    downloading.ConnectedPeer(peer.id);
    CBlockIndex* header{tree.Add(active->pprev, /*active=*/false)};
    BOOST_CHECK(downloading.ReceivedStaleTip(peer.id, tree.active_chain, *header, /*have_block=*/true));
    BOOST_CHECK(downloading.GetStaleTips(tree.active_chain).empty());
    BOOST_CHECK(downloading.GetBlockRequestsToSend(peer.id, tree.active_chain, 0s, 1, AnyBlock) == Blocks{header});
}

BOOST_AUTO_TEST_CASE(announced_branch_blocks)
{
    LOCK(::cs_main);

    BlockTree tree;
    CBlockIndex* active{tree.AddChain(/*height=*/4, {.have_data = true})};
    CBlockIndex* fork{Assert(active->pprev)};
    CBlockIndex* parent{tree.Add(fork, /*active=*/false)};
    CBlockIndex* tip{tree.Add(parent, /*active=*/false)};
    CBlockIndex* other{tree.Add(fork, /*active=*/false)};

    const CChain& chain{tree.active_chain};

    node::StaleTipRelayManager manager{{}};
    manager.ConnectedPeer(0);
    manager.ConnectedPeer(1);

    // Only a branch announced with block data counts.
    manager.ReceivedStaleTip(0, chain, *tip, /*have_block=*/false);
    BOOST_CHECK(!manager.IsAnnouncedStaleBranchBlock(0, chain, *tip));
    manager.ReceivedStaleTip(0, chain, *tip, /*have_block=*/true);
    BOOST_CHECK(manager.IsAnnouncedStaleBranchBlock(0, chain, *tip));
    BOOST_CHECK(manager.IsAnnouncedStaleBranchBlock(0, chain, *parent));
    BOOST_CHECK(!manager.IsAnnouncedStaleBranchBlock(0, chain, *other));
    BOOST_CHECK(!manager.IsAnnouncedStaleBranchBlock(1, chain, *tip));
    // The branch ends where it joins the active chain.
    BOOST_CHECK(!manager.IsAnnouncedStaleBranchBlock(0, chain, *fork));
    BOOST_CHECK(!manager.IsAnnouncedStaleBranchBlock(0, chain, *Assert(fork->pprev)));

    // Announcing the tip again without block data keeps the offer.
    manager.ReceivedStaleTip(0, chain, *tip, /*have_block=*/false);
    BOOST_CHECK(manager.IsAnnouncedStaleBranchBlock(0, chain, *tip));

    // A block known to be invalid is still covered, as it may have turned out
    // invalid after the offer.
    parent->nStatus |= BLOCK_FAILED_VALID;
    BOOST_CHECK(manager.IsAnnouncedStaleBranchBlock(0, chain, *parent));

    manager.DisconnectedPeer(0);
    manager.DisconnectedPeer(1);
    BOOST_CHECK(!manager.IsAnnouncedStaleBranchBlock(0, chain, *tip));
}

BOOST_AUTO_TEST_CASE(announces_servable_branches)
{
    LOCK(::cs_main);

    BlockTree tree;
    CBlockIndex* active{tree.AddChain(/*height=*/5, {.have_data = true})};
    CBlockIndex* first{tree.Add(active->GetAncestor(2), /*active=*/false)};
    CBlockIndex* middle{tree.Add(first, /*active=*/false)};
    CBlockIndex* tip{tree.Add(middle, /*active=*/false)};

    node::StaleTipRelayManager manager{{}};
    BOOST_CHECK(manager.AcceptedStaleTip(tree.active_chain, *tip));
    NodeId next_id{0};
    const auto have_block_by_depth{[&](std::optional<int> max_serve_depth) EXCLUSIVE_LOCKS_REQUIRED(::cs_main) {
        const TestPeer peer{.id = next_id++, .best_known = active};
        manager.ConnectedPeer(peer.id);
        const auto announcements{Announce(manager, tree.active_chain, peer, max_serve_depth)};
        BOOST_REQUIRE_EQUAL(announcements.size(), 1U);
        return announcements.front().have_block;
    }};

    // have_block follows the tip's block, even while its ancestors are
    // missing.
    BOOST_CHECK(!have_block_by_depth(std::nullopt));
    tip->RaiseValidity(BLOCK_VALID_TRANSACTIONS);
    tip->nStatus |= BLOCK_HAVE_DATA;
    BOOST_CHECK(have_block_by_depth(std::nullopt));

    // Every block on the branch must be shallow enough to serve to the peer,
    // as it fetches them oldest first.
    const int first_depth{tree.active_chain.Height() - first->nHeight};
    BOOST_CHECK(have_block_by_depth(first_depth));
    BOOST_CHECK(!have_block_by_depth(first_depth - 1));

    const auto have_block_by_time{[&]() EXCLUSIVE_LOCKS_REQUIRED(::cs_main) {
        const TestPeer peer{.id = next_id++, .best_known = active};
        manager.ConnectedPeer(peer.id);
        const auto announcements{manager.GetStaleTipsToAnnounce(
            peer.id, tree.active_chain,
            // Like the historical block serving limit, refuse blocks older
            // than a cutoff, here the first block after the fork point.
            /*can_serve_block=*/[&](const CBlockIndex& block) { return block.GetBlockTime() >= first->GetBlockTime(); },
            /*peer_has_header=*/[&](const CBlockIndex& block) { return peer.HasHeader(block); })};
        BOOST_REQUIRE_EQUAL(announcements.size(), 1U);
        return announcements.front().have_block;
    }};

    BOOST_CHECK(have_block_by_time());

    // Block times need not increase along the branch, so an interior block
    // older than both ends is checked too.
    middle->nTime = first->nTime - 1;
    BOOST_CHECK(!have_block_by_time());
}

BOOST_AUTO_TEST_CASE(block_request_lifecycle)
{
    LOCK(::cs_main);

    BlockTree tree;
    CBlockIndex* active{tree.AddChain(/*height=*/3, {.have_data = true})};
    CBlockIndex* parent{tree.Add(active->GetAncestor(1), /*active=*/false)};
    CBlockIndex* tip{tree.Add(parent, /*active=*/false)};
    const CChain& chain{tree.active_chain};

    node::StaleTipRelayManager manager{{.fetch_blocks = true, .block_request_timeout = 10s}};
    manager.ConnectedPeer(0);
    manager.ConnectedPeer(1);
    manager.ReceivedStaleTip(0, chain, *tip, /*have_block=*/false);
    BOOST_CHECK(manager.GetBlockRequestsToSend(0, chain, 0s, 2, AnyBlock).empty());
    BOOST_CHECK(manager.ReceivedStaleTip(0, chain, *tip, /*have_block=*/true));
    manager.ReceivedStaleTip(1, chain, *tip, /*have_block=*/true);

    BOOST_CHECK(manager.GetBlockRequestsToSend(0, chain, 0s, 0, AnyBlock).empty());
    BOOST_CHECK(manager.GetBlockRequestsToSend(0, chain, 0s, 1, AnyBlock) == Blocks{tip});
    BOOST_CHECK(!manager.TakeBlockRequest(1, tip->GetBlockHash()));
    BOOST_CHECK(manager.GetBlockRequestsToSend(0, chain, 0s, 1, AnyBlock).empty());
    BOOST_CHECK(manager.GetBlockRequestsToSend(1, chain, 0s, 1, AnyBlock) == Blocks{parent});

    // Storage from any source frees a slot. Neither storage nor timeout
    // requires a network response from the peer we selected.
    parent->nStatus |= BLOCK_HAVE_DATA;
    BOOST_CHECK(manager.GetBlockRequestsToSend(1, chain, 9s, 1, AnyBlock).empty());
    BOOST_CHECK(!manager.TakeBlockRequest(1, parent->GetBlockHash()));
    BOOST_CHECK(manager.GetBlockRequestsToSend(0, chain, 10s, 1, AnyBlock).empty());
    BOOST_CHECK(!manager.ReceivedStaleTip(0, chain, *tip, /*have_block=*/true));
    BOOST_CHECK(manager.GetBlockRequestsToSend(0, chain, 10s, 1, AnyBlock).empty());
    BOOST_CHECK(manager.GetBlockRequestsToSend(1, chain, 10s, 1, AnyBlock) == Blocks{tip});

    BOOST_CHECK(manager.TakeBlockRequest(1, tip->GetBlockHash()));
    BOOST_CHECK(!manager.TakeBlockRequest(1, tip->GetBlockHash()));
    BOOST_CHECK(manager.GetBlockRequestsToSend(1, chain, 10s, 1, AnyBlock).empty());
    manager.DisconnectedPeer(0);
    manager.DisconnectedPeer(1);
    manager.CheckIsEmpty();
}

BOOST_AUTO_TEST_CASE(higher_work_block_requests)
{
    LOCK(::cs_main);

    BlockTree tree;
    CBlockIndex* active{tree.AddChain(/*height=*/MAX_STALETIP_HEADERS + 1, {.have_data = true})};
    CBlockIndex* stale{tree.Add(active->pprev, /*active=*/false)};
    CBlockIndex* prefix{tree.active_chain.Genesis()};
    while (prefix->nHeight < active->nHeight - 1) {
        prefix = tree.Add(prefix, /*active=*/false, {.have_data = true});
        prefix->m_chain_tx_count = prefix->nHeight + 1;
    }
    CBlockIndex* parent{tree.Add(prefix, /*active=*/false)};
    CBlockIndex* tip{tree.Add(parent, /*active=*/false)};
    const CChain& chain{tree.active_chain};

    for (bool fetch_blocks : {false, true}) {
        node::StaleTipRelayManager manager{{.fetch_blocks = fetch_blocks}};
        manager.ConnectedPeer(0);
        manager.ReceivedStaleTip(0, chain, *stale, /*have_block=*/true);
        BOOST_CHECK(manager.ReceivedStaleTip(0, chain, *tip, /*have_block=*/true));

        // The older fork already has a complete prefix. The higher-work
        // offer gets the first slot, even in headers mode.
        BOOST_CHECK(manager.GetBlockRequestsToSend(0, chain, 0s, 1, AnyBlock) == Blocks{tip});
        BOOST_CHECK(manager.TakeBlockRequest(0, tip->GetBlockHash()));
        tip->nStatus |= BLOCK_HAVE_DATA;
        BOOST_CHECK(manager.GetBlockRequestsToSend(0, chain, 0s, 1, [](const CBlockIndex&) { return false; }).empty());
        BOOST_CHECK(manager.GetBlockRequestsToSend(0, chain, 0s, 1, AnyBlock) == Blocks{parent});
        const auto remaining{manager.GetBlockRequestsToSend(0, chain, 0s, 2, AnyBlock)};
        BOOST_CHECK(remaining == (fetch_blocks ? Blocks{stale} : Blocks{}));

        manager.DisconnectedPeer(0);
        manager.CheckIsEmpty();
        tip->nStatus &= ~BLOCK_HAVE_DATA;
    }
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
