// Copyright (c) 2018-2019 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <llmq/quorums.h>
#include <llmq/quorums_commitment.h>
#include <llmq/quorums_signing.h>
#include <llmq/quorums_utils.h>
#include <llmq/quorums_signing_shares.h>

#include <masternode/activemasternode.h>
#include <bls/bls_batchverifier.h>
#include <chainparams.h>
#include <cxxtimer.hpp>
#include <init.h>
#include <net_processing.h>
#include <netmessagemaker.h>
#include <scheduler.h>
#include <validation.h>

#include <algorithm>
#include <unordered_set>

namespace llmq
{

CSigningManager* quorumSigningManager;

UniValue CRecoveredSig::ToJson() const
{
    UniValue ret(UniValue::VOBJ);
    ret.pushKV("llmqType", (int)llmqType);
    ret.pushKV("quorumHash", quorumHash.ToString());
    ret.pushKV("id", id.ToString());
    ret.pushKV("msgHash", msgHash.ToString());
    ret.pushKV("sig", sig.Get().ToString());
    ret.pushKV("hash", sig.Get().GetHash().ToString());
    return ret;
}

CRecoveredSigsDb::CRecoveredSigsDb(CDBWrapper& _db) :
    db(_db)
{

}

// This converts time values in "rs_t" from host endianness to big endianness, which is required to have proper ordering of the keys
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
    LogPrint(BCLog::LLMQ, "CRecoveredSigsDb::%s -- adding rs_vt keys with current time\n", __func__);

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

    LogPrint(BCLog::LLMQ, "CRecoveredSigsDb::%s -- added %d rs_vt entries\n", __func__, cnt);
}

bool CRecoveredSigsDb::HasRecoveredSig(uint8_t llmqType, const uint256& id, const uint256& msgHash) const
{
    auto k = std::make_tuple(std::string("rs_r"), llmqType, id, msgHash);
    return db.Exists(k);
}

bool CRecoveredSigsDb::HasRecoveredSigForId(uint8_t llmqType, const uint256& id)
{
    auto cacheKey = std::make_pair(llmqType, id);
    bool ret;
    {
        LOCK(cs);
        if (hasSigForIdCache.get(cacheKey, ret)) {
            return ret;
        }
    }


    auto k = std::make_tuple(std::string("rs_r"), llmqType, id);
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

bool CRecoveredSigsDb::ReadRecoveredSig(uint8_t llmqType, const uint256& id, CRecoveredSig& ret) const
{
    auto k = std::make_tuple(std::string("rs_r"), llmqType, id);

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

    return ReadRecoveredSig(k2.first, k2.second, ret);
}

bool CRecoveredSigsDb::GetRecoveredSigById(uint8_t llmqType, const uint256& id, CRecoveredSig& ret)
{
    return ReadRecoveredSig(llmqType, id, ret);
}

void CRecoveredSigsDb::WriteRecoveredSig(const llmq::CRecoveredSig& recSig)
{
    CDBBatch batch(db);

    uint32_t curTime = GetAdjustedTime();

    // we put these close to each other to leverage leveldb's key compaction
    // this way, the second key can be used for fast HasRecoveredSig checks while the first key stores the recSig
    auto k1 = std::make_tuple(std::string("rs_r"), recSig.llmqType, recSig.id);
    auto k2 = std::make_tuple(std::string("rs_r"), recSig.llmqType, recSig.id, recSig.msgHash);
    batch.Write(k1, recSig);
    // this key is also used to store the current time, so that we can easily get to the "rs_t" key when we have the id
    batch.Write(k2, curTime);

    // store by object hash
    auto k3 = std::make_tuple(std::string("rs_h"), recSig.GetHash());
    batch.Write(k3, std::make_pair(recSig.llmqType, recSig.id));

    // store by signHash
    auto signHash = CLLMQUtils::BuildSignHash(recSig);
    auto k4 = std::make_tuple(std::string("rs_s"), signHash);
    batch.Write(k4, (uint8_t)1);

    // store by current time. Allows fast cleanup of old recSigs
    auto k5 = std::make_tuple(std::string("rs_t"), (uint32_t)htobe32(curTime), recSig.llmqType, recSig.id);
    batch.Write(k5, (uint8_t)1);

    db.WriteBatch(batch);

    {
        LOCK(cs);
        hasSigForIdCache.insert(std::make_pair(recSig.llmqType, recSig.id), true);
        hasSigForSessionCache.insert(signHash, true);
        hasSigForHashCache.insert(recSig.GetHash(), true);
    }
}

void CRecoveredSigsDb::RemoveRecoveredSig(CDBBatch& batch, uint8_t llmqType, const uint256& id, bool deleteHashKey, bool deleteTimeKey)
{
    AssertLockHeld(cs);

    CRecoveredSig recSig;
    if (!ReadRecoveredSig(llmqType, id, recSig)) {
        return;
    }

    auto signHash = CLLMQUtils::BuildSignHash(recSig);

    auto k1 = std::make_tuple(std::string("rs_r"), recSig.llmqType, recSig.id);
    auto k2 = std::make_tuple(std::string("rs_r"), recSig.llmqType, recSig.id, recSig.msgHash);
    auto k3 = std::make_tuple(std::string("rs_h"), recSig.GetHash());
    auto k4 = std::make_tuple(std::string("rs_s"), signHash);
    batch.Erase(k1);
    batch.Erase(k2);
    if (deleteHashKey) {
        batch.Erase(k3);
    }
    batch.Erase(k4);

    if (deleteTimeKey) {
        CDataStream writeTimeDs(SER_DISK, CLIENT_VERSION);
        // TODO remove the size() == sizeof(uint32_t) in a future version (when we stop supporting upgrades from < 0.14.1)
        if (db.ReadDataStream(k2, writeTimeDs) && writeTimeDs.size() == sizeof(uint32_t)) {
            uint32_t writeTime;
            writeTimeDs >> writeTime;
            auto k5 = std::make_tuple(std::string("rs_t"), (uint32_t) htobe32(writeTime), recSig.llmqType, recSig.id);
            batch.Erase(k5);
        }
    }

    hasSigForIdCache.erase(std::make_pair(recSig.llmqType, recSig.id));
    hasSigForSessionCache.erase(signHash);
    if (deleteHashKey) {
        hasSigForHashCache.erase(recSig.GetHash());
    }
}

// Completely remove any traces of the recovered sig
void CRecoveredSigsDb::RemoveRecoveredSig(uint8_t llmqType, const uint256& id)
{
    AssertLockHeld(cs);
    CDBBatch batch(db);
    RemoveRecoveredSig(batch, llmqType, id, true, true);
    db.WriteBatch(batch);
}

// Remove the recovered sig itself and all keys required to get from id -> recSig
// This will leave the byHash key in-place so that HasRecoveredSigForHash still returns true
void CRecoveredSigsDb::TruncateRecoveredSig(uint8_t llmqType, const uint256& id)
{
    LOCK(cs);
    CDBBatch batch(db);
    RemoveRecoveredSig(batch, llmqType, id, false, false);
    db.WriteBatch(batch);
}

void CRecoveredSigsDb::CleanupOldRecoveredSigs(int64_t maxAge)
{
    std::unique_ptr<CDBIterator> pcursor(db.NewIterator());

    auto start = std::make_tuple(std::string("rs_t"), (uint32_t)0, (uint8_t)0, uint256());
    uint32_t endTime = (uint32_t)(GetAdjustedTime() - maxAge);
    pcursor->Seek(start);

    std::vector<std::pair<uint8_t, uint256>> toDelete;
    std::vector<decltype(start)> toDelete2;

    while (pcursor->Valid()) {
        decltype(start) k;

        if (!pcursor->GetKey(k) || std::get<0>(k) != "rs_t") {
            break;
        }
        if (be32toh(std::get<1>(k)) >= endTime) {
            break;
        }

        toDelete.emplace_back(std::get<2>(k), std::get<3>(k));
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
        for (const auto& e : toDelete) {
            RemoveRecoveredSig(batch, e.first, e.second, true, false);

            if (batch.SizeEstimate() >= (1 << 24)) {
                db.WriteBatch(batch);
                batch.Clear();
            }
        }
    }

    for (const auto& e : toDelete2) {
        batch.Erase(e);
    }

    db.WriteBatch(batch);

    LogPrint(BCLog::LLMQ, "CRecoveredSigsDb::%d -- deleted %d entries\n", __func__, toDelete.size());
}

bool CRecoveredSigsDb::HasVotedOnId(uint8_t llmqType, const uint256& id) const
{
    auto k = std::make_tuple(std::string("rs_v"), llmqType, id);
    return db.Exists(k);
}

bool CRecoveredSigsDb::GetVoteForId(uint8_t llmqType, const uint256& id, uint256& msgHashRet) const
{
    auto k = std::make_tuple(std::string("rs_v"), llmqType, id);
    return db.Read(k, msgHashRet);
}

void CRecoveredSigsDb::WriteVoteForId(uint8_t llmqType, const uint256& id, const uint256& msgHash)
{
    auto k1 = std::make_tuple(std::string("rs_v"), llmqType, id);
    auto k2 = std::make_tuple(std::string("rs_vt"), (uint32_t)htobe32(GetAdjustedTime()), llmqType, id);

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

    LogPrint(BCLog::LLMQ, "CRecoveredSigsDb::%d -- deleted %d entries\n", __func__, cnt);
}

//////////////////

CSigningManager::CSigningManager(CDBWrapper& llmqDb, bool fMemory, CConnman& _connman, PeerManager& _peerman, ChainstateManager& _chainman) :
    db(llmqDb),
    connman(_connman),
    peerman(_peerman),
    chainman(_chainman)
{
}

bool CSigningManager::AlreadyHave(const uint256& hash)
{
    {
        LOCK(cs);
        if (pendingReconstructedRecoveredSigs.count(hash)) {
            return true;
        }
    }

    return db.HasRecoveredSigForHash(hash);
}

bool CSigningManager::GetRecoveredSigForGetData(const uint256& hash, CRecoveredSig& ret)
{
    if (!db.GetRecoveredSigByHash(hash, ret)) {
        return false;
    }
    if (!CLLMQUtils::IsQuorumActive(ret.llmqType, ret.quorumHash)) {
        // we don't want to propagate sigs from inactive quorums
        return false;
    }
    return true;
}

void CSigningManager::ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv)
{
    if (strCommand == NetMsgType::QSIGREC) {
        auto recoveredSig = std::make_shared<CRecoveredSig>();
        vRecv >> *recoveredSig;
        ProcessMessageRecoveredSig(pfrom, recoveredSig);
    }
}

void CSigningManager::ProcessMessageRecoveredSig(CNode* pfrom, const std::shared_ptr<const CRecoveredSig>& recoveredSig)
{
    const uint256& hash = recoveredSig->GetHash();
    {
        LOCK(cs_main);
        pfrom->AddKnownTx(hash);
        peerman.ReceivedResponse(pfrom->GetId(), hash);
    }
    bool ban = false;
    if (!PreVerifyRecoveredSig(*recoveredSig, ban)) {
        if (ban) {
            {
                LOCK(cs_main);
                peerman.ForgetTxHash(pfrom->GetId(), hash);
            }
            peerman.Misbehaving(pfrom->GetId(), 100, "error PreVerifyRecoveredSig");
        }
        return;
    }

    // It's important to only skip seen *valid* sig shares here. See comment for CBatchedSigShare
    // We don't receive recovered sigs in batches, but we do batched verification per node on these
    if (db.HasRecoveredSigForHash(hash)) {
        {
            LOCK(cs_main);
            peerman.ForgetTxHash(pfrom->GetId(), hash);
        }
        return;
    }
    const std::string& signHash = CLLMQUtils::BuildSignHash(*recoveredSig).ToString();
    LogPrint(BCLog::LLMQ, "CSigningManager::%s -- signHash=%s, id=%s, msgHash=%s, node=%d\n", __func__,
    signHash, recoveredSig->id.ToString(), recoveredSig->msgHash.ToString(), pfrom->GetId());
    {
        LOCK(cs);
        if (pendingReconstructedRecoveredSigs.count(recoveredSig->GetHash())) {
            // no need to perform full verification
            LogPrint(BCLog::LLMQ, "CSigningManager::%s -- already pending reconstructed sig, signHash=%s, id=%s, msgHash=%s, node=%d\n", __func__,
                    signHash, recoveredSig->id.ToString(), recoveredSig->msgHash.ToString(), pfrom->GetId());
        } else {
            pendingRecoveredSigs[pfrom->GetId()].emplace_back(recoveredSig);
        }
    }
    {
        LOCK(cs_main);
        peerman.ForgetTxHash(pfrom->GetId(), hash);
    }
}

bool CSigningManager::PreVerifyRecoveredSig(const CRecoveredSig& recoveredSig, bool& retBan)
{
    retBan = false;

    auto llmqType = recoveredSig.llmqType;
    if (!Params().GetConsensus().llmqs.count(llmqType)) {
        retBan = true;
        return false;
    }

    CQuorumCPtr quorum = quorumManager->GetQuorum(llmqType, recoveredSig.quorumHash);

    if (!quorum) {
        LogPrint(BCLog::LLMQ, "CSigningManager::%s -- quorum %s not found\n", __func__,
                  recoveredSig.quorumHash.ToString());
        return false;
    }
    if (!CLLMQUtils::IsQuorumActive(llmqType, quorum->qc->quorumHash)) {
        return false;
    }

    return true;
}

void CSigningManager::CollectPendingRecoveredSigsToVerify(
        size_t maxUniqueSessions,
        std::unordered_map<NodeId, std::list<std::shared_ptr<const CRecoveredSig>>>& retSigShares,
        std::unordered_map<std::pair<uint8_t, uint256>, CQuorumCPtr, StaticSaltedHasher>& retQuorums)
{
    {
        LOCK(cs);
        if (pendingRecoveredSigs.empty()) {
            return;
        }

        std::unordered_set<std::pair<NodeId, uint256>, StaticSaltedHasher> uniqueSignHashes;
        CLLMQUtils::IterateNodesRandom(pendingRecoveredSigs, [&]() {
            return uniqueSignHashes.size() < maxUniqueSessions;
        }, [&](NodeId nodeId, std::list<std::shared_ptr<const CRecoveredSig>>& ns) {
            if (ns.empty()) {
                return false;
            }
            auto& recSig = *ns.begin();

            bool alreadyHave = db.HasRecoveredSigForHash(recSig->GetHash());
            if (!alreadyHave) {
                uniqueSignHashes.emplace(nodeId, CLLMQUtils::BuildSignHash(*recSig));
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
            const auto& recSig = *it;

            uint8_t llmqType = recSig->llmqType;
            auto quorumKey = std::make_pair(recSig->llmqType, recSig->quorumHash);
            if (!retQuorums.count(quorumKey)) {
                CQuorumCPtr quorum = quorumManager->GetQuorum(llmqType, recSig->quorumHash);
                if (!quorum) {
                    LogPrint(BCLog::LLMQ, "CSigningManager::%s -- quorum %s not found, node=%d\n", __func__,
                              recSig->quorumHash.ToString(), nodeId);
                    it = v.erase(it);
                    continue;
                }
                if (!CLLMQUtils::IsQuorumActive(llmqType, quorum->qc->quorumHash)) {
                    LogPrint(BCLog::LLMQ, "CSigningManager::%s -- quorum %s not active anymore, node=%d\n", __func__,
                              recSig->quorumHash.ToString(), nodeId);
                    it = v.erase(it);
                    continue;
                }

                retQuorums.try_emplace(quorumKey, quorum);
            }

            ++it;
        }
    }
}

void CSigningManager::ProcessPendingReconstructedRecoveredSigs()
{
    decltype(pendingReconstructedRecoveredSigs) m;
    {
        LOCK(cs);
        m = std::move(pendingReconstructedRecoveredSigs);
    }
    for (const auto& p : m) {
        ProcessRecoveredSig(-1, p.second);
    }
}

bool CSigningManager::ProcessPendingRecoveredSigs()
{
    std::unordered_map<NodeId, std::list<std::shared_ptr<const CRecoveredSig>>> recSigsByNode;
    std::unordered_map<std::pair<uint8_t, uint256>, CQuorumCPtr, StaticSaltedHasher> quorums;

    ProcessPendingReconstructedRecoveredSigs();

    const size_t nMaxBatchSize{32};
    CollectPendingRecoveredSigsToVerify(nMaxBatchSize, recSigsByNode, quorums);
    if (recSigsByNode.empty()) {
        return false;
    }

    // It's ok to perform insecure batched verification here as we verify against the quorum public keys, which are not
    // craftable by individual entities, making the rogue public key attack impossible
    CBLSBatchVerifier<NodeId, uint256> batchVerifier(false, false);

    size_t verifyCount = 0;
    for (const auto& p : recSigsByNode) {
        NodeId nodeId = p.first;
        const auto& v = p.second;

        for (const auto& recSig : v) {
            // we didn't verify the lazy signature until now
            if (!recSig->sig.Get().IsValid()) {
                batchVerifier.badSources.emplace(nodeId);
                break;
            }

            const auto& quorum = quorums.at(std::make_pair(recSig->llmqType, recSig->quorumHash));
            batchVerifier.PushMessage(nodeId, recSig->GetHash(), CLLMQUtils::BuildSignHash(*recSig), recSig->sig.Get(), quorum->qc->quorumPublicKey);
            verifyCount++;
        }
    }

    cxxtimer::Timer verifyTimer(true);
    batchVerifier.Verify();
    verifyTimer.stop();

    LogPrint(BCLog::LLMQ, "CSigningManager::%s -- verified recovered sig(s). count=%d, vt=%d, nodes=%d\n", __func__, verifyCount, verifyTimer.count(), recSigsByNode.size());

    std::unordered_set<uint256, StaticSaltedHasher> processed;
    for (const auto& p : recSigsByNode) {
        NodeId nodeId = p.first;
        const auto& v = p.second;

        if (batchVerifier.badSources.count(nodeId)) {
            LogPrint(BCLog::LLMQ, "CSigningManager::%s -- invalid recSig from other node, banning peer=%d\n", __func__, nodeId);
            peerman.Misbehaving(nodeId, 100, "invalid recSig from other node");
            continue;
        }

        for (auto& recSig : v) {
            if (!processed.emplace(recSig->GetHash()).second) {
                continue;
            }

            ProcessRecoveredSig(nodeId, recSig);
        }
    }

    return recSigsByNode.size() >= nMaxBatchSize;
}

// signature must be verified already
void CSigningManager::ProcessRecoveredSig(NodeId nodeId, const std::shared_ptr<const CRecoveredSig>& recoveredSig)
{
    auto llmqType = recoveredSig->llmqType;
    const uint256& hash = recoveredSig->GetHash();
    CInv inv(MSG_QUORUM_RECOVERED_SIG, hash);
    {
        LOCK(cs_main);
        peerman.ReceivedResponse(nodeId, hash);
    }

    if (db.HasRecoveredSigForHash(hash)) {
        {
            LOCK(cs_main);
            peerman.ForgetTxHash(nodeId, hash);
        }
        return;
    }

    std::vector<CRecoveredSigsListener*> listeners;
    bool bAlreadyKnown = false;
    bool bAlreadyKnownReturn = false;
    {
        LOCK(cs);
        listeners = recoveredSigsListeners;

        auto signHash = CLLMQUtils::BuildSignHash(*recoveredSig);

        LogPrint(BCLog::LLMQ, "CSigningManager::%s -- valid recSig. signHash=%s, id=%s, msgHash=%s\n", __func__,
                signHash.ToString(), recoveredSig->id   .ToString(), recoveredSig->msgHash.ToString());
        if (db.HasRecoveredSigForId(llmqType, recoveredSig->id)) {
            CRecoveredSig otherRecoveredSig;
            if (db.GetRecoveredSigById(llmqType, recoveredSig->id, otherRecoveredSig)) {
                auto otherSignHash = CLLMQUtils::BuildSignHash(otherRecoveredSig);
                if (signHash != otherSignHash) {
                    // this should really not happen, as each masternode is participating in only one vote,
                    // even if it's a member of multiple quorums. so a majority is only possible on one quorum and one msgHash per id
                    LogPrintf("CSigningManager::%s -- conflicting recoveredSig for signHash=%s, id=%s, msgHash=%s, otherSignHash=%s\n", __func__,
                              signHash.ToString(), recoveredSig->id.ToString(), recoveredSig->msgHash.ToString(), otherSignHash.ToString());
                } else {
                    bAlreadyKnown = true;
                    bAlreadyKnownReturn = true;
                }
                if(!bAlreadyKnown) {
                    return;
                }
            } else {
                // This case is very unlikely. It can only happen when cleanup caused this specific recSig to vanish
                // between the HasRecoveredSigForId and GetRecoveredSigById call. If that happens, treat it as if we
                // never had that recSig
                bAlreadyKnown = true;
            }
        }
        if(!bAlreadyKnownReturn) {
            db.WriteRecoveredSig(*recoveredSig);
            pendingReconstructedRecoveredSigs.erase(hash);
        }
    }
    if(bAlreadyKnown) {
        LOCK(cs_main);
        // Looks like we're trying to process a recSig that is already known. This might happen if the same
        // recSig comes in through regular QRECSIG messages and at the same time through some other message
        // which allowed to reconstruct a recSig (e.g. ISLOCK). In this case, just bail out.
        peerman.ForgetTxHash(nodeId, hash);
        if(bAlreadyKnownReturn) {
            return;
        }
    }
    if (fMasternodeMode) {
        connman.ForEachNode([&](CNode* pnode) {
            if (pnode->fSendRecSigs) {
                pnode->PushOtherInventory(inv);
            }
        });
    }

    for (auto& l : listeners) {
        l->HandleNewRecoveredSig(*recoveredSig);
    }
    {
        LOCK(cs_main);
        peerman.ForgetTxHash(nodeId, hash);
    }
}

void CSigningManager::PushReconstructedRecoveredSig(const std::shared_ptr<const llmq::CRecoveredSig>& recoveredSig)
{
    LOCK(cs);
    pendingReconstructedRecoveredSigs.try_emplace(recoveredSig->GetHash(), recoveredSig);
}

void CSigningManager::TruncateRecoveredSig(uint8_t llmqType, const uint256& id)
{
    db.TruncateRecoveredSig(llmqType, id);
}

void CSigningManager::Cleanup()
{
    int64_t now = GetTimeMillis();
    if (now - lastCleanupTime < 5000) {
        return;
    }

    int64_t maxAge = gArgs.GetArg("-maxrecsigsage", DEFAULT_MAX_RECOVERED_SIGS_AGE);

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

bool CSigningManager::AsyncSignIfMember(uint8_t llmqType, const uint256& id, const uint256& msgHash, const uint256& quorumHash, bool allowReSign)
{
    if (!fMasternodeMode || WITH_LOCK(activeMasternodeInfoCs, return activeMasternodeInfo.proTxHash.IsNull())) {
        return false;
    }
    CQuorumCPtr quorum;
    if (quorumHash.IsNull()) {
        // This might end up giving different results on different members
        // This might happen when we are on the brink of confirming a new quorum
        // This gives a slight risk of not getting enough shares to recover a signature
        // But at least it shouldn't be possible to get conflicting recovered signatures
        // TODO fix this by re-signing when the next block arrives, but only when that block results in a change of the quorum list and no recovered signature has been created in the mean time
        quorum = SelectQuorumForSigning(chainman, llmqType, id);
    } else {
        quorum = quorumManager->GetQuorum(llmqType, quorumHash);
    }

    if (!quorum) {
        LogPrint(BCLog::LLMQ, "CSigningManager::%s -- failed to select quorum. id=%s, msgHash=%s\n", __func__, id.ToString(), msgHash.ToString());
        return false;
    }

    if (!WITH_LOCK(activeMasternodeInfoCs, return quorum->IsValidMember(activeMasternodeInfo.proTxHash))) {
        return false;
    }
    {
        LOCK(cs);

        bool hasVoted = db.HasVotedOnId(llmqType, id);
        if (hasVoted) {
            uint256 prevMsgHash;
            db.GetVoteForId(llmqType, id, prevMsgHash);
            if (msgHash != prevMsgHash) {
                LogPrintf("CSigningManager::%s -- already voted for id=%s and msgHash=%s. Not voting on conflicting msgHash=%s\n", __func__,
                        id.ToString(), prevMsgHash.ToString(), msgHash.ToString());
                return false;
            } else if (allowReSign) {
                LogPrint(BCLog::LLMQ, "CSigningManager::%s -- already voted for id=%s and msgHash=%s. Resigning!\n", __func__,
                         id.ToString(), prevMsgHash.ToString());
            } else {
                LogPrint(BCLog::LLMQ, "CSigningManager::%s -- already voted for id=%s and msgHash=%s. Not voting again.\n", __func__,
                          id.ToString(), prevMsgHash.ToString());
                return false;
            }
        }

        if (db.HasRecoveredSigForId(llmqType, id)) {
            // no need to sign it if we already have a recovered sig
            return true;
        }
        if (!hasVoted) {
            db.WriteVoteForId(llmqType, id, msgHash);
        }
    }
    
    if (allowReSign) {
        // make us re-announce all known shares (other nodes might have run into a timeout)
        quorumSigSharesManager->ForceReAnnouncement(quorum, llmqType, id, msgHash);
    }
    quorumSigSharesManager->AsyncSign(quorum, id, msgHash);

    return true;
}

bool CSigningManager::HasRecoveredSig(uint8_t llmqType, const uint256& id, const uint256& msgHash)
{
    return db.HasRecoveredSig(llmqType, id, msgHash);
}

bool CSigningManager::HasRecoveredSigForId(uint8_t llmqType, const uint256& id)
{
    return db.HasRecoveredSigForId(llmqType, id);
}

bool CSigningManager::HasRecoveredSigForSession(const uint256& signHash)
{
    return db.HasRecoveredSigForSession(signHash);
}

bool CSigningManager::GetRecoveredSigForId(uint8_t llmqType, const uint256& id, llmq::CRecoveredSig& retRecSig)
{
    if (!db.GetRecoveredSigById(llmqType, id, retRecSig)) {
        return false;
    }
    return true;
}

bool CSigningManager::IsConflicting(uint8_t llmqType, const uint256& id, const uint256& msgHash)
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

bool CSigningManager::HasVotedOnId(uint8_t llmqType, const uint256& id)
{
    return db.HasVotedOnId(llmqType, id);
}

bool CSigningManager::GetVoteForId(uint8_t llmqType, const uint256& id, uint256& msgHashRet)
{
    return db.GetVoteForId(llmqType, id, msgHashRet);
}

CQuorumCPtr CSigningManager::SelectQuorumForSigning(ChainstateManager& chainman, uint8_t llmqType, const uint256& selectionHash, int signHeight, int signOffset)
{
    auto& llmqParams = Params().GetConsensus().llmqs.at(llmqType);
    size_t poolSize = (size_t)llmqParams.signingActiveQuorumCount;
    std::vector<CQuorumCPtr> quorums;
    CBlockIndex* pindexStart;
    {
        LOCK(cs_main);
        if (signHeight == -1) {
            signHeight = chainman.ActiveHeight();
        }
        int startBlockHeight = signHeight - signOffset;
        if (startBlockHeight > chainman.ActiveHeight() || startBlockHeight < 0) {
            return {};
        }
        pindexStart = chainman.ActiveChain()[startBlockHeight];
    }
    quorumManager->ScanQuorums(llmqType, pindexStart, poolSize, quorums);
    if (quorums.empty()) {
        return nullptr;
    }
    std::vector<std::pair<uint256, size_t>> scores;
    scores.reserve(quorums.size());
    for (size_t i = 0; i < quorums.size(); i++) {
        CHashWriter h(SER_NETWORK, 0);
        h << llmqType;
        h << quorums[i]->qc->quorumHash;
        h << selectionHash;
        scores.emplace_back(h.GetHash(), i);
    }
    std::sort(scores.begin(), scores.end());
    return quorums[scores.front().second];
}

bool CSigningManager::VerifyRecoveredSig(ChainstateManager& chainman, uint8_t llmqType, int signedAtHeight, const uint256& id, const uint256& msgHash, const CBLSSignature& sig, const int signOffset)
{
    auto quorum = SelectQuorumForSigning(chainman, llmqType, id, signedAtHeight, signOffset);
    if (!quorum) {
        return false;
    }

    uint256 signHash = CLLMQUtils::BuildSignHash(llmqType, quorum->qc->quorumHash, id, msgHash);
    return sig.VerifyInsecure(quorum->qc->quorumPublicKey, signHash);
}

} // namespace llmq
