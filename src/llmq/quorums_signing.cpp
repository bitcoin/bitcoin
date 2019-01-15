// Copyright (c) 2018 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "quorums_signing.h"
#include "quorums_utils.h"
#include "quorums_signing_shares.h"

#include "activemasternode.h"
#include "bls/bls_batchverifier.h"
#include "cxxtimer.hpp"
#include "init.h"
#include "net_processing.h"
#include "netmessagemaker.h"
#include "scheduler.h"
#include "validation.h"

#include <algorithm>
#include <limits>

namespace llmq
{

CSigningManager* quorumSigningManager;

CRecoveredSigsDb::CRecoveredSigsDb(bool fMemory) :
    db(fMemory ? "" : (GetDataDir() / "llmq"), 1 << 20, fMemory)
{
}

bool CRecoveredSigsDb::HasRecoveredSig(Consensus::LLMQType llmqType, const uint256& id, const uint256& msgHash)
{
    auto k = std::make_tuple('r', (uint8_t)llmqType, id, msgHash);
    return db.Exists(k);
}

bool CRecoveredSigsDb::HasRecoveredSigForId(Consensus::LLMQType llmqType, const uint256& id)
{
    auto k = std::make_tuple('r', (uint8_t)llmqType, id);
    return db.Exists(k);
}

bool CRecoveredSigsDb::HasRecoveredSigForSession(const uint256& signHash)
{
    auto k = std::make_tuple('s', signHash);
    return db.Exists(k);
}

bool CRecoveredSigsDb::HasRecoveredSigForHash(const uint256& hash)
{
    auto k = std::make_tuple('h', hash);
    return db.Exists(k);
}

bool CRecoveredSigsDb::ReadRecoveredSig(Consensus::LLMQType llmqType, const uint256& id, CRecoveredSig& ret)
{
    auto k = std::make_tuple('r', (uint8_t)llmqType, id);

    CDataStream ds(SER_DISK, CLIENT_VERSION);
    if (!db.ReadDataStream(k, ds)) {
        return false;
    }

    try {
        ret.Unserialize(ds, false);
        return true;
    } catch (std::exception&) {
        return false;
    }
}

bool CRecoveredSigsDb::GetRecoveredSigByHash(const uint256& hash, CRecoveredSig& ret)
{
    auto k1 = std::make_tuple('h', hash);
    std::pair<uint8_t, uint256> k2;
    if (!db.Read(k1, k2)) {
        return false;
    }

    return ReadRecoveredSig((Consensus::LLMQType)k2.first, k2.second, ret);
}

bool CRecoveredSigsDb::GetRecoveredSigById(Consensus::LLMQType llmqType, const uint256& id, CRecoveredSig& ret)
{
    return ReadRecoveredSig(llmqType, id, ret);
}

void CRecoveredSigsDb::WriteRecoveredSig(const llmq::CRecoveredSig& recSig)
{
    CDBBatch batch(db);

    // we put these close to each other to leverage leveldb's key compaction
    // this way, the second key can be used for fast HasRecoveredSig checks while the first key stores the recSig
    auto k1 = std::make_tuple('r', recSig.llmqType, recSig.id);
    auto k2 = std::make_tuple('r', recSig.llmqType, recSig.id, recSig.msgHash);
    batch.Write(k1, recSig);
    batch.Write(k2, (uint8_t)1);

    // store by object hash
    auto k3 = std::make_tuple('h', recSig.GetHash());
    batch.Write(k3, std::make_pair(recSig.llmqType, recSig.id));

    // store by signHash
    auto k4 = std::make_tuple('s', CLLMQUtils::BuildSignHash(recSig));
    batch.Write(k4, (uint8_t)1);

    // remove the votedForId entry as we won't need it anymore
    auto k5 = std::make_tuple('v', recSig.llmqType, recSig.id);
    batch.Erase(k5);

    // store by current time. Allows fast cleanup of old recSigs
    auto k6 = std::make_tuple('t', (uint32_t)GetAdjustedTime(), recSig.llmqType, recSig.id);
    batch.Write(k6, (uint8_t)1);

    db.WriteBatch(batch);
}

void CRecoveredSigsDb::CleanupOldRecoveredSigs(int64_t maxAge)
{
    std::unique_ptr<CDBIterator> pcursor(db.NewIterator());

    auto start = std::make_tuple('t', (uint32_t)0, (uint8_t)0, uint256());
    auto end = std::make_tuple('t', (uint32_t)(GetAdjustedTime() - maxAge), (uint8_t)0, uint256());
    pcursor->Seek(start);

    std::vector<std::pair<Consensus::LLMQType, uint256>> toDelete;
    std::vector<decltype(start)> toDelete2;

    while (pcursor->Valid()) {
        decltype(start) k;

        if (!pcursor->GetKey(k)) {
            break;
        }
        if (k >= end) {
            break;
        }

        toDelete.emplace_back((Consensus::LLMQType)std::get<2>(k), std::get<3>(k));
        toDelete2.emplace_back(k);

        pcursor->Next();
    }
    pcursor.reset();

    if (toDelete.empty()) {
        return;
    }

    CDBBatch batch(db);
    for (auto& e : toDelete) {
        CRecoveredSig recSig;
        if (!ReadRecoveredSig(e.first, e.second, recSig)) {
            continue;
        }

        auto k1 = std::make_tuple('r', recSig.llmqType, recSig.id);
        auto k2 = std::make_tuple('r', recSig.llmqType, recSig.id, recSig.msgHash);
        auto k3 = std::make_tuple('h', recSig.GetHash());
        auto k4 = std::make_tuple('s', CLLMQUtils::BuildSignHash(recSig));
        auto k5 = std::make_tuple('v', recSig.llmqType, recSig.id);
        batch.Erase(k1);
        batch.Erase(k2);
        batch.Erase(k3);
        batch.Erase(k4);
        batch.Erase(k5);
    }

    for (auto& e : toDelete2) {
        batch.Erase(e);
    }

    db.WriteBatch(batch);
}

bool CRecoveredSigsDb::HasVotedOnId(Consensus::LLMQType llmqType, const uint256& id)
{
    auto k = std::make_tuple('v', (uint8_t)llmqType, id);
    return db.Exists(k);
}

bool CRecoveredSigsDb::GetVoteForId(Consensus::LLMQType llmqType, const uint256& id, uint256& msgHashRet)
{
    auto k = std::make_tuple('v', (uint8_t)llmqType, id);
    return db.Read(k, msgHashRet);
}

void CRecoveredSigsDb::WriteVoteForId(Consensus::LLMQType llmqType, const uint256& id, const uint256& msgHash)
{
    auto k = std::make_tuple('v', (uint8_t)llmqType, id);
    db.Write(k, msgHash);
}

//////////////////

CSigningManager::CSigningManager(bool fMemory) :
    db(fMemory)
{
}

bool CSigningManager::AlreadyHave(const CInv& inv)
{
    LOCK(cs);
    return inv.type == MSG_QUORUM_RECOVERED_SIG && db.HasRecoveredSigForHash(inv.hash);
}

bool CSigningManager::GetRecoveredSigForGetData(const uint256& hash, CRecoveredSig& ret)
{
    if (!db.GetRecoveredSigByHash(hash, ret)) {
        return false;
    }
    if (!CLLMQUtils::IsQuorumActive((Consensus::LLMQType)(ret.llmqType), ret.quorumHash)) {
        // we don't want to propagate sigs from inactive quorums
        return false;
    }
    return true;
}

void CSigningManager::ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, CConnman& connman)
{
    if (strCommand == NetMsgType::QSIGREC) {
        CRecoveredSig recoveredSig;
        vRecv >> recoveredSig;
        ProcessMessageRecoveredSig(pfrom, recoveredSig, connman);
    }
}

void CSigningManager::ProcessMessageRecoveredSig(CNode* pfrom, const CRecoveredSig& recoveredSig, CConnman& connman)
{
    bool ban = false;
    if (!PreVerifyRecoveredSig(pfrom->id, recoveredSig, ban)) {
        if (ban) {
            LOCK(cs_main);
            Misbehaving(pfrom->id, 100);
            return;
        }
        return;
    }

    // It's important to only skip seen *valid* sig shares here. See comment for CBatchedSigShare
    // We don't receive recovered sigs in batches, but we do batched verification per node on these
    if (db.HasRecoveredSigForHash(recoveredSig.GetHash())) {
        return;
    }

    LogPrint("llmq", "CSigningManager::%s -- signHash=%s, node=%d\n", __func__, CLLMQUtils::BuildSignHash(recoveredSig).ToString(), pfrom->id);

    LOCK(cs);
    pendingRecoveredSigs[pfrom->id].emplace_back(recoveredSig);
}

bool CSigningManager::PreVerifyRecoveredSig(NodeId nodeId, const CRecoveredSig& recoveredSig, bool& retBan)
{
    retBan = false;

    auto llmqType = (Consensus::LLMQType)recoveredSig.llmqType;
    if (!Params().GetConsensus().llmqs.count(llmqType)) {
        retBan = true;
        return false;
    }

    CQuorumCPtr quorum;
    {
        LOCK(cs_main);

        quorum = quorumManager->GetQuorum(llmqType, recoveredSig.quorumHash);
        if (!quorum) {
            LogPrintf("CSigningManager::%s -- quorum %s not found, node=%d\n", __func__,
                      recoveredSig.quorumHash.ToString(), nodeId);
            return false;
        }
        if (!CLLMQUtils::IsQuorumActive(llmqType, quorum->quorumHash)) {
            return false;
        }
    }

    if (!recoveredSig.sig.IsValid()) {
        retBan = true;
        return false;
    }

    return true;
}

void CSigningManager::CollectPendingRecoveredSigsToVerify(
        size_t maxUniqueSessions,
        std::map<NodeId, std::list<CRecoveredSig>>& retSigShares,
        std::map<std::pair<Consensus::LLMQType, uint256>, CQuorumCPtr>& retQuorums)
{
    {
        LOCK(cs);
        if (pendingRecoveredSigs.empty()) {
            return;
        }

        std::set<std::pair<NodeId, uint256>> uniqueSignHashes;
        CLLMQUtils::IterateNodesRandom(pendingRecoveredSigs, [&]() {
            return uniqueSignHashes.size() < maxUniqueSessions;
        }, [&](NodeId nodeId, std::list<CRecoveredSig>& ns) {
            if (ns.empty()) {
                return false;
            }
            auto& recSig = *ns.begin();

            bool alreadyHave = db.HasRecoveredSigForHash(recSig.GetHash());
            if (!alreadyHave) {
                uniqueSignHashes.emplace(nodeId, CLLMQUtils::BuildSignHash(recSig));
                retSigShares[nodeId].emplace_back(recSig);
            }
            ns.erase(ns.begin());
            return !ns.empty();
        }, rnd);

        if (retSigShares.empty()) {
            return;
        }
    }

    {
        LOCK(cs_main);
        for (auto& p : retSigShares) {
            NodeId nodeId = p.first;
            auto& v = p.second;

            for (auto it = v.begin(); it != v.end();) {
                auto& recSig = *it;

                Consensus::LLMQType llmqType = (Consensus::LLMQType) recSig.llmqType;
                auto quorumKey = std::make_pair((Consensus::LLMQType)recSig.llmqType, recSig.quorumHash);
                if (!retQuorums.count(quorumKey)) {
                    CQuorumCPtr quorum = quorumManager->GetQuorum(llmqType, recSig.quorumHash);
                    if (!quorum) {
                        LogPrintf("CSigningManager::%s -- quorum %s not found, node=%d\n", __func__,
                                  recSig.quorumHash.ToString(), nodeId);
                        it = v.erase(it);
                        continue;
                    }
                    if (!CLLMQUtils::IsQuorumActive(llmqType, quorum->quorumHash)) {
                        LogPrintf("CSigningManager::%s -- quorum %s not active anymore, node=%d\n", __func__,
                                  recSig.quorumHash.ToString(), nodeId);
                        it = v.erase(it);
                        continue;
                    }

                    retQuorums.emplace(quorumKey, quorum);
                }

                ++it;
            }
        }
    }
}

void CSigningManager::ProcessPendingRecoveredSigs(CConnman& connman)
{
    std::map<NodeId, std::list<CRecoveredSig>> recSigsByNode;
    std::map<std::pair<Consensus::LLMQType, uint256>, CQuorumCPtr> quorums;

    CollectPendingRecoveredSigsToVerify(32, recSigsByNode, quorums);
    if (recSigsByNode.empty()) {
        return;
    }

    CBLSInsecureBatchVerifier<NodeId, uint256> batchVerifier;

    size_t verifyCount = 0;
    for (auto& p : recSigsByNode) {
        NodeId nodeId = p.first;
        auto& v = p.second;

        for (auto& recSig : v) {
            const auto& quorum = quorums.at(std::make_pair((Consensus::LLMQType)recSig.llmqType, recSig.quorumHash));
            batchVerifier.PushMessage(nodeId, recSig.GetHash(), CLLMQUtils::BuildSignHash(recSig), recSig.sig, quorum->quorumPublicKey);
            verifyCount++;
        }
    }

    cxxtimer::Timer verifyTimer;
    batchVerifier.Verify(false);
    verifyTimer.stop();

    LogPrint("llmq", "CSigningManager::%s -- verified recovered sig(s). count=%d, vt=%d, nodes=%d\n", __func__, verifyCount, verifyTimer.count(), recSigsByNode.size());

    std::set<uint256> processed;
    for (auto& p : recSigsByNode) {
        NodeId nodeId = p.first;
        auto& v = p.second;

        if (batchVerifier.badSources.count(nodeId)) {
            LOCK(cs_main);
            LogPrint("llmq", "CSigningManager::%s -- invalid recSig from other node, banning peer=%d\n", __func__, nodeId);
            Misbehaving(nodeId, 100);
            continue;
        }

        for (auto& recSig : v) {
            if (!processed.emplace(recSig.GetHash()).second) {
                continue;
            }

            const auto& quorum = quorums.at(std::make_pair((Consensus::LLMQType)recSig.llmqType, recSig.quorumHash));
            ProcessRecoveredSig(nodeId, recSig, quorum, connman);
        }
    }
}

// signature must be verified already
void CSigningManager::ProcessRecoveredSig(NodeId nodeId, const CRecoveredSig& recoveredSig, const CQuorumCPtr& quorum, CConnman& connman)
{
    auto llmqType = (Consensus::LLMQType)recoveredSig.llmqType;

    {
        LOCK(cs_main);
        connman.RemoveAskFor(recoveredSig.GetHash());
    }

    {
        LOCK(cs);

        auto signHash = CLLMQUtils::BuildSignHash(recoveredSig);

        LogPrintf("CSigningManager::%s -- valid recSig. signHash=%s, id=%s, msgHash=%s, node=%d\n", __func__,
                signHash.ToString(), recoveredSig.id.ToString(), recoveredSig.msgHash.ToString(), nodeId);

        if (db.HasRecoveredSigForId(llmqType, recoveredSig.id)) {
            // this should really not happen, as each masternode is participating in only one vote,
            // even if it's a member of multiple quorums. so a majority is only possible on one quorum and one msgHash per id
            LogPrintf("CSigningManager::%s -- conflicting recoveredSig for id=%s, msgHash=%s\n", __func__,
                      recoveredSig.id.ToString(), recoveredSig.msgHash.ToString());
            return;
        }

        db.WriteRecoveredSig(recoveredSig);
    }

    CInv inv(MSG_QUORUM_RECOVERED_SIG, recoveredSig.GetHash());
    g_connman->RelayInv(inv);
}

void CSigningManager::Cleanup()
{
    int64_t now = GetTimeMillis();
    if (now - lastCleanupTime < 5000) {
        return;
    }

    int64_t maxAge = GetArg("-recsigsmaxage", DEFAULT_MAX_RECOVERED_SIGS_AGE);

    db.CleanupOldRecoveredSigs(maxAge);

    lastCleanupTime = GetTimeMillis();
}

bool CSigningManager::AsyncSignIfMember(Consensus::LLMQType llmqType, const uint256& id, const uint256& msgHash)
{
    auto& params = Params().GetConsensus().llmqs.at(llmqType);

    if (!fMasternodeMode || activeMasternodeInfo.proTxHash.IsNull()) {
        return false;
    }

    {
        LOCK(cs);

        if (db.HasVotedOnId(llmqType, id)) {
            uint256 prevMsgHash;
            db.GetVoteForId(llmqType, id, prevMsgHash);
            LogPrintf("CSigningManager::%s -- already voted for id=%s and msgHash=%s. Not voting on conflicting msgHash=%s\n", __func__,
                      id.ToString(), prevMsgHash.ToString(), msgHash.ToString());
            return false;
        }

        if (db.HasRecoveredSigForId(llmqType, id)) {
            // no need to sign it if we already have a recovered sig
            return false;
        }
        db.WriteVoteForId(llmqType, id, msgHash);
    }

    // This might end up giving different results on different members
    // This might happen when we are on the brink of confirming a new quorum
    // This gives a slight risk of not getting enough shares to recover a signature
    // But at least it shouldn't be possible to get conflicting recovered signatures
    // TODO fix this by re-signing when the next block arrives, but only when that block results in a change of the quorum list and no recovered signature has been created in the mean time
    CQuorumCPtr quorum = quorumManager->SelectQuorum(llmqType, id, params.signingActiveQuorumCount);
    if (!quorum) {
        LogPrintf("CSigningManager::%s -- failed to select quorum. id=%s, msgHash=%s\n", __func__, id.ToString(), msgHash.ToString());
        return false;
    }

    if (!quorum->IsValidMember(activeMasternodeInfo.proTxHash)) {
        //LogPrintf("CSigningManager::%s -- we're not a valid member of quorum %s\n", __func__, quorum->quorumHash.ToString());
        return false;
    }

    quorumSigSharesManager->AsyncSign(quorum, id, msgHash);

    return true;
}

bool CSigningManager::HasRecoveredSig(Consensus::LLMQType llmqType, const uint256& id, const uint256& msgHash)
{
    return db.HasRecoveredSig(llmqType, id, msgHash);
}

bool CSigningManager::HasRecoveredSigForId(Consensus::LLMQType llmqType, const uint256& id)
{
    return db.HasRecoveredSigForId(llmqType, id);
}

bool CSigningManager::HasRecoveredSigForSession(const uint256& signHash)
{
    return db.HasRecoveredSigForSession(signHash);
}

bool CSigningManager::IsConflicting(Consensus::LLMQType llmqType, const uint256& id, const uint256& msgHash)
{
    if (!db.HasRecoveredSigForId(llmqType, id)) {
        // no recovered sig present, so no conflict
        return false;
    }

    if (!db.HasRecoveredSig(llmqType, id, msgHash)) {
        // recovered sig is present, but not for the given msgHash. That's a conflict!
        return true;
    }

    // all good
    return false;
}

}
