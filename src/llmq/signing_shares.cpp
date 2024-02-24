// Copyright (c) 2018-2024 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/signing_shares.h>

#include <llmq/options.h>
#include <llmq/quorums.h>
#include <llmq/commitment.h>
#include <llmq/signing.h>

#include <bls/bls_batchverifier.h>
#include <chainparams.h>
#include <evo/deterministicmns.h>
#include <masternode/node.h>
#include <net_processing.h>
#include <netmessagemaker.h>
#include <spork.h>
#include <util/irange.h>
#include <util/thread.h>
#include <util/time.h>
#include <util/underlying.h>

#include <cxxtimer.hpp>

namespace llmq
{
void CSigShare::UpdateKey()
{
    key.first = this->buildSignHash();
    key.second = quorumMember;
}

std::string CSigSesAnn::ToString() const
{
    return strprintf("sessionId=%d, llmqType=%d, quorumHash=%s, id=%s, msgHash=%s",
                     sessionId, ToUnderlying(getLlmqType()), getQuorumHash().ToString(), getId().ToString(), getMsgHash().ToString());
}

void CSigSharesInv::Merge(const CSigSharesInv& inv2)
{
    for (const auto i : irange::range(inv.size())) {
        if (inv2.inv[i]) {
            inv[i] = inv2.inv[i];
        }
    }
}

size_t CSigSharesInv::CountSet() const
{
    return (size_t)std::count(inv.begin(), inv.end(), true);
}

std::string CSigSharesInv::ToString() const
{
    std::string str = "(";
    bool first = true;
    for (const auto i : irange::range(inv.size())) {
        if (!inv[i]) {
            continue;
        }

        if (!first) {
            str += ",";
        }
        first = false;
        str += strprintf("%d", i);
    }
    str += ")";
    return str;
}

void CSigSharesInv::Init(size_t size)
{
    inv.resize(size, false);
}

void CSigSharesInv::Set(uint16_t quorumMember, bool v)
{
    assert(quorumMember < inv.size());
    inv[quorumMember] = v;
}

void CSigSharesInv::SetAll(bool v)
{
    std::fill(inv.begin(), inv.end(), v);
}

std::string CBatchedSigShares::ToInvString() const
{
    CSigSharesInv inv;
    // we use 400 here no matter what the real size is. We don't really care about that size as we just want to call ToString()
    inv.Init(400);
    for (const auto& sigShare : sigShares) {
        inv.inv[sigShare.first] = true;
    }
    return inv.ToString();
}

static void InitSession(CSigSharesNodeState::Session& s, const uint256& signHash, CSigBase from)
{
    const auto& llmq_params_opt = Params().GetLLMQ(from.getLlmqType());
    assert(llmq_params_opt.has_value());
    const auto& llmq_params = llmq_params_opt.value();

    s.llmqType = from.getLlmqType();
    s.quorumHash = from.getQuorumHash();
    s.id = from.getId();
    s.msgHash = from.getMsgHash();
    s.signHash = signHash;
    s.announced.Init((size_t)llmq_params.size);
    s.requested.Init((size_t)llmq_params.size);
    s.knows.Init((size_t)llmq_params.size);
}

CSigSharesNodeState::Session& CSigSharesNodeState::GetOrCreateSessionFromShare(const llmq::CSigShare& sigShare)
{
    auto& s = sessions[sigShare.GetSignHash()];
    if (s.announced.inv.empty()) {
        InitSession(s, sigShare.GetSignHash(), sigShare);
    }
    return s;
}

CSigSharesNodeState::Session& CSigSharesNodeState::GetOrCreateSessionFromAnn(const llmq::CSigSesAnn& ann)
{
    auto signHash = ann.buildSignHash();
    auto& s = sessions[signHash];
    if (s.announced.inv.empty()) {
        InitSession(s, signHash, ann);
    }
    return s;
}

CSigSharesNodeState::Session* CSigSharesNodeState::GetSessionBySignHash(const uint256& signHash)
{
    auto it = sessions.find(signHash);
    if (it == sessions.end()) {
        return nullptr;
    }
    return &it->second;
}

CSigSharesNodeState::Session* CSigSharesNodeState::GetSessionByRecvId(uint32_t sessionId)
{
    auto it = sessionByRecvId.find(sessionId);
    if (it == sessionByRecvId.end()) {
        return nullptr;
    }
    return it->second;
}

bool CSigSharesNodeState::GetSessionInfoByRecvId(uint32_t sessionId, SessionInfo& retInfo)
{
    const auto* s = GetSessionByRecvId(sessionId);
    if (s == nullptr) {
        return false;
    }
    retInfo.llmqType = s->llmqType;
    retInfo.quorumHash = s->quorumHash;
    retInfo.id = s->id;
    retInfo.msgHash = s->msgHash;
    retInfo.signHash = s->signHash;
    retInfo.quorum = s->quorum;

    return true;
}

void CSigSharesNodeState::RemoveSession(const uint256& signHash)
{
    if (const auto it = sessions.find(signHash); it != sessions.end()) {
        sessionByRecvId.erase(it->second.recvSessionId);
        sessions.erase(it);
    }
    requestedSigShares.EraseAllForSignHash(signHash);
    pendingIncomingSigShares.EraseAllForSignHash(signHash);
}

//////////////////////

void CSigSharesManager::StartWorkerThread()
{
    // can't start new thread if we have one running already
    if (workThread.joinable()) {
        assert(false);
    }

    workThread = std::thread(&util::TraceThread, "sigshares", [this] { WorkThreadMain(); });
}

void CSigSharesManager::StopWorkerThread()
{
    // make sure to call InterruptWorkerThread() first
    if (!workInterrupt) {
        assert(false);
    }

    if (workThread.joinable()) {
        workThread.join();
    }
}

void CSigSharesManager::RegisterAsRecoveredSigsListener()
{
    sigman.RegisterRecoveredSigsListener(this);
}

void CSigSharesManager::UnregisterAsRecoveredSigsListener()
{
    sigman.UnregisterRecoveredSigsListener(this);
}

void CSigSharesManager::InterruptWorkerThread()
{
    workInterrupt();
}

void CSigSharesManager::ProcessMessage(const CNode& pfrom, const CSporkManager& sporkManager, const std::string& msg_type, CDataStream& vRecv)
{
    // non-masternodes are not interested in sigshares
    if (!fMasternodeMode || WITH_LOCK(activeMasternodeInfoCs, return activeMasternodeInfo.proTxHash.IsNull())) {
        return;
    }

    if (sporkManager.IsSporkActive(SPORK_21_QUORUM_ALL_CONNECTED) && msg_type == NetMsgType::QSIGSHARE) {
        std::vector<CSigShare> receivedSigShares;
        vRecv >> receivedSigShares;

        if (receivedSigShares.size() > MAX_MSGS_SIG_SHARES) {
            LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- too many sigs in QSIGSHARE message. cnt=%d, max=%d, node=%d\n", __func__, receivedSigShares.size(), MAX_MSGS_SIG_SHARES, pfrom.GetId());
            BanNode(pfrom.GetId());
            return;
        }

        for (const auto& sigShare : receivedSigShares) {
            ProcessMessageSigShare(pfrom.GetId(), sigShare);
        }
    }

    if (msg_type == NetMsgType::QSIGSESANN) {
        std::vector<CSigSesAnn> msgs;
        vRecv >> msgs;
        if (msgs.size() > MAX_MSGS_CNT_QSIGSESANN) {
            LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- too many announcements in QSIGSESANN message. cnt=%d, max=%d, node=%d\n", __func__, msgs.size(), MAX_MSGS_CNT_QSIGSESANN, pfrom.GetId());
            BanNode(pfrom.GetId());
            return;
        }
        if (!ranges::all_of(msgs,
                            [this, &pfrom](const auto& ann){ return ProcessMessageSigSesAnn(pfrom, ann); })) {
            BanNode(pfrom.GetId());
            return;
        }
    } else if (msg_type == NetMsgType::QSIGSHARESINV) {
        std::vector<CSigSharesInv> msgs;
        vRecv >> msgs;
        if (msgs.size() > MAX_MSGS_CNT_QSIGSHARESINV) {
            LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- too many invs in QSIGSHARESINV message. cnt=%d, max=%d, node=%d\n", __func__, msgs.size(), MAX_MSGS_CNT_QSIGSHARESINV, pfrom.GetId());
            BanNode(pfrom.GetId());
            return;
        }
        if (!ranges::all_of(msgs,
                            [this, &pfrom](const auto& inv){ return ProcessMessageSigSharesInv(pfrom, inv); })) {
            BanNode(pfrom.GetId());
            return;
        }
    } else if (msg_type == NetMsgType::QGETSIGSHARES) {
        std::vector<CSigSharesInv> msgs;
        vRecv >> msgs;
        if (msgs.size() > MAX_MSGS_CNT_QGETSIGSHARES) {
            LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- too many invs in QGETSIGSHARES message. cnt=%d, max=%d, node=%d\n", __func__, msgs.size(), MAX_MSGS_CNT_QGETSIGSHARES, pfrom.GetId());
            BanNode(pfrom.GetId());
            return;
        }
        if (!ranges::all_of(msgs,
                            [this, &pfrom](const auto& inv){ return ProcessMessageGetSigShares(pfrom, inv); })) {
            BanNode(pfrom.GetId());
            return;
        }
    } else if (msg_type == NetMsgType::QBSIGSHARES) {
        std::vector<CBatchedSigShares> msgs;
        vRecv >> msgs;
        size_t totalSigsCount = 0;
        for (const auto& bs : msgs) {
            totalSigsCount += bs.sigShares.size();
        }
        if (totalSigsCount > MAX_MSGS_TOTAL_BATCHED_SIGS) {
            LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- too many sigs in QBSIGSHARES message. cnt=%d, max=%d, node=%d\n", __func__, msgs.size(), MAX_MSGS_TOTAL_BATCHED_SIGS, pfrom.GetId());
            BanNode(pfrom.GetId());
            return;
        }
        if (!ranges::all_of(msgs,
                            [this, &pfrom](const auto& bs){ return ProcessMessageBatchedSigShares(pfrom, bs); })) {
            BanNode(pfrom.GetId());
            return;
        }
    }
}

bool CSigSharesManager::ProcessMessageSigSesAnn(const CNode& pfrom, const CSigSesAnn& ann)
{
    auto llmqType = ann.getLlmqType();
    if (!Params().GetLLMQ(llmqType).has_value()) {
        return false;
    }
    if (ann.getSessionId() == UNINITIALIZED_SESSION_ID || ann.getQuorumHash().IsNull() || ann.getId().IsNull() || ann.getMsgHash().IsNull()) {
        return false;
    }

    LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- ann={%s}, node=%d\n", __func__, ann.ToString(), pfrom.GetId());

    auto quorum = qman.GetQuorum(llmqType, ann.getQuorumHash());
    if (!quorum) {
        // TODO should we ban here?
        LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- quorum %s not found, node=%d\n", __func__,
                  ann.getQuorumHash().ToString(), pfrom.GetId());
        return true; // let's still try other announcements from the same message
    }

    LOCK(cs);
    auto& nodeState = nodeStates[pfrom.GetId()];
    auto& session = nodeState.GetOrCreateSessionFromAnn(ann);
    nodeState.sessionByRecvId.erase(session.recvSessionId);
    nodeState.sessionByRecvId.erase(ann.getSessionId());
    session.recvSessionId = ann.getSessionId();
    session.quorum = quorum;
    nodeState.sessionByRecvId.try_emplace(ann.getSessionId(), &session);

    return true;
}

bool CSigSharesManager::VerifySigSharesInv(Consensus::LLMQType llmqType, const CSigSharesInv& inv)
{
    const auto& llmq_params_opt = Params().GetLLMQ(llmqType);
    return llmq_params_opt.has_value() && (inv.inv.size() == size_t(llmq_params_opt->size));
}

bool CSigSharesManager::ProcessMessageSigSharesInv(const CNode& pfrom, const CSigSharesInv& inv)
{
    CSigSharesNodeState::SessionInfo sessionInfo;
    if (!GetSessionInfoByRecvId(pfrom.GetId(), inv.sessionId, sessionInfo)) {
        return true;
    }

    if (!VerifySigSharesInv(sessionInfo.llmqType, inv)) {
        return false;
    }

    // TODO for PoSe, we should consider propagating shares even if we already have a recovered sig
    if (sigman.HasRecoveredSigForSession(sessionInfo.signHash)) {
        return true;
    }

    LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- signHash=%s, inv={%s}, node=%d\n", __func__,
            sessionInfo.signHash.ToString(), inv.ToString(), pfrom.GetId());

    if (!sessionInfo.quorum->HasVerificationVector()) {
        // TODO we should allow to ask other nodes for the quorum vvec if we missed it in the DKG
        LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- we don't have the quorum vvec for %s, not requesting sig shares. node=%d\n", __func__,
                  sessionInfo.quorumHash.ToString(), pfrom.GetId());
        return true;
    }

    LOCK(cs);
    auto& nodeState = nodeStates[pfrom.GetId()];
    auto* session = nodeState.GetSessionByRecvId(inv.sessionId);
    if (session == nullptr) {
        return true;
    }
    session->announced.Merge(inv);
    session->knows.Merge(inv);
    return true;
}

bool CSigSharesManager::ProcessMessageGetSigShares(const CNode& pfrom, const CSigSharesInv& inv)
{
    CSigSharesNodeState::SessionInfo sessionInfo;
    if (!GetSessionInfoByRecvId(pfrom.GetId(), inv.sessionId, sessionInfo)) {
        return true;
    }

    if (!VerifySigSharesInv(sessionInfo.llmqType, inv)) {
        return false;
    }

    // TODO for PoSe, we should consider propagating shares even if we already have a recovered sig
    if (sigman.HasRecoveredSigForSession(sessionInfo.signHash)) {
        return true;
    }

    LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- signHash=%s, inv={%s}, node=%d\n", __func__,
            sessionInfo.signHash.ToString(), inv.ToString(), pfrom.GetId());

    LOCK(cs);
    auto& nodeState = nodeStates[pfrom.GetId()];
    auto* session = nodeState.GetSessionByRecvId(inv.sessionId);
    if (session == nullptr) {
        return true;
    }
    session->requested.Merge(inv);
    session->knows.Merge(inv);
    return true;
}

bool CSigSharesManager::ProcessMessageBatchedSigShares(const CNode& pfrom, const CBatchedSigShares& batchedSigShares)
{
    CSigSharesNodeState::SessionInfo sessionInfo;
    if (!GetSessionInfoByRecvId(pfrom.GetId(), batchedSigShares.sessionId, sessionInfo)) {
        return true;
    }

    if (bool ban{false}; !PreVerifyBatchedSigShares(qman, sessionInfo, batchedSigShares, ban)) {
        return !ban;
    }

    std::vector<CSigShare> sigSharesToProcess;
    sigSharesToProcess.reserve(batchedSigShares.sigShares.size());

    {
        LOCK(cs);
        auto& nodeState = nodeStates[pfrom.GetId()];

        for (const auto& sigSharetmp : batchedSigShares.sigShares) {
            CSigShare sigShare = RebuildSigShare(sessionInfo, sigSharetmp);
            nodeState.requestedSigShares.Erase(sigShare.GetKey());

            // TODO track invalid sig shares received for PoSe?
            // It's important to only skip seen *valid* sig shares here. If a node sends us a
            // batch of mostly valid sig shares with a single invalid one and thus batched
            // verification fails, we'd skip the valid ones in the future if received from other nodes
            if (sigShares.Has(sigShare.GetKey())) {
                continue;
            }

            // TODO for PoSe, we should consider propagating shares even if we already have a recovered sig
            if (sigman.HasRecoveredSigForId(sigShare.getLlmqType(), sigShare.getId())) {
                continue;
            }

            sigSharesToProcess.emplace_back(sigShare);
        }
    }

    LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- signHash=%s, shares=%d, new=%d, inv={%s}, node=%d\n", __func__,
             sessionInfo.signHash.ToString(), batchedSigShares.sigShares.size(), sigSharesToProcess.size(), batchedSigShares.ToInvString(), pfrom.GetId());

    if (sigSharesToProcess.empty()) {
        return true;
    }

    LOCK(cs);
    auto& nodeState = nodeStates[pfrom.GetId()];
    for (const auto& s : sigSharesToProcess) {
        nodeState.pendingIncomingSigShares.Add(s.GetKey(), s);
    }
    return true;
}

void CSigSharesManager::ProcessMessageSigShare(NodeId fromId, const CSigShare& sigShare)
{
    auto quorum = qman.GetQuorum(sigShare.getLlmqType(), sigShare.getQuorumHash());
    if (!quorum) {
        return;
    }
    if (!IsQuorumActive(sigShare.getLlmqType(), qman, quorum->qc->quorumHash)) {
        // quorum is too old
        return;
    }
    if (!quorum->IsMember(WITH_LOCK(activeMasternodeInfoCs, return activeMasternodeInfo.proTxHash))) {
        // we're not a member so we can't verify it (we actually shouldn't have received it)
        return;
    }
    if (!quorum->HasVerificationVector()) {
        // TODO we should allow to ask other nodes for the quorum vvec if we missed it in the DKG
        LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- we don't have the quorum vvec for %s, no verification possible. node=%d\n", __func__,
                 quorum->qc->quorumHash.ToString(), fromId);
        return;
    }

    if (sigShare.getQuorumMember() >= quorum->members.size()) {
        LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- quorumMember out of bounds\n", __func__);
        BanNode(fromId);
        return;
    }
    if (!quorum->qc->validMembers[sigShare.getQuorumMember()]) {
        LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- quorumMember not valid\n", __func__);
        BanNode(fromId);
        return;
    }

    {
        LOCK(cs);

        if (sigShares.Has(sigShare.GetKey())) {
            return;
        }

        if (sigman.HasRecoveredSigForId(sigShare.getLlmqType(), sigShare.getId())) {
            return;
        }

        auto& nodeState = nodeStates[fromId];
        nodeState.pendingIncomingSigShares.Add(sigShare.GetKey(), sigShare);
    }

    LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- signHash=%s, id=%s, msgHash=%s, member=%d, node=%d\n", __func__,
             sigShare.GetSignHash().ToString(), sigShare.getId().ToString(), sigShare.getMsgHash().ToString(), sigShare.getQuorumMember(), fromId);
}

bool CSigSharesManager::PreVerifyBatchedSigShares(const CQuorumManager& quorum_manager, const CSigSharesNodeState::SessionInfo& session, const CBatchedSigShares& batchedSigShares, bool& retBan)
{
    retBan = false;

    if (!IsQuorumActive(session.llmqType, quorum_manager, session.quorum->qc->quorumHash)) {
        // quorum is too old
        return false;
    }
    if (!session.quorum->IsMember(WITH_LOCK(activeMasternodeInfoCs, return activeMasternodeInfo.proTxHash))) {
        // we're not a member so we can't verify it (we actually shouldn't have received it)
        return false;
    }
    if (!session.quorum->HasVerificationVector()) {
        // TODO we should allow to ask other nodes for the quorum vvec if we missed it in the DKG
        LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- we don't have the quorum vvec for %s, no verification possible.\n", __func__,
                  session.quorumHash.ToString());
        return false;
    }

    std::unordered_set<uint16_t> dupMembers;

    for (const auto& [quorumMember, _] : batchedSigShares.sigShares) {
        if (!dupMembers.emplace(quorumMember).second) {
            retBan = true;
            return false;
        }

        if (quorumMember >= session.quorum->members.size()) {
            LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- quorumMember out of bounds\n", __func__);
            retBan = true;
            return false;
        }
        if (!session.quorum->qc->validMembers[quorumMember]) {
            LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- quorumMember not valid\n", __func__);
            retBan = true;
            return false;
        }
    }
    return true;
}

void CSigSharesManager::CollectPendingSigSharesToVerify(
        size_t maxUniqueSessions,
        std::unordered_map<NodeId, std::vector<CSigShare>>& retSigShares,
        std::unordered_map<std::pair<Consensus::LLMQType, uint256>, CQuorumCPtr, StaticSaltedHasher>& retQuorums)
{
    {
        LOCK(cs);
        if (nodeStates.empty()) {
            return;
        }

        // This will iterate node states in random order and pick one sig share at a time. This avoids processing
        // of large batches at once from the same node while other nodes also provided shares. If we wouldn't do this,
        // other nodes would be able to poison us with a large batch with N-1 valid shares and the last one being
        // invalid, making batch verification fail and revert to per-share verification, which in turn would slow down
        // the whole verification process

        std::unordered_set<std::pair<NodeId, uint256>, StaticSaltedHasher> uniqueSignHashes;
        IterateNodesRandom(nodeStates, [&]() {
            return uniqueSignHashes.size() < maxUniqueSessions;
        }, [&](NodeId nodeId, CSigSharesNodeState& ns) {
            if (ns.pendingIncomingSigShares.Empty()) {
                return false;
            }
            const auto& sigShare = *ns.pendingIncomingSigShares.GetFirst();

            AssertLockHeld(cs);
            if (const bool alreadyHave = this->sigShares.Has(sigShare.GetKey()); !alreadyHave) {
                uniqueSignHashes.emplace(nodeId, sigShare.GetSignHash());
                retSigShares[nodeId].emplace_back(sigShare);
            }
            ns.pendingIncomingSigShares.Erase(sigShare.GetKey());
            return !ns.pendingIncomingSigShares.Empty();
        }, rnd);

        if (retSigShares.empty()) {
            return;
        }
    }

    // For the convenience of the caller, also build a map of quorumHash -> quorum

    for (const auto& [_, vecSigShares] : retSigShares) {
        for (const auto& sigShare : vecSigShares) {
            auto llmqType = sigShare.getLlmqType();

            auto k = std::make_pair(llmqType, sigShare.getQuorumHash());
            if (retQuorums.count(k) != 0) {
                continue;
            }

            CQuorumCPtr quorum = qman.GetQuorum(llmqType, sigShare.getQuorumHash());
            assert(quorum != nullptr);
            retQuorums.try_emplace(k, quorum);
        }
    }
}

bool CSigSharesManager::ProcessPendingSigShares(const CConnman& connman)
{
    std::unordered_map<NodeId, std::vector<CSigShare>> sigSharesByNodes;
    std::unordered_map<std::pair<Consensus::LLMQType, uint256>, CQuorumCPtr, StaticSaltedHasher> quorums;

    const size_t nMaxBatchSize{32};
    CollectPendingSigSharesToVerify(nMaxBatchSize, sigSharesByNodes, quorums);
    if (sigSharesByNodes.empty()) {
        return false;
    }

    // It's ok to perform insecure batched verification here as we verify against the quorum public key shares,
    // which are not craftable by individual entities, making the rogue public key attack impossible
    CBLSBatchVerifier<NodeId, SigShareKey> batchVerifier(false, true);

    cxxtimer::Timer prepareTimer(true);
    size_t verifyCount = 0;
    for (const auto& [nodeId, v] : sigSharesByNodes) {
        for (const auto& sigShare : v) {
            if (sigman.HasRecoveredSigForId(sigShare.getLlmqType(), sigShare.getId())) {
                continue;
            }

            // we didn't check this earlier because we use a lazy BLS signature and tried to avoid doing the expensive
            // deserialization in the message thread
            if (!sigShare.sigShare.Get().IsValid()) {
                BanNode(nodeId);
                // don't process any additional shares from this node
                break;
            }

            auto quorum = quorums.at(std::make_pair(sigShare.getLlmqType(), sigShare.getQuorumHash()));
            auto pubKeyShare = quorum->GetPubKeyShare(sigShare.getQuorumMember());

            if (!pubKeyShare.IsValid()) {
                // this should really not happen (we already ensured we have the quorum vvec,
                // so we should also be able to create all pubkey shares)
                LogPrintf("CSigSharesManager::%s -- pubKeyShare is invalid, which should not be possible here\n", __func__);
                assert(false);
            }

            batchVerifier.PushMessage(nodeId, sigShare.GetKey(), sigShare.GetSignHash(), sigShare.sigShare.Get(), pubKeyShare);
            verifyCount++;
        }
    }
    prepareTimer.stop();

    cxxtimer::Timer verifyTimer(true);
    batchVerifier.Verify();
    verifyTimer.stop();

    LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- verified sig shares. count=%d, pt=%d, vt=%d, nodes=%d\n", __func__, verifyCount, prepareTimer.count(), verifyTimer.count(), sigSharesByNodes.size());

    for (const auto& [nodeId, v] : sigSharesByNodes) {
        if (batchVerifier.badSources.count(nodeId) != 0) {
            LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- invalid sig shares from other node, banning peer=%d\n",
                     __func__, nodeId);
            // this will also cause re-requesting of the shares that were sent by this node
            BanNode(nodeId);
            continue;
        }

        ProcessPendingSigShares(v, quorums, connman);
    }

    return sigSharesByNodes.size() >= nMaxBatchSize;
}

// It's ensured that no duplicates are passed to this method
void CSigSharesManager::ProcessPendingSigShares(const std::vector<CSigShare>& sigSharesToProcess,
        const std::unordered_map<std::pair<Consensus::LLMQType, uint256>, CQuorumCPtr, StaticSaltedHasher>& quorums,
        const CConnman& connman)
{
    cxxtimer::Timer t(true);
    for (const auto& sigShare : sigSharesToProcess) {
        auto quorumKey = std::make_pair(sigShare.getLlmqType(), sigShare.getQuorumHash());
        ProcessSigShare(sigShare, connman, quorums.at(quorumKey));
    }
    t.stop();

    LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- processed sigShare batch. shares=%d, time=%ds\n", __func__,
             sigSharesToProcess.size(), t.count());
}

// sig shares are already verified when entering this method
void CSigSharesManager::ProcessSigShare(const CSigShare& sigShare, const CConnman& connman, const CQuorumCPtr& quorum)
{
    auto llmqType = quorum->params.type;

    bool canTryRecovery = false;

    // prepare node set for direct-push in case this is our sig share
    std::set<NodeId> quorumNodes;
    if (!IsAllMembersConnectedEnabled(llmqType) && sigShare.getQuorumMember() == quorum->GetMemberIndex(WITH_LOCK(activeMasternodeInfoCs, return activeMasternodeInfo.proTxHash))) {
        quorumNodes = connman.GetMasternodeQuorumNodes(sigShare.getLlmqType(), sigShare.getQuorumHash());
    }

    if (sigman.HasRecoveredSigForId(llmqType, sigShare.getId())) {
        return;
    }

    {
        LOCK(cs);

        if (!sigShares.Add(sigShare.GetKey(), sigShare)) {
            return;
        }
        if (!IsAllMembersConnectedEnabled(llmqType)) {
            sigSharesQueuedToAnnounce.Add(sigShare.GetKey(), true);
        }

        // Update the time we've seen the last sigShare
        timeSeenForSessions[sigShare.GetSignHash()] = GetTime<std::chrono::seconds>().count();

        if (!quorumNodes.empty()) {
            // don't announce and wait for other nodes to request this share and directly send it to them
            // there is no way the other nodes know about this share as this is the one created on this node
            for (auto otherNodeId : quorumNodes) {
                auto& nodeState = nodeStates[otherNodeId];
                auto& session = nodeState.GetOrCreateSessionFromShare(sigShare);
                session.quorum = quorum;
                session.requested.Set(sigShare.getQuorumMember(), true);
                session.knows.Set(sigShare.getQuorumMember(), true);
            }
        }

        size_t sigShareCount = sigShares.CountForSignHash(sigShare.GetSignHash());
        if (sigShareCount >= size_t(quorum->params.threshold)) {
            canTryRecovery = true;
        }
    }

    if (canTryRecovery) {
        TryRecoverSig(quorum, sigShare.getId(), sigShare.getMsgHash());
    }
}

void CSigSharesManager::TryRecoverSig(const CQuorumCPtr& quorum, const uint256& id, const uint256& msgHash)
{
    if (sigman.HasRecoveredSigForId(quorum->params.type, id)) {
        return;
    }

    std::vector<CBLSSignature> sigSharesForRecovery;
    std::vector<CBLSId> idsForRecovery;
    {
        LOCK(cs);

        auto signHash = BuildSignHash(quorum->params.type, quorum->qc->quorumHash, id, msgHash);
        const auto* sigSharesForSignHash = sigShares.GetAllForSignHash(signHash);
        if (sigSharesForSignHash == nullptr) {
            return;
        }

        sigSharesForRecovery.reserve((size_t) quorum->params.threshold);
        idsForRecovery.reserve((size_t) quorum->params.threshold);
        for (auto it = sigSharesForSignHash->begin(); it != sigSharesForSignHash->end() && sigSharesForRecovery.size() < size_t(quorum->params.threshold); ++it) {
            const auto& sigShare = it->second;
            sigSharesForRecovery.emplace_back(sigShare.sigShare.Get());
            idsForRecovery.emplace_back(quorum->members[sigShare.getQuorumMember()]->proTxHash);
        }

        // check if we can recover the final signature
        if (sigSharesForRecovery.size() < size_t(quorum->params.threshold)) {
            return;
        }
    }

    // now recover it
    cxxtimer::Timer t(true);
    CBLSSignature recoveredSig;
    if (!recoveredSig.Recover(sigSharesForRecovery, idsForRecovery)) {
        LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- failed to recover signature. id=%s, msgHash=%s, time=%d\n", __func__,
                  id.ToString(), msgHash.ToString(), t.count());
        return;
    }

    LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- recovered signature. id=%s, msgHash=%s, time=%d\n", __func__,
              id.ToString(), msgHash.ToString(), t.count());

    auto rs = std::make_shared<CRecoveredSig>(quorum->params.type, quorum->qc->quorumHash, id, msgHash, recoveredSig);

    // There should actually be no need to verify the self-recovered signatures as it should always succeed. Let's
    // however still verify it from time to time, so that we have a chance to catch bugs. We do only this sporadic
    // verification because this is unbatched and thus slow verification that happens here.
    if (((recoveredSigsCounter++) % 100) == 0) {
        auto signHash = rs->buildSignHash();
        bool valid = recoveredSig.VerifyInsecure(quorum->qc->quorumPublicKey, signHash);
        if (!valid) {
            // this should really not happen as we have verified all signature shares before
            LogPrintf("CSigSharesManager::%s -- own recovered signature is invalid. id=%s, msgHash=%s\n", __func__,
                      id.ToString(), msgHash.ToString());
            return;
        }
    }

    sigman.ProcessRecoveredSig(rs);
}

CDeterministicMNCPtr CSigSharesManager::SelectMemberForRecovery(const CQuorumCPtr& quorum, const uint256 &id, size_t attempt)
{
    assert(size_t(attempt) < quorum->members.size());

    std::vector<std::pair<uint256, CDeterministicMNCPtr>> v;
    v.reserve(quorum->members.size());
    for (const auto& dmn : quorum->members) {
        auto h = ::SerializeHash(std::make_pair(dmn->proTxHash, id));
        v.emplace_back(h, dmn);
    }
    std::sort(v.begin(), v.end());

    return v[attempt].second;
}

void CSigSharesManager::CollectSigSharesToRequest(std::unordered_map<NodeId, std::unordered_map<uint256, CSigSharesInv, StaticSaltedHasher>>& sigSharesToRequest)
{
    AssertLockHeld(cs);

    int64_t now = GetTime<std::chrono::seconds>().count();
    const size_t maxRequestsForNode = 32;

    // avoid requesting from same nodes all the time
    std::vector<NodeId> shuffledNodeIds;
    shuffledNodeIds.reserve(nodeStates.size());
    for (const auto& [nodeId, nodeState] : nodeStates) {
        if (nodeState.sessions.empty()) {
            continue;
        }
        shuffledNodeIds.emplace_back(nodeId);
    }
    Shuffle(shuffledNodeIds.begin(), shuffledNodeIds.end(), rnd);

    for (const auto& nodeId : shuffledNodeIds) {
        auto& nodeState = nodeStates[nodeId];

        if (nodeState.banned) {
            continue;
        }

        nodeState.requestedSigShares.EraseIf([&now, &nodeId](const SigShareKey& k, int64_t t) {
            if (now - t >= SIG_SHARE_REQUEST_TIMEOUT) {
                // timeout while waiting for this one, so retry it with another node
                LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::CollectSigSharesToRequest -- timeout while waiting for %s-%d, node=%d\n",
                         k.first.ToString(), k.second, nodeId);
                return true;
            }
            return false;
        });

        decltype(sigSharesToRequest.begin()->second)* invMap = nullptr;

        for (auto& [signHash, session] : nodeState.sessions) {
            if (IsAllMembersConnectedEnabled(session.llmqType)) {
                continue;
            }

            if (sigman.HasRecoveredSigForSession(signHash)) {
                continue;
            }

            for (const auto i : irange::range(session.announced.inv.size())) {
                if (!session.announced.inv[i]) {
                    continue;
                }
                auto k = std::make_pair(signHash, (uint16_t) i);
                if (sigShares.Has(k)) {
                    // we already have it
                    session.announced.inv[i] = false;
                    continue;
                }
                if (nodeState.requestedSigShares.Size() >= maxRequestsForNode) {
                    // too many pending requests for this node
                    break;
                }
                if (auto *const p = sigSharesRequested.Get(k)) {
                    if (now - p->second >= SIG_SHARE_REQUEST_TIMEOUT && nodeId != p->first) {
                        // other node timed out, re-request from this node
                        LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- other node timeout while waiting for %s-%d, re-request from=%d, node=%d\n", __func__,
                                 k.first.ToString(), k.second, nodeId, p->first);
                    } else {
                        continue;
                    }
                }
                // if we got this far we should do a request

                // track when we initiated the request so that we can detect timeouts
                nodeState.requestedSigShares.Add(k, now);

                // don't request it from other nodes until a timeout happens
                auto& r = sigSharesRequested.GetOrAdd(k);
                r.first = nodeId;
                r.second = now;

                if (invMap == nullptr) {
                    invMap = &sigSharesToRequest[nodeId];
                }
                auto& inv = (*invMap)[signHash];
                if (inv.inv.empty()) {
                    const auto& llmq_params_opt = Params().GetLLMQ(session.llmqType);
                    assert(llmq_params_opt.has_value());
                    inv.Init(llmq_params_opt->size);
                }
                inv.inv[k.second] = true;

                // don't request it again from this node
                session.announced.inv[i] = false;
            }
        }
    }
}

void CSigSharesManager::CollectSigSharesToSend(std::unordered_map<NodeId, std::unordered_map<uint256, CBatchedSigShares, StaticSaltedHasher>>& sigSharesToSend)
{
    AssertLockHeld(cs);

    for (auto& [nodeId, nodeState] : nodeStates) {
        if (nodeState.banned) {
            continue;
        }

        decltype(sigSharesToSend.begin()->second)* sigSharesToSend2 = nullptr;

        for (auto& [signHash, session] : nodeState.sessions) {
            if (IsAllMembersConnectedEnabled(session.llmqType)) {
                continue;
            }

            if (sigman.HasRecoveredSigForSession(signHash)) {
                continue;
            }

            CBatchedSigShares batchedSigShares;

            for (const auto i : irange::range(session.requested.inv.size())) {
                if (!session.requested.inv[i]) {
                    continue;
                }
                session.requested.inv[i] = false;

                auto k = std::make_pair(signHash, (uint16_t)i);
                const CSigShare* sigShare = sigShares.Get(k);
                if (sigShare == nullptr) {
                    // he requested something we don't have
                    session.requested.inv[i] = false;
                    continue;
                }

                batchedSigShares.sigShares.emplace_back((uint16_t)i, sigShare->sigShare);
            }

            if (!batchedSigShares.sigShares.empty()) {
                if (sigSharesToSend2 == nullptr) {
                    // only create the map if we actually add a batched sig
                    sigSharesToSend2 = &sigSharesToSend[nodeId];
                }
                sigSharesToSend2->try_emplace(signHash, std::move(batchedSigShares));
            }
        }
    }
}

void CSigSharesManager::CollectSigSharesToSendConcentrated(std::unordered_map<NodeId, std::vector<CSigShare>>& sigSharesToSend, const std::vector<CNode*>& vNodes)
{
    AssertLockHeld(cs);

    std::unordered_map<uint256, CNode*, StaticSaltedHasher> proTxToNode;
    for (const auto& pnode : vNodes) {
        auto verifiedProRegTxHash = pnode->GetVerifiedProRegTxHash();
        if (verifiedProRegTxHash.IsNull()) {
            continue;
        }
        proTxToNode.try_emplace(verifiedProRegTxHash, pnode);
    }

    auto curTime = GetTime<std::chrono::milliseconds>().count();

    for (auto& [_, signedSession] : signedSessions) {
        if (!IsAllMembersConnectedEnabled(signedSession.quorum->params.type)) {
            continue;
        }

        if (signedSession.attempt >= signedSession.quorum->params.recoveryMembers) {
            continue;
        }

        if (curTime >= signedSession.nextAttemptTime) {
            int64_t waitTime = exp2(signedSession.attempt) * EXP_SEND_FOR_RECOVERY_TIMEOUT;
            waitTime = std::min(MAX_SEND_FOR_RECOVERY_TIMEOUT, waitTime);
            signedSession.nextAttemptTime = curTime + waitTime;
            auto dmn = SelectMemberForRecovery(signedSession.quorum, signedSession.sigShare.getId(), signedSession.attempt);
            signedSession.attempt++;

            LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- signHash=%s, sending to %s, attempt=%d\n", __func__,
                     signedSession.sigShare.GetSignHash().ToString(), dmn->proTxHash.ToString(), signedSession.attempt);

            auto it = proTxToNode.find(dmn->proTxHash);
            if (it == proTxToNode.end()) {
                continue;
            }

            auto& m = sigSharesToSend[it->second->GetId()];
            m.emplace_back(signedSession.sigShare);
        }
    }
}

void CSigSharesManager::CollectSigSharesToAnnounce(std::unordered_map<NodeId, std::unordered_map<uint256, CSigSharesInv, StaticSaltedHasher>>& sigSharesToAnnounce)
{
    AssertLockHeld(cs);

    std::unordered_map<std::pair<Consensus::LLMQType, uint256>, std::unordered_set<NodeId>, StaticSaltedHasher> quorumNodesMap;

    sigSharesQueuedToAnnounce.ForEach([this, &quorumNodesMap, &sigSharesToAnnounce](const SigShareKey& sigShareKey, bool) {
        AssertLockHeld(cs);
        const auto& signHash = sigShareKey.first;
        auto quorumMember = sigShareKey.second;
        const CSigShare* sigShare = sigShares.Get(sigShareKey);
        if (sigShare == nullptr) {
            return;
        }

        // announce to the nodes which we know through the intra-quorum-communication system
        auto quorumKey = std::make_pair(sigShare->getLlmqType(), sigShare->getQuorumHash());
        auto it = quorumNodesMap.find(quorumKey);
        if (it == quorumNodesMap.end()) {
            auto nodeIds = connman.GetMasternodeQuorumNodes(quorumKey.first, quorumKey.second);
            it = quorumNodesMap.emplace(std::piecewise_construct, std::forward_as_tuple(quorumKey), std::forward_as_tuple(nodeIds.begin(), nodeIds.end())).first;
        }

        const auto& quorumNodes = it->second;

        for (const auto& nodeId : quorumNodes) {
            auto& nodeState = nodeStates[nodeId];

            if (nodeState.banned) {
                continue;
            }

            auto& session = nodeState.GetOrCreateSessionFromShare(*sigShare);

            if (session.knows.inv[quorumMember]) {
                // he already knows that one
                continue;
            }

            auto& inv = sigSharesToAnnounce[nodeId][signHash];
            if (inv.inv.empty()) {
                const auto& llmq_params_opt = Params().GetLLMQ(sigShare->getLlmqType());
                assert(llmq_params_opt.has_value());
                inv.Init(llmq_params_opt->size);
            }
            inv.inv[quorumMember] = true;
            session.knows.inv[quorumMember] = true;
        }
    });

    // don't announce these anymore
    sigSharesQueuedToAnnounce.Clear();
}

bool CSigSharesManager::SendMessages()
{
    std::unordered_map<NodeId, std::unordered_map<uint256, CSigSharesInv, StaticSaltedHasher>> sigSharesToRequest;
    std::unordered_map<NodeId, std::unordered_map<uint256, CBatchedSigShares, StaticSaltedHasher>> sigShareBatchesToSend;
    std::unordered_map<NodeId, std::vector<CSigShare>> sigSharesToSend;
    std::unordered_map<NodeId, std::unordered_map<uint256, CSigSharesInv, StaticSaltedHasher>> sigSharesToAnnounce;
    std::unordered_map<NodeId, std::vector<CSigSesAnn>> sigSessionAnnouncements;

    auto addSigSesAnnIfNeeded = [&](NodeId nodeId, const uint256& signHash) {
        AssertLockHeld(cs);
        auto& nodeState = nodeStates[nodeId];
        auto* session = nodeState.GetSessionBySignHash(signHash);
        assert(session);
        if (session->sendSessionId == UNINITIALIZED_SESSION_ID) {
            session->sendSessionId = nodeState.nextSendSessionId++;

            sigSessionAnnouncements[nodeId].emplace_back(
                    CSigSesAnn(/*sessionId=*/session->sendSessionId, /*llmqType=*/session->llmqType,
                               /*quorumHash=*/session->quorumHash, /*id=*/session->id, /*msgHash=*/session->msgHash)
            );
        }
        return session->sendSessionId;
    };

    std::vector<CNode*> vNodesCopy = connman.CopyNodeVector(CConnman::FullyConnectedOnly);

    {
        LOCK(cs);
        CollectSigSharesToRequest(sigSharesToRequest);
        CollectSigSharesToSend(sigShareBatchesToSend);
        CollectSigSharesToAnnounce(sigSharesToAnnounce);
        CollectSigSharesToSendConcentrated(sigSharesToSend, vNodesCopy);

        for (auto& [nodeId, sigShareMap] : sigSharesToRequest) {
            for (auto& [hash, sigShareInv] : sigShareMap) {
                sigShareInv.sessionId = addSigSesAnnIfNeeded(nodeId, hash);
            }
        }
        for (auto& [nodeId, sigShareBatchesMap] : sigShareBatchesToSend) {
            for (auto& [hash, sigShareBatch] : sigShareBatchesMap) {
                sigShareBatch.sessionId = addSigSesAnnIfNeeded(nodeId, hash);
            }
        }
        for (auto& [nodeId, sigShareMap] : sigSharesToAnnounce) {
            for (auto& [hash, sigShareInv] : sigShareMap) {
                sigShareInv.sessionId = addSigSesAnnIfNeeded(nodeId, hash);
            }
        }
    }

    bool didSend = false;

    for (auto& pnode : vNodesCopy) {
        CNetMsgMaker msgMaker(pnode->GetCommonVersion());

        if (const auto it1 = sigSessionAnnouncements.find(pnode->GetId()); it1 != sigSessionAnnouncements.end()) {
            std::vector<CSigSesAnn> msgs;
            msgs.reserve(it1->second.size());
            for (auto& sigSesAnn : it1->second) {
                LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::SendMessages -- QSIGSESANN signHash=%s, sessionId=%d, node=%d\n",
                         sigSesAnn.buildSignHash().ToString(), sigSesAnn.getSessionId(), pnode->GetId());
                msgs.emplace_back(sigSesAnn);
                if (msgs.size() == MAX_MSGS_CNT_QSIGSESANN) {
                    connman.PushMessage(pnode, msgMaker.Make(NetMsgType::QSIGSESANN, msgs));
                    msgs.clear();
                    didSend = true;
                }
            }
            if (!msgs.empty()) {
                connman.PushMessage(pnode, msgMaker.Make(NetMsgType::QSIGSESANN, msgs));
                didSend = true;
            }
        }

        if (const auto it = sigSharesToRequest.find(pnode->GetId()); it != sigSharesToRequest.end()) {
            std::vector<CSigSharesInv> msgs;
            for (const auto& [signHash, inv] : it->second) {
                assert(inv.CountSet() != 0);
                LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::SendMessages -- QGETSIGSHARES signHash=%s, inv={%s}, node=%d\n",
                         signHash.ToString(), inv.ToString(), pnode->GetId());
                msgs.emplace_back(inv);
                if (msgs.size() == MAX_MSGS_CNT_QGETSIGSHARES) {
                    connman.PushMessage(pnode, msgMaker.Make(NetMsgType::QGETSIGSHARES, msgs));
                    msgs.clear();
                    didSend = true;
                }
            }
            if (!msgs.empty()) {
                connman.PushMessage(pnode, msgMaker.Make(NetMsgType::QGETSIGSHARES, msgs));
                didSend = true;
            }
        }

        if (const auto jt = sigShareBatchesToSend.find(pnode->GetId()); jt != sigShareBatchesToSend.end()) {
            size_t totalSigsCount = 0;
            std::vector<CBatchedSigShares> msgs;
            for (const auto& [signHash, inv] : jt->second) {
                assert(!inv.sigShares.empty());
                LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::SendMessages -- QBSIGSHARES signHash=%s, inv={%s}, node=%d\n",
                         signHash.ToString(), inv.ToInvString(), pnode->GetId());
                if (totalSigsCount + inv.sigShares.size() > MAX_MSGS_TOTAL_BATCHED_SIGS) {
                    connman.PushMessage(pnode, msgMaker.Make(NetMsgType::QBSIGSHARES, msgs));
                    msgs.clear();
                    totalSigsCount = 0;
                    didSend = true;
                }
                totalSigsCount += inv.sigShares.size();
                msgs.emplace_back(inv);
            }
            if (!msgs.empty()) {
                connman.PushMessage(pnode, msgMaker.Make(NetMsgType::QBSIGSHARES, std::move(msgs)));
                didSend = true;
            }
        }

        if (const auto kt = sigSharesToAnnounce.find(pnode->GetId()); kt != sigSharesToAnnounce.end()) {
            std::vector<CSigSharesInv> msgs;
            for (const auto& [signHash, inv] : kt->second) {
                assert(inv.CountSet() != 0);
                LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::SendMessages -- QSIGSHARESINV signHash=%s, inv={%s}, node=%d\n",
                         signHash.ToString(), inv.ToString(), pnode->GetId());
                msgs.emplace_back(inv);
                if (msgs.size() == MAX_MSGS_CNT_QSIGSHARESINV) {
                    connman.PushMessage(pnode, msgMaker.Make(NetMsgType::QSIGSHARESINV, msgs));
                    msgs.clear();
                    didSend = true;
                }
            }
            if (!msgs.empty()) {
                connman.PushMessage(pnode, msgMaker.Make(NetMsgType::QSIGSHARESINV, msgs));
                didSend = true;
            }
        }

        auto lt = sigSharesToSend.find(pnode->GetId());
        if (lt != sigSharesToSend.end()) {
            std::vector<CSigShare> msgs;
            for (auto& sigShare : lt->second) {
                LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::SendMessages -- QSIGSHARE signHash=%s, node=%d\n",
                         sigShare.GetSignHash().ToString(), pnode->GetId());
                msgs.emplace_back(std::move(sigShare));
                if (msgs.size() == MAX_MSGS_SIG_SHARES) {
                    connman.PushMessage(pnode, msgMaker.Make(NetMsgType::QSIGSHARE, msgs));
                    msgs.clear();
                    didSend = true;
                }
            }
            if (!msgs.empty()) {
                connman.PushMessage(pnode, msgMaker.Make(NetMsgType::QSIGSHARE, msgs));
                didSend = true;
            }
        }
    }

    // looped through all nodes, release them
    connman.ReleaseNodeVector(vNodesCopy);

    return didSend;
}

bool CSigSharesManager::GetSessionInfoByRecvId(NodeId nodeId, uint32_t sessionId, CSigSharesNodeState::SessionInfo& retInfo)
{
    LOCK(cs);
    return nodeStates[nodeId].GetSessionInfoByRecvId(sessionId, retInfo);
}

CSigShare CSigSharesManager::RebuildSigShare(const CSigSharesNodeState::SessionInfo& session, const std::pair<uint16_t, CBLSLazySignature>& in)
{
    const auto& [member, sig] = in;
    CSigShare sigShare(session.llmqType, session.quorumHash, session.id, session.msgHash, member, sig);
    sigShare.UpdateKey();
    return sigShare;
}

void CSigSharesManager::Cleanup()
{
    int64_t now = GetTime<std::chrono::seconds>().count();
    if (now - lastCleanupTime < 5) {
        return;
    }

    // This map is first filled with all quorums found in all sig shares. Then we remove all inactive quorums and
    // loop through all sig shares again to find the ones belonging to the inactive quorums. We then delete the
    // sessions belonging to the sig shares. At the same time, we use this map as a cache when we later need to resolve
    // quorumHash -> quorumPtr (as GetQuorum() requires cs_main, leading to deadlocks with cs held)
    std::unordered_map<std::pair<Consensus::LLMQType, uint256>, CQuorumCPtr, StaticSaltedHasher> quorums;

    {
        LOCK(cs);
        sigShares.ForEach([&quorums](const SigShareKey&, const CSigShare& sigShare) {
            quorums.try_emplace(std::make_pair(sigShare.getLlmqType(), sigShare.getQuorumHash()), nullptr);
        });
    }

    // Find quorums which became inactive
    for (auto it = quorums.begin(); it != quorums.end(); ) {
        if (IsQuorumActive(it->first.first, qman, it->first.second)) {
            it->second = qman.GetQuorum(it->first.first, it->first.second);
            ++it;
        } else {
            it = quorums.erase(it);
        }
    }

    {
        // Now delete sessions which are for inactive quorums
        LOCK(cs);
        std::unordered_set<uint256, StaticSaltedHasher> inactiveQuorumSessions;
        sigShares.ForEach([&quorums, &inactiveQuorumSessions](const SigShareKey&, const CSigShare& sigShare) {
            if (quorums.count(std::make_pair(sigShare.getLlmqType(), sigShare.getQuorumHash())) == 0) {
                inactiveQuorumSessions.emplace(sigShare.GetSignHash());
            }
        });
        for (const auto& signHash : inactiveQuorumSessions) {
            RemoveSigSharesForSession(signHash);
        }
    }

    {
        LOCK(cs);

        // Remove sessions which were successfully recovered
        std::unordered_set<uint256, StaticSaltedHasher> doneSessions;
        sigShares.ForEach([&doneSessions, this](const SigShareKey&, const CSigShare& sigShare) {
            if (doneSessions.count(sigShare.GetSignHash()) != 0) {
                return;
            }
            if (sigman.HasRecoveredSigForSession(sigShare.GetSignHash())) {
                doneSessions.emplace(sigShare.GetSignHash());
            }
        });
        for (const auto& signHash : doneSessions) {
            RemoveSigSharesForSession(signHash);
        }

        // Remove sessions which timed out
        std::unordered_set<uint256, StaticSaltedHasher> timeoutSessions;
        for (const auto& [signHash, lastSeenTime] : timeSeenForSessions) {
            if (now - lastSeenTime >= SESSION_NEW_SHARES_TIMEOUT) {
                timeoutSessions.emplace(signHash);
            }
        }
        for (const auto& signHash : timeoutSessions) {

            if (const size_t count = sigShares.CountForSignHash(signHash); count > 0) {
                const auto* m = sigShares.GetAllForSignHash(signHash);
                assert(m);

                const auto& oneSigShare = m->begin()->second;

                std::string strMissingMembers;
                if (LogAcceptCategory(BCLog::LLMQ_SIGS)) {
                    if (const auto quorumIt = quorums.find(std::make_pair(oneSigShare.getLlmqType(), oneSigShare.getQuorumHash())); quorumIt != quorums.end()) {
                        const auto& quorum = quorumIt->second;
                        for (const auto i : irange::range(quorum->members.size())) {
                            if (m->count((uint16_t)i) == 0) {
                                const auto& dmn = quorum->members[i];
                                strMissingMembers += strprintf("\n  %s", dmn->proTxHash.ToString());
                            }
                        }
                    }
                }

                LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- signing session timed out. signHash=%s, id=%s, msgHash=%s, sigShareCount=%d, missingMembers=%s\n", __func__,
                          signHash.ToString(), oneSigShare.getId().ToString(), oneSigShare.getMsgHash().ToString(), count, strMissingMembers);
            } else {
                LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- signing session timed out. signHash=%s, sigShareCount=%d\n", __func__,
                          signHash.ToString(), count);
            }
            RemoveSigSharesForSession(signHash);
        }
    }

    // Find node states for peers that disappeared from CConnman
    std::unordered_set<NodeId> nodeStatesToDelete;
    {
        LOCK(cs);
        for (const auto& [nodeId, _] : nodeStates) {
            nodeStatesToDelete.emplace(nodeId);
        }
    }
    connman.ForEachNode([&nodeStatesToDelete](const CNode* pnode) {
        nodeStatesToDelete.erase(pnode->GetId());
    });

    // Now delete these node states
    LOCK(cs);
    for (const auto& nodeId : nodeStatesToDelete) {
        auto it = nodeStates.find(nodeId);
        if (it == nodeStates.end()) {
            continue;
        }
        // remove global requested state to force a re-request from another node
        it->second.requestedSigShares.ForEach([this](const SigShareKey& k, bool) {
            AssertLockHeld(cs);
            sigSharesRequested.Erase(k);
        });
        nodeStates.erase(nodeId);
    }

    lastCleanupTime = GetTime<std::chrono::seconds>().count();
}

void CSigSharesManager::RemoveSigSharesForSession(const uint256& signHash)
{
    AssertLockHeld(cs);

    for (auto& [_, nodeState] : nodeStates) {
        nodeState.RemoveSession(signHash);
    }

    sigSharesRequested.EraseAllForSignHash(signHash);
    sigSharesQueuedToAnnounce.EraseAllForSignHash(signHash);
    sigShares.EraseAllForSignHash(signHash);
    signedSessions.erase(signHash);
    timeSeenForSessions.erase(signHash);
}

void CSigSharesManager::RemoveBannedNodeStates()
{
    // Called regularly to cleanup local node states for banned nodes

    LOCK(cs);
    for (auto it = nodeStates.begin(); it != nodeStates.end();) {
        if (m_peerman->IsBanned(it->first)) {
            // re-request sigshares from other nodes
            it->second.requestedSigShares.ForEach([this](const SigShareKey& k, int64_t) {
                AssertLockHeld(cs);
                sigSharesRequested.Erase(k);
            });
            it = nodeStates.erase(it);
        } else {
            ++it;
        }
    }
}

void CSigSharesManager::BanNode(NodeId nodeId)
{
    if (nodeId == -1) {
        return;
    }

    {
        m_peerman->Misbehaving(nodeId, 100);
    }

    LOCK(cs);
    auto it = nodeStates.find(nodeId);
    if (it == nodeStates.end()) {
        return;
    }
    auto& nodeState = it->second;

    // Whatever we requested from him, let's request it from someone else now
    nodeState.requestedSigShares.ForEach([this](const SigShareKey& k, int64_t) {
        AssertLockHeld(cs);
        sigSharesRequested.Erase(k);
    });
    nodeState.requestedSigShares.Clear();

    nodeState.banned = true;
}

void CSigSharesManager::WorkThreadMain()
{
    int64_t lastSendTime = 0;

    while (!workInterrupt) {
        RemoveBannedNodeStates();

        bool fMoreWork = ProcessPendingSigShares(connman);
        SignPendingSigShares();

        if (GetTimeMillis() - lastSendTime > 100) {
            SendMessages();
            lastSendTime = GetTimeMillis();
        }

        Cleanup();

        // TODO Wakeup when pending signing is needed?
        if (!fMoreWork && !workInterrupt.sleep_for(std::chrono::milliseconds(100))) {
            return;
        }
    }
}

void CSigSharesManager::AsyncSign(const CQuorumCPtr& quorum, const uint256& id, const uint256& msgHash)
{
    LOCK(cs);
    pendingSigns.emplace_back(quorum, id, msgHash);
}

void CSigSharesManager::SignPendingSigShares()
{
    std::vector<PendingSignatureData> v;
    {
        LOCK(cs);
        v = std::move(pendingSigns);
    }

    for (const auto& [pQuorum, id, msgHash] : v) {
        auto opt_sigShare = CreateSigShare(pQuorum, id, msgHash);

        if (opt_sigShare.has_value() && opt_sigShare->sigShare.Get().IsValid()) {
            auto sigShare = *opt_sigShare;
            ProcessSigShare(sigShare, connman, pQuorum);

            if (IsAllMembersConnectedEnabled(pQuorum->params.type)) {
                LOCK(cs);
                auto& session = signedSessions[sigShare.GetSignHash()];
                session.sigShare = sigShare;
                session.quorum = pQuorum;
                session.nextAttemptTime = 0;
                session.attempt = 0;
            }
        }
    }
}

std::optional<CSigShare> CSigSharesManager::CreateSigShare(const CQuorumCPtr& quorum, const uint256& id, const uint256& msgHash) const
{
    cxxtimer::Timer t(true);
    auto activeMasterNodeProTxHash = WITH_LOCK(activeMasternodeInfoCs, return activeMasternodeInfo.proTxHash);

    if (!quorum->IsValidMember(activeMasterNodeProTxHash)) {
        return std::nullopt;
    }

    const CBLSSecretKey& skShare = quorum->GetSkShare();
    if (!skShare.IsValid()) {
        LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- we don't have our skShare for quorum %s\n", __func__, quorum->qc->quorumHash.ToString());
        return std::nullopt;
    }

    int memberIdx = quorum->GetMemberIndex(activeMasterNodeProTxHash);
    if (memberIdx == -1) {
        // this should really not happen (IsValidMember gave true)
        return std::nullopt;
    }

    CSigShare sigShare(quorum->params.type, quorum->qc->quorumHash, id, msgHash, uint16_t(memberIdx), {});
    uint256 signHash = sigShare.buildSignHash();

    sigShare.sigShare.Set(skShare.Sign(signHash), bls::bls_legacy_scheme.load());
    if (!sigShare.sigShare.Get().IsValid()) {
        LogPrintf("CSigSharesManager::%s -- failed to sign sigShare. signHash=%s, id=%s, msgHash=%s, time=%s\n", __func__,
                  signHash.ToString(), sigShare.getId().ToString(), sigShare.getMsgHash().ToString(), t.count());
        return std::nullopt;
    }

    sigShare.UpdateKey();

    LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- created sigShare. signHash=%s, id=%s, msgHash=%s, llmqType=%d, quorum=%s, time=%s\n", __func__,
              signHash.ToString(), sigShare.getId().ToString(), sigShare.getMsgHash().ToString(), ToUnderlying(quorum->params.type), quorum->qc->quorumHash.ToString(), t.count());

    return sigShare;
}

// causes all known sigShares to be re-announced
void CSigSharesManager::ForceReAnnouncement(const CQuorumCPtr& quorum, Consensus::LLMQType llmqType, const uint256& id, const uint256& msgHash)
{
    if (IsAllMembersConnectedEnabled(llmqType)) {
        return;
    }

    LOCK(cs);
    auto signHash = BuildSignHash(llmqType, quorum->qc->quorumHash, id, msgHash);
    if (const auto *const sigs = sigShares.GetAllForSignHash(signHash)) {
        for (const auto& [quorumMemberIndex, _] : *sigs) {
            // re-announce every sigshare to every node
            sigSharesQueuedToAnnounce.Add(std::make_pair(signHash, quorumMemberIndex), true);
        }
    }
    for (auto& [_, nodeState] : nodeStates) {
        auto* session = nodeState.GetSessionBySignHash(signHash);
        if (session == nullptr) {
            continue;
        }
        // pretend that the other node doesn't know about any shares so that we re-announce everything
        session->knows.SetAll(false);
        // we need to use a new session id as we don't know if the other node has run into a timeout already
        session->sendSessionId = UNINITIALIZED_SESSION_ID;
    }
}

void CSigSharesManager::HandleNewRecoveredSig(const llmq::CRecoveredSig& recoveredSig)
{
    LOCK(cs);
    RemoveSigSharesForSession(recoveredSig.buildSignHash());
}

} // namespace llmq
