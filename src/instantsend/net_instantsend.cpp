// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <instantsend/net_instantsend.h>

#include <bls/bls_batchverifier.h>
#include <cxxtimer.hpp>
#include <instantsend/instantsend.h>
#include <llmq/commitment.h>
#include <llmq/quorums.h>
#include <llmq/signhash.h>
#include <llmq/signing.h>
#include <util/thread.h>
#include <validation.h>

void NetInstantSend::ProcessMessage(CNode& pfrom, const std::string& msg_type, CDataStream& vRecv)
{
    if (msg_type != NetMsgType::ISDLOCK) {
        return;
    }

    if (!m_is_manager.IsInstantSendEnabled()) return;

    auto islock = std::make_shared<instantsend::InstantSendLock>();
    vRecv >> *islock;

    uint256 hash = ::SerializeHash(*islock);

    WITH_LOCK(::cs_main, m_peer_manager->PeerEraseObjectRequest(pfrom.GetId(), CInv{MSG_ISDLOCK, hash}));

    if (!islock->TriviallyValid()) {
        m_peer_manager->PeerMisbehaving(pfrom.GetId(), 100);
        return;
    }

    auto cycleHeightOpt = m_is_manager.GetBlockHeight(islock->cycleHash);
    if (!cycleHeightOpt) {
        const auto blockIndex = WITH_LOCK(::cs_main, return m_chainstate.m_blockman.LookupBlockIndex(islock->cycleHash));
        if (blockIndex == nullptr) {
            // Maybe we don't have the block yet or maybe some peer spams invalid values for cycleHash
            m_peer_manager->PeerMisbehaving(pfrom.GetId(), 1);
            return;
        }
        m_is_manager.CacheBlockHeight(blockIndex);
        cycleHeightOpt = blockIndex->nHeight;
    }
    const int block_height = *cycleHeightOpt;

    // Deterministic islocks MUST use rotation based llmq
    auto llmqType = Params().GetConsensus().llmqTypeDIP0024InstantSend;
    const auto& llmq_params_opt = Params().GetLLMQ(llmqType);
    assert(llmq_params_opt);
    if (block_height % llmq_params_opt->dkgInterval != 0) {
        m_peer_manager->PeerMisbehaving(pfrom.GetId(), 100);
        return;
    }

    if (!m_is_manager.AlreadyHave(CInv{MSG_ISDLOCK, hash})) {
        LogPrint(BCLog::INSTANTSEND, "NetInstantSend -- ISDLOCK txid=%s, islock=%s: received islock, peer=%d\n",
                 islock->txid.ToString(), hash.ToString(), pfrom.GetId());

        m_is_manager.EnqueueInstantSendLock(pfrom.GetId(), hash, std::move(islock));
    }
}

void NetInstantSend::Start()
{
    // can't start new thread if we have one running already
    if (workThread.joinable()) {
        assert(false);
    }

    workThread = std::thread(&util::TraceThread, "isman", [this] { WorkThreadMain(); });

    if (auto signer = m_is_manager.Signer(); signer) {
        signer->Start();
    }
}

void NetInstantSend::Stop()
{
    if (auto signer = m_is_manager.Signer(); signer) {
        signer->Stop();
    }

    // make sure to call Interrupt() first
    if (!workInterrupt) {
        assert(false);
    }

    if (workThread.joinable()) {
        workThread.join();
    }
}

Uint256HashSet NetInstantSend::ProcessPendingInstantSendLocks(
    const Consensus::LLMQParams& llmq_params, int signOffset, bool ban,
    const std::vector<std::pair<uint256, instantsend::PendingISLockFromPeer>>& pend)
{
    CBLSBatchVerifier<NodeId, uint256> batchVerifier(false, true, 8);
    Uint256HashMap<llmq::CRecoveredSig> recSigs;

    size_t verifyCount = 0;
    size_t alreadyVerified = 0;
    for (const auto& p : pend) {
        const auto& hash = p.first;
        auto nodeId = p.second.node_id;
        const auto& islock = p.second.islock;

        if (batchVerifier.badSources.count(nodeId)) {
            continue;
        }

        if (!islock->sig.Get().IsValid()) {
            batchVerifier.badSources.emplace(nodeId);
            continue;
        }

        auto id = islock->GetRequestId();

        // no need to verify an ISLOCK if we already have verified the recovered sig that belongs to it
        if (m_is_manager.Sigman().HasRecoveredSig(llmq_params.type, id, islock->txid)) {
            alreadyVerified++;
            continue;
        }

        auto cycleHeightOpt = m_is_manager.GetBlockHeight(islock->cycleHash);
        if (!cycleHeightOpt) {
            batchVerifier.badSources.emplace(nodeId);
            continue;
        }

        int nSignHeight{-1};
        const auto dkgInterval = llmq_params.dkgInterval;
        const int tipHeight = m_is_manager.GetTipHeight();
        const int cycleHeight = *cycleHeightOpt;
        if (cycleHeight + dkgInterval < tipHeight) {
            nSignHeight = cycleHeight + dkgInterval - 1;
        }
        // For RegTest non-rotating quorum cycleHash has directly quorum hash
        auto quorum = llmq_params.useRotation ? llmq::SelectQuorumForSigning(llmq_params, m_chainstate.m_chain, m_qman,
                                                                             id, nSignHeight, signOffset)
                                              : m_qman.GetQuorum(llmq_params.type, islock->cycleHash);

        if (!quorum) {
            // should not happen, but if one fails to select, all others will also fail to select
            return {};
        }
        uint256 signHash = llmq::SignHash{llmq_params.type, quorum->qc->quorumHash, id, islock->txid}.Get();
        batchVerifier.PushMessage(nodeId, hash, signHash, islock->sig.Get(), quorum->qc->quorumPublicKey);
        verifyCount++;

        // We can reconstruct the CRecoveredSig objects from the islock and pass it to the signing manager, which
        // avoids unnecessary double-verification of the signature. We however only do this when verification here
        // turns out to be good (which is checked further down)
        if (!m_is_manager.Sigman().HasRecoveredSigForId(llmq_params.type, id)) {
            recSigs.try_emplace(hash, llmq::CRecoveredSig(llmq_params.type, quorum->qc->quorumHash, id, islock->txid,
                                                          islock->sig));
        }
    }

    cxxtimer::Timer verifyTimer(true);
    batchVerifier.Verify();
    verifyTimer.stop();

    LogPrint(BCLog::INSTANTSEND, "NetInstantSend::%s -- verified locks. count=%d, alreadyVerified=%d, vt=%d, nodes=%d\n",
             __func__, verifyCount, alreadyVerified, verifyTimer.count(), batchVerifier.GetUniqueSourceCount());

    Uint256HashSet badISLocks;

    if (ban && !batchVerifier.badSources.empty()) {
        for (const auto& nodeId : batchVerifier.badSources) {
            // Let's not be too harsh, as the peer might simply be unlucky and might have sent us an old lock which
            // does not validate anymore due to changed quorums
            m_peer_manager->PeerMisbehaving(nodeId, 20);
        }
    }
    for (const auto& p : pend) {
        const auto& hash = p.first;
        auto nodeId = p.second.node_id;
        const auto& islock = p.second.islock;

        if (batchVerifier.badMessages.count(hash)) {
            LogPrint(BCLog::INSTANTSEND, "NetInstantSend::%s -- txid=%s, islock=%s: invalid sig in islock, peer=%d\n",
                     __func__, islock->txid.ToString(), hash.ToString(), nodeId);
            badISLocks.emplace(hash);
            continue;
        }

        CInv inv(MSG_ISDLOCK, hash);
        auto ret = m_is_manager.ProcessInstantSendLock(nodeId, hash, islock);
        if (std::holds_alternative<uint256>(ret)) {
            m_peer_manager->PeerRelayInvFiltered(inv, std::get<uint256>(ret));
            m_peer_manager->PeerAskPeersForTransaction(islock->txid);
        } else if (std::holds_alternative<CTransactionRef>(ret)) {
            m_peer_manager->PeerRelayInvFiltered(inv, *std::get<CTransactionRef>(ret));
        } else {
            assert(std::holds_alternative<std::monostate>(ret));
        }

        // See comment further on top. We pass a reconstructed recovered sig to the signing manager to avoid
        // double-verification of the sig.
        auto it = recSigs.find(hash);
        if (it != recSigs.end()) {
            auto recSig = std::make_shared<llmq::CRecoveredSig>(std::move(it->second));
            if (!m_is_manager.Sigman().HasRecoveredSigForId(llmq_params.type, recSig->getId())) {
                LogPrint(BCLog::INSTANTSEND, /* Continued */
                         "NetInstantSend::%s -- txid=%s, islock=%s: "
                         "passing reconstructed recSig to signing mgr, peer=%d\n",
                         __func__, islock->txid.ToString(), hash.ToString(), nodeId);
                m_is_manager.Sigman().PushReconstructedRecoveredSig(recSig);
            }
        }
    }

    return badISLocks;
}


void NetInstantSend::ProcessPendingISLocks(std::vector<std::pair<uint256, instantsend::PendingISLockFromPeer>>&& locks_to_process)
{
    // TODO Investigate if leaving this is ok
    auto llmqType = Params().GetConsensus().llmqTypeDIP0024InstantSend;
    const auto& llmq_params_opt = Params().GetLLMQ(llmqType);
    assert(llmq_params_opt);
    const auto& llmq_params = llmq_params_opt.value();
    auto dkgInterval = llmq_params.dkgInterval;

    // First check against the current active set and don't ban
    auto bad_is_locks = ProcessPendingInstantSendLocks(llmq_params, /*signOffset=*/0, /*ban=*/false, locks_to_process);
    if (!bad_is_locks.empty()) {
        LogPrint(BCLog::INSTANTSEND, "NetInstantSend::%s -- doing verification on old active set\n", __func__);

        // filter out valid IS locks from "pend" - keep only bad ones
        std::vector<std::pair<uint256, instantsend::PendingISLockFromPeer>> still_pending;
        still_pending.reserve(bad_is_locks.size());
        for (auto& p : locks_to_process) {
            if (bad_is_locks.contains(p.first)) {
                still_pending.emplace_back(std::move(p));
            }
        }
        // Now check against the previous active set and perform banning if this fails
        ProcessPendingInstantSendLocks(llmq_params, dkgInterval, /*ban=*/true, still_pending);
    }
}

void NetInstantSend::WorkThreadMain()
{
    while (!workInterrupt) {
        bool fMoreWork = [&]() -> bool {
            if (!m_is_manager.IsInstantSendEnabled()) return false;

            auto [more_work, locks] = m_is_manager.FetchPendingLocks();
            if (!locks.empty()) {
                ProcessPendingISLocks(std::move(locks));
            }
            if (auto signer = m_is_manager.Signer(); signer) {
                signer->ProcessPendingRetryLockTxs(m_is_manager.PrepareTxToRetry());
            }
            return more_work;
        }();

        if (!fMoreWork && !workInterrupt.sleep_for(std::chrono::milliseconds(100))) {
            return;
        }
    }
}
