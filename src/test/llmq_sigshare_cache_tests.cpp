// Copyright (c) 2026 The Syscoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/quorums.h>
#include <llmq/quorums_btccheckpoints.h>
#include <llmq/quorums_blockprocessor.h>
#include <llmq/quorums_chainlocks.h>
#include <llmq/quorums_commitment.h>
#include <chainparams.h>
#include <random.h>
#include <test/util/setup_common.h>
#include <util/time.h>
#include <validation.h>

#include <array>
#include <boost/test/unit_test.hpp>

namespace llmq_tests
{

class CQuorumManagerTestAccess
{
public:
    static llmq::CQuorumPtr MakeQuorum(
        llmq::CQuorumManager& manager,
        const CBlockIndex* base,
        const uint256& mined_block_hash,
        const uint256& commitment_tag)
    {
        auto commitment = std::make_unique<llmq::CFinalCommitment>(base->GetBlockHash());
        commitment->quorumVvecHash = commitment_tag;
        auto quorum = std::make_shared<llmq::CQuorum>(manager.blsWorker);
        quorum->Init(
            std::move(commitment),
            base,
            mined_block_hash,
            Span<CDeterministicMNCPtr>{});
        return quorum;
    }

    static void CacheQuorum(
        llmq::CQuorumManager& manager,
        const llmq::CQuorumCPtr& quorum)
    {
        LOCK(manager.cs_quorums);
        manager.vecQuorumsCache.emplace_back(quorum);
    }

    static void RemoveCachedBase(
        llmq::CQuorumManager& manager,
        const uint256& quorum_hash)
    {
        LOCK(manager.cs_quorums);
        manager.vecQuorumsCache.erase(
            std::remove_if(
                manager.vecQuorumsCache.begin(),
                manager.vecQuorumsCache.end(),
                [&](const llmq::CQuorumCPtr& quorum) {
                    return quorum->m_quorum_base_block_index->GetBlockHash() == quorum_hash;
                }),
            manager.vecQuorumsCache.end());
    }

    static llmq::CQuorumCPtr GetQuorum(
        llmq::CQuorumManager& manager,
        const CBlockIndex* base)
    {
        return manager.GetQuorum(base);
    }

    static uint256 MakeQuorumKey(const llmq::CQuorum& quorum)
    {
        return llmq::CQuorumManager::MakeQuorumKey(quorum);
    }

    static bool IsQuorumMinedOnChain(
        const llmq::CQuorum& quorum,
        const CBlockIndex* tip)
    {
        return llmq::CQuorumManager::IsQuorumMinedOnChain(quorum, tip);
    }
};

class CChainLocksHandlerTestAccess
{
public:
    static void MarkSigChecked(
        llmq::CChainLocksHandler& handler,
        const uint256& hash,
        const uint256& fingerprint = uint256())
    {
        LOCK(handler.cs);
        handler.sigChecked.emplace(
            std::make_pair(hash, fingerprint),
            TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now()));
    }

    static bool IsSigChecked(
        llmq::CChainLocksHandler& handler,
        const uint256& hash,
        const uint256& fingerprint)
    {
        LOCK(handler.cs);
        return handler.sigChecked.count(std::make_pair(hash, fingerprint)) != 0;
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
        llmq::CChainLocksHandler::CQuorumContext context;
        if (!handler.BuildQuorumContext(pindexScan, context)) {
            return false;
        }
        return handler.VerifyChainLockShare(
            clsig, idIn, ret, hash, context);
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
        const CBlockIndex* current_index,
        const llmq::CChainLockSig& previous_share,
        const CBlockIndex* previous_share_index,
        int32_t dkg_interval)
    {
        return llmq::CChainLocksHandler::SelectAlternativeSigningTarget(
            height, current_index, previous_share, previous_share_index, dkg_interval);
    }

    static bool IsAlternativeCommitmentWindowStable(
        int32_t height,
        int32_t common_height,
        int32_t dkg_interval,
        int32_t mining_window_start,
        int32_t mining_window_end)
    {
        return llmq::CChainLocksHandler::IsAlternativeCommitmentWindowStable(
            height,
            common_height,
            dkg_interval,
            mining_window_start,
            mining_window_end);
    }

    static bool SameQuorumIdentity(
        const llmq::CQuorumCPtr& lhs,
        const llmq::CQuorumCPtr& rhs)
    {
        return llmq::CChainLocksHandler::SameQuorumIdentity(lhs, rhs);
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

BOOST_AUTO_TEST_CASE(quorum_cache_distinguishes_reorg_commitments)
{
    BOOST_REQUIRE(llmq::quorumManager != nullptr);
    BOOST_REQUIRE(llmq::quorumBlockProcessor != nullptr);

    const uint256 base_hash = GetRandHash();
    CBlockIndex base;
    base.phashBlock = &base_hash;
    base.nHeight = 0;

    const uint256 old_mined_hash = GetRandHash();
    const uint256 new_mined_hash = GetRandHash();
    auto old_quorum = llmq_tests::CQuorumManagerTestAccess::MakeQuorum(
        *llmq::quorumManager, &base, old_mined_hash, GetRandHash());
    auto new_quorum = llmq_tests::CQuorumManagerTestAccess::MakeQuorum(
        *llmq::quorumManager, &base, new_mined_hash, GetRandHash());

    llmq_tests::CQuorumManagerTestAccess::CacheQuorum(
        *llmq::quorumManager, old_quorum);
    llmq_tests::CQuorumManagerTestAccess::CacheQuorum(
        *llmq::quorumManager, new_quorum);
    llmq::quorumBlockProcessor->m_commitment_evoDb.WriteCache(
        base_hash, std::make_pair(*new_quorum->qc, new_mined_hash));

    const auto resolved = llmq_tests::CQuorumManagerTestAccess::GetQuorum(
        *llmq::quorumManager, &base);

    llmq::quorumBlockProcessor->m_commitment_evoDb.EraseCache(base_hash);
    llmq_tests::CQuorumManagerTestAccess::RemoveCachedBase(
        *llmq::quorumManager, base_hash);

    BOOST_REQUIRE(resolved != nullptr);
    BOOST_CHECK_EQUAL(resolved->minedBlockHash, new_mined_hash);
    BOOST_CHECK_EQUAL(
        ::SerializeHash(*resolved->qc),
        ::SerializeHash(*new_quorum->qc));
}

BOOST_AUTO_TEST_CASE(quorum_contribution_key_is_commitment_specific)
{
    BOOST_REQUIRE(llmq::quorumManager != nullptr);

    const uint256 base_hash = GetRandHash();
    CBlockIndex base;
    base.phashBlock = &base_hash;
    base.nHeight = 0;

    auto quorum_a = llmq_tests::CQuorumManagerTestAccess::MakeQuorum(
        *llmq::quorumManager, &base, GetRandHash(), GetRandHash());
    auto quorum_b = llmq_tests::CQuorumManagerTestAccess::MakeQuorum(
        *llmq::quorumManager, &base, GetRandHash(), GetRandHash());

    BOOST_CHECK(
        llmq_tests::CQuorumManagerTestAccess::MakeQuorumKey(*quorum_a) !=
        llmq_tests::CQuorumManagerTestAccess::MakeQuorumKey(*quorum_b));
    BOOST_CHECK(llmq_tests::CChainLocksHandlerTestAccess::SameQuorumIdentity(
        quorum_a, quorum_a));
    BOOST_CHECK(!llmq_tests::CChainLocksHandlerTestAccess::SameQuorumIdentity(
        quorum_a, quorum_b));
}

BOOST_AUTO_TEST_CASE(quorum_mining_block_must_be_in_target_ancestry)
{
    BOOST_REQUIRE(llmq::quorumManager != nullptr);

    std::array<CBlockIndex, 4> branch_a;
    std::array<uint256, 4> branch_a_hashes;
    std::array<CBlockIndex, 3> branch_b;
    std::array<uint256, 3> branch_b_hashes;
    for (size_t i = 0; i < branch_a.size(); ++i) {
        branch_a_hashes[i] = GetRandHash();
        branch_a[i].phashBlock = &branch_a_hashes[i];
        branch_a[i].nHeight = i;
        branch_a[i].pprev = i == 0 ? nullptr : &branch_a[i - 1];
    }
    for (size_t i = 0; i < branch_b.size(); ++i) {
        branch_b_hashes[i] = GetRandHash();
        branch_b[i].phashBlock = &branch_b_hashes[i];
        branch_b[i].nHeight = i + 1;
        branch_b[i].pprev = i == 0 ? &branch_a[0] : &branch_b[i - 1];
    }

    auto quorum = llmq_tests::CQuorumManagerTestAccess::MakeQuorum(
        *llmq::quorumManager,
        &branch_a[0],
        branch_a[2].GetBlockHash(),
        GetRandHash());

    BOOST_CHECK(llmq_tests::CQuorumManagerTestAccess::IsQuorumMinedOnChain(
        *quorum, &branch_a[3]));
    BOOST_CHECK(!llmq_tests::CQuorumManagerTestAccess::IsQuorumMinedOnChain(
        *quorum, &branch_a[1]));
    BOOST_CHECK(!llmq_tests::CQuorumManagerTestAccess::IsQuorumMinedOnChain(
        *quorum, &branch_b[2]));
}

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

BOOST_AUTO_TEST_CASE(chainlock_signature_cache_is_quorum_context_bound)
{
    BOOST_REQUIRE(llmq::chainLocksHandler != nullptr);

    const uint256 hash = GetRandHash();
    const uint256 old_fingerprint = GetRandHash();
    const uint256 new_fingerprint = GetRandHash();
    llmq_tests::CChainLocksHandlerTestAccess::MarkSigChecked(
        *llmq::chainLocksHandler, hash, old_fingerprint);

    BOOST_CHECK(llmq_tests::CChainLocksHandlerTestAccess::IsSigChecked(
        *llmq::chainLocksHandler, hash, old_fingerprint));
    BOOST_CHECK(!llmq_tests::CChainLocksHandlerTestAccess::IsSigChecked(
        *llmq::chainLocksHandler, hash, new_fingerprint));
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

    llmq::CChainLockSig previous_share;
    previous_share.nHeight = 10;
    previous_share.blockHash = fork_blocks.back().GetBlockHash();
    const CBlockIndex* selected =
        llmq_tests::CChainLocksHandlerTestAccess::SelectAlternativeSigningTarget(
            10, &active_blocks.back(), previous_share, &fork_blocks.back(), 12);
    BOOST_REQUIRE(selected == &fork_blocks.back());

    // A DKG boundary after the common ancestor makes ScanQuorums branch-specific.
    BOOST_CHECK(
        llmq_tests::CChainLocksHandlerTestAccess::SelectAlternativeSigningTarget(
            10, &active_blocks.back(), previous_share, &fork_blocks.back(), 5) == nullptr);

    previous_share.nHeight = 9;
    BOOST_CHECK(
        llmq_tests::CChainLocksHandlerTestAccess::SelectAlternativeSigningTarget(
            10, &active_blocks.back(), previous_share, &fork_blocks.back(), 12) == nullptr);

    // A target selected from a previously observed share must still be
    // rejected before signing if a lower, non-ancestor ChainLock wins.
    previous_share.nHeight = 10;
    previous_share.blockHash = active_blocks.back().GetBlockHash();
    selected = llmq_tests::CChainLocksHandlerTestAccess::SelectAlternativeSigningTarget(
        10, &fork_blocks.back(), previous_share, &active_blocks.back(), 12);
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

BOOST_AUTO_TEST_CASE(chainlock_alternative_rejects_unresolved_commitment_window)
{
    constexpr int32_t interval = 24;
    constexpr int32_t mining_start = 10;
    constexpr int32_t mining_end = 18;
    const auto stable = [&](int32_t height, int32_t common_height) {
        return llmq_tests::CChainLocksHandlerTestAccess::
            IsAlternativeCommitmentWindowStable(
                height,
                common_height,
                interval,
                mining_start,
                mining_end);
    };

    BOOST_CHECK(stable(9, 4));
    BOOST_CHECK(!stable(12, 7));
    BOOST_CHECK(!stable(20, 15));
    BOOST_CHECK(stable(20, 18));

    // The current round has not started at height 25, but a fork before the
    // preceding round's end can still replace that round's final commitment.
    BOOST_CHECK(!stable(25, 17));
    BOOST_CHECK(stable(25, 18));
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
