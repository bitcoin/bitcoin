// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/net_signing.h>

#include <llmq/signhash.h>
#include <llmq/signing.h>

#include <bls/bls_batchverifier.h>
#include <cxxtimer.hpp>
#include <logging.h>
#include <streams.h>
#include <util/thread.h>
#include <validationinterface.h>

#include <unordered_map>

void NetSigning::ProcessMessage(CNode& pfrom, const std::string& msg_type, CDataStream& vRecv)
{
    if (msg_type != NetMsgType::QSIGREC) return;

    auto recoveredSig = std::make_shared<llmq::CRecoveredSig>();
    vRecv >> *recoveredSig;

    WITH_LOCK(cs_main, m_peer_manager->PeerEraseObjectRequest(pfrom.GetId(),
                                                              CInv{MSG_QUORUM_RECOVERED_SIG, recoveredSig->GetHash()}));

    if (!Params().GetLLMQ(recoveredSig->getLlmqType()).has_value()) {
        m_peer_manager->PeerMisbehaving(pfrom.GetId(), 100);
        return;
    }

    m_sig_manager.VerifyAndProcessRecoveredSig(pfrom.GetId(), std::move(recoveredSig));
}

void NetSigning::Start()
{
    // can't start new thread if we have one running already
    if (workThread.joinable()) {
        assert(false);
    }

    workThread = std::thread(&util::TraceThread, "recsigs", [this] { WorkThreadMain(); });
}

void NetSigning::Stop()
{
    // make sure to call InterruptWorkerThread() first
    if (!workInterrupt) {
        assert(false);
    }

    if (workThread.joinable()) {
        workThread.join();
    }
}

void NetSigning::ProcessRecoveredSig(std::shared_ptr<const llmq::CRecoveredSig> recoveredSig, bool consider_proactive_relay)
{
    if (!m_sig_manager.ProcessRecoveredSig(recoveredSig)) return;

    auto listeners = m_sig_manager.GetListeners();
    for (auto& l : listeners) {
        m_peer_manager->PeerPostProcessMessage(l->HandleNewRecoveredSig(*recoveredSig));
    }

    // TODO refactor to use a better abstraction analogous to IsAllMembersConnectedEnabled
    auto proactive_relay = consider_proactive_relay && recoveredSig->getLlmqType() != Consensus::LLMQType::LLMQ_100_67 &&
                           recoveredSig->getLlmqType() != Consensus::LLMQType::LLMQ_400_60 &&
                           recoveredSig->getLlmqType() != Consensus::LLMQType::LLMQ_400_85;
    GetMainSignals().NotifyRecoveredSig(recoveredSig, recoveredSig->GetHash().ToString(), proactive_relay);
}

bool NetSigning::ProcessPendingRecoveredSigs()
{
    Uint256HashMap<std::shared_ptr<const llmq::CRecoveredSig>> pending{m_sig_manager.FetchPendingReconstructed()};

    for (const auto& p : pending) {
        ProcessRecoveredSig(p.second, true);
    }

    std::unordered_map<NodeId, std::list<std::shared_ptr<const llmq::CRecoveredSig>>> recSigsByNode;
    std::unordered_map<std::pair<Consensus::LLMQType, uint256>, CBLSPublicKey, StaticSaltedHasher> pubkeys;

    const size_t nMaxBatchSize{32};
    bool more_work = m_sig_manager.CollectPendingRecoveredSigsToVerify(nMaxBatchSize, recSigsByNode, pubkeys);
    if (recSigsByNode.empty()) {
        return false;
    }

    // It's ok to perform insecure batched verification here as we verify against the quorum public keys, which are not
    // craftable by individual entities, making the rogue public key attack impossible
    CBLSBatchVerifier<NodeId, uint256> batchVerifier(false, false);

    size_t verifyCount = 0;
    for (const auto& [nodeId, v] : recSigsByNode) {
        for (const auto& recSig : v) {
            // we didn't verify the lazy signature until now
            if (!recSig->sig.Get().IsValid()) {
                batchVerifier.badSources.emplace(nodeId);
                break;
            }

            const auto& pubkey = pubkeys.at(std::make_pair(recSig->getLlmqType(), recSig->getQuorumHash()));
            batchVerifier.PushMessage(nodeId, recSig->GetHash(), recSig->buildSignHash().Get(), recSig->sig.Get(), pubkey);
            verifyCount++;
        }
    }

    cxxtimer::Timer verifyTimer(true);
    batchVerifier.Verify();
    verifyTimer.stop();

    LogPrint(BCLog::LLMQ, "NetSigning::%s -- verified recovered sig(s). count=%d, vt=%d, nodes=%d\n", __func__,
             verifyCount, verifyTimer.count(), recSigsByNode.size());

    Uint256HashSet processed;
    for (const auto& [nodeId, v] : recSigsByNode) {
        if (batchVerifier.badSources.count(nodeId)) {
            LogPrint(BCLog::LLMQ, "NetSigning::%s -- invalid recSig from other node, banning peer=%d\n", __func__, nodeId);
            m_peer_manager->PeerMisbehaving(nodeId, 100);
            continue;
        }

        for (const auto& recSig : v) {
            if (!processed.emplace(recSig->GetHash()).second) {
                continue;
            }

            ProcessRecoveredSig(recSig, nodeId == -1);
        }
    }

    return more_work;
}

void NetSigning::WorkThreadMain()
{
    while (!workInterrupt) {
        bool fMoreWork = ProcessPendingRecoveredSigs();

        constexpr auto CLEANUP_INTERVAL{5s};
        if (cleanupThrottler.TryCleanup(CLEANUP_INTERVAL)) {
            m_sig_manager.Cleanup();
        }

        // TODO Wakeup when pending signing is needed?
        if (!fMoreWork && !workInterrupt.sleep_for(std::chrono::milliseconds(100))) {
            return;
        }
    }
}
