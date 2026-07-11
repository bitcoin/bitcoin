// Copyright (c) 2026 The Syscoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/quorums.h>
#include <llmq/quorums_btccheckpoints.h>
#include <llmq/quorums_chainlocks.h>
#include <chainparams.h>
#include <random.h>
#include <test/util/setup_common.h>
#include <util/time.h>
#include <validation.h>

#include <array>
#include <boost/test/unit_test.hpp>

namespace llmq_tests
{

class CChainLocksHandlerTestAccess
{
public:
    static void MarkSigChecked(llmq::CChainLocksHandler& handler, const uint256& hash)
    {
        LOCK(handler.cs);
        handler.sigChecked.emplace(hash, TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now()));
    }

    static void MarkRejected(llmq::CChainLocksHandler& handler, const uint256& hash)
    {
        handler.MarkRejectedChainLock(hash);
    }

    static bool VerifyShare(
        llmq::CChainLocksHandler& handler,
        const llmq::CChainLockSig& clsig,
        const CBlockIndex* pindexScan,
        const uint256& idIn,
        std::pair<int, llmq::CQuorumCPtr>& ret,
        const uint256& hash)
    {
        return handler.VerifyChainLockShare(clsig, pindexScan, idIn, ret, hash);
    }

    static bool IsCandidateStillAdmissible(
        const CChain& active_chain,
        const llmq::CChainLockSig& best_chainlock,
        const llmq::CChainLockSig& candidate,
        const CBlockIndex* candidate_index)
    {
        return llmq::CChainLocksHandler::IsCandidateStillAdmissible(
            active_chain, best_chainlock, candidate, candidate_index);
    }

    static const CBlockIndex* SelectAlternativeSigningTarget(
        int32_t height,
        const uint256& current_hash,
        const llmq::CChainLockSig& previous_share,
        const CBlockIndex* previous_share_index)
    {
        return llmq::CChainLocksHandler::SelectAlternativeSigningTarget(
            height, current_hash, previous_share, previous_share_index);
    }
};

class CBTCCheckpointsHandlerTestAccess
{
public:
    static void MarkSigChecked(llmq::CBTCCheckpointsHandler& handler, const uint256& hash)
    {
        LOCK(handler.cs);
        handler.sigChecked.emplace(hash, TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now()));
    }

    static void MarkRejected(llmq::CBTCCheckpointsHandler& handler, const uint256& hash)
    {
        handler.MarkRejectedBTCCheckpointSig(hash);
    }

    static bool VerifyShare(
        llmq::CBTCCheckpointsHandler& handler,
        const llmq::CBTCCheckpointSig& btcsig,
        const CBlockIndex* pindexScan,
        const uint256& idIn,
        std::pair<int, llmq::CQuorumCPtr>& ret,
        const uint256& hash)
    {
        return handler.VerifyBTCCheckpointShare(btcsig, pindexScan, idIn, ret, hash);
    }
};

} // namespace llmq_tests

BOOST_FIXTURE_TEST_SUITE(llmq_sigshare_cache_tests, RegTestingSetup)

BOOST_AUTO_TEST_CASE(chainlock_share_cache_hit_requires_signer_context)
{
    BOOST_REQUIRE(llmq::chainLocksHandler != nullptr);
    BOOST_REQUIRE(llmq::quorumManager != nullptr);

    const CBlockIndex* pindex_tip = WITH_LOCK(::cs_main, return m_node.chainman->ActiveChain().Tip());
    BOOST_REQUIRE(pindex_tip != nullptr);

    const size_t signing_active_quorum_count = Params().GetConsensus().llmqTypeChainLocks.signingActiveQuorumCount;
    BOOST_REQUIRE(signing_active_quorum_count > 0);
    BOOST_CHECK(llmq::quorumManager->ScanQuorums(pindex_tip, signing_active_quorum_count).empty());

    llmq::CChainLockSig clsig;
    clsig.nHeight = pindex_tip->nHeight;
    clsig.blockHash = pindex_tip->GetBlockHash();
    clsig.signers.assign(signing_active_quorum_count, false);

    std::pair<int, llmq::CQuorumCPtr> ret{11, nullptr};
    const uint256 hash = GetRandHash();
    const uint256 id = GetRandHash();
    llmq_tests::CChainLocksHandlerTestAccess::MarkSigChecked(*llmq::chainLocksHandler, hash);

    // Regression: cache-hit path previously returned true without setting ret,
    // which allowed callers to index shares under a nullptr quorum key.
    const bool ok = llmq_tests::CChainLocksHandlerTestAccess::VerifyShare(
        *llmq::chainLocksHandler, clsig, pindex_tip, id, ret, hash);
    BOOST_CHECK(!ok);
    BOOST_CHECK_EQUAL(ret.first, 11);
    BOOST_CHECK(ret.second == nullptr);
}

BOOST_AUTO_TEST_CASE(chainlock_rejected_cache_suppresses_repeated_requests)
{
    BOOST_REQUIRE(llmq::chainLocksHandler != nullptr);

    const uint256 hash = GetRandHash();
    BOOST_CHECK(!llmq::chainLocksHandler->AlreadyHave(hash));

    llmq_tests::CChainLocksHandlerTestAccess::MarkRejected(*llmq::chainLocksHandler, hash);
    BOOST_CHECK(llmq::chainLocksHandler->AlreadyHave(hash));
}

BOOST_AUTO_TEST_CASE(chainlock_aggregate_cache_hit_requires_aggregate_structure)
{
    BOOST_REQUIRE(llmq::chainLocksHandler != nullptr);

    const CBlockIndex* pindex_tip = WITH_LOCK(::cs_main, return m_node.chainman->ActiveChain().Tip());
    BOOST_REQUIRE(pindex_tip != nullptr);

    const size_t signing_active_quorum_count = Params().GetConsensus().llmqTypeChainLocks.signingActiveQuorumCount;
    BOOST_REQUIRE(signing_active_quorum_count > 1);

    llmq::CChainLockSig clsig;
    clsig.nHeight = pindex_tip->nHeight;
    clsig.blockHash = pindex_tip->GetBlockHash();

    clsig.signers.assign(signing_active_quorum_count, false);
    uint256 hash = ::SerializeHash(clsig);
    llmq_tests::CChainLocksHandlerTestAccess::MarkSigChecked(*llmq::chainLocksHandler, hash);
    BOOST_CHECK(!llmq::chainLocksHandler->VerifyAggregatedChainLock(clsig, pindex_tip, hash));

    clsig.signers.assign(signing_active_quorum_count, false);
    clsig.signers.front() = true;
    hash = ::SerializeHash(clsig);
    llmq_tests::CChainLocksHandlerTestAccess::MarkSigChecked(*llmq::chainLocksHandler, hash);
    BOOST_CHECK(!llmq::chainLocksHandler->VerifyAggregatedChainLock(clsig, pindex_tip, hash));

    clsig.signers.assign(signing_active_quorum_count - 1, true);
    hash = ::SerializeHash(clsig);
    llmq_tests::CChainLocksHandlerTestAccess::MarkSigChecked(*llmq::chainLocksHandler, hash);
    BOOST_CHECK(!llmq::chainLocksHandler->VerifyAggregatedChainLock(clsig, pindex_tip, hash));
}

BOOST_AUTO_TEST_CASE(chainlock_publication_rechecks_anchor_and_winner)
{
    std::array<CBlockIndex, 21> active_blocks;
    std::array<uint256, 21> active_hashes;
    for (size_t i = 0; i < active_blocks.size(); ++i) {
        active_hashes[i] = GetRandHash();
        active_blocks[i].phashBlock = &active_hashes[i];
        active_blocks[i].nHeight = i;
        active_blocks[i].pprev = i == 0 ? nullptr : &active_blocks[i - 1];
    }

    CChain active_chain;
    active_chain.SetTip(active_blocks.back());

    llmq::CChainLockSig candidate;
    candidate.nHeight = 10;
    candidate.blockHash = active_blocks[10].GetBlockHash();
    llmq::CChainLockSig no_winner;

    // Tip movement alone must not invalidate an in-flight, anchor-compatible
    // aggregate. The expected signing height is intentionally not rechecked.
    BOOST_CHECK(llmq_tests::CChainLocksHandlerTestAccess::IsCandidateStillAdmissible(
        active_chain, no_winner, candidate, &active_blocks[10]));

    std::array<CBlockIndex, 6> fork_blocks;
    std::array<uint256, 6> fork_hashes;
    for (size_t i = 0; i < fork_blocks.size(); ++i) {
        fork_hashes[i] = GetRandHash();
        fork_blocks[i].phashBlock = &fork_hashes[i];
        fork_blocks[i].nHeight = i + 5;
        fork_blocks[i].pprev = i == 0 ? &active_blocks[4] : &fork_blocks[i - 1];
    }
    candidate.blockHash = fork_blocks.back().GetBlockHash();
    BOOST_CHECK(!llmq_tests::CChainLocksHandlerTestAccess::IsCandidateStillAdmissible(
        active_chain, no_winner, candidate, &fork_blocks.back()));

    candidate.blockHash = active_blocks[10].GetBlockHash();
    llmq::CChainLockSig existing_winner;
    existing_winner.nHeight = candidate.nHeight;
    existing_winner.blockHash = candidate.blockHash;
    BOOST_CHECK(!llmq_tests::CChainLocksHandlerTestAccess::IsCandidateStillAdmissible(
        active_chain, existing_winner, candidate, &active_blocks[10]));

    // A higher candidate must descend from the already-published winner, even
    // while the active chain has not yet enforced that winner.
    existing_winner.blockHash = fork_blocks.back().GetBlockHash();
    llmq::CChainLockSig higher_candidate;
    higher_candidate.nHeight = 15;
    higher_candidate.blockHash = active_blocks[15].GetBlockHash();
    BOOST_CHECK(!llmq_tests::CChainLocksHandlerTestAccess::IsCandidateStillAdmissible(
        active_chain, existing_winner, higher_candidate, &active_blocks[15]));

    // If the higher window publishes first, the lower candidate cannot replace it.
    llmq::CChainLockSig higher_winner = higher_candidate;
    candidate.blockHash = fork_blocks.back().GetBlockHash();
    BOOST_CHECK(!llmq_tests::CChainLocksHandlerTestAccess::IsCandidateStillAdmissible(
        active_chain, higher_winner, candidate, &fork_blocks.back()));
}

BOOST_AUTO_TEST_CASE(chainlock_alternative_tip_selects_exact_signing_hash)
{
    const uint256 current_hash = GetRandHash();
    const uint256 alternative_hash = GetRandHash();
    CBlockIndex current;
    current.phashBlock = &current_hash;
    current.nHeight = 10;
    CBlockIndex alternative;
    alternative.phashBlock = &alternative_hash;
    alternative.nHeight = 10;

    llmq::CChainLockSig previous_share;
    previous_share.nHeight = 10;
    previous_share.blockHash = alternative_hash;
    const CBlockIndex* selected =
        llmq_tests::CChainLocksHandlerTestAccess::SelectAlternativeSigningTarget(
            10, current_hash, previous_share, &alternative);
    BOOST_REQUIRE(selected != nullptr);
    BOOST_CHECK_EQUAL(selected->GetBlockHash(), alternative_hash);

    alternative.nHeight = 9;
    BOOST_CHECK(
        llmq_tests::CChainLocksHandlerTestAccess::SelectAlternativeSigningTarget(
            10, current_hash, previous_share, &alternative) == nullptr);
    alternative.nHeight = 10;
    previous_share.blockHash = current_hash;
    BOOST_CHECK(
        llmq_tests::CChainLocksHandlerTestAccess::SelectAlternativeSigningTarget(
            10, current_hash, previous_share, &current) == nullptr);

    // A target selected from a previously observed share must still be
    // rejected before signing if a lower, non-ancestor ChainLock wins.
    std::array<CBlockIndex, 11> active_blocks;
    std::array<uint256, 11> active_hashes;
    for (size_t i = 0; i < active_blocks.size(); ++i) {
        active_hashes[i] = GetRandHash();
        active_blocks[i].phashBlock = &active_hashes[i];
        active_blocks[i].nHeight = i;
        active_blocks[i].pprev = i == 0 ? nullptr : &active_blocks[i - 1];
    }
    std::array<CBlockIndex, 6> fork_blocks;
    std::array<uint256, 6> fork_hashes;
    for (size_t i = 0; i < fork_blocks.size(); ++i) {
        fork_hashes[i] = GetRandHash();
        fork_blocks[i].phashBlock = &fork_hashes[i];
        fork_blocks[i].nHeight = i + 5;
        fork_blocks[i].pprev = i == 0 ? &active_blocks[4] : &fork_blocks[i - 1];
    }

    previous_share.blockHash = active_blocks.back().GetBlockHash();
    selected = llmq_tests::CChainLocksHandlerTestAccess::SelectAlternativeSigningTarget(
        10, fork_blocks.back().GetBlockHash(), previous_share, &active_blocks.back());
    BOOST_REQUIRE(selected == &active_blocks.back());

    CChain active_chain;
    active_chain.SetTip(active_blocks.back());
    llmq::CChainLockSig signing_candidate;
    signing_candidate.nHeight = 10;
    signing_candidate.blockHash = selected->GetBlockHash();
    llmq::CChainLockSig no_winner;
    BOOST_CHECK(llmq_tests::CChainLocksHandlerTestAccess::IsCandidateStillAdmissible(
        active_chain, no_winner, signing_candidate, selected));

    llmq::CChainLockSig lower_winner;
    lower_winner.nHeight = 5;
    lower_winner.blockHash = fork_blocks.front().GetBlockHash();
    BOOST_CHECK(!llmq_tests::CChainLocksHandlerTestAccess::IsCandidateStillAdmissible(
        active_chain, lower_winner, signing_candidate, selected));
}

BOOST_AUTO_TEST_CASE(btccheckpoint_share_cache_hit_requires_signer_context)
{
    BOOST_REQUIRE(llmq::btcCheckpointsHandler != nullptr);
    BOOST_REQUIRE(llmq::quorumManager != nullptr);

    const CBlockIndex* pindex_tip = WITH_LOCK(::cs_main, return m_node.chainman->ActiveChain().Tip());
    BOOST_REQUIRE(pindex_tip != nullptr);

    const size_t signing_active_quorum_count = Params().GetConsensus().llmqTypeChainLocks.signingActiveQuorumCount;
    BOOST_REQUIRE(signing_active_quorum_count > 0);
    BOOST_CHECK(llmq::quorumManager->ScanQuorums(pindex_tip, signing_active_quorum_count).empty());

    llmq::CBTCCheckpointSig btcsig;
    btcsig.nHeight = pindex_tip->nHeight;
    btcsig.sysHash = pindex_tip->GetBlockHash();
    btcsig.signers.assign(signing_active_quorum_count, false);

    std::pair<int, llmq::CQuorumCPtr> ret{13, nullptr};
    const uint256 hash = GetRandHash();
    const uint256 id = GetRandHash();
    llmq_tests::CBTCCheckpointsHandlerTestAccess::MarkSigChecked(*llmq::btcCheckpointsHandler, hash);

    // Regression: cache-hit path must not report success unless signer/quorum
    // context can be recovered for callers that key state by ret.
    const bool ok = llmq_tests::CBTCCheckpointsHandlerTestAccess::VerifyShare(
        *llmq::btcCheckpointsHandler, btcsig, pindex_tip, id, ret, hash);
    BOOST_CHECK(!ok);
    BOOST_CHECK_EQUAL(ret.first, 13);
    BOOST_CHECK(ret.second == nullptr);
}

BOOST_AUTO_TEST_CASE(btccheckpoint_rejected_cache_suppresses_repeated_requests)
{
    BOOST_REQUIRE(llmq::btcCheckpointsHandler != nullptr);

    const uint256 hash = GetRandHash();
    BOOST_CHECK(!llmq::btcCheckpointsHandler->AlreadyHave(hash));

    llmq_tests::CBTCCheckpointsHandlerTestAccess::MarkRejected(*llmq::btcCheckpointsHandler, hash);
    BOOST_CHECK(llmq::btcCheckpointsHandler->AlreadyHave(hash));
}

BOOST_AUTO_TEST_CASE(btccheckpoint_aggregate_cache_hit_requires_aggregate_structure)
{
    BOOST_REQUIRE(llmq::btcCheckpointsHandler != nullptr);

    const CBlockIndex* pindex_tip = WITH_LOCK(::cs_main, return m_node.chainman->ActiveChain().Tip());
    BOOST_REQUIRE(pindex_tip != nullptr);

    const size_t signing_active_quorum_count = Params().GetConsensus().llmqTypeChainLocks.signingActiveQuorumCount;
    BOOST_REQUIRE(signing_active_quorum_count > 1);

    llmq::CBTCCheckpointSig btcsig;
    btcsig.nHeight = pindex_tip->nHeight;
    btcsig.sysHash = pindex_tip->GetBlockHash();

    btcsig.signers.assign(signing_active_quorum_count, false);
    uint256 hash = ::SerializeHash(btcsig);
    llmq_tests::CBTCCheckpointsHandlerTestAccess::MarkSigChecked(*llmq::btcCheckpointsHandler, hash);
    BOOST_CHECK(!llmq::btcCheckpointsHandler->VerifyAggregatedBTCCheckpoint(btcsig, pindex_tip));

    btcsig.signers.assign(signing_active_quorum_count, false);
    btcsig.signers.front() = true;
    hash = ::SerializeHash(btcsig);
    llmq_tests::CBTCCheckpointsHandlerTestAccess::MarkSigChecked(*llmq::btcCheckpointsHandler, hash);
    BOOST_CHECK(!llmq::btcCheckpointsHandler->VerifyAggregatedBTCCheckpoint(btcsig, pindex_tip));

    btcsig.signers.assign(signing_active_quorum_count - 1, true);
    hash = ::SerializeHash(btcsig);
    llmq_tests::CBTCCheckpointsHandlerTestAccess::MarkSigChecked(*llmq::btcCheckpointsHandler, hash);
    BOOST_CHECK(!llmq::btcCheckpointsHandler->VerifyAggregatedBTCCheckpoint(btcsig, pindex_tip));
}

BOOST_AUTO_TEST_SUITE_END()
