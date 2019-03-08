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

CRecoveredSigsDb::CRecoveredSigsDb(CDBWrapper& _db) :
    db(_db)
{
}

bool CRecoveredSigsDb::HasRecoveredSig(Consensus::LLMQType llmqType, const uint256& id, const uint256& msgHash)
{
    auto k = std::make_tuple('r', (uint8_t)llmqType, id, msgHash);
    return db.Exists(k);
}

bool CRecoveredSigsDb::HasRecoveredSigForId(Consensus::LLMQType llmqType, const uint256& id)
{
    int64_t t = GetTimeMillis();

    auto cacheKey = std::make_pair(llmqType, id);
    {
        LOCK(cs);
        auto it = hasSigForIdCache.find(cacheKey);
        if (it != hasSigForIdCache.end()) {
            it->second.second = t;
            return it->second.first;
        }
    }


    auto k = std::make_tuple('r', (uint8_t)llmqType, id);
    bool ret = db.Exists(k);

    LOCK(cs);
    hasSigForIdCache.emplace(cacheKey, std::make_pair(ret, t));
    return ret;
}

bool CRecoveredSigsDb::HasRecoveredSigForSession(const uint256& signHash)
{
    int64_t t = GetTimeMillis();

    {
        LOCK(cs);
        auto it = hasSigForSessionCache.find(signHash);
        if (it != hasSigForSessionCache.end()) {
            it->second.second = t;
            return it->second.first;
        }
    }

    auto k = std::make_tuple('s', signHash);
    bool ret = db.Exists(k);

    LOCK(cs);
    hasSigForSessionCache.emplace(signHash, std::make_pair(ret, t));
    return ret;
}

bool CRecoveredSigsDb::HasRecoveredSigForHash(const uint256& hash)
{
    int64_t t = GetTimeMillis();

    {
        LOCK(cs);
        auto it = hasSigForHashCache.find(hash);
        if (it != hasSigForHashCache.end()) {
            it->second.second = t;
            return it->second.first;
        }
    }

    auto k = std::make_tuple('h', hash);
    bool ret = db.Exists(k);

    LOCK(cs);
    hasSigForHashCache.emplace(hash, std::make_pair(ret, t));
    return ret;
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
    auto signHash = CLLMQUtils::BuildSignHash(recSig);
    auto k4 = std::make_tuple('s', signHash);
    batch.Write(k4, (uint8_t)1);

    // remove the votedForId entry as we won't need it anymore
    auto k5 = std::make_tuple('v', recSig.llmqType, recSig.id);
    batch.Erase(k5);

    // store by current time. Allows fast cleanup of old recSigs
    auto k6 = std::make_tuple('t', (uint32_t)GetAdjustedTime(), recSig.llmqType, recSig.id);
    batch.Write(k6, (uint8_t)1);

    db.WriteBatch(batch);

    {
        int64_t t = GetTimeMillis();

        LOCK(cs);
        hasSigForIdCache[std::make_pair((Consensus::LLMQType)recSig.llmqType, recSig.id)] = std::make_pair(true, t);
        hasSigForSessionCache[signHash] = std::make_pair(true, t);
        hasSigForHashCache[recSig.GetHash()] = std::make_pair(true, t);
    }
}

template<typename K, typename H>
static void TruncateCacheMap(std::unordered_map<K, std::pair<bool, int64_t>, H>& m, size_t maxSize, size_t truncateThreshold)
{
    typedef typename std::unordered_map<K, std::pair<bool, int64_t>, H> Map;
    typedef typename Map::iterator Iterator;

    if (m.size() <= truncateThreshold) {
        return;
    }

    std::vector<Iterator> vec;
    vec.reserve(m.size());
    for (auto it = m.begin(); it != m.end(); ++it) {
        vec.emplace_back(it);
    }
    // sort by last access time (descending order)
    std::sort(vec.begin(), vec.end(), [](const Iterator& it1, const Iterator& it2) {
        return it1->second.second > it2->second.second;
    });

    for (size_t i = maxSize; i < vec.size(); i++) {
        m.erase(vec[i]);
    }
}

void CRecoveredSigsDb::CleanupOldRecoveredSigs(int64_t maxAge)
{
    std::unique_ptr<CDBIterator> pcursor(db.NewIterator());

    static const uint256 maxUint256 = uint256S("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
    auto start = std::make_tuple('t', (uint32_t)0, (uint8_t)0, uint256());
    auto end = std::make_tuple('t', (uint32_t)(GetAdjustedTime() - maxAge), (uint8_t)255, maxUint256);
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
    {
        LOCK(cs);
        for (auto& e : toDelete) {
            CRecoveredSig recSig;
            if (!ReadRecoveredSig(e.first, e.second, recSig)) {
                continue;
            }

            auto signHash = CLLMQUtils::BuildSignHash(recSig);

            auto k1 = std::make_tuple('r', recSig.llmqType, recSig.id);
            auto k2 = std::make_tuple('r', recSig.llmqType, recSig.id, recSig.msgHash);
            auto k3 = std::make_tuple('h', recSig.GetHash());
            auto k4 = std::make_tuple('s', signHash);
            auto k5 = std::make_tuple('v', recSig.llmqType, recSig.id);
            batch.Erase(k1);
            batch.Erase(k2);
            batch.Erase(k3);
            batch.Erase(k4);
            batch.Erase(k5);

            hasSigForIdCache.erase(std::make_pair((Consensus::LLMQType)recSig.llmqType, recSig.id));
            hasSigForSessionCache.erase(signHash);
            hasSigForHashCache.erase(recSig.GetHash());
        }

        TruncateCacheMap(hasSigForIdCache, MAX_CACHE_SIZE, MAX_CACHE_TRUNCATE_THRESHOLD);
        TruncateCacheMap(hasSigForSessionCache, MAX_CACHE_SIZE, MAX_CACHE_TRUNCATE_THRESHOLD);
        TruncateCacheMap(hasSigForHashCache, MAX_CACHE_SIZE, MAX_CACHE_TRUNCATE_THRESHOLD);
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
        LogPrintf("CSigningManager::%s -- quorum %s not found, node=%d\n", __func__,
                  recoveredSig.quorumHash.ToString(), nodeId);
        return false;
    }
    if (!CLLMQUtils::IsQuorumActive(llmqType, quorum->quorumHash)) {
        return false;
    }

    if (!recoveredSig.sig.IsValid()) {
        retBan = true;
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
            const auto& quorum = quorums.at(std::make_pair((Consensus::LLMQType)recSig.llmqType, recSig.quorumHash));
            batchVerifier.PushMessage(nodeId, recSig.GetHash(), CLLMQUtils::BuildSignHash(recSig), recSig.sig, quorum->quorumPublicKey);
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

        LogPrintf("CSigningManager::%s -- valid recSig. signHash=%s, id=%s, msgHash=%s, node=%d\n", __func__,
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
    g_connman->RelayInv(inv);

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
                LogPrintf("CSigningManager::%s -- already voted for id=%s and msgHash=%s. Not voting again.\n", __func__,
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
        h << quorums[i]->quorumHash;
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

    uint256 signHash = CLLMQUtils::BuildSignHash(llmqParams.type, quorum->quorumHash, id, msgHash);
    return sig.VerifyInsecure(quorum->quorumPublicKey, signHash);
}

}
