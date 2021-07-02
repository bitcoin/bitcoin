// Copyright (c) 2018-2021 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/quorums.h>
#include <llmq/quorums_commitment.h>
#include <llmq/quorums_signing.h>
#include <llmq/quorums_signing_shares.h>
#include <llmq/quorums_utils.h>


#include <evo/deterministicmns.h>
#include <masternode/activemasternode.h>
#include <bls/bls_batchverifier.h>
#include <net_processing.h>
#include <netmessagemaker.h>
#include <spork.h>
#include <validation.h>

#include <cxxtimer.hpp>

namespace llmq
{

CSigSharesManager* quorumSigSharesManager = nullptr;

void CSigShare::UpdateKey()
{
    key.first = CLLMQUtils::BuildSignHash(*this);
    key.second = quorumMember;
}

std::string CSigSesAnn::ToString() const
{
    return strprintf("sessionId=%d, llmqType=%d, quorumHash=%s, id=%s, msgHash=%s",
                     sessionId, llmqType, quorumHash.ToString(), id.ToString(), msgHash.ToString());
}

void CSigSharesInv::Merge(const CSigSharesInv& inv2)
{
    for (size_t i = 0; i < inv.size(); i++) {
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
    for (size_t i = 0; i < inv.size(); i++) {
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

bool CSigSharesInv::IsSet(uint16_t quorumMember) const
{
    assert(quorumMember < inv.size());
    return inv[quorumMember];
}

void CSigSharesInv::Set(uint16_t quorumMember, bool v)
{
    assert(quorumMember < inv.size());
    inv[quorumMember] = v;
}

void CSigSharesInv::SetAll(bool v)
{
    for (size_t i = 0; i < inv.size(); i++) {
        inv[i] = v;
    }
}

std::string CBatchedSigShares::ToInvString() const
{
    CSigSharesInv inv;
    // we use 400 here no matter what the real size is. We don't really care about that size as we just want to call ToString()
    inv.Init(400);
    for (size_t i = 0; i < sigShares.size(); i++) {
        inv.inv[sigShares[i].first] = true;
    }
    return inv.ToString();
}

template<typename T>
static void InitSession(CSigSharesNodeState::Session& s, const uint256& signHash, T& from)
{
    const auto& params = Params().GetConsensus().llmqs.at((Consensus::LLMQType)from.llmqType);

    s.llmqType = (Consensus::LLMQType)from.llmqType;
    s.quorumHash = from.quorumHash;
    s.id = from.id;
    s.msgHash = from.msgHash;
    s.signHash = signHash;
    s.announced.Init((size_t)params.size);
    s.requested.Init((size_t)params.size);
    s.knows.Init((size_t)params.size);
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
    auto signHash = CLLMQUtils::BuildSignHash(ann.llmqType, ann.quorumHash, ann.id, ann.msgHash);
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
    auto s = GetSessionByRecvId(sessionId);
    if (!s) {
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
    auto it = sessions.find(signHash);
    if (it != sessions.end()) {
        sessionByRecvId.erase(it->second.recvSessionId);
        sessions.erase(it);
    }
    requestedSigShares.EraseAllForSignHash(signHash);
    pendingIncomingSigShares.EraseAllForSignHash(signHash);
}

//////////////////////

CSigSharesManager::CSigSharesManager()
{
    workInterrupt.reset();
}

CSigSharesManager::~CSigSharesManager() = default;

void CSigSharesManager::StartWorkerThread()
{
    // can't start new thread if we have one running already
    if (workThread.joinable()) {
        assert(false);
    }

    workThread = std::thread(&TraceThread<std::function<void()> >,
        "sigshares",
        std::function<void()>(std::bind(&CSigSharesManager::WorkThreadMain, this)));
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
    quorumSigningManager->RegisterRecoveredSigsListener(this);
}

void CSigSharesManager::UnregisterAsRecoveredSigsListener()
{
    quorumSigningManager->UnregisterRecoveredSigsListener(this);
}

void CSigSharesManager::InterruptWorkerThread()
{
    workInterrupt();
}

void CSigSharesManager::ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv)
{
    // non-masternodes are not interested in sigshares
    if (!fMasternodeMode || activeMasternodeInfo.proTxHash.IsNull()) {
        return;
    }

    if (sporkManager.IsSporkActive(SPORK_21_QUORUM_ALL_CONNECTED) && strCommand == NetMsgType::QSIGSHARE) {
        std::vector<CSigShare> sigShares;
        vRecv >> sigShares;

        if (sigShares.size() > MAX_MSGS_SIG_SHARES) {
            LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- too many sigs in QSIGSHARE message. cnt=%d, max=%d, node=%d\n", __func__, sigShares.size(), MAX_MSGS_SIG_SHARES, pfrom->GetId());
            BanNode(pfrom->GetId());
            return;
        }

        for (const auto& sigShare : sigShares) {
            ProcessMessageSigShare(pfrom->GetId(), sigShare);
        }
    }

    if (strCommand == NetMsgType::QSIGSESANN) {
        std::vector<CSigSesAnn> msgs;
        vRecv >> msgs;
        if (msgs.size() > MAX_MSGS_CNT_QSIGSESANN) {
            LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- too many announcements in QSIGSESANN message. cnt=%d, max=%d, node=%d\n", __func__, msgs.size(), MAX_MSGS_CNT_QSIGSESANN, pfrom->GetId());
            BanNode(pfrom->GetId());
            return;
        }
        for (const auto& ann : msgs) {
            if (!ProcessMessageSigSesAnn(pfrom, ann)) {
                BanNode(pfrom->GetId());
                return;
            }
        }
    } else if (strCommand == NetMsgType::QSIGSHARESINV) {
        std::vector<CSigSharesInv> msgs;
        vRecv >> msgs;
        if (msgs.size() > MAX_MSGS_CNT_QSIGSHARESINV) {
            LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- too many invs in QSIGSHARESINV message. cnt=%d, max=%d, node=%d\n", __func__, msgs.size(), MAX_MSGS_CNT_QSIGSHARESINV, pfrom->GetId());
            BanNode(pfrom->GetId());
            return;
        }
        for (const auto& inv : msgs) {
            if (!ProcessMessageSigSharesInv(pfrom, inv)) {
                BanNode(pfrom->GetId());
                return;
            }
        }
    } else if (strCommand == NetMsgType::QGETSIGSHARES) {
        std::vector<CSigSharesInv> msgs;
        vRecv >> msgs;
        if (msgs.size() > MAX_MSGS_CNT_QGETSIGSHARES) {
            LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- too many invs in QGETSIGSHARES message. cnt=%d, max=%d, node=%d\n", __func__, msgs.size(), MAX_MSGS_CNT_QGETSIGSHARES, pfrom->GetId());
            BanNode(pfrom->GetId());
            return;
        }
        for (const auto& inv : msgs) {
            if (!ProcessMessageGetSigShares(pfrom, inv)) {
                BanNode(pfrom->GetId());
                return;
            }
        }
    } else if (strCommand == NetMsgType::QBSIGSHARES) {
        std::vector<CBatchedSigShares> msgs;
        vRecv >> msgs;
        size_t totalSigsCount = 0;
        for (const auto& bs : msgs) {
            totalSigsCount += bs.sigShares.size();
        }
        if (totalSigsCount > MAX_MSGS_TOTAL_BATCHED_SIGS) {
            LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- too many sigs in QBSIGSHARES message. cnt=%d, max=%d, node=%d\n", __func__, msgs.size(), MAX_MSGS_TOTAL_BATCHED_SIGS, pfrom->GetId());
            BanNode(pfrom->GetId());
            return;
        }
        for (const auto& bs : msgs) {
            if (!ProcessMessageBatchedSigShares(pfrom, bs)) {
                BanNode(pfrom->GetId());
                return;
            }
        }
    }
}

bool CSigSharesManager::ProcessMessageSigSesAnn(CNode* pfrom, const CSigSesAnn& ann)
{
    auto llmqType = ann.llmqType;
    if (!Params().GetConsensus().llmqs.count(llmqType)) {
        return false;
    }
    if (ann.sessionId == (uint32_t)-1 || ann.quorumHash.IsNull() || ann.id.IsNull() || ann.msgHash.IsNull()) {
        return false;
    }

    LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- ann={%s}, node=%d\n", __func__, ann.ToString(), pfrom->GetId());

    auto quorum = quorumManager->GetQuorum(llmqType, ann.quorumHash);
    if (!quorum) {
        // TODO should we ban here?
        LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- quorum %s not found, node=%d\n", __func__,
                  ann.quorumHash.ToString(), pfrom->GetId());
        return true; // let's still try other announcements from the same message
    }

    LOCK(cs);
    auto& nodeState = nodeStates[pfrom->GetId()];
    auto& session = nodeState.GetOrCreateSessionFromAnn(ann);
    nodeState.sessionByRecvId.erase(session.recvSessionId);
    nodeState.sessionByRecvId.erase(ann.sessionId);
    session.recvSessionId = ann.sessionId;
    session.quorum = quorum;
    nodeState.sessionByRecvId.emplace(ann.sessionId, &session);

    return true;
}

bool CSigSharesManager::VerifySigSharesInv(Consensus::LLMQType llmqType, const CSigSharesInv& inv)
{
    size_t quorumSize = (size_t)Params().GetConsensus().llmqs.at(llmqType).size;

    if (inv.inv.size() != quorumSize) {
        return false;
    }
    return true;
}

bool CSigSharesManager::ProcessMessageSigSharesInv(CNode* pfrom, const CSigSharesInv& inv)
{
    CSigSharesNodeState::SessionInfo sessionInfo;
    if (!GetSessionInfoByRecvId(pfrom->GetId(), inv.sessionId, sessionInfo)) {
        return true;
    }

    if (!VerifySigSharesInv(sessionInfo.llmqType, inv)) {
        return false;
    }

    // TODO for PoSe, we should consider propagating shares even if we already have a recovered sig
    if (quorumSigningManager->HasRecoveredSigForSession(sessionInfo.signHash)) {
        return true;
    }

    LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- signHash=%s, inv={%s}, node=%d\n", __func__,
            sessionInfo.signHash.ToString(), inv.ToString(), pfrom->GetId());

    if (sessionInfo.quorum->quorumVvec == nullptr) {
        // TODO we should allow to ask other nodes for the quorum vvec if we missed it in the DKG
        LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- we don't have the quorum vvec for %s, not requesting sig shares. node=%d\n", __func__,
                  sessionInfo.quorumHash.ToString(), pfrom->GetId());
        return true;
    }

    LOCK(cs);
    auto& nodeState = nodeStates[pfrom->GetId()];
    auto session = nodeState.GetSessionByRecvId(inv.sessionId);
    if (!session) {
        return true;
    }
    session->announced.Merge(inv);
    session->knows.Merge(inv);
    return true;
}

bool CSigSharesManager::ProcessMessageGetSigShares(CNode* pfrom, const CSigSharesInv& inv)
{
    CSigSharesNodeState::SessionInfo sessionInfo;
    if (!GetSessionInfoByRecvId(pfrom->GetId(), inv.sessionId, sessionInfo)) {
        return true;
    }

    if (!VerifySigSharesInv(sessionInfo.llmqType, inv)) {
        return false;
    }

    // TODO for PoSe, we should consider propagating shares even if we already have a recovered sig
    if (quorumSigningManager->HasRecoveredSigForSession(sessionInfo.signHash)) {
        return true;
    }

    LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- signHash=%s, inv={%s}, node=%d\n", __func__,
            sessionInfo.signHash.ToString(), inv.ToString(), pfrom->GetId());

    LOCK(cs);
    auto& nodeState = nodeStates[pfrom->GetId()];
    auto session = nodeState.GetSessionByRecvId(inv.sessionId);
    if (!session) {
        return true;
    }
    session->requested.Merge(inv);
    session->knows.Merge(inv);
    return true;
}

bool CSigSharesManager::ProcessMessageBatchedSigShares(CNode* pfrom, const CBatchedSigShares& batchedSigShares)
{
    CSigSharesNodeState::SessionInfo sessionInfo;
    if (!GetSessionInfoByRecvId(pfrom->GetId(), batchedSigShares.sessionId, sessionInfo)) {
        return true;
    }

    bool ban = false;
    if (!PreVerifyBatchedSigShares(sessionInfo, batchedSigShares, ban)) {
        return !ban;
    }

    std::vector<CSigShare> sigShares;
    sigShares.reserve(batchedSigShares.sigShares.size());

    {
        LOCK(cs);
        auto& nodeState = nodeStates[pfrom->GetId()];

        for (size_t i = 0; i < batchedSigShares.sigShares.size(); i++) {
            CSigShare sigShare = RebuildSigShare(sessionInfo, batchedSigShares, i);
            nodeState.requestedSigShares.Erase(sigShare.GetKey());

            // TODO track invalid sig shares received for PoSe?
            // It's important to only skip seen *valid* sig shares here. If a node sends us a
            // batch of mostly valid sig shares with a single invalid one and thus batched
            // verification fails, we'd skip the valid ones in the future if received from other nodes
            if (this->sigShares.Has(sigShare.GetKey())) {
                continue;
            }

            // TODO for PoSe, we should consider propagating shares even if we already have a recovered sig
            if (quorumSigningManager->HasRecoveredSigForId(sigShare.llmqType, sigShare.id)) {
                continue;
            }

            sigShares.emplace_back(sigShare);
        }
    }

    LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- signHash=%s, shares=%d, new=%d, inv={%s}, node=%d\n", __func__,
             sessionInfo.signHash.ToString(), batchedSigShares.sigShares.size(), sigShares.size(), batchedSigShares.ToInvString(), pfrom->GetId());

    if (sigShares.empty()) {
        return true;
    }

    LOCK(cs);
    auto& nodeState = nodeStates[pfrom->GetId()];
    for (const auto& s : sigShares) {
        nodeState.pendingIncomingSigShares.Add(s.GetKey(), s);
    }
    return true;
}

void CSigSharesManager::ProcessMessageSigShare(NodeId fromId, const CSigShare& sigShare)
{
    auto quorum = quorumManager->GetQuorum(sigShare.llmqType, sigShare.quorumHash);
    if (!quorum) {
        return;
    }
    if (!CLLMQUtils::IsQuorumActive(sigShare.llmqType, quorum->qc->quorumHash)) {
        // quorum is too old
        return;
    }
    if (!quorum->IsMember(activeMasternodeInfo.proTxHash)) {
        // we're not a member so we can't verify it (we actually shouldn't have received it)
        return;
    }
    if (quorum->quorumVvec == nullptr) {
        // TODO we should allow to ask other nodes for the quorum vvec if we missed it in the DKG
        LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- we don't have the quorum vvec for %s, no verification possible. node=%d\n", __func__,
                 quorum->qc->quorumHash.ToString(), fromId);
        return;
    }

    if (sigShare.quorumMember >= quorum->members.size()) {
        LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- quorumMember out of bounds\n", __func__);
        BanNode(fromId);
        return;
    }
    if (!quorum->qc->validMembers[sigShare.quorumMember]) {
        LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- quorumMember not valid\n", __func__);
        BanNode(fromId);
        return;
    }

    {
        LOCK(cs);

        if (sigShares.Has(sigShare.GetKey())) {
            return;
        }

        if (quorumSigningManager->HasRecoveredSigForId(sigShare.llmqType, sigShare.id)) {
            return;
        }

        auto& nodeState = nodeStates[fromId];
        nodeState.pendingIncomingSigShares.Add(sigShare.GetKey(), sigShare);
    }

    LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- signHash=%s, id=%s, msgHash=%s, member=%d, node=%d\n", __func__,
             sigShare.GetSignHash().ToString(), sigShare.id.ToString(), sigShare.msgHash.ToString(), sigShare.quorumMember, fromId);
}

bool CSigSharesManager::PreVerifyBatchedSigShares(const CSigSharesNodeState::SessionInfo& session, const CBatchedSigShares& batchedSigShares, bool& retBan)
{
    retBan = false;

    if (!CLLMQUtils::IsQuorumActive(session.llmqType, session.quorum->qc->quorumHash)) {
        // quorum is too old
        return false;
    }
    if (!session.quorum->IsMember(activeMasternodeInfo.proTxHash)) {
        // we're not a member so we can't verify it (we actually shouldn't have received it)
        return false;
    }
    if (session.quorum->quorumVvec == nullptr) {
        // TODO we should allow to ask other nodes for the quorum vvec if we missed it in the DKG
        LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- we don't have the quorum vvec for %s, no verification possible.\n", __func__,
                  session.quorumHash.ToString());
        return false;
    }

    std::unordered_set<uint16_t> dupMembers;

    for (const auto& sigShare : batchedSigShares.sigShares) {
        auto quorumMember = sigShare.first;
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
        CLLMQUtils::IterateNodesRandom(nodeStates, [&]() {
            return uniqueSignHashes.size() < maxUniqueSessions;
        }, [&](NodeId nodeId, CSigSharesNodeState& ns) {
            if (ns.pendingIncomingSigShares.Empty()) {
                return false;
            }
            auto& sigShare = *ns.pendingIncomingSigShares.GetFirst();

            bool alreadyHave = this->sigShares.Has(sigShare.GetKey());
            if (!alreadyHave) {
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

    {
        LOCK(cs_main);

        // For the convenience of the caller, also build a map of quorumHash -> quorum

        for (auto& p : retSigShares) {
            for (auto& sigShare : p.second) {
                auto llmqType = sigShare.llmqType;

                auto k = std::make_pair(llmqType, sigShare.quorumHash);
                if (retQuorums.count(k)) {
                    continue;
                }

                CQuorumCPtr quorum = quorumManager->GetQuorum(llmqType, sigShare.quorumHash);
                assert(quorum != nullptr);
                retQuorums.emplace(k, quorum);
            }
        }
    }
}

bool CSigSharesManager::ProcessPendingSigShares(CConnman& connman)
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
    for (auto& p : sigSharesByNodes) {
        auto nodeId = p.first;
        auto& v = p.second;

        for (auto& sigShare : v) {
            if (quorumSigningManager->HasRecoveredSigForId(sigShare.llmqType, sigShare.id)) {
                continue;
            }

            // we didn't check this earlier because we use a lazy BLS signature and tried to avoid doing the expensive
            // deserialization in the message thread
            if (!sigShare.sigShare.Get().IsValid()) {
                BanNode(nodeId);
                // don't process any additional shares from this node
                break;
            }

            auto quorum = quorums.at(std::make_pair(sigShare.llmqType, sigShare.quorumHash));
            auto pubKeyShare = quorum->GetPubKeyShare(sigShare.quorumMember);

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

    for (const auto& p : sigSharesByNodes) {
        auto nodeId = p.first;
        const auto& v = p.second;

        if (batchVerifier.badSources.count(nodeId)) {
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
void CSigSharesManager::ProcessPendingSigShares(const std::vector<CSigShare>& sigShares,
        const std::unordered_map<std::pair<Consensus::LLMQType, uint256>, CQuorumCPtr, StaticSaltedHasher>& quorums,
        CConnman& connman)
{
    cxxtimer::Timer t(true);
    for (auto& sigShare : sigShares) {
        auto quorumKey = std::make_pair(sigShare.llmqType, sigShare.quorumHash);
        ProcessSigShare(sigShare, connman, quorums.at(quorumKey));
    }
    t.stop();

    LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- processed sigShare batch. shares=%d, time=%ds\n", __func__,
             sigShares.size(), t.count());
}

// sig shares are already verified when entering this method
void CSigSharesManager::ProcessSigShare(const CSigShare& sigShare, CConnman& connman, const CQuorumCPtr& quorum)
{
    auto llmqType = quorum->params.type;

    bool canTryRecovery = false;

    // prepare node set for direct-push in case this is our sig share
    std::set<NodeId> quorumNodes;
    if (!CLLMQUtils::IsAllMembersConnectedEnabled(llmqType) && sigShare.quorumMember == quorum->GetMemberIndex(activeMasternodeInfo.proTxHash)) {
        quorumNodes = connman.GetMasternodeQuorumNodes(sigShare.llmqType, sigShare.quorumHash);
    }

    if (quorumSigningManager->HasRecoveredSigForId(llmqType, sigShare.id)) {
        return;
    }

    {
        LOCK(cs);

        if (!sigShares.Add(sigShare.GetKey(), sigShare)) {
            return;
        }
        if (!CLLMQUtils::IsAllMembersConnectedEnabled(llmqType)) {
            sigSharesQueuedToAnnounce.Add(sigShare.GetKey(), true);
        }

        // Update the time we've seen the last sigShare
        timeSeenForSessions[sigShare.GetSignHash()] = GetAdjustedTime();

        if (!quorumNodes.empty()) {
            // don't announce and wait for other nodes to request this share and directly send it to them
            // there is no way the other nodes know about this share as this is the one created on this node
            for (auto otherNodeId : quorumNodes) {
                auto& nodeState = nodeStates[otherNodeId];
                auto& session = nodeState.GetOrCreateSessionFromShare(sigShare);
                session.quorum = quorum;
                session.requested.Set(sigShare.quorumMember, true);
                session.knows.Set(sigShare.quorumMember, true);
            }
        }

        size_t sigShareCount = sigShares.CountForSignHash(sigShare.GetSignHash());
        if (sigShareCount >= quorum->params.threshold) {
            canTryRecovery = true;
        }
    }

    if (canTryRecovery) {
        TryRecoverSig(quorum, sigShare.id, sigShare.msgHash);
    }
}

void CSigSharesManager::TryRecoverSig(const CQuorumCPtr& quorum, const uint256& id, const uint256& msgHash)
{
    if (quorumSigningManager->HasRecoveredSigForId(quorum->params.type, id)) {
        return;
    }

    std::vector<CBLSSignature> sigSharesForRecovery;
    std::vector<CBLSId> idsForRecovery;
    {
        LOCK(cs);

        auto signHash = CLLMQUtils::BuildSignHash(quorum->params.type, quorum->qc->quorumHash, id, msgHash);
        auto sigShares = this->sigShares.GetAllForSignHash(signHash);
        if (!sigShares) {
            return;
        }

        sigSharesForRecovery.reserve((size_t) quorum->params.threshold);
        idsForRecovery.reserve((size_t) quorum->params.threshold);
        for (auto it = sigShares->begin(); it != sigShares->end() && sigSharesForRecovery.size() < quorum->params.threshold; ++it) {
            auto& sigShare = it->second;
            sigSharesForRecovery.emplace_back(sigShare.sigShare.Get());
            idsForRecovery.emplace_back(quorum->members[sigShare.quorumMember]->proTxHash);
        }

        // check if we can recover the final signature
        if (sigSharesForRecovery.size() < quorum->params.threshold) {
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

    auto rs = std::make_shared<CRecoveredSig>();
    rs->llmqType = quorum->params.type;
    rs->quorumHash = quorum->qc->quorumHash;
    rs->id = id;
    rs->msgHash = msgHash;
    rs->sig.Set(recoveredSig);
    rs->UpdateHash();

    // There should actually be no need to verify the self-recovered signatures as it should always succeed. Let's
    // however still verify it from time to time, so that we have a chance to catch bugs. We do only this sporadic
    // verification because this is unbatched and thus slow verification that happens here.
    if (((recoveredSigsCounter++) % 100) == 0) {
        auto signHash = CLLMQUtils::BuildSignHash(*rs);
        bool valid = recoveredSig.VerifyInsecure(quorum->qc->quorumPublicKey, signHash);
        if (!valid) {
            // this should really not happen as we have verified all signature shares before
            LogPrintf("CSigSharesManager::%s -- own recovered signature is invalid. id=%s, msgHash=%s\n", __func__,
                      id.ToString(), msgHash.ToString());
            return;
        }
    }

    quorumSigningManager->ProcessRecoveredSig(rs);
}

CDeterministicMNCPtr CSigSharesManager::SelectMemberForRecovery(const CQuorumCPtr& quorum, const uint256 &id, int attempt)
{
    assert(attempt < quorum->members.size());

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

    int64_t now = GetAdjustedTime();
    const size_t maxRequestsForNode = 32;

    // avoid requesting from same nodes all the time
    std::vector<NodeId> shuffledNodeIds;
    shuffledNodeIds.reserve(nodeStates.size());
    for (const auto& p : nodeStates) {
        if (p.second.sessions.empty()) {
            continue;
        }
        shuffledNodeIds.emplace_back(p.first);
    }
    Shuffle(shuffledNodeIds.begin(), shuffledNodeIds.end(), rnd);

    for (const auto& nodeId : shuffledNodeIds) {
        auto& nodeState = nodeStates[nodeId];

        if (nodeState.banned) {
            continue;
        }

        nodeState.requestedSigShares.EraseIf([&](const SigShareKey& k, int64_t t) {
            if (now - t >= SIG_SHARE_REQUEST_TIMEOUT) {
                // timeout while waiting for this one, so retry it with another node
                LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::CollectSigSharesToRequest -- timeout while waiting for %s-%d, node=%d\n",
                         k.first.ToString(), k.second, nodeId);
                return true;
            }
            return false;
        });

        decltype(sigSharesToRequest.begin()->second)* invMap = nullptr;

        for (auto& p2 : nodeState.sessions) {
            auto& signHash = p2.first;
            auto& session = p2.second;

            if (CLLMQUtils::IsAllMembersConnectedEnabled(session.llmqType)) {
                continue;
            }

            if (quorumSigningManager->HasRecoveredSigForSession(signHash)) {
                continue;
            }

            for (size_t i = 0; i < session.announced.inv.size(); i++) {
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
                auto p = sigSharesRequested.Get(k);
                if (p) {
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

                if (!invMap) {
                    invMap = &sigSharesToRequest[nodeId];
                }
                auto& inv = (*invMap)[signHash];
                if (inv.inv.empty()) {
                    const auto& params = Params().GetConsensus().llmqs.at(session.llmqType);
                    inv.Init((size_t)params.size);
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

    for (auto& p : nodeStates) {
        auto nodeId = p.first;
        auto& nodeState = p.second;

        if (nodeState.banned) {
            continue;
        }

        decltype(sigSharesToSend.begin()->second)* sigSharesToSend2 = nullptr;

        for (auto& p2 : nodeState.sessions) {
            auto& signHash = p2.first;
            auto& session = p2.second;

            if (CLLMQUtils::IsAllMembersConnectedEnabled(session.llmqType)) {
                continue;
            }

            if (quorumSigningManager->HasRecoveredSigForSession(signHash)) {
                continue;
            }

            CBatchedSigShares batchedSigShares;

            for (size_t i = 0; i < session.requested.inv.size(); i++) {
                if (!session.requested.inv[i]) {
                    continue;
                }
                session.requested.inv[i] = false;

                auto k = std::make_pair(signHash, (uint16_t)i);
                const CSigShare* sigShare = sigShares.Get(k);
                if (!sigShare) {
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
                (*sigSharesToSend2).emplace(signHash, std::move(batchedSigShares));
            }
        }
    }
}

void CSigSharesManager::CollectSigSharesToSendConcentrated(std::unordered_map<NodeId, std::vector<CSigShare>>& sigSharesToSend, const std::vector<CNode*>& vNodes)
{
    AssertLockHeld(cs);

    std::unordered_map<uint256, CNode*> proTxToNode;
    for (const auto& pnode : vNodes) {
        if (pnode->verifiedProRegTxHash.IsNull()) {
            continue;
        }
        proTxToNode.emplace(pnode->verifiedProRegTxHash, pnode);
    }

    auto curTime = GetTime<std::chrono::milliseconds>().count();

    for (auto& p : signedSessions) {
        if (!CLLMQUtils::IsAllMembersConnectedEnabled(p.second.quorum->params.type)) {
            continue;
        }

        if (p.second.attempt > p.second.quorum->params.recoveryMembers) {
            continue;
        }

        if (curTime >= p.second.nextAttemptTime) {
            int64_t waitTime = exp2(p.second.attempt) * EXP_SEND_FOR_RECOVERY_TIMEOUT;
            waitTime = std::min(MAX_SEND_FOR_RECOVERY_TIMEOUT, waitTime);
            p.second.nextAttemptTime = curTime + waitTime;
            auto dmn = SelectMemberForRecovery(p.second.quorum, p.second.sigShare.id, p.second.attempt);
            p.second.attempt++;

            LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- signHash=%s, sending to %s, attempt=%d\n", __func__,
                     p.second.sigShare.GetSignHash().ToString(), dmn->proTxHash.ToString(), p.second.attempt);

            auto it = proTxToNode.find(dmn->proTxHash);
            if (it == proTxToNode.end()) {
                continue;
            }

            auto& m = sigSharesToSend[it->second->GetId()];
            m.emplace_back(p.second.sigShare);
        }
    }
}

void CSigSharesManager::CollectSigSharesToAnnounce(std::unordered_map<NodeId, std::unordered_map<uint256, CSigSharesInv, StaticSaltedHasher>>& sigSharesToAnnounce)
{
    AssertLockHeld(cs);

    std::unordered_map<std::pair<Consensus::LLMQType, uint256>, std::unordered_set<NodeId>, StaticSaltedHasher> quorumNodesMap;

    sigSharesQueuedToAnnounce.ForEach([&](const SigShareKey& sigShareKey, bool) {
        auto& signHash = sigShareKey.first;
        auto quorumMember = sigShareKey.second;
        const CSigShare* sigShare = sigShares.Get(sigShareKey);
        if (!sigShare) {
            return;
        }

        // announce to the nodes which we know through the intra-quorum-communication system
        auto quorumKey = std::make_pair(sigShare->llmqType, sigShare->quorumHash);
        auto it = quorumNodesMap.find(quorumKey);
        if (it == quorumNodesMap.end()) {
            auto nodeIds = g_connman->GetMasternodeQuorumNodes(quorumKey.first, quorumKey.second);
            it = quorumNodesMap.emplace(std::piecewise_construct, std::forward_as_tuple(quorumKey), std::forward_as_tuple(nodeIds.begin(), nodeIds.end())).first;
        }

        const auto& quorumNodes = it->second;

        for (auto& nodeId : quorumNodes) {
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
                const auto& params = Params().GetConsensus().llmqs.at(sigShare->llmqType);
                inv.Init((size_t)params.size);
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
        auto session = nodeState.GetSessionBySignHash(signHash);
        assert(session);
        if (session->sendSessionId == (uint32_t)-1) {
            session->sendSessionId = nodeState.nextSendSessionId++;

            CSigSesAnn sigSesAnn;
            sigSesAnn.sessionId = session->sendSessionId;
            sigSesAnn.llmqType = session->llmqType;
            sigSesAnn.quorumHash = session->quorumHash;
            sigSesAnn.id = session->id;
            sigSesAnn.msgHash = session->msgHash;

            sigSessionAnnouncements[nodeId].emplace_back(sigSesAnn);
        }
        return session->sendSessionId;
    };

    std::vector<CNode*> vNodesCopy = g_connman->CopyNodeVector(CConnman::FullyConnectedOnly);

    {
        LOCK(cs);
        CollectSigSharesToRequest(sigSharesToRequest);
        CollectSigSharesToSend(sigShareBatchesToSend);
        CollectSigSharesToAnnounce(sigSharesToAnnounce);
        CollectSigSharesToSendConcentrated(sigSharesToSend, vNodesCopy);

        for (auto& p : sigSharesToRequest) {
            for (auto& p2 : p.second) {
                p2.second.sessionId = addSigSesAnnIfNeeded(p.first, p2.first);
            }
        }
        for (auto& p : sigShareBatchesToSend) {
            for (auto& p2 : p.second) {
                p2.second.sessionId = addSigSesAnnIfNeeded(p.first, p2.first);
            }
        }
        for (auto& p : sigSharesToAnnounce) {
            for (auto& p2 : p.second) {
                p2.second.sessionId = addSigSesAnnIfNeeded(p.first, p2.first);
            }
        }
    }

    bool didSend = false;

    for (auto& pnode : vNodesCopy) {
        CNetMsgMaker msgMaker(pnode->GetSendVersion());

        auto it1 = sigSessionAnnouncements.find(pnode->GetId());
        if (it1 != sigSessionAnnouncements.end()) {
            std::vector<CSigSesAnn> msgs;
            msgs.reserve(it1->second.size());
            for (auto& sigSesAnn : it1->second) {
                LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::SendMessages -- QSIGSESANN signHash=%s, sessionId=%d, node=%d\n",
                         CLLMQUtils::BuildSignHash(sigSesAnn).ToString(), sigSesAnn.sessionId, pnode->GetId());
                msgs.emplace_back(sigSesAnn);
                if (msgs.size() == MAX_MSGS_CNT_QSIGSESANN) {
                    g_connman->PushMessage(pnode, msgMaker.Make(NetMsgType::QSIGSESANN, msgs));
                    msgs.clear();
                    didSend = true;
                }
            }
            if (!msgs.empty()) {
                g_connman->PushMessage(pnode, msgMaker.Make(NetMsgType::QSIGSESANN, msgs));
                didSend = true;
            }
        }

        auto it = sigSharesToRequest.find(pnode->GetId());
        if (it != sigSharesToRequest.end()) {
            std::vector<CSigSharesInv> msgs;
            for (auto& p : it->second) {
                assert(p.second.CountSet() != 0);
                LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::SendMessages -- QGETSIGSHARES signHash=%s, inv={%s}, node=%d\n",
                         p.first.ToString(), p.second.ToString(), pnode->GetId());
                msgs.emplace_back(std::move(p.second));
                if (msgs.size() == MAX_MSGS_CNT_QGETSIGSHARES) {
                    g_connman->PushMessage(pnode, msgMaker.Make(NetMsgType::QGETSIGSHARES, msgs));
                    msgs.clear();
                    didSend = true;
                }
            }
            if (!msgs.empty()) {
                g_connman->PushMessage(pnode, msgMaker.Make(NetMsgType::QGETSIGSHARES, msgs));
                didSend = true;
            }
        }

        auto jt = sigShareBatchesToSend.find(pnode->GetId());
        if (jt != sigShareBatchesToSend.end()) {
            size_t totalSigsCount = 0;
            std::vector<CBatchedSigShares> msgs;
            for (auto& p : jt->second) {
                assert(!p.second.sigShares.empty());
                LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::SendMessages -- QBSIGSHARES signHash=%s, inv={%s}, node=%d\n",
                         p.first.ToString(), p.second.ToInvString(), pnode->GetId());
                if (totalSigsCount + p.second.sigShares.size() > MAX_MSGS_TOTAL_BATCHED_SIGS) {
                    g_connman->PushMessage(pnode, msgMaker.Make(NetMsgType::QBSIGSHARES, msgs));
                    msgs.clear();
                    totalSigsCount = 0;
                    didSend = true;
                }
                totalSigsCount += p.second.sigShares.size();
                msgs.emplace_back(std::move(p.second));

            }
            if (!msgs.empty()) {
                g_connman->PushMessage(pnode, msgMaker.Make(NetMsgType::QBSIGSHARES, std::move(msgs)));
                didSend = true;
            }
        }

        auto kt = sigSharesToAnnounce.find(pnode->GetId());
        if (kt != sigSharesToAnnounce.end()) {
            std::vector<CSigSharesInv> msgs;
            for (auto& p : kt->second) {
                assert(p.second.CountSet() != 0);
                LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::SendMessages -- QSIGSHARESINV signHash=%s, inv={%s}, node=%d\n",
                         p.first.ToString(), p.second.ToString(), pnode->GetId());
                msgs.emplace_back(std::move(p.second));
                if (msgs.size() == MAX_MSGS_CNT_QSIGSHARESINV) {
                    g_connman->PushMessage(pnode, msgMaker.Make(NetMsgType::QSIGSHARESINV, msgs));
                    msgs.clear();
                    didSend = true;
                }
            }
            if (!msgs.empty()) {
                g_connman->PushMessage(pnode, msgMaker.Make(NetMsgType::QSIGSHARESINV, msgs));
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
                    g_connman->PushMessage(pnode, msgMaker.Make(NetMsgType::QSIGSHARE, msgs));
                    msgs.clear();
                    didSend = true;
                }
            }
            if (!msgs.empty()) {
                g_connman->PushMessage(pnode, msgMaker.Make(NetMsgType::QSIGSHARE, msgs));
                didSend = true;
            }
        }
    }

    // looped through all nodes, release them
    g_connman->ReleaseNodeVector(vNodesCopy);

    return didSend;
}

bool CSigSharesManager::GetSessionInfoByRecvId(NodeId nodeId, uint32_t sessionId, CSigSharesNodeState::SessionInfo& retInfo)
{
    LOCK(cs);
    return nodeStates[nodeId].GetSessionInfoByRecvId(sessionId, retInfo);
}

CSigShare CSigSharesManager::RebuildSigShare(const CSigSharesNodeState::SessionInfo& session, const CBatchedSigShares& batchedSigShares, size_t idx)
{
    assert(idx < batchedSigShares.sigShares.size());
    auto& s = batchedSigShares.sigShares[idx];
    CSigShare sigShare;
    sigShare.llmqType = session.llmqType;
    sigShare.quorumHash = session.quorumHash;
    sigShare.quorumMember = s.first;
    sigShare.id = session.id;
    sigShare.msgHash = session.msgHash;
    sigShare.sigShare = s.second;
    sigShare.UpdateKey();
    return sigShare;
}

void CSigSharesManager::Cleanup()
{
    int64_t now = GetAdjustedTime();
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
        sigShares.ForEach([&](const SigShareKey& k, const CSigShare& sigShare) {
            quorums.emplace(std::make_pair(sigShare.llmqType, sigShare.quorumHash), nullptr);
        });
    }

    // Find quorums which became inactive
    for (auto it = quorums.begin(); it != quorums.end(); ) {
        if (CLLMQUtils::IsQuorumActive(it->first.first, it->first.second)) {
            it->second = quorumManager->GetQuorum(it->first.first, it->first.second);
            ++it;
        } else {
            it = quorums.erase(it);
        }
    }

    {
        // Now delete sessions which are for inactive quorums
        LOCK(cs);
        std::unordered_set<uint256, StaticSaltedHasher> inactiveQuorumSessions;
        sigShares.ForEach([&](const SigShareKey& k, const CSigShare& sigShare) {
            if (!quorums.count(std::make_pair(sigShare.llmqType, sigShare.quorumHash))) {
                inactiveQuorumSessions.emplace(sigShare.GetSignHash());
            }
        });
        for (auto& signHash : inactiveQuorumSessions) {
            RemoveSigSharesForSession(signHash);
        }
    }

    {
        LOCK(cs);

        // Remove sessions which were successfully recovered
        std::unordered_set<uint256, StaticSaltedHasher> doneSessions;
        sigShares.ForEach([&](const SigShareKey& k, const CSigShare& sigShare) {
            if (doneSessions.count(sigShare.GetSignHash())) {
                return;
            }
            if (quorumSigningManager->HasRecoveredSigForSession(sigShare.GetSignHash())) {
                doneSessions.emplace(sigShare.GetSignHash());
            }
        });
        for (auto& signHash : doneSessions) {
            RemoveSigSharesForSession(signHash);
        }

        // Remove sessions which timed out
        std::unordered_set<uint256, StaticSaltedHasher> timeoutSessions;
        for (const auto& p : timeSeenForSessions) {
            auto& signHash = p.first;
            int64_t lastSeenTime = p.second;

            if (now - lastSeenTime >= SESSION_NEW_SHARES_TIMEOUT) {
                timeoutSessions.emplace(signHash);
            }
        }
        for (auto& signHash : timeoutSessions) {
            size_t count = sigShares.CountForSignHash(signHash);

            if (count > 0) {
                auto m = sigShares.GetAllForSignHash(signHash);
                assert(m);

                auto& oneSigShare = m->begin()->second;

                std::string strMissingMembers;
                if (LogAcceptCategory(BCLog::LLMQ_SIGS)) {
                    auto quorumIt = quorums.find(std::make_pair(oneSigShare.llmqType, oneSigShare.quorumHash));
                    if (quorumIt != quorums.end()) {
                        const auto& quorum = quorumIt->second;
                        for (size_t i = 0; i < quorum->members.size(); i++) {
                            if (!m->count((uint16_t)i)) {
                                auto& dmn = quorum->members[i];
                                strMissingMembers += strprintf("\n  %s", dmn->proTxHash.ToString());
                            }
                        }
                    }
                }

                LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- signing session timed out. signHash=%s, id=%s, msgHash=%s, sigShareCount=%d, missingMembers=%s\n", __func__,
                          signHash.ToString(), oneSigShare.id.ToString(), oneSigShare.msgHash.ToString(), count, strMissingMembers);
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
        for (const auto& p : nodeStates) {
            nodeStatesToDelete.emplace(p.first);
        }
    }
    g_connman->ForEachNode([&](CNode* pnode) {
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
        it->second.requestedSigShares.ForEach([&](const SigShareKey& k, bool) {
            sigSharesRequested.Erase(k);
        });
        nodeStates.erase(nodeId);
    }

    lastCleanupTime = GetAdjustedTime();
}

void CSigSharesManager::RemoveSigSharesForSession(const uint256& signHash)
{
    AssertLockHeld(cs);

    for (auto& p : nodeStates) {
        auto& ns = p.second;
        ns.RemoveSession(signHash);
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

    LOCK2(cs_main, cs);
    std::unordered_set<NodeId> toRemove;
    for (auto it = nodeStates.begin(); it != nodeStates.end();) {
        if (IsBanned(it->first)) {
            // re-request sigshares from other nodes
            it->second.requestedSigShares.ForEach([&](const SigShareKey& k, int64_t) {
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
        LOCK(cs_main);
        Misbehaving(nodeId, 100);
    }

    LOCK(cs);
    auto it = nodeStates.find(nodeId);
    if (it == nodeStates.end()) {
        return;
    }
    auto& nodeState = it->second;

    // Whatever we requested from him, let's request it from someone else now
    nodeState.requestedSigShares.ForEach([&](const SigShareKey& k, int64_t) {
        sigSharesRequested.Erase(k);
    });
    nodeState.requestedSigShares.Clear();

    nodeState.banned = true;
}

void CSigSharesManager::WorkThreadMain()
{
    int64_t lastSendTime = 0;

    while (!workInterrupt) {
        if (!quorumSigningManager || !g_connman) {
            if (!workInterrupt.sleep_for(std::chrono::milliseconds(100))) {
                return;
            }
            continue;
        }

        bool fMoreWork{false};

        RemoveBannedNodeStates();
        fMoreWork |= quorumSigningManager->ProcessPendingRecoveredSigs();
        fMoreWork |= ProcessPendingSigShares(*g_connman);
        SignPendingSigShares();

        if (GetTimeMillis() - lastSendTime > 100) {
            SendMessages();
            lastSendTime = GetTimeMillis();
        }

        Cleanup();
        quorumSigningManager->Cleanup();

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
    std::vector<std::tuple<const CQuorumCPtr, uint256, uint256>> v;
    {
        LOCK(cs);
        v = std::move(pendingSigns);
    }

    for (auto& t : v) {
        const CQuorumCPtr pQuorum = std::get<0>(t);
        CSigShare sigShare = CreateSigShare(pQuorum, std::get<1>(t), std::get<2>(t));

        if (sigShare.sigShare.Get().IsValid()) {

            ProcessSigShare(sigShare, *g_connman, pQuorum);

            if (CLLMQUtils::IsAllMembersConnectedEnabled(pQuorum->params.type)) {
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

CSigShare CSigSharesManager::CreateSigShare(const CQuorumCPtr& quorum, const uint256& id, const uint256& msgHash) const
{
    cxxtimer::Timer t(true);

    if (!quorum->IsValidMember(activeMasternodeInfo.proTxHash)) {
        return {};
    }

    const CBLSSecretKey& skShare = quorum->GetSkShare();
    if (!skShare.IsValid()) {
        LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- we don't have our skShare for quorum %s\n", __func__, quorum->qc->quorumHash.ToString());
        return {};
    }

    int memberIdx = quorum->GetMemberIndex(activeMasternodeInfo.proTxHash);
    if (memberIdx == -1) {
        // this should really not happen (IsValidMember gave true)
        return {};
    }

    CSigShare sigShare;
    sigShare.llmqType = quorum->params.type;
    sigShare.quorumHash = quorum->qc->quorumHash;
    sigShare.id = id;
    sigShare.msgHash = msgHash;
    sigShare.quorumMember = (uint16_t)memberIdx;
    uint256 signHash = CLLMQUtils::BuildSignHash(sigShare);

    sigShare.sigShare.Set(skShare.Sign(signHash));
    if (!sigShare.sigShare.Get().IsValid()) {
        LogPrintf("CSigSharesManager::%s -- failed to sign sigShare. signHash=%s, id=%s, msgHash=%s, time=%s\n", __func__,
                  signHash.ToString(), sigShare.id.ToString(), sigShare.msgHash.ToString(), t.count());
        return {};
    }

    sigShare.UpdateKey();

    LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- created sigShare. signHash=%s, id=%s, msgHash=%s, llmqType=%d, quorum=%s, time=%s\n", __func__,
              signHash.ToString(), sigShare.id.ToString(), sigShare.msgHash.ToString(), quorum->params.type, quorum->qc->quorumHash.ToString(), t.count());

    return sigShare;
}

// causes all known sigShares to be re-announced
void CSigSharesManager::ForceReAnnouncement(const CQuorumCPtr& quorum, Consensus::LLMQType llmqType, const uint256& id, const uint256& msgHash)
{
    if (CLLMQUtils::IsAllMembersConnectedEnabled(llmqType)) {
        return;
    }

    LOCK(cs);
    auto signHash = CLLMQUtils::BuildSignHash(llmqType, quorum->qc->quorumHash, id, msgHash);
    auto sigs = sigShares.GetAllForSignHash(signHash);
    if (sigs) {
        for (auto& p : *sigs) {
            // re-announce every sigshare to every node
            sigSharesQueuedToAnnounce.Add(std::make_pair(signHash, p.first), true);
        }
    }
    for (auto& p : nodeStates) {
        CSigSharesNodeState& nodeState = p.second;
        auto session = nodeState.GetSessionBySignHash(signHash);
        if (!session) {
            continue;
        }
        // pretend that the other node doesn't know about any shares so that we re-announce everything
        session->knows.SetAll(false);
        // we need to use a new session id as we don't know if the other node has run into a timeout already
        session->sendSessionId = (uint32_t)-1;
    }
}

void CSigSharesManager::HandleNewRecoveredSig(const llmq::CRecoveredSig& recoveredSig)
{
    LOCK(cs);
    RemoveSigSharesForSession(CLLMQUtils::BuildSignHash(recoveredSig));
}

} // namespace llmq
