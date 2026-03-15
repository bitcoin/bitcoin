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
};

class CBTCCheckpointsHandlerTestAccess
{
public:
    static void MarkSigChecked(llmq::CBTCCheckpointsHandler& handler, const uint256& hash)
    {
        LOCK(handler.cs);
        handler.sigChecked.emplace(hash, TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now()));
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

BOOST_AUTO_TEST_SUITE_END()
