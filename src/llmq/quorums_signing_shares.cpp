// Copyright (c) 2018-2019 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "quorums_signing.h"
#include "quorums_signing_shares.h"
#include "quorums_utils.h"

#include "activemasternode.h"
#include "bls/bls_batchverifier.h"
#include "init.h"
#include "net_processing.h"
#include "netmessagemaker.h"
#include "validation.h"

#include "cxxtimer.hpp"

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

void CSigSharesInv::Init(Consensus::LLMQType _llmqType)
{
    size_t llmqSize = (size_t)(Params().GetConsensus().llmqs.at(_llmqType).size);
    inv.resize(llmqSize, false);
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

CSigSharesInv CBatchedSigShares::ToInv(Consensus::LLMQType llmqType) const
{
    CSigSharesInv inv;
    inv.Init(llmqType);
    for (size_t i = 0; i < sigShares.size(); i++) {
        inv.inv[sigShares[i].first] = true;
    }
    return inv;
}

CSigSharesNodeState::Session& CSigSharesNodeState::GetOrCreateSession(Consensus::LLMQType llmqType, const uint256& signHash)
{
    auto& s = sessions[signHash];
    if (s.announced.inv.empty()) {
        s.announced.Init(llmqType, signHash);
        s.requested.Init(llmqType, signHash);
        s.knows.Init(llmqType, signHash);
    } else {
        assert(s.announced.llmqType == llmqType);
        assert(s.requested.llmqType == llmqType);
        assert(s.knows.llmqType == llmqType);
    }
    return s;
}

void CSigSharesNodeState::MarkAnnounced(const uint256& signHash, const CSigSharesInv& inv)
{
    GetOrCreateSession((Consensus::LLMQType)inv.llmqType, signHash).announced.Merge(inv);
}

void CSigSharesNodeState::MarkRequested(const uint256& signHash, const CSigSharesInv& inv)
{
    GetOrCreateSession((Consensus::LLMQType)inv.llmqType, signHash).requested.Merge(inv);
}

void CSigSharesNodeState::MarkKnows(const uint256& signHash, const CSigSharesInv& inv)
{
    GetOrCreateSession((Consensus::LLMQType)inv.llmqType, signHash).knows.Merge(inv);
}

void CSigSharesNodeState::MarkAnnounced(Consensus::LLMQType llmqType, const uint256& signHash, uint16_t quorumMember)
{
    GetOrCreateSession(llmqType, signHash).announced.Set(quorumMember, true);
}

void CSigSharesNodeState::MarkRequested(Consensus::LLMQType llmqType, const uint256& signHash, uint16_t quorumMember)
{
    GetOrCreateSession(llmqType, signHash).requested.Set(quorumMember, true);
}

void CSigSharesNodeState::MarkKnows(Consensus::LLMQType llmqType, const uint256& signHash, uint16_t quorumMember)
{
    GetOrCreateSession(llmqType, signHash).knows.Set(quorumMember, true);
}

void CSigSharesNodeState::RemoveSession(const uint256& signHash)
{
    sessions.erase(signHash);
    requestedSigShares.EraseAllForSignHash(signHash);
    pendingIncomingSigShares.EraseAllForSignHash(signHash);
}

//////////////////////

CSigSharesManager::CSigSharesManager()
{
    workInterrupt.reset();
}

CSigSharesManager::~CSigSharesManager()
{
}

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

void CSigSharesManager::ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, CConnman& connman)
{
    // non-masternodes are not interested in sigshares
    if (!fMasternodeMode || activeMasternodeInfo.proTxHash.IsNull()) {
        return;
    }

    if (strCommand == NetMsgType::QSIGSESANN) {
        CSigSesAnn ann;
        vRecv >> ann;
        ProcessMessageSigSesAnn(pfrom, ann, connman);
    } else if (strCommand == NetMsgType::QSIGSHARESINV) {
        CSigSharesInv inv;
        vRecv >> inv;
        ProcessMessageSigSharesInv(pfrom, inv, connman);
    } else if (strCommand == NetMsgType::QGETSIGSHARES) {
        CSigSharesInv inv;
        vRecv >> inv;
        ProcessMessageGetSigShares(pfrom, inv, connman);
    } else if (strCommand == NetMsgType::QBSIGSHARES) {
        CBatchedSigShares batchedSigShares;
        vRecv >> batchedSigShares;
        ProcessMessageBatchedSigShares(pfrom, batchedSigShares, connman);
    }
}

void CSigSharesManager::ProcessMessageSigSesAnn(CNode* pfrom, const CSigSesAnn& ann, CConnman& connman)
{
    auto llmqType = (Consensus::LLMQType)ann.llmqType;
    if (!Params().GetConsensus().llmqs.count(llmqType)) {
        BanNode(pfrom->id);
        return;
    }
    if (ann.sessionId == (uint32_t)-1 || ann.quorumHash.IsNull() || ann.id.IsNull() || ann.msgHash.IsNull()) {
        BanNode(pfrom->id);
        return;
    }

    LogPrint("llmq", "CSigSharesManager::%s -- ann={%s}, node=%d\n", __func__, ann.ToString(), pfrom->id);

    auto quorum = quorumManager->GetQuorum(llmqType, ann.quorumHash);
    if (!quorum) {
        // TODO should we ban here?
        LogPrintf("CSigSharesManager::%s -- quorum %s not found, node=%d\n", __func__,
                  ann.quorumHash.ToString(), pfrom->id);
        return;
    }

    auto signHash = CLLMQUtils::BuildSignHash(llmqType, ann.quorumHash, ann.id, ann.msgHash);
}

bool CSigSharesManager::VerifySigSharesInv(NodeId from, const CSigSharesInv& inv)
{
    Consensus::LLMQType llmqType = (Consensus::LLMQType)inv.llmqType;
    if (!Params().GetConsensus().llmqs.count(llmqType) || inv.signHash.IsNull()) {
        BanNode(from);
        return false;
    }

    if (!fMasternodeMode || activeMasternodeInfo.proTxHash.IsNull()) {
        return false;
    }

    size_t quorumSize = (size_t)Params().GetConsensus().llmqs.at(llmqType).size;

    if (inv.inv.size() != quorumSize) {
        BanNode(from);
        return false;
    }
    return true;
}

void CSigSharesManager::ProcessMessageSigSharesInv(CNode* pfrom, const CSigSharesInv& inv, CConnman& connman)
{
    if (!VerifySigSharesInv(pfrom->id, inv)) {
        return;
    }

    // TODO for PoSe, we should consider propagating shares even if we already have a recovered sig
    if (quorumSigningManager->HasRecoveredSigForSession(inv.signHash)) {
        return;
    }

    LogPrint("llmq", "CSigSharesManager::%s -- inv={%s}, node=%d\n", __func__, inv.ToString(), pfrom->id);

    LOCK(cs);
    auto& nodeState = nodeStates[pfrom->id];
    nodeState.MarkAnnounced(inv.signHash, inv);
    nodeState.MarkKnows(inv.signHash, inv);
}

void CSigSharesManager::ProcessMessageGetSigShares(CNode* pfrom, const CSigSharesInv& inv, CConnman& connman)
{
    if (!VerifySigSharesInv(pfrom->id, inv)) {
        return;
    }

    // TODO for PoSe, we should consider propagating shares even if we already have a recovered sig
    if (quorumSigningManager->HasRecoveredSigForSession(inv.signHash)) {
        return;
    }

    LogPrint("llmq", "CSigSharesManager::%s -- inv={%s}, node=%d\n", __func__, inv.ToString(), pfrom->id);

    LOCK(cs);
    auto& nodeState = nodeStates[pfrom->id];
    nodeState.MarkRequested(inv.signHash, inv);
    nodeState.MarkKnows(inv.signHash, inv);
}

void CSigSharesManager::ProcessMessageBatchedSigShares(CNode* pfrom, const CBatchedSigShares& batchedSigShares, CConnman& connman)
{
    bool ban = false;
    if (!PreVerifyBatchedSigShares(pfrom->id, batchedSigShares, ban)) {
        if (ban) {
            BanNode(pfrom->id);
            return;
        }
        return;
    }

    std::vector<CSigShare> sigShares;
    sigShares.reserve(batchedSigShares.sigShares.size());

    {
        LOCK(cs);
        auto& nodeState = nodeStates[pfrom->id];

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
            if (quorumSigningManager->HasRecoveredSigForId((Consensus::LLMQType)sigShare.llmqType, sigShare.id)) {
                continue;
            }

            sigShares.emplace_back(sigShare);
        }
    }

    LogPrint("llmq", "CSigSharesManager::%s -- shares=%d, new=%d, inv={%s}, node=%d\n", __func__,
             batchedSigShares.sigShares.size(), sigShares.size(), batchedSigShares.ToInv().ToString(), pfrom->id);

    if (sigShares.empty()) {
        return;
    }

    LOCK(cs);
    auto& nodeState = nodeStates[pfrom->id];
    for (auto& s : sigShares) {
        nodeState.pendingIncomingSigShares.Add(s.GetKey(), s);
    }
}

bool CSigSharesManager::PreVerifyBatchedSigShares(NodeId nodeId, const CBatchedSigShares& batchedSigShares, bool& retBan)
{
    retBan = false;

    auto llmqType = (Consensus::LLMQType)batchedSigShares.llmqType;
    if (!Params().GetConsensus().llmqs.count(llmqType)) {
        retBan = true;
        return false;
    }

    CQuorumCPtr quorum = quorumManager->GetQuorum(llmqType, batchedSigShares.quorumHash);
    if (!quorum) {
        // TODO should we ban here?
        LogPrintf("CSigSharesManager::%s -- quorum %s not found, node=%d\n", __func__,
                  batchedSigShares.quorumHash.ToString(), nodeId);
        return false;
    }
    if (!CLLMQUtils::IsQuorumActive(llmqType, quorum->quorumHash)) {
        // quorum is too old
        return false;
    }
    if (!quorum->IsMember(activeMasternodeInfo.proTxHash)) {
        // we're not a member so we can't verify it (we actually shouldn't have received it)
        return false;
    }
    if (quorum->quorumVvec == nullptr) {
        // TODO we should allow to ask other nodes for the quorum vvec if we missed it in the DKG
        LogPrintf("CSigSharesManager::%s -- we don't have the quorum vvec for %s, no verification possible. node=%d\n", __func__,
                  batchedSigShares.quorumHash.ToString(), nodeId);
        return false;
    }

    std::unordered_set<uint16_t> dupMembers;

    for (size_t i = 0; i < batchedSigShares.sigShares.size(); i++) {
        auto quorumMember = batchedSigShares.sigShares[i].first;
        if (!dupMembers.emplace(quorumMember).second) {
            retBan = true;
            return false;
        }

        if (quorumMember >= quorum->members.size()) {
            LogPrintf("CSigSharesManager::%s -- quorumMember out of bounds\n", __func__);
            retBan = true;
            return false;
        }
        if (!quorum->validMembers[quorumMember]) {
            LogPrintf("CSigSharesManager::%s -- quorumMember not valid\n", __func__);
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
                auto llmqType = (Consensus::LLMQType) sigShare.llmqType;

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

    CollectPendingSigSharesToVerify(32, sigSharesByNodes, quorums);
    if (sigSharesByNodes.empty()) {
        return false;
    }

    // It's ok to perform insecure batched verification here as we verify against the quorum public key shares,
    // which are not craftable by individual entities, making the rogue public key attack impossible
    CBLSBatchVerifier<NodeId, SigShareKey> batchVerifier(false, true);

    size_t verifyCount = 0;
    for (auto& p : sigSharesByNodes) {
        auto nodeId = p.first;
        auto& v = p.second;

        for (auto& sigShare : v) {
            if (quorumSigningManager->HasRecoveredSigForId((Consensus::LLMQType)sigShare.llmqType, sigShare.id)) {
                continue;
            }

            // we didn't check this earlier because we use a lazy BLS signature and tried to avoid doing the expensive
            // deserialization in the message thread
            if (!sigShare.sigShare.GetSig().IsValid()) {
                BanNode(nodeId);
                // don't process any additional shares from this node
                break;
            }

            auto quorum = quorums.at(std::make_pair((Consensus::LLMQType)sigShare.llmqType, sigShare.quorumHash));
            auto pubKeyShare = quorum->GetPubKeyShare(sigShare.quorumMember);

            if (!pubKeyShare.IsValid()) {
                // this should really not happen (we already ensured we have the quorum vvec,
                // so we should also be able to create all pubkey shares)
                LogPrintf("CSigSharesManager::%s -- pubKeyShare is invalid, which should not be possible here");
                assert(false);
            }

            batchVerifier.PushMessage(nodeId, sigShare.GetKey(), sigShare.GetSignHash(), sigShare.sigShare.GetSig(), pubKeyShare);
            verifyCount++;
        }
    }

    cxxtimer::Timer verifyTimer;
    batchVerifier.Verify();
    verifyTimer.stop();

    LogPrint("llmq", "CSigSharesManager::%s -- verified sig shares. count=%d, vt=%d, nodes=%d\n", __func__, verifyCount, verifyTimer.count(), sigSharesByNodes.size());

    for (auto& p : sigSharesByNodes) {
        auto nodeId = p.first;
        auto& v = p.second;

        if (batchVerifier.badSources.count(nodeId)) {
            LogPrint("llmq", "CSigSharesManager::%s -- invalid sig shares from other node, banning peer=%d\n",
                     __func__, nodeId);
            // this will also cause re-requesting of the shares that were sent by this node
            BanNode(nodeId);
            continue;
        }

        ProcessPendingSigSharesFromNode(nodeId, v, quorums, connman);
    }

    return true;
}

// It's ensured that no duplicates are passed to this method
void CSigSharesManager::ProcessPendingSigSharesFromNode(NodeId nodeId,
        const std::vector<CSigShare>& sigShares,
        const std::unordered_map<std::pair<Consensus::LLMQType, uint256>, CQuorumCPtr, StaticSaltedHasher>& quorums,
        CConnman& connman)
{
    auto& nodeState = nodeStates[nodeId];

    cxxtimer::Timer t(true);
    for (auto& sigShare : sigShares) {
        // he sent us some valid sig shares, so he must be part of this quorum and is thus interested in our sig shares as well
        // if this is the first time we received a sig share from this node, we won't announce the currently locally known sig shares to him.
        // only the upcoming sig shares will be announced to him. this means the first signing session for a fresh quorum will be a bit
        // slower than for older ones. TODO: fix this (risk of DoS when announcing all at once?)
        auto quorumKey = std::make_pair((Consensus::LLMQType)sigShare.llmqType, sigShare.quorumHash);
        nodeState.interestedIn.emplace(quorumKey);

        ProcessSigShare(nodeId, sigShare, connman, quorums.at(quorumKey));
    }
    t.stop();

    LogPrint("llmq", "CSigSharesManager::%s -- processed sigShare batch. shares=%d, time=%d, node=%d\n", __func__,
             sigShares.size(), t.count(), nodeId);
}

// sig shares are already verified when entering this method
void CSigSharesManager::ProcessSigShare(NodeId nodeId, const CSigShare& sigShare, CConnman& connman, const CQuorumCPtr& quorum)
{
    auto llmqType = quorum->params.type;

    bool canTryRecovery = false;

    // prepare node set for direct-push in case this is our sig share
    std::set<NodeId> quorumNodes;
    if (sigShare.quorumMember == quorum->GetMemberIndex(activeMasternodeInfo.proTxHash)) {
        quorumNodes = connman.GetMasternodeQuorumNodes((Consensus::LLMQType) sigShare.llmqType, sigShare.quorumHash);
        // make sure node states are created for these nodes (we might have not received any message from these yet)
        for (auto otherNodeId : quorumNodes) {
            nodeStates[otherNodeId].interestedIn.emplace(std::make_pair((Consensus::LLMQType)sigShare.llmqType, sigShare.quorumHash));
        }
    }

    if (quorumSigningManager->HasRecoveredSigForId(llmqType, sigShare.id)) {
        return;
    }

    {
        LOCK(cs);

        if (!sigShares.Add(sigShare.GetKey(), sigShare)) {
            return;
        }
        
        sigSharesToAnnounce.Add(sigShare.GetKey(), true);

        auto it = timeSeenForSessions.find(sigShare.GetSignHash());
        if (it == timeSeenForSessions.end()) {
            auto t = GetTimeMillis();
            // insert first-seen and last-seen time
            timeSeenForSessions.emplace(sigShare.GetSignHash(), std::make_pair(t, t));
        } else {
            // update last-seen time
            it->second.second = GetTimeMillis();
        }

        if (!quorumNodes.empty()) {
            // don't announce and wait for other nodes to request this share and directly send it to them
            // there is no way the other nodes know about this share as this is the one created on this node
            // this will also indicate interest to the other nodes in sig shares for this quorum
            for (auto& p : nodeStates) {
                if (!quorumNodes.count(p.first) && !p.second.interestedIn.count(std::make_pair((Consensus::LLMQType)sigShare.llmqType, sigShare.quorumHash))) {
                    continue;
                }
                p.second.MarkRequested((Consensus::LLMQType)sigShare.llmqType, sigShare.GetSignHash(), sigShare.quorumMember);
                p.second.MarkKnows((Consensus::LLMQType)sigShare.llmqType, sigShare.GetSignHash(), sigShare.quorumMember);
            }
        }

        size_t sigShareCount = sigShares.CountForSignHash(sigShare.GetSignHash());
        if (sigShareCount >= quorum->params.threshold) {
            canTryRecovery = true;
        }
    }

    if (canTryRecovery) {
        TryRecoverSig(quorum, sigShare.id, sigShare.msgHash, connman);
    }
}

void CSigSharesManager::TryRecoverSig(const CQuorumCPtr& quorum, const uint256& id, const uint256& msgHash, CConnman& connman)
{
    if (quorumSigningManager->HasRecoveredSigForId(quorum->params.type, id)) {
        return;
    }

    std::vector<CBLSSignature> sigSharesForRecovery;
    std::vector<CBLSId> idsForRecovery;
    {
        LOCK(cs);

        auto k = std::make_pair(quorum->params.type, id);

        auto signHash = CLLMQUtils::BuildSignHash(quorum->params.type, quorum->quorumHash, id, msgHash);
        auto sigShares = this->sigShares.GetAllForSignHash(signHash);
        if (!sigShares) {
            return;
        }

        sigSharesForRecovery.reserve((size_t) quorum->params.threshold);
        idsForRecovery.reserve((size_t) quorum->params.threshold);
        for (auto it = sigShares->begin(); it != sigShares->end() && sigSharesForRecovery.size() < quorum->params.threshold; ++it) {
            auto& sigShare = it->second;
            sigSharesForRecovery.emplace_back(sigShare.sigShare.GetSig());
            idsForRecovery.emplace_back(CBLSId::FromHash(quorum->members[sigShare.quorumMember]->proTxHash));
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
        LogPrintf("CSigSharesManager::%s -- failed to recover signature. id=%s, msgHash=%s, time=%d\n", __func__,
                  id.ToString(), msgHash.ToString(), t.count());
        return;
    }

    LogPrintf("CSigSharesManager::%s -- recovered signature. id=%s, msgHash=%s, time=%d\n", __func__,
              id.ToString(), msgHash.ToString(), t.count());

    CRecoveredSig rs;
    rs.llmqType = quorum->params.type;
    rs.quorumHash = quorum->quorumHash;
    rs.id = id;
    rs.msgHash = msgHash;
    rs.sig = recoveredSig;
    rs.UpdateHash();

    auto signHash = CLLMQUtils::BuildSignHash(rs);
    bool valid = rs.sig.VerifyInsecure(quorum->quorumPublicKey, signHash);
    if (!valid) {
        // this should really not happen as we have verified all signature shares before
        LogPrintf("CSigSharesManager::%s -- own recovered signature is invalid. id=%s, msgHash=%s\n", __func__,
                  id.ToString(), msgHash.ToString());
        return;
    }

    quorumSigningManager->ProcessRecoveredSig(-1, rs, quorum, connman);
}

// cs must be held
void CSigSharesManager::CollectSigSharesToRequest(std::unordered_map<NodeId, std::unordered_map<uint256, CSigSharesInv, StaticSaltedHasher>>& sigSharesToRequest)
{
    int64_t now = GetTimeMillis();
    const size_t maxRequestsForNode = 32;

    // avoid requesting from same nodes all the time
    std::vector<NodeId> shuffledNodeIds;
    shuffledNodeIds.reserve(nodeStates.size());
    for (auto& p : nodeStates) {
        if (p.second.sessions.empty()) {
            continue;
        }
        shuffledNodeIds.emplace_back(p.first);
    }
    std::random_shuffle(shuffledNodeIds.begin(), shuffledNodeIds.end(), rnd);

    for (auto& nodeId : shuffledNodeIds) {
        auto& nodeState = nodeStates[nodeId];

        if (nodeState.banned) {
            continue;
        }

        nodeState.requestedSigShares.EraseIf([&](const SigShareKey& k, int64_t t) {
            if (now - t >= SIG_SHARE_REQUEST_TIMEOUT) {
                // timeout while waiting for this one, so retry it with another node
                LogPrint("llmq", "CSigSharesManager::CollectSigSharesToRequest -- timeout while waiting for %s-%d, node=%d\n",
                         k.first.ToString(), k.second, nodeId);
                return true;
            }
            return false;
        });

        decltype(sigSharesToRequest.begin()->second)* invMap = nullptr;

        for (auto& p2 : nodeState.sessions) {
            auto& signHash = p2.first;
            auto& session = p2.second;

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
                        LogPrint("llmq", "CSigSharesManager::%s -- other node timeout while waiting for %s-%d, re-request from=%d, node=%d\n", __func__,
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
                    inv.Init((Consensus::LLMQType)session.announced.llmqType, signHash);
                }
                inv.inv[k.second] = true;

                // dont't request it again from this node
                session.announced.inv[i] = false;
            }
        }
    }
}

// cs must be held
void CSigSharesManager::CollectSigSharesToSend(std::unordered_map<NodeId, std::unordered_map<uint256, CBatchedSigShares, StaticSaltedHasher>>& sigSharesToSend)
{
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
                    // he requested something we don'have
                    session.requested.inv[i] = false;
                    continue;
                }

                if (batchedSigShares.sigShares.empty()) {
                    batchedSigShares.llmqType = sigShare->llmqType;
                    batchedSigShares.quorumHash = sigShare->quorumHash;
                    batchedSigShares.id = sigShare->id;
                    batchedSigShares.msgHash = sigShare->msgHash;
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

// cs must be held
void CSigSharesManager::CollectSigSharesToAnnounce(std::unordered_map<NodeId, std::unordered_map<uint256, CSigSharesInv, StaticSaltedHasher>>& sigSharesToAnnounce)
{
    std::unordered_set<std::pair<Consensus::LLMQType, uint256>, StaticSaltedHasher> quorumNodesPrepared;

    this->sigSharesToAnnounce.ForEach([&](const SigShareKey& sigShareKey, bool) {
        auto& signHash = sigShareKey.first;
        auto quorumMember = sigShareKey.second;
        const CSigShare* sigShare = sigShares.Get(sigShareKey);
        if (!sigShare) {
            return;
        }

        auto quorumKey = std::make_pair((Consensus::LLMQType)sigShare->llmqType, sigShare->quorumHash);
        if (quorumNodesPrepared.emplace(quorumKey).second) {
            // make sure we announce to at least the nodes which we know through the inter-quorum-communication system
            auto nodeIds = g_connman->GetMasternodeQuorumNodes(quorumKey.first, quorumKey.second);
            for (auto nodeId : nodeIds) {
                auto& nodeState = nodeStates[nodeId];
                nodeState.interestedIn.emplace(quorumKey);
            }
        }

        for (auto& p : nodeStates) {
            auto nodeId = p.first;
            auto& nodeState = p.second;

            if (nodeState.banned) {
                continue;
            }

            if (!nodeState.interestedIn.count(quorumKey)) {
                // node is not interested in this sig share
                // we only consider nodes to be interested if they sent us valid sig share before
                // the sig share that we sign by ourself circumvents the inv system and is directly sent to all quorum members
                // which are known by the deterministic inter-quorum-communication system. This is also the sig share that
                // will tell the other nodes that we are interested in future sig shares
                continue;
            }

            auto& session = nodeState.GetOrCreateSession((Consensus::LLMQType)sigShare->llmqType, signHash);

            if (session.knows.inv[quorumMember]) {
                // he already knows that one
                continue;
            }

            auto& inv = sigSharesToAnnounce[nodeId][signHash];
            if (inv.inv.empty()) {
                inv.Init((Consensus::LLMQType)sigShare->llmqType, signHash);
            }
            inv.inv[quorumMember] = true;
            session.knows.inv[quorumMember] = true;
        }
    });

    // don't announce these anymore
    // nodes which did not send us a valid sig share before were left out now, but this is ok as it only results in slower
    // propagation for the first signing session of a fresh quorum. The sig shares should still arrive on all nodes due to
    // the deterministic inter-quorum-communication system
    this->sigSharesToAnnounce.Clear();
}

bool CSigSharesManager::SendMessages()
{
    std::multimap<CService, NodeId> nodesByAddress;
    g_connman->ForEachNode([&nodesByAddress](CNode* pnode) {
        nodesByAddress.emplace(pnode->addr, pnode->id);
    });

    std::unordered_map<NodeId, std::unordered_map<uint256, CSigSharesInv, StaticSaltedHasher>> sigSharesToRequest;
    std::unordered_map<NodeId, std::unordered_map<uint256, CBatchedSigShares, StaticSaltedHasher>> sigSharesToSend;
    std::unordered_map<NodeId, std::unordered_map<uint256, CSigSharesInv, StaticSaltedHasher>> sigSharesToAnnounce;

    {
        LOCK(cs);
        CollectSigSharesToRequest(sigSharesToRequest);
        CollectSigSharesToSend(sigSharesToSend);
        CollectSigSharesToAnnounce(sigSharesToAnnounce);
    }

    bool didSend = false;

    g_connman->ForEachNode([&](CNode* pnode) {
        CNetMsgMaker msgMaker(pnode->GetSendVersion());

        auto it = sigSharesToRequest.find(pnode->id);
        if (it != sigSharesToRequest.end()) {
            for (auto& p : it->second) {
                assert(p.second.CountSet() != 0);
                LogPrint("llmq", "CSigSharesManager::SendMessages -- QGETSIGSHARES inv={%s}, node=%d\n",
                         p.second.ToString(), pnode->id);
                g_connman->PushMessage(pnode, msgMaker.Make(NetMsgType::QGETSIGSHARES, p.second), false);
                didSend = true;
            }
        }

        auto jt = sigSharesToSend.find(pnode->id);
        if (jt != sigSharesToSend.end()) {
            for (auto& p : jt->second) {
                assert(!p.second.sigShares.empty());
                LogPrint("llmq", "CSigSharesManager::SendMessages -- QBSIGSHARES inv={%s}, node=%d\n",
                         p.second.ToInv().ToString(), pnode->id);
                g_connman->PushMessage(pnode, msgMaker.Make(NetMsgType::QBSIGSHARES, p.second), false);
                didSend = true;
            }
        }

        auto kt = sigSharesToAnnounce.find(pnode->id);
        if (kt != sigSharesToAnnounce.end()) {
            for (auto& p : kt->second) {
                assert(p.second.CountSet() != 0);
                LogPrint("llmq", "CSigSharesManager::SendMessages -- QSIGSHARESINV inv={%s}, node=%d\n",
                         p.second.ToString(), pnode->id);
                g_connman->PushMessage(pnode, msgMaker.Make(NetMsgType::QSIGSHARESINV, p.second), false);
                didSend = true;
            }
        }

        return true;
    });

    if (didSend) {
        g_connman->WakeSelect();
    }

    return didSend;
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
    int64_t now = GetTimeMillis();
    if (now - lastCleanupTime < 5000) {
        return;
    }

    std::unordered_set<std::pair<Consensus::LLMQType, uint256>, StaticSaltedHasher> quorumsToCheck;

    {
        LOCK(cs);

        // Remove sessions which were succesfully recovered
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
        for (auto& p : timeSeenForSessions) {
            auto& signHash = p.first;
            int64_t firstSeenTime = p.second.first;
            int64_t lastSeenTime = p.second.second;

            if (now - firstSeenTime >= SESSION_TOTAL_TIMEOUT || now - lastSeenTime >= SESSION_NEW_SHARES_TIMEOUT) {
                timeoutSessions.emplace(signHash);
            }
        }
        for (auto& signHash : timeoutSessions) {
            size_t count = sigShares.CountForSignHash(signHash);

            if (count > 0) {
                auto m = sigShares.GetAllForSignHash(signHash);
                assert(m);

                auto& oneSigShare = m->begin()->second;
                LogPrintf("CSigSharesManager::%s -- signing session timed out. signHash=%s, id=%s, msgHash=%s, sigShareCount=%d\n", __func__,
                          signHash.ToString(), oneSigShare.id.ToString(), oneSigShare.msgHash.ToString(), count);
            } else {
                LogPrintf("CSigSharesManager::%s -- signing session timed out. signHash=%s, sigShareCount=%d\n", __func__,
                          signHash.ToString(), count);
            }
            RemoveSigSharesForSession(signHash);
        }

        sigShares.ForEach([&](const SigShareKey& k, const CSigShare& sigShare) {
            quorumsToCheck.emplace((Consensus::LLMQType) sigShare.llmqType, sigShare.quorumHash);
        });
    }

    // Find quorums which became inactive
    for (auto it = quorumsToCheck.begin(); it != quorumsToCheck.end(); ) {
        if (CLLMQUtils::IsQuorumActive(it->first, it->second)) {
            it = quorumsToCheck.erase(it);
        } else {
            ++it;
        }
    }

    {
        // Now delete sessions which are for inactive quorums
        LOCK(cs);
        std::unordered_set<uint256, StaticSaltedHasher> inactiveQuorumSessions;
        sigShares.ForEach([&](const SigShareKey& k, const CSigShare& sigShare) {
            if (quorumsToCheck.count(std::make_pair((Consensus::LLMQType)sigShare.llmqType, sigShare.quorumHash))) {
                inactiveQuorumSessions.emplace(sigShare.GetSignHash());
            }
        });
        for (auto& signHash : inactiveQuorumSessions) {
            RemoveSigSharesForSession(signHash);
        }
    }

    // Find node states for peers that disappeared from CConnman
    std::unordered_set<NodeId> nodeStatesToDelete;
    for (auto& p : nodeStates) {
        nodeStatesToDelete.emplace(p.first);
    }
    g_connman->ForEachNode([&](CNode* pnode) {
        nodeStatesToDelete.erase(pnode->id);
    });

    // Now delete these node states
    LOCK(cs);
    for (auto nodeId : nodeStatesToDelete) {
        auto& nodeState = nodeStates[nodeId];
        // remove global requested state to force a re-request from another node
        nodeState.requestedSigShares.ForEach([&](const SigShareKey& k, bool) {
            sigSharesRequested.Erase(k);
        });
        nodeStates.erase(nodeId);
    }

    lastCleanupTime = GetTimeMillis();
}

void CSigSharesManager::RemoveSigSharesForSession(const uint256& signHash)
{
    for (auto& p : nodeStates) {
        auto& ns = p.second;
        ns.RemoveSession(signHash);
    }

    sigSharesRequested.EraseAllForSignHash(signHash);
    sigSharesToAnnounce.EraseAllForSignHash(signHash);
    sigShares.EraseAllForSignHash(signHash);
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
        if (!quorumSigningManager || !g_connman || !quorumSigningManager) {
            if (!workInterrupt.sleep_for(std::chrono::milliseconds(100))) {
                return;
            }
            continue;
        }

        bool didWork = false;

        RemoveBannedNodeStates();
        didWork |= quorumSigningManager->ProcessPendingRecoveredSigs(*g_connman);
        didWork |= ProcessPendingSigShares(*g_connman);
        didWork |= SignPendingSigShares();

        if (GetTimeMillis() - lastSendTime > 100) {
            SendMessages();
            lastSendTime = GetTimeMillis();
        }

        Cleanup();
        quorumSigningManager->Cleanup();

        // TODO Wakeup when pending signing is needed?
        if (!didWork) {
            if (!workInterrupt.sleep_for(std::chrono::milliseconds(100))) {
                return;
            }
        }
    }
}

void CSigSharesManager::AsyncSign(const CQuorumCPtr& quorum, const uint256& id, const uint256& msgHash)
{
    LOCK(cs);
    pendingSigns.emplace_back(quorum, id, msgHash);
}

bool CSigSharesManager::SignPendingSigShares()
{
    std::vector<std::tuple<const CQuorumCPtr, uint256, uint256>> v;
    {
        LOCK(cs);
        v = std::move(pendingSigns);
    }

    for (auto& t : v) {
        Sign(std::get<0>(t), std::get<1>(t), std::get<2>(t));
    }

    return !v.empty();
}

void CSigSharesManager::Sign(const CQuorumCPtr& quorum, const uint256& id, const uint256& msgHash)
{
    cxxtimer::Timer t(true);

    if (!quorum->IsValidMember(activeMasternodeInfo.proTxHash)) {
        return;
    }

    CBLSSecretKey skShare = quorum->GetSkShare();
    if (!skShare.IsValid()) {
        LogPrintf("CSigSharesManager::%s -- we don't have our skShare for quorum %s\n", __func__, quorum->quorumHash.ToString());
        return;
    }

    int memberIdx = quorum->GetMemberIndex(activeMasternodeInfo.proTxHash);
    if (memberIdx == -1) {
        // this should really not happen (IsValidMember gave true)
        return;
    }

    CSigShare sigShare;
    sigShare.llmqType = quorum->params.type;
    sigShare.quorumHash = quorum->quorumHash;
    sigShare.id = id;
    sigShare.msgHash = msgHash;
    sigShare.quorumMember = (uint16_t)memberIdx;
    uint256 signHash = CLLMQUtils::BuildSignHash(sigShare);

    sigShare.sigShare.SetSig(skShare.Sign(signHash));
    if (!sigShare.sigShare.GetSig().IsValid()) {
        LogPrintf("CSigSharesManager::%s -- failed to sign sigShare. id=%s, msgHash=%s, time=%s\n", __func__,
                  sigShare.id.ToString(), sigShare.msgHash.ToString(), t.count());
        return;
    }

    sigShare.UpdateKey();

    LogPrintf("CSigSharesManager::%s -- signed sigShare. id=%s, msgHash=%s, time=%s\n", __func__,
              sigShare.id.ToString(), sigShare.msgHash.ToString(), t.count());
    ProcessSigShare(-1, sigShare, *g_connman, quorum);
}

void CSigSharesManager::HandleNewRecoveredSig(const llmq::CRecoveredSig& recoveredSig)
{
    LOCK(cs);
    RemoveSigSharesForSession(CLLMQUtils::BuildSignHash(recoveredSig));
}

}
