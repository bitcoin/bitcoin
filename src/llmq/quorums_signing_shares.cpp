// Copyright (c) 2018 The Dash Core developers
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

template<typename M>
static std::pair<typename M::const_iterator, typename M::const_iterator> FindBySignHash(const M& m, const uint256& signHash)
{
    return std::make_pair(
            m.lower_bound(std::make_pair(signHash, (uint16_t)0)),
            m.upper_bound(std::make_pair(signHash, std::numeric_limits<uint16_t>::max()))
    );
}
template<typename M>
static size_t CountBySignHash(const M& m, const uint256& signHash)
{
    auto itPair = FindBySignHash(m, signHash);
    size_t count = 0;
    while (itPair.first != itPair.second) {
        count++;
        ++itPair.first;
    }
    return count;
}

template<typename M>
static void RemoveBySignHash(M& m, const uint256& signHash)
{
    auto itPair = FindBySignHash(m, signHash);
    m.erase(itPair.first, itPair.second);
}

void CSigShare::UpdateKey()
{
    key.first = CLLMQUtils::BuildSignHash(*this);
    key.second = quorumMember;
}

void CSigSharesInv::Merge(const CSigSharesInv& inv2)
{
    assert(llmqType == inv2.llmqType);
    assert(signHash == inv2.signHash);
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
    std::string str = strprintf("signHash=%s, inv=(", signHash.ToString());
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

void CSigSharesInv::Init(Consensus::LLMQType _llmqType, const uint256& _signHash)
{
    llmqType = _llmqType;
    signHash = _signHash;

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
    //pendingIncomingRecSigs.erase(signHash);
    RemoveBySignHash(requestedSigShares, signHash);
    RemoveBySignHash(pendingIncomingSigShares, signHash);
}

CSigSharesInv CBatchedSigShares::ToInv() const
{
    CSigSharesInv inv;
    inv.Init((Consensus::LLMQType)llmqType, CLLMQUtils::BuildSignHash(*this));
    for (size_t i = 0; i < sigShares.size(); i++) {
        inv.inv[sigShares[i].first] = true;
    }
    return inv;
}

//////////////////////

CSigSharesManager::CSigSharesManager()
{
}

CSigSharesManager::~CSigSharesManager()
{
    StopWorkerThread();
}

void CSigSharesManager::StartWorkerThread()
{
    workThread = std::thread([this]() {
        RenameThread("quorum-sigshares");
        WorkThreadMain();
    });
}

void CSigSharesManager::StopWorkerThread()
{
    if (stopWorkThread) {
        return;
    }

    stopWorkThread = true;
    if (workThread.joinable()) {
        workThread.join();
    }
}

void CSigSharesManager::ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, CConnman& connman)
{
    // non-masternodes are not interested in sigshares
    if (!fMasternodeMode || activeMasternodeInfo.proTxHash.IsNull()) {
        return;
    }

    if (strCommand == NetMsgType::QSIGSHARESINV) {
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
            CSigShare sigShare = batchedSigShares.RebuildSigShare(i);
            nodeState.requestedSigShares.erase(sigShare.GetKey());

            // TODO track invalid sig shares received for PoSe?
            // It's important to only skip seen *valid* sig shares here. If a node sends us a
            // batch of mostly valid sig shares with a single invalid one and thus batched
            // verification fails, we'd skip the valid ones in the future if received from other nodes
            if (this->sigShares.count(sigShare.GetKey())) {
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
        nodeState.pendingIncomingSigShares.emplace(s.GetKey(), s);
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

    CQuorumCPtr quorum;
    {
        LOCK(cs_main);

        quorum = quorumManager->GetQuorum(llmqType, batchedSigShares.quorumHash);
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
    }

    std::set<uint16_t> dupMembers;

    for (size_t i = 0; i < batchedSigShares.sigShares.size(); i++) {
        auto quorumMember = batchedSigShares.sigShares[i].first;
        auto& sigShare = batchedSigShares.sigShares[i].second;
        if (!sigShare.IsValid()) {
            retBan = true;
            return false;
        }
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
        std::map<NodeId, std::vector<CSigShare>>& retSigShares,
        std::map<std::pair<Consensus::LLMQType, uint256>, CQuorumCPtr>& retQuorums)
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

        std::set<std::pair<NodeId, uint256>> uniqueSignHashes;
        CLLMQUtils::IterateNodesRandom(nodeStates, [&]() {
            return uniqueSignHashes.size() < maxUniqueSessions;
        }, [&](NodeId nodeId, CSigSharesNodeState& ns) {
            if (ns.pendingIncomingSigShares.empty()) {
                return false;
            }
            auto& sigShare = ns.pendingIncomingSigShares.begin()->second;

            bool alreadyHave = this->sigShares.count(sigShare.GetKey()) != 0;
            if (!alreadyHave) {
                uniqueSignHashes.emplace(nodeId, sigShare.GetSignHash());
                retSigShares[nodeId].emplace_back(sigShare);
            }
            ns.pendingIncomingSigShares.erase(ns.pendingIncomingSigShares.begin());
            return !ns.pendingIncomingSigShares.empty();
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

void CSigSharesManager::ProcessPendingSigShares(CConnman& connman)
{
    std::map<NodeId, std::vector<CSigShare>> sigSharesByNodes;
    std::map<std::pair<Consensus::LLMQType, uint256>, CQuorumCPtr> quorums;

    CollectPendingSigSharesToVerify(32, sigSharesByNodes, quorums);
    if (sigSharesByNodes.empty()) {
        return;
    }

    CBLSInsecureBatchVerifier<NodeId, SigShareKey> batchVerifier;

    size_t verifyCount = 0;
    for (auto& p : sigSharesByNodes) {
        auto nodeId = p.first;
        auto& v = p.second;

        for (auto& sigShare : v) {
            if (quorumSigningManager->HasRecoveredSigForId((Consensus::LLMQType)sigShare.llmqType, sigShare.id)) {
                continue;
            }

            auto quorum = quorums.at(std::make_pair((Consensus::LLMQType)sigShare.llmqType, sigShare.quorumHash));
            auto pubKeyShare = quorum->GetPubKeyShare(sigShare.quorumMember);

            if (!pubKeyShare.IsValid()) {
                // this should really not happen (we already ensured we have the quorum vvec,
                // so we should also be able to create all pubkey shares)
                LogPrintf("CSigSharesManager::%s -- pubKeyShare is invalid, which should not be possible here");
                assert(false);
            }

            batchVerifier.PushMessage(nodeId, sigShare.GetKey(), sigShare.GetSignHash(), sigShare.sigShare, pubKeyShare);
            verifyCount++;
        }
    }

    cxxtimer::Timer verifyTimer;
    batchVerifier.Verify(true);
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
}

// It's ensured that no duplicates are passed to this method
void CSigSharesManager::ProcessPendingSigSharesFromNode(NodeId nodeId, const std::vector<CSigShare>& sigShares, const std::map<std::pair<Consensus::LLMQType, uint256>, CQuorumCPtr>& quorums, CConnman& connman)
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
        
        if (!sigShares.emplace(sigShare.GetKey(), sigShare).second) {
            return;
        }
        
        sigSharesToAnnounce.emplace(sigShare.GetKey());
        firstSeenForSessions.emplace(sigShare.GetSignHash(), GetTimeMillis());

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

        size_t sigShareCount = CountBySignHash(sigShares, sigShare.GetSignHash());
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
        auto itPair = FindBySignHash(sigShares, signHash);

        sigSharesForRecovery.reserve((size_t) quorum->params.threshold);
        idsForRecovery.reserve((size_t) quorum->params.threshold);
        for (auto it = itPair.first; it != itPair.second && sigSharesForRecovery.size() < quorum->params.threshold; ++it) {
            auto& sigShare = it->second;
            sigSharesForRecovery.emplace_back(sigShare.sigShare);
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
void CSigSharesManager::CollectSigSharesToRequest(std::map<NodeId, std::map<uint256, CSigSharesInv>>& sigSharesToRequest)
{
    int64_t now = GetTimeMillis();
    std::map<SigShareKey, std::vector<NodeId>> nodesBySigShares;

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

        for (auto it = nodeState.requestedSigShares.begin(); it != nodeState.requestedSigShares.end();) {
            if (now - it->second >= SIG_SHARE_REQUEST_TIMEOUT) {
                // timeout while waiting for this one, so retry it with another node
                LogPrint("llmq", "CSigSharesManager::%s -- timeout while waiting for %s-%d, node=%d\n", __func__,
                         it->first.first.ToString(), it->first.second, nodeId);
                it = nodeState.requestedSigShares.erase(it);
            } else {
                ++it;
            }
        }

        std::map<uint256, CSigSharesInv>* invMap = nullptr;

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
                if (sigShares.count(k)) {
                    // we already have it
                    session.announced.inv[i] = false;
                    continue;
                }
                if (nodeState.requestedSigShares.size() >= maxRequestsForNode) {
                    // too many pending requests for this node
                    break;
                }
                auto it = sigSharesRequested.find(k);
                if (it != sigSharesRequested.end()) {
                    if (now - it->second.second >= SIG_SHARE_REQUEST_TIMEOUT && nodeId != it->second.first) {
                        // other node timed out, re-request from this node
                        LogPrint("llmq", "CSigSharesManager::%s -- other node timeout while waiting for %s-%d, re-request from=%d, node=%d\n", __func__,
                                 it->first.first.ToString(), it->first.second, nodeId, it->second.first);
                    } else {
                        continue;
                    }
                }
                // if we got this far we should do a request

                // track when we initiated the request so that we can detect timeouts
                nodeState.requestedSigShares.emplace(k, now);

                // don't request it from other nodes until a timeout happens
                auto& r = sigSharesRequested[k];
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
void CSigSharesManager::CollectSigSharesToSend(std::map<NodeId, std::map<uint256, CBatchedSigShares>>& sigSharesToSend)
{
    for (auto& p : nodeStates) {
        auto nodeId = p.first;
        auto& nodeState = p.second;

        if (nodeState.banned) {
            continue;
        }

        std::map<uint256, CBatchedSigShares>* sigSharesToSend2 = nullptr;

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
                auto it = sigShares.find(k);
                if (it == sigShares.end()) {
                    // he requested something we don'have
                    session.requested.inv[i] = false;
                    continue;
                }

                auto& sigShare = it->second;
                if (batchedSigShares.sigShares.empty()) {
                    batchedSigShares.llmqType = sigShare.llmqType;
                    batchedSigShares.quorumHash = sigShare.quorumHash;
                    batchedSigShares.id = sigShare.id;
                    batchedSigShares.msgHash = sigShare.msgHash;
                }
                batchedSigShares.sigShares.emplace_back((uint16_t)i, sigShare.sigShare);
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
void CSigSharesManager::CollectSigSharesToAnnounce(std::map<NodeId, std::map<uint256, CSigSharesInv>>& sigSharesToAnnounce)
{
    std::set<std::pair<Consensus::LLMQType, uint256>> quorumNodesPrepared;

    for (auto& sigShareKey : this->sigSharesToAnnounce) {
        auto& signHash = sigShareKey.first;
        auto quorumMember = sigShareKey.second;
        auto sigShareIt = sigShares.find(sigShareKey);
        if (sigShareIt == sigShares.end()) {
            continue;
        }
        auto& sigShare = sigShareIt->second;

        auto quorumKey = std::make_pair((Consensus::LLMQType)sigShare.llmqType, sigShare.quorumHash);
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

            auto& session = nodeState.GetOrCreateSession((Consensus::LLMQType)sigShare.llmqType, signHash);

            if (session.knows.inv[quorumMember]) {
                // he already knows that one
                continue;
            }

            auto& inv = sigSharesToAnnounce[nodeId][signHash];
            if (inv.inv.empty()) {
                inv.Init((Consensus::LLMQType)sigShare.llmqType, signHash);
            }
            inv.inv[quorumMember] = true;
            session.knows.inv[quorumMember] = true;
        }
    }

    // don't announce these anymore
    // nodes which did not send us a valid sig share before were left out now, but this is ok as it only results in slower
    // propagation for the first signing session of a fresh quorum. The sig shares should still arrive on all nodes due to
    // the deterministic inter-quorum-communication system
    this->sigSharesToAnnounce.clear();
}

void CSigSharesManager::SendMessages()
{
    std::multimap<CService, NodeId> nodesByAddress;
    g_connman->ForEachNode([&nodesByAddress](CNode* pnode) {
        nodesByAddress.emplace(pnode->addr, pnode->id);
    });

    std::map<NodeId, std::map<uint256, CSigSharesInv>> sigSharesToRequest;
    std::map<NodeId, std::map<uint256, CBatchedSigShares>> sigSharesToSend;
    std::map<NodeId, std::map<uint256, CSigSharesInv>> sigSharesToAnnounce;

    {
        LOCK(cs);
        CollectSigSharesToRequest(sigSharesToRequest);
        CollectSigSharesToSend(sigSharesToSend);
        CollectSigSharesToAnnounce(sigSharesToAnnounce);
    }

    g_connman->ForEachNode([&](CNode* pnode) {
        CNetMsgMaker msgMaker(pnode->GetSendVersion());

        auto it = sigSharesToRequest.find(pnode->id);
        if (it != sigSharesToRequest.end()) {
            for (auto& p : it->second) {
                assert(p.second.CountSet() != 0);
                LogPrint("llmq", "CSigSharesManager::SendMessages -- QGETSIGSHARES inv={%s}, node=%d\n",
                         p.second.ToString(), pnode->id);
                g_connman->PushMessage(pnode, msgMaker.Make(NetMsgType::QGETSIGSHARES, p.second));
            }
        }

        auto jt = sigSharesToSend.find(pnode->id);
        if (jt != sigSharesToSend.end()) {
            for (auto& p : jt->second) {
                assert(!p.second.sigShares.empty());
                LogPrint("llmq", "CSigSharesManager::SendMessages -- QBSIGSHARES inv={%s}, node=%d\n",
                         p.second.ToInv().ToString(), pnode->id);
                g_connman->PushMessage(pnode, msgMaker.Make(NetMsgType::QBSIGSHARES, p.second));
            }
        }

        auto kt = sigSharesToAnnounce.find(pnode->id);
        if (kt != sigSharesToAnnounce.end()) {
            for (auto& p : kt->second) {
                assert(p.second.CountSet() != 0);
                LogPrint("llmq", "CSigSharesManager::SendMessages -- QSIGSHARESINV inv={%s}, node=%d\n",
                         p.second.ToString(), pnode->id);
                g_connman->PushMessage(pnode, msgMaker.Make(NetMsgType::QSIGSHARESINV, p.second));
            }
        }

        return true;
    });
}

void CSigSharesManager::Cleanup()
{
    int64_t now = GetTimeMillis();
    if (now - lastCleanupTime < 5000) {
        return;
    }

    std::set<std::pair<Consensus::LLMQType, uint256>> quorumsToCheck;

    {
        LOCK(cs);

        // Remove sessions which timed out
        std::set<uint256> timeoutSessions;
        for (auto& p : firstSeenForSessions) {
            auto& signHash = p.first;
            int64_t time = p.second;

            if (now - time >= SIGNING_SESSION_TIMEOUT) {
                timeoutSessions.emplace(signHash);
            }
        }
        for (auto& signHash : timeoutSessions) {
            size_t count = CountBySignHash(sigShares, signHash);

            if (count > 0) {
                auto itPair = FindBySignHash(sigShares, signHash);
                auto& firstSigShare = itPair.first->second;
                LogPrintf("CSigSharesManager::%s -- signing session timed out. signHash=%s, id=%s, msgHash=%s, sigShareCount=%d\n", __func__,
                          signHash.ToString(), firstSigShare.id.ToString(), firstSigShare.msgHash.ToString(), count);
            } else {
                LogPrintf("CSigSharesManager::%s -- signing session timed out. signHash=%s, sigShareCount=%d\n", __func__,
                          signHash.ToString(), count);
            }
            RemoveSigSharesForSession(signHash);
        }

        // Remove sessions which were succesfully recovered
        std::set<uint256> doneSessions;
        for (auto& p : sigShares) {
            if (doneSessions.count(p.second.GetSignHash())) {
                continue;
            }
            if (quorumSigningManager->HasRecoveredSigForSession(p.second.GetSignHash())) {
                doneSessions.emplace(p.second.GetSignHash());
            }
        }
        for (auto& signHash : doneSessions) {
            RemoveSigSharesForSession(signHash);
        }

        for (auto& p : sigShares) {
            quorumsToCheck.emplace((Consensus::LLMQType)p.second.llmqType, p.second.quorumHash);
        }
    }

    {
        // Find quorums which became inactive
        LOCK(cs_main);
        for (auto it = quorumsToCheck.begin(); it != quorumsToCheck.end(); ) {
            if (CLLMQUtils::IsQuorumActive(it->first, it->second)) {
                it = quorumsToCheck.erase(it);
            } else {
                ++it;
            }
        }
    }
    {
        // Now delete sessions which are for inactive quorums
        LOCK(cs);
        std::set<uint256> inactiveQuorumSessions;
        for (auto& p : sigShares) {
            if (quorumsToCheck.count(std::make_pair((Consensus::LLMQType)p.second.llmqType, p.second.quorumHash))) {
                inactiveQuorumSessions.emplace(p.second.GetSignHash());
            }
        }
        for (auto& signHash : inactiveQuorumSessions) {
            RemoveSigSharesForSession(signHash);
        }
    }

    // Find node states for peers that disappeared from CConnman
    std::set<NodeId> nodeStatesToDelete;
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
        for (auto& p : nodeState.requestedSigShares) {
            sigSharesRequested.erase(p.first);
        }
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

    RemoveBySignHash(sigSharesRequested, signHash);
    RemoveBySignHash(sigSharesToAnnounce, signHash);
    RemoveBySignHash(sigShares, signHash);
    firstSeenForSessions.erase(signHash);
}

void CSigSharesManager::RemoveBannedNodeStates()
{
    // Called regularly to cleanup local node states for banned nodes

    LOCK2(cs_main, cs);
    std::set<NodeId> toRemove;
    for (auto it = nodeStates.begin(); it != nodeStates.end();) {
        if (IsBanned(it->first)) {
            // re-request sigshares from other nodes
            for (auto& p : it->second.requestedSigShares) {
                sigSharesRequested.erase(p.first);
            }
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
    for (auto& p : nodeState.requestedSigShares) {
        sigSharesRequested.erase(p.first);
    }
    nodeState.requestedSigShares.clear();

    nodeState.banned = true;
}

void CSigSharesManager::WorkThreadMain()
{
    int64_t lastProcessTime = GetTimeMillis();
    while (!stopWorkThread && !ShutdownRequested()) {
        RemoveBannedNodeStates();
        quorumSigningManager->ProcessPendingRecoveredSigs(*g_connman);
        ProcessPendingSigShares(*g_connman);
        SignPendingSigShares();
        SendMessages();
        Cleanup();
        quorumSigningManager->Cleanup();

        // TODO Wakeup when pending signing is needed?
        MilliSleep(100);
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
        Sign(std::get<0>(t), std::get<1>(t), std::get<2>(t));
    }
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

    sigShare.sigShare = skShare.Sign(signHash);
    if (!sigShare.sigShare.IsValid()) {
        LogPrintf("CSigSharesManager::%s -- failed to sign sigShare. id=%s, msgHash=%s, time=%s\n", __func__,
                  sigShare.id.ToString(), sigShare.msgHash.ToString(), t.count());
        return;
    }

    sigShare.UpdateKey();

    LogPrintf("CSigSharesManager::%s -- signed sigShare. id=%s, msgHash=%s, time=%s\n", __func__,
              sigShare.id.ToString(), sigShare.msgHash.ToString(), t.count());
    ProcessSigShare(-1, sigShare, *g_connman, quorum);
}

}
