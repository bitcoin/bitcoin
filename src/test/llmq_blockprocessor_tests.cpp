// Copyright (c) 2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/llmq_tests.h>
#include <test/util/net.h>
#include <test/util/setup_common.h>

#include <chainparams.h>
#include <consensus/params.h>
#include <hash.h>
#include <masternode/meta.h>
#include <net.h>
#include <net_processing.h>
#include <streams.h>
#include <util/time.h>
#include <validation.h>
#include <version.h>

#include <llmq/blockprocessor.h>
#include <llmq/commitment.h>
#include <llmq/context.h>
#include <msg_result.h>
#include <protocol.h>

#include <boost/test/unit_test.hpp>

using namespace llmq;
using namespace llmq::testutils;

BOOST_AUTO_TEST_SUITE(llmq_blockprocessor_tests)

namespace {
CDataStream Serialized(const CFinalCommitment& qc)
{
    CDataStream payload{SER_NETWORK, PROTOCOL_VERSION};
    payload << qc;
    return payload;
}
} // namespace

// A QFCOMMITMENT is only ever sent in reply to a GETDATA, so one the peer was never asked for must
// be dropped before CQuorumBlockProcessor does any lookup work on its behalf. The block processor
// holds no PeerManagerInternal -- net_processing already includes its header, so a reference would
// be circular -- and reaches the in-flight check through a predicate injected at the call site.
// Everything below therefore goes through PeerManagerImpl::ProcessMessage: calling the handler
// directly would test the gate but not the injection, which is the part that has no compiler to
// keep it honest.
BOOST_FIXTURE_TEST_CASE(unrequested_qfcommit_is_dropped_and_scored, TestChain100Setup)
{
    LOCK(NetEventsInterface::g_msgproc_mutex);

    // INV announcements for non-spork objects are only tracked outside IBD; the 100 mined blocks
    // of this fixture already take us out of it.
    BOOST_REQUIRE(!m_node.chainman->ActiveChainstate().IsInitialBlockDownload());

    // Every Dash-specific message is offered to CMNAuth first, which asserts a loaded metadata
    // manager. The fixture leaves it unloaded, so initialise an empty cache here.
    BOOST_REQUIRE(m_node.mn_metaman->LoadCache(/*load_cache=*/false));

    auto& blockprocessor = *m_node.llmq_ctx->quorum_block_processor;
    const auto& llmq_params = GetLLMQParams(Consensus::LLMQType::LLMQ_TEST_V17);
    BOOST_REQUIRE(Params().GetLLMQ(llmq_params.type).has_value());

    auto unsolicited_peer{MakeTestPeer(/*id=*/51)};
    auto announcing_peer{MakeTestPeer(/*id=*/52)};
    m_node.peerman->InitializeNode(*unsolicited_peer, NODE_NETWORK);
    m_node.peerman->InitializeNode(*announcing_peer, NODE_NETWORK);

    // quorumHash names no block we know of. That is the one rejection below the gate that
    // deliberately carries no penalty -- we may simply be behind or on another chain -- so any
    // score this payload collects can only have come from the gate, and the checks that do score
    // cannot be mistaken for it.
    const auto unsolicited_qc = CreateValidCommitment(llmq_params, GetTestBlockHash(51));
    const uint256 unsolicited_hash = ::SerializeHash(unsolicited_qc);

    // Sent twice on purpose: an unsolicited peer must not be able to repeat the block lookups and
    // mineable-commitment probes for free, so both copies have to cost it.
    for (int i = 0; i < 2; ++i) {
        SendMessage(*m_node.peerman, *unsolicited_peer, NetMsgType::QFCOMMITMENT, Serialized(unsolicited_qc));
    }
    BOOST_CHECK_EQUAL(MisbehaviorScore(*m_node.peerman, *unsolicited_peer),
                      2 * UNREQUESTED_OBJECT_MISBEHAVIOR_SCORE);
    // Nothing below the gate ran: had the commitment been processed it could only have ended up
    // here or been rejected, and this is the observable half of that.
    BOOST_CHECK(!blockprocessor.HasMineableCommitment(unsolicited_hash));

    // The gate also runs ahead of the null-commitment check, which scores 100. A null payload from
    // a peer we never asked must still cost exactly the unsolicited price -- reaching the 100 would
    // mean the gate had let it through.
    {
        const int score_before_null = MisbehaviorScore(*m_node.peerman, *unsolicited_peer);
        SendMessage(*m_node.peerman, *unsolicited_peer, NetMsgType::QFCOMMITMENT, Serialized(CFinalCommitment{}));
        BOOST_CHECK_EQUAL(MisbehaviorScore(*m_node.peerman, *unsolicited_peer),
                          score_before_null + UNREQUESTED_OBJECT_MISBEHAVIOR_SCORE);
    }

    // Announcing the commitment is NOT enough to authorise it. An INV creates a candidate
    // immediately, but the GETDATA only goes out later from SendMessages, so accepting on the
    // announcement alone would let a peer authorise its own payload by racing INV and payload back
    // to back -- which costs it nothing and defeats the gate entirely.
    const auto announced_qc = CreateValidCommitment(llmq_params, GetTestBlockHash(52));
    const CInv announced_inv{MSG_QUORUM_FINAL_COMMITMENT, ::SerializeHash(announced_qc)};

    AnnounceInv(*m_node.peerman, *announcing_peer, announced_inv);
    {
        const int score_before_race = MisbehaviorScore(*m_node.peerman, *announcing_peer);
        SendMessage(*m_node.peerman, *announcing_peer, NetMsgType::QFCOMMITMENT, Serialized(announced_qc));
        BOOST_CHECK_EQUAL(MisbehaviorScore(*m_node.peerman, *announcing_peer),
                          score_before_race + UNREQUESTED_OBJECT_MISBEHAVIOR_SCORE);
    }

    // Once SendMessages has actually issued the GETDATA the same payload is authorised. The
    // rejection above must not have consumed the candidate, or no GETDATA would go out at all.
    SetMockTime(GetTime<std::chrono::seconds>() + 61s);
    m_node.peerman->SendMessages(announcing_peer.get());
    const int score_before = MisbehaviorScore(*m_node.peerman, *announcing_peer);

    SendMessage(*m_node.peerman, *announcing_peer, NetMsgType::QFCOMMITMENT, Serialized(announced_qc));

    // Unchanged, because the unknown quorum block this commitment names is rejected without any
    // penalty -- which is what proves the message got past the gate. Asserting the total exactly is
    // what would catch the gate also charging a peer we did ask.
    BOOST_CHECK_EQUAL(MisbehaviorScore(*m_node.peerman, *announcing_peer), score_before);

    // One GETDATA authorises exactly one answer. Both the in-flight request and the late-answer
    // grace are spent, so a replay of the very payload we asked for is unsolicited again -- a peer
    // must not be able to induce one request and then repeat the payload for free.
    SendMessage(*m_node.peerman, *announcing_peer, NetMsgType::QFCOMMITMENT, Serialized(announced_qc));
    BOOST_CHECK_EQUAL(MisbehaviorScore(*m_node.peerman, *announcing_peer),
                      score_before + UNREQUESTED_OBJECT_MISBEHAVIOR_SCORE);

    m_node.peerman->FinalizeNode(*unsolicited_peer);
    m_node.peerman->FinalizeNode(*announcing_peer);
    SetMockTime(0s);
}

// A relayed commitment for a quorum we already hold a mineable commitment for is only worth
// verifying, and relaying, when it has more signers than ours.
BOOST_FIXTURE_TEST_CASE(qfcommit_with_more_signers_replaces_stored_one, TestChain100Setup)
{
    LOCK(NetEventsInterface::g_msgproc_mutex);
    BOOST_REQUIRE(!m_node.chainman->ActiveChainstate().IsInitialBlockDownload());
    BOOST_REQUIRE(m_node.mn_metaman->LoadCache(/*load_cache=*/false));

    auto& blockprocessor = *m_node.llmq_ctx->quorum_block_processor;
    const auto& llmq_params = GetLLMQParams(Consensus::LLMQType::LLMQ_TEST_V17);
    BOOST_REQUIRE(Params().GetLLMQ(llmq_params.type).has_value());
    BOOST_REQUIRE_EQUAL(llmq_params.size, 3);

    const CBlockIndex* base_index{WITH_LOCK(::cs_main, return m_node.chainman->ActiveChain().Tip())};
    base_index = base_index->GetAncestor(base_index->nHeight - (base_index->nHeight % llmq_params.dkgInterval));

    auto with_signers = [&](const CFinalCommitment& qc, size_t count) {
        CFinalCommitment ret{qc};
        for (size_t i = 0; i < ret.signers.size(); ++i) {
            ret.signers[i] = i < count;
        }
        return ret;
    };
    const auto base_qc = CreateValidCommitment(llmq_params, base_index->GetBlockHash());
    const auto qc_one = with_signers(base_qc, 1);
    const auto qc_two = with_signers(base_qc, 2);
    const auto qc_three = with_signers(base_qc, 3);
    auto qc_two_other{qc_three};
    qc_two_other.signers[0] = false;

    BOOST_REQUIRE(blockprocessor.AddMineableCommitment(qc_two).has_value());

    NodeId next_id{61};
    // Returns the misbehavior score the peer collected. The payloads carry random signatures, so a
    // commitment that reaches verification is rejected with 100, while one dropped by the
    // signer-count pre-check costs nothing.
    auto deliver = [&](const CFinalCommitment& qc) EXCLUSIVE_LOCKS_REQUIRED(NetEventsInterface::g_msgproc_mutex) {
        auto peer{MakeTestPeer(next_id++)};
        m_node.peerman->InitializeNode(*peer, NODE_NETWORK);
        AnnounceInv(*m_node.peerman, *peer, CInv{MSG_QUORUM_FINAL_COMMITMENT, ::SerializeHash(qc)});
        SetMockTime(GetTime<std::chrono::seconds>() + 61s);
        m_node.peerman->SendMessages(peer.get());
        SendMessage(*m_node.peerman, *peer, NetMsgType::QFCOMMITMENT, Serialized(qc));
        const int score{MisbehaviorScore(*m_node.peerman, *peer)};
        m_node.peerman->FinalizeNode(*peer);
        return score;
    };

    BOOST_CHECK_EQUAL(deliver(qc_one), 0);
    BOOST_CHECK_EQUAL(deliver(qc_two_other), 0);
    BOOST_CHECK_EQUAL(deliver(qc_three), 100);

    BOOST_CHECK(!blockprocessor.AddMineableCommitment(qc_one).has_value());
    BOOST_CHECK(blockprocessor.HasMineableCommitment(::SerializeHash(qc_two)));

    const auto inv = blockprocessor.AddMineableCommitment(qc_three);
    BOOST_REQUIRE(inv.has_value());
    BOOST_CHECK(inv->hash == ::SerializeHash(qc_three));
    BOOST_CHECK(blockprocessor.HasMineableCommitment(::SerializeHash(qc_three)));
    BOOST_CHECK(!blockprocessor.HasMineableCommitment(::SerializeHash(qc_two)));

    SetMockTime(0s);
}

BOOST_AUTO_TEST_SUITE_END()
