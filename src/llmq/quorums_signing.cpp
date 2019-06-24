// Copyright (c) 2018-2019 The Dash Core developers
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
#include <unordered_set>

namespace llmq
{

CSigningManager* quorumSigningManager;

UniValue CRecoveredSig::ToJson() const
{
    UniValue ret(UniValue::VOBJ);
    ret.push_back(Pair("llmqType", (int)llmqType));
    ret.push_back(Pair("quorumHash", quorumHash.ToString()));
    ret.push_back(Pair("id", id.ToString()));
    ret.push_back(Pair("msgHash", msgHash.ToString()));
    ret.push_back(Pair("sig", sig.Get().ToString()));
    ret.push_back(Pair("hash", sig.Get().GetHash().ToString()));
    return ret;
}

CRecoveredSigsDb::CRecoveredSigsDb(CDBWrapper& _db) :
    db(_db)
{
    if (Params().NetworkIDString() == CBaseChainParams::TESTNET) {
        // TODO this can be completely removed after some time (when we're pretty sure the conversion has been run on most testnet MNs)
        if (db.Exists(std::string("rs_upgraded"))) {
            return;
        }

        ConvertInvalidTimeKeys();
        AddVoteTimeKeys();

        db.Write(std::string("rs_upgraded"), (uint8_t)1);
    }
}

// This converts time values in "rs_t" from host endiannes to big endiannes, which is required to have proper ordering of the keys
void CRecoveredSigsDb::ConvertInvalidTimeKeys()
{
    LogPrintf("CRecoveredSigsDb::%s -- converting invalid rs_t keys\n", __func__);

    std::unique_ptr<CDBIterator> pcursor(db.NewIterator());

    auto start = std::make_tuple(std::string("rs_t"), (uint32_t)0, (uint8_t)0, uint256());
    pcursor->Seek(start);

    CDBBatch batch(db);
    size_t cnt = 0;
    while (pcursor->Valid()) {
        decltype(start) k;

        if (!pcursor->GetKey(k) || std::get<0>(k) != "rs_t") {
            break;
        }

        batch.Erase(k);
        std::get<1>(k) = htobe32(std::get<1>(k));
        batch.Write(k, (uint8_t)1);

        cnt++;

        pcursor->Next();
    }
    pcursor.reset();

    db.WriteBatch(batch);

    LogPrintf("CRecoveredSigsDb::%s -- converted %d invalid rs_t keys\n", __func__, cnt);
}

// This adds rs_vt keys for every rs_v entry to the DB. The time in the key is set to the current time.
// This causes cleanup of all these votes a week later.
void CRecoveredSigsDb::AddVoteTimeKeys()
{
    LogPrintf("CRecoveredSigsDb::%s -- adding rs_vt keys with current time\n", __func__);

    auto curTime = GetAdjustedTime();

    std::unique_ptr<CDBIterator> pcursor(db.NewIterator());

    auto start = std::make_tuple(std::string("rs_v"), (uint8_t)0, uint256());
    pcursor->Seek(start);

    CDBBatch batch(db);
    size_t cnt = 0;
    while (pcursor->Valid()) {
        decltype(start) k;

        if (!pcursor->GetKey(k) || std::get<0>(k) != "rs_v") {
            break;
        }

        uint8_t llmqType = std::get<1>(k);
        const uint256& id = std::get<2>(k);

        auto k2 = std::make_tuple(std::string("rs_vt"), (uint32_t)htobe32(curTime), llmqType, id);
        batch.Write(k2, (uint8_t)1);

        cnt++;

        pcursor->Next();
    }
    pcursor.reset();

    db.WriteBatch(batch);

    LogPrintf("CRecoveredSigsDb::%s -- added %d rs_vt entries\n", __func__, cnt);
}

bool CRecoveredSigsDb::HasRecoveredSig(Consensus::LLMQType llmqType, const uint256& id, const uint256& msgHash)
{
    auto k = std::make_tuple(std::string("rs_r"), (uint8_t)llmqType, id, msgHash);
    return db.Exists(k);
}

bool CRecoveredSigsDb::HasRecoveredSigForId(Consensus::LLMQType llmqType, const uint256& id)
{
    auto cacheKey = std::make_pair(llmqType, id);
    bool ret;
    {
        LOCK(cs);
        if (hasSigForIdCache.get(cacheKey, ret)) {
            return ret;
        }
    }


    auto k = std::make_tuple(std::string("rs_r"), (uint8_t)llmqType, id);
    ret = db.Exists(k);

    LOCK(cs);
    hasSigForIdCache.insert(cacheKey, ret);
    return ret;
}

bool CRecoveredSigsDb::HasRecoveredSigForSession(const uint256& signHash)
{
    bool ret;
    {
        LOCK(cs);
        if (hasSigForSessionCache.get(signHash, ret)) {
            return ret;
        }
    }

    auto k = std::make_tuple(std::string("rs_s"), signHash);
    ret = db.Exists(k);

    LOCK(cs);
    hasSigForSessionCache.insert(signHash, ret);
    return ret;
}

bool CRecoveredSigsDb::HasRecoveredSigForHash(const uint256& hash)
{
    bool ret;
    {
        LOCK(cs);
        if (hasSigForHashCache.get(hash, ret)) {
            return ret;
        }
    }

    auto k = std::make_tuple(std::string("rs_h"), hash);
    ret = db.Exists(k);

    LOCK(cs);
    hasSigForHashCache.insert(hash, ret);
    return ret;
}

bool CRecoveredSigsDb::ReadRecoveredSig(Consensus::LLMQType llmqType, const uint256& id, CRecoveredSig& ret)
{
    auto k = std::make_tuple(std::string("rs_r"), (uint8_t)llmqType, id);

    CDataStream ds(SER_DISK, CLIENT_VERSION);
    if (!db.ReadDataStream(k, ds)) {
        return false;
    }

    try {
        ret.Unserialize(ds);
        return true;
    } catch (std::exception&) {
        return false;
    }
}

bool CRecoveredSigsDb::GetRecoveredSigByHash(const uint256& hash, CRecoveredSig& ret)
{
    auto k1 = std::make_tuple(std::string("rs_h"), hash);
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
    auto k1 = std::make_tuple(std::string("rs_r"), recSig.llmqType, recSig.id);
    auto k2 = std::make_tuple(std::string("rs_r"), recSig.llmqType, recSig.id, recSig.msgHash);
    batch.Write(k1, recSig);
    batch.Write(k2, (uint8_t)1);

    // store by object hash
    auto k3 = std::make_tuple(std::string("rs_h"), recSig.GetHash());
    batch.Write(k3, std::make_pair(recSig.llmqType, recSig.id));

    // store by signHash
    auto signHash = CLLMQUtils::BuildSignHash(recSig);
    auto k4 = std::make_tuple(std::string("rs_s"), signHash);
    batch.Write(k4, (uint8_t)1);

    // store by current time. Allows fast cleanup of old recSigs
    auto k5 = std::make_tuple(std::string("rs_t"), (uint32_t)htobe32(GetAdjustedTime()), recSig.llmqType, recSig.id);
    batch.Write(k5, (uint8_t)1);

    db.WriteBatch(batch);

    {
        int64_t t = GetTimeMillis();

        LOCK(cs);
        hasSigForIdCache.insert(std::make_pair((Consensus::LLMQType)recSig.llmqType, recSig.id), true);
        hasSigForSessionCache.insert(signHash, true);
        hasSigForHashCache.insert(recSig.GetHash(), true);
    }
}

void CRecoveredSigsDb::CleanupOldRecoveredSigs(int64_t maxAge)
{
    std::unique_ptr<CDBIterator> pcursor(db.NewIterator());

    auto start = std::make_tuple(std::string("rs_t"), (uint32_t)0, (uint8_t)0, uint256());
    uint32_t endTime = (uint32_t)(GetAdjustedTime() - maxAge);
    pcursor->Seek(start);

    std::vector<std::pair<Consensus::LLMQType, uint256>> toDelete;
    std::vector<decltype(start)> toDelete2;

    while (pcursor->Valid()) {
        decltype(start) k;

        if (!pcursor->GetKey(k) || std::get<0>(k) != "rs_t") {
            break;
        }
        if (be32toh(std::get<1>(k)) >= endTime) {
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
    {
        LOCK(cs);
        for (auto& e : toDelete) {
            CRecoveredSig recSig;
            if (!ReadRecoveredSig(e.first, e.second, recSig)) {
                continue;
            }

            auto signHash = CLLMQUtils::BuildSignHash(recSig);

            auto k1 = std::make_tuple(std::string("rs_r"), recSig.llmqType, recSig.id);
            auto k2 = std::make_tuple(std::string("rs_r"), recSig.llmqType, recSig.id, recSig.msgHash);
            auto k3 = std::make_tuple(std::string("rs_h"), recSig.GetHash());
            auto k4 = std::make_tuple(std::string("rs_s"), signHash);
            batch.Erase(k1);
            batch.Erase(k2);
            batch.Erase(k3);
            batch.Erase(k4);

            hasSigForIdCache.erase(std::make_pair((Consensus::LLMQType)recSig.llmqType, recSig.id));
            hasSigForSessionCache.erase(signHash);
            hasSigForHashCache.erase(recSig.GetHash());

            if (batch.SizeEstimate() >= (1 << 24)) {
                db.WriteBatch(batch);
                batch.Clear();
            }
        }
    }

    for (auto& e : toDelete2) {
        batch.Erase(e);
    }

    db.WriteBatch(batch);

    LogPrint("llmq", "CRecoveredSigsDb::%d -- deleted %d entries\n", __func__, toDelete.size());
}

bool CRecoveredSigsDb::HasVotedOnId(Consensus::LLMQType llmqType, const uint256& id)
{
    auto k = std::make_tuple(std::string("rs_v"), (uint8_t)llmqType, id);
    return db.Exists(k);
}

bool CRecoveredSigsDb::GetVoteForId(Consensus::LLMQType llmqType, const uint256& id, uint256& msgHashRet)
{
    auto k = std::make_tuple(std::string("rs_v"), (uint8_t)llmqType, id);
    return db.Read(k, msgHashRet);
}

void CRecoveredSigsDb::WriteVoteForId(Consensus::LLMQType llmqType, const uint256& id, const uint256& msgHash)
{
    auto k1 = std::make_tuple(std::string("rs_v"), (uint8_t)llmqType, id);
    auto k2 = std::make_tuple(std::string("rs_vt"), (uint32_t)htobe32(GetAdjustedTime()), (uint8_t)llmqType, id);

    CDBBatch batch(db);
    batch.Write(k1, msgHash);
    batch.Write(k2, (uint8_t)1);

    db.WriteBatch(batch);
}

void CRecoveredSigsDb::CleanupOldVotes(int64_t maxAge)
{
    std::unique_ptr<CDBIterator> pcursor(db.NewIterator());

    auto start = std::make_tuple(std::string("rs_vt"), (uint32_t)0, (uint8_t)0, uint256());
    uint32_t endTime = (uint32_t)(GetAdjustedTime() - maxAge);
    pcursor->Seek(start);

    CDBBatch batch(db);
    size_t cnt = 0;
    while (pcursor->Valid()) {
        decltype(start) k;

        if (!pcursor->GetKey(k) || std::get<0>(k) != "rs_vt") {
            break;
        }
        if (be32toh(std::get<1>(k)) >= endTime) {
            break;
        }

        uint8_t llmqType = std::get<2>(k);
        const uint256& id = std::get<3>(k);

        batch.Erase(k);
        batch.Erase(std::make_tuple(std::string("rs_v"), llmqType, id));

        cnt++;

        pcursor->Next();
    }
    pcursor.reset();

    if (cnt == 0) {
        return;
    }

    db.WriteBatch(batch);

    LogPrint("llmq", "CRecoveredSigsDb::%d -- deleted %d entries\n", __func__, cnt);
}

//////////////////

CSigningManager::CSigningManager(CDBWrapper& llmqDb, bool fMemory) :
    db(llmqDb)
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

    CQuorumCPtr quorum = quorumManager->GetQuorum(llmqType, recoveredSig.quorumHash);

    if (!quorum) {
        LogPrint("llmq", "CSigningManager::%s -- quorum %s not found, node=%d\n", __func__,
                  recoveredSig.quorumHash.ToString(), nodeId);
        return false;
    }
    if (!CLLMQUtils::IsQuorumActive(llmqType, quorum->qc.quorumHash)) {
        return false;
    }

    return true;
}

void CSigningManager::CollectPendingRecoveredSigsToVerify(
        size_t maxUniqueSessions,
        std::unordered_map<NodeId, std::list<CRecoveredSig>>& retSigShares,
        std::unordered_map<std::pair<Consensus::LLMQType, uint256>, CQuorumCPtr, StaticSaltedHasher>& retQuorums)
{
    {
        LOCK(cs);
        if (pendingRecoveredSigs.empty()) {
            return;
        }

        std::unordered_set<std::pair<NodeId, uint256>, StaticSaltedHasher> uniqueSignHashes;
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
                    LogPrint("llmq", "CSigningManager::%s -- quorum %s not found, node=%d\n", __func__,
                              recSig.quorumHash.ToString(), nodeId);
                    it = v.erase(it);
                    continue;
                }
                if (!CLLMQUtils::IsQuorumActive(llmqType, quorum->qc.quorumHash)) {
                    LogPrint("llmq", "CSigningManager::%s -- quorum %s not active anymore, node=%d\n", __func__,
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

void CSigningManager::ProcessPendingReconstructedRecoveredSigs()
{
    decltype(pendingReconstructedRecoveredSigs) l;
    {
        LOCK(cs);
        l = std::move(pendingReconstructedRecoveredSigs);
    }
    for (auto& p : l) {
        ProcessRecoveredSig(-1, p.first, p.second, *g_connman);
    }
}

bool CSigningManager::ProcessPendingRecoveredSigs(CConnman& connman)
{
    std::unordered_map<NodeId, std::list<CRecoveredSig>> recSigsByNode;
    std::unordered_map<std::pair<Consensus::LLMQType, uint256>, CQuorumCPtr, StaticSaltedHasher> quorums;

    ProcessPendingReconstructedRecoveredSigs();

    CollectPendingRecoveredSigsToVerify(32, recSigsByNode, quorums);
    if (recSigsByNode.empty()) {
        return false;
    }

    // It's ok to perform insecure batched verification here as we verify against the quorum public keys, which are not
    // craftable by individual entities, making the rogue public key attack impossible
    CBLSBatchVerifier<NodeId, uint256> batchVerifier(false, false);

    size_t verifyCount = 0;
    for (auto& p : recSigsByNode) {
        NodeId nodeId = p.first;
        auto& v = p.second;

        for (auto& recSig : v) {
            // we didn't verify the lazy signature until now
            if (!recSig.sig.Get().IsValid()) {
                batchVerifier.badSources.emplace(nodeId);
                break;
            }

            const auto& quorum = quorums.at(std::make_pair((Consensus::LLMQType)recSig.llmqType, recSig.quorumHash));
            batchVerifier.PushMessage(nodeId, recSig.GetHash(), CLLMQUtils::BuildSignHash(recSig), recSig.sig.Get(), quorum->qc.quorumPublicKey);
            verifyCount++;
        }
    }

    cxxtimer::Timer verifyTimer(true);
    batchVerifier.Verify();
    verifyTimer.stop();

    LogPrint("llmq", "CSigningManager::%s -- verified recovered sig(s). count=%d, vt=%d, nodes=%d\n", __func__, verifyCount, verifyTimer.count(), recSigsByNode.size());

    std::unordered_set<uint256, StaticSaltedHasher> processed;
    for (auto& p : recSigsByNode) {
        NodeId nodeId = p.first;
        auto& v = p.second;

        if (batchVerifier.badSources.count(nodeId)) {
            LOCK(cs_main);
            LogPrintf("CSigningManager::%s -- invalid recSig from other node, banning peer=%d\n", __func__, nodeId);
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

    return true;
}

// signature must be verified already
void CSigningManager::ProcessRecoveredSig(NodeId nodeId, const CRecoveredSig& recoveredSig, const CQuorumCPtr& quorum, CConnman& connman)
{
    auto llmqType = (Consensus::LLMQType)recoveredSig.llmqType;

    {
        LOCK(cs_main);
        connman.RemoveAskFor(recoveredSig.GetHash());
    }

    std::vector<CRecoveredSigsListener*> listeners;
    {
        LOCK(cs);
        listeners = recoveredSigsListeners;

        auto signHash = CLLMQUtils::BuildSignHash(recoveredSig);

        LogPrint("llmq", "CSigningManager::%s -- valid recSig. signHash=%s, id=%s, msgHash=%s, node=%d\n", __func__,
                signHash.ToString(), recoveredSig.id.ToString(), recoveredSig.msgHash.ToString(), nodeId);

        if (db.HasRecoveredSigForId(llmqType, recoveredSig.id)) {
            CRecoveredSig otherRecoveredSig;
            if (db.GetRecoveredSigById(llmqType, recoveredSig.id, otherRecoveredSig)) {
                auto otherSignHash = CLLMQUtils::BuildSignHash(recoveredSig);
                if (signHash != otherSignHash) {
                    // this should really not happen, as each masternode is participating in only one vote,
                    // even if it's a member of multiple quorums. so a majority is only possible on one quorum and one msgHash per id
                    LogPrintf("CSigningManager::%s -- conflicting recoveredSig for signHash=%s, id=%s, msgHash=%s, otherSignHash=%s\n", __func__,
                              signHash.ToString(), recoveredSig.id.ToString(), recoveredSig.msgHash.ToString(), otherSignHash.ToString());
                } else {
                    // Looks like we're trying to process a recSig that is already known. This might happen if the same
                    // recSig comes in through regular QRECSIG messages and at the same time through some other message
                    // which allowed to reconstruct a recSig (e.g. ISLOCK). In this case, just bail out.
                }
                return;
            } else {
                // This case is very unlikely. It can only happen when cleanup caused this specific recSig to vanish
                // between the HasRecoveredSigForId and GetRecoveredSigById call. If that happens, treat it as if we
                // never had that recSig
            }
        }

        db.WriteRecoveredSig(recoveredSig);
    }

    CInv inv(MSG_QUORUM_RECOVERED_SIG, recoveredSig.GetHash());
    g_connman->ForEachNode([&](CNode* pnode) {
        if (pnode->nVersion >= LLMQS_PROTO_VERSION && pnode->fSendRecSigs) {
            pnode->PushInventory(inv);
        }
    });

    for (auto& l : listeners) {
        l->HandleNewRecoveredSig(recoveredSig);
    }
}

void CSigningManager::PushReconstructedRecoveredSig(const llmq::CRecoveredSig& recoveredSig, const llmq::CQuorumCPtr& quorum)
{
    LOCK(cs);
    pendingReconstructedRecoveredSigs.emplace_back(recoveredSig, quorum);
}

void CSigningManager::Cleanup()
{
    int64_t now = GetTimeMillis();
    if (now - lastCleanupTime < 5000) {
        return;
    }

    int64_t maxAge = GetArg("-recsigsmaxage", DEFAULT_MAX_RECOVERED_SIGS_AGE);

    db.CleanupOldRecoveredSigs(maxAge);
    db.CleanupOldVotes(maxAge);

    lastCleanupTime = GetTimeMillis();
}

void CSigningManager::RegisterRecoveredSigsListener(CRecoveredSigsListener* l)
{
    LOCK(cs);
    recoveredSigsListeners.emplace_back(l);
}

void CSigningManager::UnregisterRecoveredSigsListener(CRecoveredSigsListener* l)
{
    LOCK(cs);
    auto itRem = std::remove(recoveredSigsListeners.begin(), recoveredSigsListeners.end(), l);
    recoveredSigsListeners.erase(itRem, recoveredSigsListeners.end());
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
            if (msgHash != prevMsgHash) {
                LogPrintf("CSigningManager::%s -- already voted for id=%s and msgHash=%s. Not voting on conflicting msgHash=%s\n", __func__,
                        id.ToString(), prevMsgHash.ToString(), msgHash.ToString());
            } else {
                LogPrint("llmq", "CSigningManager::%s -- already voted for id=%s and msgHash=%s. Not voting again.\n", __func__,
                          id.ToString(), prevMsgHash.ToString());
            }
            return false;
        }

        if (db.HasRecoveredSigForId(llmqType, id)) {
            // no need to sign it if we already have a recovered sig
            return true;
        }
        db.WriteVoteForId(llmqType, id, msgHash);
    }

    int tipHeight;
    {
        LOCK(cs_main);
        tipHeight = chainActive.Height();
    }

    // This might end up giving different results on different members
    // This might happen when we are on the brink of confirming a new quorum
    // This gives a slight risk of not getting enough shares to recover a signature
    // But at least it shouldn't be possible to get conflicting recovered signatures
    // TODO fix this by re-signing when the next block arrives, but only when that block results in a change of the quorum list and no recovered signature has been created in the mean time
    CQuorumCPtr quorum = SelectQuorumForSigning(llmqType, tipHeight, id);
    if (!quorum) {
        LogPrint("llmq", "CSigningManager::%s -- failed to select quorum. id=%s, msgHash=%s\n", __func__, id.ToString(), msgHash.ToString());
        return false;
    }

    if (!quorum->IsValidMember(activeMasternodeInfo.proTxHash)) {
        //LogPrint("llmq", "CSigningManager::%s -- we're not a valid member of quorum %s\n", __func__, quorum->quorumHash.ToString());
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

bool CSigningManager::GetRecoveredSigForId(Consensus::LLMQType llmqType, const uint256& id, llmq::CRecoveredSig& retRecSig)
{
    if (!db.GetRecoveredSigById(llmqType, id, retRecSig)) {
        return false;
    }
    return true;
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

bool CSigningManager::HasVotedOnId(Consensus::LLMQType llmqType, const uint256& id)
{
    return db.HasVotedOnId(llmqType, id);
}

bool CSigningManager::GetVoteForId(Consensus::LLMQType llmqType, const uint256& id, uint256& msgHashRet)
{
    return db.GetVoteForId(llmqType, id, msgHashRet);
}

CQuorumCPtr CSigningManager::SelectQuorumForSigning(Consensus::LLMQType llmqType, int signHeight, const uint256& selectionHash)
{
    auto& llmqParams = Params().GetConsensus().llmqs.at(llmqType);
    size_t poolSize = (size_t)llmqParams.signingActiveQuorumCount;

    CBlockIndex* pindexStart;
    {
        LOCK(cs_main);
        int startBlockHeight = signHeight - SIGN_HEIGHT_OFFSET;
        if (startBlockHeight > chainActive.Height()) {
            return nullptr;
        }
        pindexStart = chainActive[startBlockHeight];
    }

    auto quorums = quorumManager->ScanQuorums(llmqType, pindexStart, poolSize);
    if (quorums.empty()) {
        return nullptr;
    }

    std::vector<std::pair<uint256, size_t>> scores;
    scores.reserve(quorums.size());
    for (size_t i = 0; i < quorums.size(); i++) {
        CHashWriter h(SER_NETWORK, 0);
        h << (uint8_t)llmqType;
        h << quorums[i]->qc.quorumHash;
        h << selectionHash;
        scores.emplace_back(h.GetHash(), i);
    }
    std::sort(scores.begin(), scores.end());
    return quorums[scores.front().second];
}

bool CSigningManager::VerifyRecoveredSig(Consensus::LLMQType llmqType, int signedAtHeight, const uint256& id, const uint256& msgHash, const CBLSSignature& sig)
{
    auto& llmqParams = Params().GetConsensus().llmqs.at(Params().GetConsensus().llmqChainLocks);

    auto quorum = SelectQuorumForSigning(llmqParams.type, signedAtHeight, id);
    if (!quorum) {
        return false;
    }

    uint256 signHash = CLLMQUtils::BuildSignHash(llmqParams.type, quorum->qc.quorumHash, id, msgHash);
    return sig.VerifyInsecure(quorum->qc.quorumPublicKey, signHash);
}

}
