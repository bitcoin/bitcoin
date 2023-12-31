// Copyright (c) 2018-2024 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/signing.h>

#include <llmq/commitment.h>
#include <llmq/options.h>
#include <llmq/quorums.h>
#include <llmq/signing_shares.h>

#include <bls/bls_batchverifier.h>
#include <chainparams.h>
#include <cxxtimer.hpp>
#include <dbwrapper.h>
#include <hash.h>
#include <masternode/node.h>
#include <net_processing.h>
#include <netmessagemaker.h>
#include <scheduler.h>
#include <streams.h>
#include <util/irange.h>
#include <util/thread.h>
#include <util/time.h>
#include <util/underlying.h>
#include <validation.h>

#include <algorithm>
#include <unordered_set>

namespace llmq
{
UniValue CRecoveredSig::ToJson() const
{
    UniValue ret(UniValue::VOBJ);
    ret.pushKV("llmqType", ToUnderlying(llmqType));
    ret.pushKV("quorumHash", quorumHash.ToString());
    ret.pushKV("id", id.ToString());
    ret.pushKV("msgHash", msgHash.ToString());
    ret.pushKV("sig", sig.Get().ToString());
    ret.pushKV("hash", sig.Get().GetHash().ToString());
    return ret;
}


CRecoveredSigsDb::CRecoveredSigsDb(bool fMemory, bool fWipe) :
        db(std::make_unique<CDBWrapper>(fMemory ? "" : (GetDataDir() / "llmq/recsigdb"), 8 << 20, fMemory, fWipe))
{
    MigrateRecoveredSigs();
}

CRecoveredSigsDb::~CRecoveredSigsDb() = default;

void CRecoveredSigsDb::MigrateRecoveredSigs()
{
    if (!db->IsEmpty()) return;

    LogPrint(BCLog::LLMQ, "CRecoveredSigsDb::%d -- start\n", __func__);

    CDBBatch batch(*db);
    auto oldDb = std::make_unique<CDBWrapper>(GetDataDir() / "llmq", 8 << 20);
    std::unique_ptr<CDBIterator> pcursor(oldDb->NewIterator());

    auto start_h = std::make_tuple(std::string("rs_h"), uint256());
    pcursor->Seek(start_h);

    while (pcursor->Valid()) {
        decltype(start_h) k;
        std::pair<Consensus::LLMQType, uint256> v;

        if (!pcursor->GetKey(k) || std::get<0>(k) != "rs_h") {
            break;
        }
        if (!pcursor->GetValue(v)) {
            break;
        }

        batch.Write(k, v);

        if (batch.SizeEstimate() >= (1 << 24)) {
            db->WriteBatch(batch);
            batch.Clear();
        }

        pcursor->Next();
    }

    auto start_r1 = std::make_tuple(std::string("rs_r"), (Consensus::LLMQType)0, uint256());
    pcursor->Seek(start_r1);

    while (pcursor->Valid()) {
        decltype(start_r1) k;
        CRecoveredSig v;

        if (!pcursor->GetKey(k) || std::get<0>(k) != "rs_r") {
            break;
        }
        if (!pcursor->GetValue(v)) {
            break;
        }

        batch.Write(k, v);

        if (batch.SizeEstimate() >= (1 << 24)) {
            db->WriteBatch(batch);
            batch.Clear();
        }

        pcursor->Next();
    }

    auto start_r2 = std::make_tuple(std::string("rs_r"), (Consensus::LLMQType)0, uint256(), uint256());
    pcursor->Seek(start_r2);

    while (pcursor->Valid()) {
        decltype(start_r2) k;
        uint32_t v;

        if (!pcursor->GetKey(k) || std::get<0>(k) != "rs_r") {
            break;
        }
        if (!pcursor->GetValue(v)) {
            break;
        }

        batch.Write(k, v);

        if (batch.SizeEstimate() >= (1 << 24)) {
            db->WriteBatch(batch);
            batch.Clear();
        }

        pcursor->Next();
    }

    auto start_s = std::make_tuple(std::string("rs_s"), uint256());
    pcursor->Seek(start_s);

    while (pcursor->Valid()) {
        decltype(start_s) k;
        uint8_t v;

        if (!pcursor->GetKey(k) || std::get<0>(k) != "rs_s") {
            break;
        }
        if (!pcursor->GetValue(v)) {
            break;
        }

        batch.Write(k, v);

        if (batch.SizeEstimate() >= (1 << 24)) {
            db->WriteBatch(batch);
            batch.Clear();
        }

        pcursor->Next();
    }

    auto start_t = std::make_tuple(std::string("rs_t"), (uint32_t)0, (Consensus::LLMQType)0, uint256());
    pcursor->Seek(start_t);

    while (pcursor->Valid()) {
        decltype(start_t) k;
        uint8_t v;

        if (!pcursor->GetKey(k) || std::get<0>(k) != "rs_t") {
            break;
        }
        if (!pcursor->GetValue(v)) {
            break;
        }

        batch.Write(k, v);

        if (batch.SizeEstimate() >= (1 << 24)) {
            db->WriteBatch(batch);
            batch.Clear();
        }

        pcursor->Next();
    }

    auto start_v = std::make_tuple(std::string("rs_v"), (Consensus::LLMQType)0, uint256());
    pcursor->Seek(start_v);

    while (pcursor->Valid()) {
        decltype(start_v) k;
        uint256 v;

        if (!pcursor->GetKey(k) || std::get<0>(k) != "rs_v") {
            break;
        }
        if (!pcursor->GetValue(v)) {
            break;
        }

        batch.Write(k, v);

        if (batch.SizeEstimate() >= (1 << 24)) {
            db->WriteBatch(batch);
            batch.Clear();
        }

        pcursor->Next();
    }

    auto start_vt = std::make_tuple(std::string("rs_vt"), (uint32_t)0, (Consensus::LLMQType)0, uint256());
    pcursor->Seek(start_vt);

    while (pcursor->Valid()) {
        decltype(start_vt) k;
        uint8_t v;

        if (!pcursor->GetKey(k) || std::get<0>(k) != "rs_vt") {
            break;
        }
        if (!pcursor->GetValue(v)) {
            break;
        }

        batch.Write(k, v);

        if (batch.SizeEstimate() >= (1 << 24)) {
            db->WriteBatch(batch);
            batch.Clear();
        }

        pcursor->Next();
    }

    db->WriteBatch(batch);
    pcursor.reset();
    oldDb.reset();

    LogPrint(BCLog::LLMQ, "CRecoveredSigsDb::%d -- done\n", __func__);
}

bool CRecoveredSigsDb::HasRecoveredSig(Consensus::LLMQType llmqType, const uint256& id, const uint256& msgHash) const
{
    auto k = std::make_tuple(std::string("rs_r"), llmqType, id, msgHash);
    return db->Exists(k);
}

bool CRecoveredSigsDb::HasRecoveredSigForId(Consensus::LLMQType llmqType, const uint256& id) const
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
    ret = db->Exists(k);

    LOCK(cs);
    hasSigForIdCache.insert(cacheKey, ret);
    return ret;
}

bool CRecoveredSigsDb::HasRecoveredSigForSession(const uint256& signHash) const
{
    bool ret;
    {
        LOCK(cs);
        if (hasSigForSessionCache.get(signHash, ret)) {
            return ret;
        }
    }

    auto k = std::make_tuple(std::string("rs_s"), signHash);
    ret = db->Exists(k);

    LOCK(cs);
    hasSigForSessionCache.insert(signHash, ret);
    return ret;
}

bool CRecoveredSigsDb::HasRecoveredSigForHash(const uint256& hash) const
{
    bool ret;
    {
        LOCK(cs);
        if (hasSigForHashCache.get(hash, ret)) {
            return ret;
        }
    }

    auto k = std::make_tuple(std::string("rs_h"), hash);
    ret = db->Exists(k);

    LOCK(cs);
    hasSigForHashCache.insert(hash, ret);
    return ret;
}

bool CRecoveredSigsDb::ReadRecoveredSig(Consensus::LLMQType llmqType, const uint256& id, CRecoveredSig& ret) const
{
    auto k = std::make_tuple(std::string("rs_r"), llmqType, id);

    CDataStream ds(SER_DISK, CLIENT_VERSION);
    if (!db->ReadDataStream(k, ds)) {
        return false;
    }

    try {
        ret.Unserialize(ds);
        return true;
    } catch (std::exception&) {
        return false;
    }
}

bool CRecoveredSigsDb::GetRecoveredSigByHash(const uint256& hash, CRecoveredSig& ret) const
{
    auto k1 = std::make_tuple(std::string("rs_h"), hash);
    std::pair<Consensus::LLMQType, uint256> k2;
    if (!db->Read(k1, k2)) {
        return false;
    }

    return ReadRecoveredSig(k2.first, k2.second, ret);
}

bool CRecoveredSigsDb::GetRecoveredSigById(Consensus::LLMQType llmqType, const uint256& id, CRecoveredSig& ret) const
{
    return ReadRecoveredSig(llmqType, id, ret);
}

void CRecoveredSigsDb::WriteRecoveredSig(const llmq::CRecoveredSig& recSig)
{
    CDBBatch batch(*db);

    uint32_t curTime = GetTime<std::chrono::seconds>().count();

    // we put these close to each other to leverage leveldb's key compaction
    // this way, the second key can be used for fast HasRecoveredSig checks while the first key stores the recSig
    auto k1 = std::make_tuple(std::string("rs_r"), recSig.getLlmqType(), recSig.getId());
    auto k2 = std::make_tuple(std::string("rs_r"), recSig.getLlmqType(), recSig.getId(), recSig.getMsgHash());
    batch.Write(k1, recSig);
    // this key is also used to store the current time, so that we can easily get to the "rs_t" key when we have the id
    batch.Write(k2, curTime);

    // store by object hash
    auto k3 = std::make_tuple(std::string("rs_h"), recSig.GetHash());
    batch.Write(k3, std::make_pair(recSig.getLlmqType(), recSig.getId()));

    // store by signHash
    auto signHash = recSig.buildSignHash();
    auto k4 = std::make_tuple(std::string("rs_s"), signHash);
    batch.Write(k4, (uint8_t)1);

    // store by current time. Allows fast cleanup of old recSigs
    auto k5 = std::make_tuple(std::string("rs_t"), (uint32_t)htobe32(curTime), recSig.getLlmqType(), recSig.getId());
    batch.Write(k5, (uint8_t)1);

    db->WriteBatch(batch);

    {
        LOCK(cs);
        hasSigForIdCache.insert(std::make_pair(recSig.getLlmqType(), recSig.getId()), true);
        hasSigForSessionCache.insert(signHash, true);
        hasSigForHashCache.insert(recSig.GetHash(), true);
    }
}

void CRecoveredSigsDb::RemoveRecoveredSig(CDBBatch& batch, Consensus::LLMQType llmqType, const uint256& id, bool deleteHashKey, bool deleteTimeKey)
{
    AssertLockHeld(cs);

    CRecoveredSig recSig;
    if (!ReadRecoveredSig(llmqType, id, recSig)) {
        return;
    }

    auto signHash = recSig.buildSignHash();

    auto k1 = std::make_tuple(std::string("rs_r"), recSig.getLlmqType(), recSig.getId());
    auto k2 = std::make_tuple(std::string("rs_r"), recSig.getLlmqType(), recSig.getId(), recSig.getMsgHash());
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
        if (db->ReadDataStream(k2, writeTimeDs) && writeTimeDs.size() == sizeof(uint32_t)) {
            uint32_t writeTime;
            writeTimeDs >> writeTime;
            auto k5 = std::make_tuple(std::string("rs_t"), (uint32_t) htobe32(writeTime), recSig.getLlmqType(), recSig.getId());
            batch.Erase(k5);
        }
    }

    hasSigForIdCache.erase(std::make_pair(recSig.getLlmqType(), recSig.getId()));
    hasSigForSessionCache.erase(signHash);
    if (deleteHashKey) {
        hasSigForHashCache.erase(recSig.GetHash());
    }
}

// Remove the recovered sig itself and all keys required to get from id -> recSig
// This will leave the byHash key in-place so that HasRecoveredSigForHash still returns true
void CRecoveredSigsDb::TruncateRecoveredSig(Consensus::LLMQType llmqType, const uint256& id)
{
    LOCK(cs);
    CDBBatch batch(*db);
    RemoveRecoveredSig(batch, llmqType, id, false, false);
    db->WriteBatch(batch);
}

void CRecoveredSigsDb::CleanupOldRecoveredSigs(int64_t maxAge)
{
    std::unique_ptr<CDBIterator> pcursor(db->NewIterator());

    auto start = std::make_tuple(std::string("rs_t"), (uint32_t)0, (Consensus::LLMQType)0, uint256());
    uint32_t endTime = (uint32_t)(GetTime<std::chrono::seconds>().count() - maxAge);
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

        toDelete.emplace_back(std::get<2>(k), std::get<3>(k));
        toDelete2.emplace_back(k);

        pcursor->Next();
    }
    pcursor.reset();

    if (toDelete.empty()) {
        return;
    }

    CDBBatch batch(*db);
    {
        LOCK(cs);
        for (const auto& e : toDelete) {
            RemoveRecoveredSig(batch, e.first, e.second, true, false);

            if (batch.SizeEstimate() >= (1 << 24)) {
                db->WriteBatch(batch);
                batch.Clear();
            }
        }
    }

    for (const auto& e : toDelete2) {
        batch.Erase(e);
    }

    db->WriteBatch(batch);

    LogPrint(BCLog::LLMQ, "CRecoveredSigsDb::%d -- deleted %d entries\n", __func__, toDelete.size());
}

bool CRecoveredSigsDb::HasVotedOnId(Consensus::LLMQType llmqType, const uint256& id) const
{
    auto k = std::make_tuple(std::string("rs_v"), llmqType, id);
    return db->Exists(k);
}

bool CRecoveredSigsDb::GetVoteForId(Consensus::LLMQType llmqType, const uint256& id, uint256& msgHashRet) const
{
    auto k = std::make_tuple(std::string("rs_v"), llmqType, id);
    return db->Read(k, msgHashRet);
}

void CRecoveredSigsDb::WriteVoteForId(Consensus::LLMQType llmqType, const uint256& id, const uint256& msgHash)
{
    auto k1 = std::make_tuple(std::string("rs_v"), llmqType, id);
    auto k2 = std::make_tuple(std::string("rs_vt"), (uint32_t)htobe32(GetTime<std::chrono::seconds>().count()), llmqType, id);

    CDBBatch batch(*db);
    batch.Write(k1, msgHash);
    batch.Write(k2, (uint8_t)1);

    db->WriteBatch(batch);
}

void CRecoveredSigsDb::CleanupOldVotes(int64_t maxAge)
{
    std::unique_ptr<CDBIterator> pcursor(db->NewIterator());

    auto start = std::make_tuple(std::string("rs_vt"), (uint32_t)0, (Consensus::LLMQType)0, uint256());
    uint32_t endTime = (uint32_t)(GetTime<std::chrono::seconds>().count() - maxAge);
    pcursor->Seek(start);

    CDBBatch batch(*db);
    size_t cnt = 0;
    while (pcursor->Valid()) {
        decltype(start) k;

        if (!pcursor->GetKey(k) || std::get<0>(k) != "rs_vt") {
            break;
        }
        if (be32toh(std::get<1>(k)) >= endTime) {
            break;
        }

        Consensus::LLMQType llmqType = std::get<2>(k);
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

    db->WriteBatch(batch);

    LogPrint(BCLog::LLMQ, "CRecoveredSigsDb::%d -- deleted %d entries\n", __func__, cnt);
}

//////////////////

CSigningManager::CSigningManager(CConnman& _connman, const CQuorumManager& _qman,
                                 bool fMemory, bool fWipe) :
    db(fMemory, fWipe), connman(_connman), qman(_qman)
{
}

bool CSigningManager::AlreadyHave(const CInv& inv) const
{
    if (inv.type != MSG_QUORUM_RECOVERED_SIG) {
        return false;
    }
    {
        LOCK(cs);
        if (pendingReconstructedRecoveredSigs.count(inv.hash)) {
            return true;
        }
    }

    return db.HasRecoveredSigForHash(inv.hash);
}

bool CSigningManager::GetRecoveredSigForGetData(const uint256& hash, CRecoveredSig& ret) const
{
    if (!db.GetRecoveredSigByHash(hash, ret)) {
        return false;
    }
    if (!IsQuorumActive(ret.getLlmqType(), qman, ret.getQuorumHash())) {
        // we don't want to propagate sigs from inactive quorums
        return false;
    }
    return true;
}

PeerMsgRet CSigningManager::ProcessMessage(const CNode& pfrom, gsl::not_null<PeerManager*> peerman, const std::string& msg_type, CDataStream& vRecv)
{
    if (msg_type == NetMsgType::QSIGREC) {
        auto recoveredSig = std::make_shared<CRecoveredSig>();
        vRecv >> *recoveredSig;

        return ProcessMessageRecoveredSig(pfrom, peerman, recoveredSig);
    }
    return {};
}

PeerMsgRet CSigningManager::ProcessMessageRecoveredSig(const CNode& pfrom, gsl::not_null<PeerManager*> peerman, const std::shared_ptr<const CRecoveredSig>& recoveredSig)
{
    {
        LOCK(cs_main);
        EraseObjectRequest(pfrom.GetId(), CInv(MSG_QUORUM_RECOVERED_SIG, recoveredSig->GetHash()));
    }

    bool ban = false;
    if (!PreVerifyRecoveredSig(qman, *recoveredSig, ban)) {
        if (ban) {
            return tl::unexpected{100};
        }
        return {};
    }

    // It's important to only skip seen *valid* sig shares here. See comment for CBatchedSigShare
    // We don't receive recovered sigs in batches, but we do batched verification per node on these
    if (db.HasRecoveredSigForHash(recoveredSig->GetHash())) {
        return {};
    }

    LogPrint(BCLog::LLMQ, "CSigningManager::%s -- signHash=%s, id=%s, msgHash=%s, node=%d\n", __func__,
             recoveredSig->buildSignHash().ToString(), recoveredSig->getId().ToString(), recoveredSig->getMsgHash().ToString(), pfrom.GetId());

    LOCK(cs);
    if (pendingReconstructedRecoveredSigs.count(recoveredSig->GetHash())) {
        // no need to perform full verification
        LogPrint(BCLog::LLMQ, "CSigningManager::%s -- already pending reconstructed sig, signHash=%s, id=%s, msgHash=%s, node=%d\n", __func__,
                 recoveredSig->buildSignHash().ToString(), recoveredSig->getId().ToString(), recoveredSig->getMsgHash().ToString(), pfrom.GetId());
        return {};
    }

    if (m_peerman == nullptr) {
        m_peerman = peerman;
    }
    // we should never use one CSigningManager with different PeerManager
    assert(m_peerman == peerman);

    pendingRecoveredSigs[pfrom.GetId()].emplace_back(recoveredSig);
    return {};
}

bool CSigningManager::PreVerifyRecoveredSig(const CQuorumManager& quorum_manager, const CRecoveredSig& recoveredSig, bool& retBan)
{
    retBan = false;

    auto llmqType = recoveredSig.getLlmqType();
    if (!Params().GetLLMQ(llmqType).has_value()) {
        retBan = true;
        return false;
    }

    CQuorumCPtr quorum = quorum_manager.GetQuorum(llmqType, recoveredSig.getQuorumHash());

    if (!quorum) {
        LogPrint(BCLog::LLMQ, "CSigningManager::%s -- quorum %s not found\n", __func__,
                  recoveredSig.getQuorumHash().ToString());
        return false;
    }
    if (!IsQuorumActive(llmqType, quorum_manager, quorum->qc->quorumHash)) {
        return false;
    }

    return true;
}

void CSigningManager::CollectPendingRecoveredSigsToVerify(
        size_t maxUniqueSessions,
        std::unordered_map<NodeId, std::list<std::shared_ptr<const CRecoveredSig>>>& retSigShares,
        std::unordered_map<std::pair<Consensus::LLMQType, uint256>, CQuorumCPtr, StaticSaltedHasher>& retQuorums)
{
    {
        LOCK(cs);
        if (pendingRecoveredSigs.empty()) {
            return;
        }

        // TODO: refactor it to remove duplicated code with `CSigSharesManager::CollectPendingSigSharesToVerify`
        std::unordered_set<std::pair<NodeId, uint256>, StaticSaltedHasher> uniqueSignHashes;
        IterateNodesRandom(pendingRecoveredSigs, [&]() {
            return uniqueSignHashes.size() < maxUniqueSessions;
        }, [&](NodeId nodeId, std::list<std::shared_ptr<const CRecoveredSig>>& ns) {
            if (ns.empty()) {
                return false;
            }
            auto& recSig = *ns.begin();

            bool alreadyHave = db.HasRecoveredSigForHash(recSig->GetHash());
            if (!alreadyHave) {
                uniqueSignHashes.emplace(nodeId, recSig->buildSignHash());
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

            auto llmqType = recSig->getLlmqType();
            auto quorumKey = std::make_pair(recSig->getLlmqType(), recSig->getQuorumHash());
            if (!retQuorums.count(quorumKey)) {
                CQuorumCPtr quorum = qman.GetQuorum(llmqType, recSig->getQuorumHash());
                if (!quorum) {
                    LogPrint(BCLog::LLMQ, "CSigningManager::%s -- quorum %s not found, node=%d\n", __func__,
                              recSig->getQuorumHash().ToString(), nodeId);
                    it = v.erase(it);
                    continue;
                }
                if (!IsQuorumActive(llmqType, qman, quorum->qc->quorumHash)) {
                    LogPrint(BCLog::LLMQ, "CSigningManager::%s -- quorum %s not active anymore, node=%d\n", __func__,
                              recSig->getQuorumHash().ToString(), nodeId);
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
    decltype(pendingReconstructedRecoveredSigs) m;
    {
        LOCK(cs);
        m = std::move(pendingReconstructedRecoveredSigs);
    }
    for (const auto& p : m) {
        ProcessRecoveredSig(p.second);
    }
}

bool CSigningManager::ProcessPendingRecoveredSigs()
{
    std::unordered_map<NodeId, std::list<std::shared_ptr<const CRecoveredSig>>> recSigsByNode;
    std::unordered_map<std::pair<Consensus::LLMQType, uint256>, CQuorumCPtr, StaticSaltedHasher> quorums;

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

            const auto& quorum = quorums.at(std::make_pair(recSig->getLlmqType(), recSig->getQuorumHash()));
            batchVerifier.PushMessage(nodeId, recSig->GetHash(), recSig->buildSignHash(), recSig->sig.Get(), quorum->qc->quorumPublicKey);
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
            m_peerman.load()->Misbehaving(nodeId, 100);
            continue;
        }

        for (const auto& recSig : v) {
            if (!processed.emplace(recSig->GetHash()).second) {
                continue;
            }

            ProcessRecoveredSig(recSig);
        }
    }

    return recSigsByNode.size() >= nMaxBatchSize;
}

// signature must be verified already
void CSigningManager::ProcessRecoveredSig(const std::shared_ptr<const CRecoveredSig>& recoveredSig)
{
    auto llmqType = recoveredSig->getLlmqType();

    if (db.HasRecoveredSigForHash(recoveredSig->GetHash())) {
        return;
    }

    std::vector<CRecoveredSigsListener*> listeners;
    {
        LOCK(cs);
        listeners = recoveredSigsListeners;

        auto signHash = recoveredSig->buildSignHash();

        LogPrint(BCLog::LLMQ, "CSigningManager::%s -- valid recSig. signHash=%s, id=%s, msgHash=%s\n", __func__,
                signHash.ToString(), recoveredSig->getId().ToString(), recoveredSig->getMsgHash().ToString());

        if (db.HasRecoveredSigForId(llmqType, recoveredSig->getId())) {
            CRecoveredSig otherRecoveredSig;
            if (db.GetRecoveredSigById(llmqType, recoveredSig->getId(), otherRecoveredSig)) {
                auto otherSignHash = otherRecoveredSig.buildSignHash();
                if (signHash != otherSignHash) {
                    // this should really not happen, as each masternode is participating in only one vote,
                    // even if it's a member of multiple quorums. so a majority is only possible on one quorum and one msgHash per id
                    LogPrintf("CSigningManager::%s -- conflicting recoveredSig for signHash=%s, id=%s, msgHash=%s, otherSignHash=%s\n", __func__,
                              signHash.ToString(), recoveredSig->getId().ToString(), recoveredSig->getMsgHash().ToString(), otherSignHash.ToString());
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

        db.WriteRecoveredSig(*recoveredSig);

        pendingReconstructedRecoveredSigs.erase(recoveredSig->GetHash());
    }

    if (fMasternodeMode) {
        CInv inv(MSG_QUORUM_RECOVERED_SIG, recoveredSig->GetHash());
        connman.ForEachNode([&](CNode* pnode) {
            if (pnode->fSendRecSigs) {
                pnode->PushInventory(inv);
            }
        });
    }

    for (auto& l : listeners) {
        l->HandleNewRecoveredSig(*recoveredSig);
    }

    GetMainSignals().NotifyRecoveredSig(recoveredSig);
}

void CSigningManager::PushReconstructedRecoveredSig(const std::shared_ptr<const llmq::CRecoveredSig>& recoveredSig)
{
    LOCK(cs);
    pendingReconstructedRecoveredSigs.emplace(std::piecewise_construct, std::forward_as_tuple(recoveredSig->GetHash()), std::forward_as_tuple(recoveredSig));
}

void CSigningManager::TruncateRecoveredSig(Consensus::LLMQType llmqType, const uint256& id)
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

bool CSigningManager::AsyncSignIfMember(Consensus::LLMQType llmqType, CSigSharesManager& shareman, const uint256& id, const uint256& msgHash, const uint256& quorumHash, bool allowReSign)
{
    if (!fMasternodeMode || WITH_LOCK(activeMasternodeInfoCs, return activeMasternodeInfo.proTxHash.IsNull())) {
        return false;
    }

    const CQuorumCPtr quorum = [&]() {
        if (quorumHash.IsNull()) {
            // This might end up giving different results on different members
            // This might happen when we are on the brink of confirming a new quorum
            // This gives a slight risk of not getting enough shares to recover a signature
            // But at least it shouldn't be possible to get conflicting recovered signatures
            // TODO fix this by re-signing when the next block arrives, but only when that block results in a change of the quorum list and no recovered signature has been created in the mean time
            const auto &llmq_params_opt = Params().GetLLMQ(llmqType);
            assert(llmq_params_opt.has_value());
            return SelectQuorumForSigning(llmq_params_opt.value(), qman, id);
        } else {
            return qman.GetQuorum(llmqType, quorumHash);
        }
    }();

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
        shareman.ForceReAnnouncement(quorum, llmqType, id, msgHash);
    }
    shareman.AsyncSign(quorum, id, msgHash);

    return true;
}

bool CSigningManager::HasRecoveredSig(Consensus::LLMQType llmqType, const uint256& id, const uint256& msgHash) const
{
    return db.HasRecoveredSig(llmqType, id, msgHash);
}

bool CSigningManager::HasRecoveredSigForId(Consensus::LLMQType llmqType, const uint256& id) const
{
    return db.HasRecoveredSigForId(llmqType, id);
}

bool CSigningManager::HasRecoveredSigForSession(const uint256& signHash) const
{
    return db.HasRecoveredSigForSession(signHash);
}

bool CSigningManager::GetRecoveredSigForId(Consensus::LLMQType llmqType, const uint256& id, llmq::CRecoveredSig& retRecSig) const
{
    if (!db.GetRecoveredSigById(llmqType, id, retRecSig)) {
        return false;
    }
    return true;
}

bool CSigningManager::IsConflicting(Consensus::LLMQType llmqType, const uint256& id, const uint256& msgHash) const
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

bool CSigningManager::GetVoteForId(Consensus::LLMQType llmqType, const uint256& id, uint256& msgHashRet) const
{
    return db.GetVoteForId(llmqType, id, msgHashRet);
}

void CSigningManager::StartWorkerThread()
{
    // can't start new thread if we have one running already
    if (workThread.joinable()) {
        assert(false);
    }

    workThread = std::thread(&util::TraceThread, "sigshares", [this] { WorkThreadMain(); });
}

void CSigningManager::StopWorkerThread()
{
    // make sure to call InterruptWorkerThread() first
    if (!workInterrupt) {
        assert(false);
    }

    if (workThread.joinable()) {
        workThread.join();
    }
}

void CSigningManager::InterruptWorkerThread()
{
    workInterrupt();
}

void CSigningManager::WorkThreadMain()
{
    while (!workInterrupt) {
        bool fMoreWork = ProcessPendingRecoveredSigs();

        Cleanup();

        // TODO Wakeup when pending signing is needed?
        if (!fMoreWork && !workInterrupt.sleep_for(std::chrono::milliseconds(100))) {
            return;
        }
    }
}

uint256 CSigBase::buildSignHash() const
{
    return BuildSignHash(llmqType, quorumHash, id, msgHash);
}

uint256 BuildSignHash(Consensus::LLMQType llmqType, const uint256& quorumHash, const uint256& id, const uint256& msgHash)
{
    CHashWriter h(SER_GETHASH, 0);
    h << llmqType;
    h << quorumHash;
    h << id;
    h << msgHash;
    return h.GetHash();
}

bool IsQuorumActive(Consensus::LLMQType llmqType, const CQuorumManager& qman, const uint256& quorumHash)
{
    // sig shares and recovered sigs are only accepted from recent/active quorums
    // we allow one more active quorum as specified in consensus, as otherwise there is a small window where things could
    // fail while we are on the brink of a new quorum
    const auto& llmq_params_opt = Params().GetLLMQ(llmqType);
    assert(llmq_params_opt.has_value());
    auto quorums = qman.ScanQuorums(llmqType, llmq_params_opt->keepOldConnections);
    return ranges::any_of(quorums, [&quorumHash](const auto& q){ return q->qc->quorumHash == quorumHash; });
}

} // namespace llmq
