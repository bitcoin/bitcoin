// Copyright (c) 2018-2024 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/quorums_signing.h>

#include <llmq/quorums_commitment.h>
#include <llmq/quorums.h>
#include <llmq/quorums_signing_shares.h>

#include <masternode/activemasternode.h>
#include <bls/bls_batchverifier.h>
#include <chainparams.h>
#include <cxxtimer.hpp>
#include <init.h>
#include <dbwrapper.h>
#include <hash.h>
#include <net_processing.h>
#include <netmessagemaker.h>
#include <scheduler.h>
#include <validation.h>
#include <timedata.h>

#include <algorithm>
#include <unordered_set>
#include <common/args.h>
#include <evo/deterministicmns.h>
#include <logging.h>
#include <util/thread.h>

namespace llmq
{
CSigningManager* quorumSigningManager;

UniValue CRecoveredSig::ToJson() const
{
    UniValue ret(UniValue::VOBJ);
    ret.pushKV("quorumHash", quorumHash.ToString());
    ret.pushKV("id", id.ToString());
    ret.pushKV("msgHash", msgHash.ToString());
    ret.pushKV("sig", sig.Get().ToString());
    ret.pushKV("hash", sig.Get().GetHash().ToString());
    return ret;
}


CRecoveredSigsDb::CRecoveredSigsDb(bool fMemory, bool fWipe) :
        db(std::make_unique<CDBWrapper>(DBParams{fMemory ? "" : (gArgs.GetDataDirNet() / "llmq/recsigdb"), 8 << 20, fMemory, fWipe}))
{
}

CRecoveredSigsDb::~CRecoveredSigsDb() = default;

bool CRecoveredSigsDb::HasRecoveredSig(const uint256& id, const uint256& msgHash) const
{
    auto k = std::make_tuple(std::string("rs_r"), id, msgHash);
    return db->Exists(k);
}

bool CRecoveredSigsDb::HasRecoveredSigForId(const uint256& id) const
{
    auto cacheKey = id;
    bool ret;
    {
        LOCK(cs_cache);
        if (hasSigForIdCache.get(cacheKey, ret)) {
            return ret;
        }
    }


    auto k = std::make_tuple(std::string("rs_r"), id);
    ret = db->Exists(k);

    LOCK(cs_cache);
    hasSigForIdCache.insert(cacheKey, ret);
    return ret;
}

bool CRecoveredSigsDb::HasRecoveredSigForSession(const uint256& signHash) const
{
    bool ret;
    {
        LOCK(cs_cache);
        if (hasSigForSessionCache.get(signHash, ret)) {
            return ret;
        }
    }

    auto k = std::make_tuple(std::string("rs_s"), signHash);
    ret = db->Exists(k);

    LOCK(cs_cache);
    hasSigForSessionCache.insert(signHash, ret);
    return ret;
}

bool CRecoveredSigsDb::HasRecoveredSigForHash(const uint256& hash) const
{
    bool ret;
    {
        LOCK(cs_cache);
        if (hasSigForHashCache.get(hash, ret)) {
            return ret;
        }
    }

    auto k = std::make_tuple(std::string("rs_h"), hash);
    ret = db->Exists(k);

    LOCK(cs_cache);
    hasSigForHashCache.insert(hash, ret);
    return ret;
}

bool CRecoveredSigsDb::ReadRecoveredSig(const uint256& id, CRecoveredSig& ret) const
{
    auto k = std::make_tuple(std::string("rs_r"), id);
    return db->Read(k, ret);

}

bool CRecoveredSigsDb::GetRecoveredSigByHash(const uint256& hash, CRecoveredSig& ret) const
{
    auto k1 = std::make_tuple(std::string("rs_h"), hash);
    uint256 k2;
    if (!db->Read(k1, k2)) {
        return false;
    }

    return ReadRecoveredSig(k2, ret);
}

bool CRecoveredSigsDb::GetRecoveredSigById(const uint256& id, CRecoveredSig& ret) const
{
    return ReadRecoveredSig(id, ret);
}

void CRecoveredSigsDb::WriteRecoveredSig(const llmq::CRecoveredSig& recSig)
{
    CDBBatch batch(*db);

    uint32_t curTime = GetTime<std::chrono::seconds>().count();

    // we put these close to each other to leverage leveldb's key compaction
    // this way, the second key can be used for fast HasRecoveredSig checks while the first key stores the recSig
    auto k1 = std::make_tuple(std::string("rs_r"), recSig.getId());
    auto k2 = std::make_tuple(std::string("rs_r"), recSig.getId(), recSig.getMsgHash());
    batch.Write(k1, recSig);
    // this key is also used to store the current time, so that we can easily get to the "rs_t" key when we have the id
    batch.Write(k2, curTime);

    // store by object hash
    auto k3 = std::make_tuple(std::string("rs_h"), recSig.GetHash());
    batch.Write(k3, recSig.getId());

    // store by signHash
    auto signHash = recSig.buildSignHash();
    auto k4 = std::make_tuple(std::string("rs_s"), signHash);
    batch.Write(k4, (uint8_t)1);

    // store by current time. Allows fast cleanup of old recSigs
    auto k5 = std::make_tuple(std::string("rs_t"), (uint32_t)htobe32_internal(curTime), recSig.getId());
    batch.Write(k5, (uint8_t)1);

    db->WriteBatch(batch);

    {
        LOCK(cs_cache);
        hasSigForIdCache.insert(recSig.getId(), true);
        hasSigForSessionCache.insert(signHash, true);
        hasSigForHashCache.insert(recSig.GetHash(), true);
    }
}

void CRecoveredSigsDb::RemoveRecoveredSig(CDBBatch& batch, const uint256& id, bool deleteHashKey, bool deleteTimeKey)
{
    CRecoveredSig recSig;
    if (!ReadRecoveredSig(id, recSig)) {
        return;
    }

    auto signHash = recSig.buildSignHash();

    auto k1 = std::make_tuple(std::string("rs_r"), recSig.getId());
    auto k2 = std::make_tuple(std::string("rs_r"), recSig.getId(), recSig.getMsgHash());
    auto k3 = std::make_tuple(std::string("rs_h"), recSig.GetHash());
    auto k4 = std::make_tuple(std::string("rs_s"), signHash);
    batch.Erase(k1);
    batch.Erase(k2);
    if (deleteHashKey) {
        batch.Erase(k3);
    }
    batch.Erase(k4);

    if (deleteTimeKey) {
        uint32_t writeTime;
        if (db->Read(k2, writeTime)) {
            auto k5 = std::make_tuple(std::string("rs_t"), (uint32_t) htobe32_internal(writeTime), recSig.getId());
            batch.Erase(k5);
        }
    }

    LOCK(cs_cache);
    hasSigForIdCache.erase(recSig.getId());
    hasSigForSessionCache.erase(signHash);
    if (deleteHashKey) {
        hasSigForHashCache.erase(recSig.GetHash());
    }
}

// Remove the recovered sig itself and all keys required to get from id -> recSig
// This will leave the byHash key in-place so that HasRecoveredSigForHash still returns true
void CRecoveredSigsDb::TruncateRecoveredSig(const uint256& id)
{
    CDBBatch batch(*db);
    RemoveRecoveredSig(batch, id, false, false);
    db->WriteBatch(batch);
}

void CRecoveredSigsDb::CleanupOldRecoveredSigs(int64_t maxAge)
{
    std::unique_ptr<CDBIterator> pcursor(db->NewIterator());

    auto start = std::make_tuple(std::string("rs_t"), (uint32_t)0, uint256());
    uint32_t endTime = (uint32_t)(GetTime<std::chrono::seconds>().count() - maxAge);
    pcursor->Seek(start);

    std::vector<uint256> toDelete;
    std::vector<decltype(start)> toDelete2;

    while (pcursor->Valid()) {
        decltype(start) k;

        if (!pcursor->GetKey(k) || std::get<0>(k) != "rs_t") {
            break;
        }
        if (be32toh_internal(std::get<1>(k)) >= endTime) {
            break;
        }

        toDelete.emplace_back(std::get<2>(k));
        toDelete2.emplace_back(k);

        pcursor->Next();
    }
    pcursor.reset();

    if (toDelete.empty()) {
        return;
    }

    CDBBatch batch(*db);
    for (const auto& e : toDelete) {
        RemoveRecoveredSig(batch, e, true, false);

        if (batch.SizeEstimate() >= (1 << 24)) {
            db->WriteBatch(batch);
            batch.Clear();
        }
    }

    for (const auto& e : toDelete2) {
        batch.Erase(e);
    }

    db->WriteBatch(batch);

    LogPrint(BCLog::LLMQ, "CRecoveredSigsDb::%d -- deleted %d entries\n", __func__, toDelete.size());
}

bool CRecoveredSigsDb::HasVotedOnId(const uint256& id) const
{
    auto k = std::make_tuple(std::string("rs_v"), id);
    return db->Exists(k);
}

bool CRecoveredSigsDb::GetVoteForId(const uint256& id, uint256& msgHashRet) const
{
    auto k = std::make_tuple(std::string("rs_v"), id);
    return db->Read(k, msgHashRet);
}

void CRecoveredSigsDb::WriteVoteForId(const uint256& id, const uint256& msgHash)
{
    auto k1 = std::make_tuple(std::string("rs_v"), id);
    auto k2 = std::make_tuple(std::string("rs_vt"), (uint32_t)htobe32_internal(GetTime<std::chrono::seconds>().count()), id);

    CDBBatch batch(*db);
    batch.Write(k1, msgHash);
    batch.Write(k2, (uint8_t)1);

    db->WriteBatch(batch);
}

void CRecoveredSigsDb::CleanupOldVotes(int64_t maxAge)
{
    std::unique_ptr<CDBIterator> pcursor(db->NewIterator());

    auto start = std::make_tuple(std::string("rs_vt"), (uint32_t)0, uint256());
    uint32_t endTime = (uint32_t)(GetTime<std::chrono::seconds>().count() - maxAge);
    pcursor->Seek(start);

    CDBBatch batch(*db);
    size_t cnt = 0;
    while (pcursor->Valid()) {
        decltype(start) k;

        if (!pcursor->GetKey(k) || std::get<0>(k) != "rs_vt") {
            break;
        }
        if (be32toh_internal(std::get<1>(k)) >= endTime) {
            break;
        }

        const uint256& id = std::get<2>(k);

        batch.Erase(k);
        batch.Erase(std::make_tuple(std::string("rs_v"), id));

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

CSigningManager::CSigningManager(bool fMemory, PeerManager& _peerman, ChainstateManager& _chainman, bool fWipe) :
    db(fMemory, fWipe),
    peerman(_peerman),
    chainman(_chainman)
{
}

bool CSigningManager::AlreadyHave(const uint256& hash) const
{
    {
        LOCK(cs_pending);
        if (pendingReconstructedRecoveredSigs.count(hash)) {
            return true;
        }
    }

    return db.HasRecoveredSigForHash(hash);
}

bool CSigningManager::GetRecoveredSigForGetData(const uint256& hash, CRecoveredSig& ret) const
{
    if (!db.GetRecoveredSigByHash(hash, ret)) {
        return false;
    }
    if (!IsQuorumActive(ret.getQuorumHash())) {
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

static bool PreVerifyRecoveredSig(const CRecoveredSig& recoveredSig, bool& retBan)
{
    retBan = false;

    auto quorum = quorumManager->GetQuorum(recoveredSig.getQuorumHash());

    if (!quorum) {
        LogPrint(BCLog::LLMQ, "CSigningManager::%s -- quorum %s not found\n", __func__,
                  recoveredSig.getQuorumHash().ToString());
        return false;
    }
    if (!IsQuorumActive(quorum->qc->quorumHash)) {
        return false;
    }

    return true;
}

void CSigningManager::ProcessMessageRecoveredSig(CNode* pfrom, const std::shared_ptr<const CRecoveredSig>& recoveredSig)
{
    const uint256& hash = recoveredSig->GetHash();
    PeerRef peer = peerman.GetPeerRef(pfrom->GetId());
    if (peer)
        peerman.AddKnownTx(*peer, hash);
    {
        LOCK(cs_main);
        peerman.ReceivedResponse(pfrom->GetId(), hash);
    }
    bool ban = false;
    if (!PreVerifyRecoveredSig(*recoveredSig, ban)) {
        if (ban) {
            {
                LOCK(cs_main);
                peerman.ForgetTxHash(pfrom->GetId(), hash);
            }
            if(peer)
                peerman.Misbehaving(*peer, 100, "error PreVerifyRecoveredSig");
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
    {
        LOCK(cs_main);
        peerman.ForgetTxHash(pfrom->GetId(), hash);
    }
    LogPrint(BCLog::LLMQ, "CSigningManager::%s -- signHash=%s, id=%s, msgHash=%s, node=%d\n", __func__,
             recoveredSig->buildSignHash().ToString(), recoveredSig->getId().ToString(), recoveredSig->getMsgHash().ToString(), pfrom->GetId());
    
    LOCK(cs_pending);
    if (pendingReconstructedRecoveredSigs.count(hash)) {
        // no need to perform full verification
        LogPrint(BCLog::LLMQ, "CSigningManager::%s -- already pending reconstructed sig, signHash=%s, id=%s, msgHash=%s, node=%d\n", __func__,
                recoveredSig->buildSignHash().ToString(), recoveredSig->getId().ToString(), recoveredSig->getMsgHash().ToString(), pfrom->GetId());
        return;
    }
    
    pendingRecoveredSigs[pfrom->GetId()].emplace_back(recoveredSig);
}

void CSigningManager::CollectPendingRecoveredSigsToVerify(
        size_t maxUniqueSessions,
        std::unordered_map<NodeId, std::list<std::shared_ptr<const CRecoveredSig>>>& retSigShares,
        std::unordered_map<uint256, CQuorumCPtr, StaticSaltedHasher>& retQuorums)
{
    {
        LOCK(cs_pending);
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

            auto quorumKey = recSig->getQuorumHash();
            if (!retQuorums.count(quorumKey)) {
                auto quorum = quorumManager->GetQuorum(recSig->getQuorumHash());
                if (!quorum) {
                    LogPrint(BCLog::LLMQ, "CSigningManager::%s -- quorum %s not found, node=%d\n", __func__,
                              recSig->getQuorumHash().ToString(), nodeId);
                    it = v.erase(it);
                    continue;
                }
                if (!IsQuorumActive(quorum->qc->quorumHash)) {
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
    WITH_LOCK(cs_pending, swap(m, pendingReconstructedRecoveredSigs));

    for (const auto& p : m) {
        ProcessRecoveredSig(-1, p.second);
    }
}

bool CSigningManager::ProcessPendingRecoveredSigs()
{
    std::unordered_map<NodeId, std::list<std::shared_ptr<const CRecoveredSig>>> recSigsByNode;
    std::unordered_map<uint256, CQuorumCPtr, StaticSaltedHasher> quorums;

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

            const auto& quorum = quorums.at(recSig->getQuorumHash());
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
        PeerRef peer = peerman.GetPeerRef(nodeId);

        if (batchVerifier.badSources.count(nodeId)) {
            LogPrint(BCLog::LLMQ, "CSigningManager::%s -- invalid recSig from other node, banning peer=%d\n", __func__, nodeId);
            if(peer)
                peerman.Misbehaving(*peer, 100, "invalid recSig from other node");
            continue;
        }

        for (const auto& recSig : v) {
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
    const uint256& hash = recoveredSig->GetHash();
    CInv inv(MSG_QUORUM_RECOVERED_SIG, hash);
    {
        PeerRef peer = peerman.GetPeerRef(nodeId);
        if (peer)
            peerman.AddKnownTx(*peer, hash);
        LOCK(cs_main);
        peerman.ReceivedResponse(nodeId, hash);
        // ChainLock-specific sanity checks must only apply to actual CLSIG request IDs.
        // BTCC recovered sigs intentionally sign at a different offset (+BTCCHECK_SIGN_OFFSET), so applying
        // the CL "mod-5" gate here would reject valid BTCC material and prevent aggregation.
        auto* pindex = chainman.m_blockman.LookupBlockIndex(recoveredSig->getMsgHash());
        if (pindex == nullptr) {
            LogPrintf("CSigningManager::%s -- block of recovered signature (%s) does not exist\n",
                    __func__, recoveredSig->getId().ToString());
            peerman.ForgetTxHash(nodeId, hash);
            if(peer)
                peerman.Misbehaving(*peer, 10, "invalid recovered signature");
            return;
        }
        bool is_chainlock_request = false;
        bool is_btccheck_request = false;
        const uint256 clsig_request_id = ::SerializeHash(std::make_tuple(std::string("clsig"), pindex->nHeight, recoveredSig->getQuorumHash()));
        is_chainlock_request = (clsig_request_id == recoveredSig->getId());
        const uint256 btcc_request_id = ::SerializeHash(std::make_tuple(std::string("btcc"), pindex->nHeight, recoveredSig->getQuorumHash()));
        is_btccheck_request = (btcc_request_id == recoveredSig->getId());
        if (!pindex->IsValid(BLOCK_VALID_SCRIPTS)) {
            LogPrintf("CSigningManager::%s -- block invalid. Block (%s) rejected\n",
                    __func__, pindex->ToString());
            peerman.ForgetTxHash(nodeId, hash);
            if(peer)
                peerman.Misbehaving(*peer, 10, "recovered signature of invalid block");
            return;
        }
        if (is_chainlock_request) {
            if((pindex->nHeight%SIGN_HEIGHT_OFFSET) != 0) {
                LogPrintf("CSigningManager::%s -- block height(%d) of recovered signature (%s) is not a factor of 5\n",
                        __func__, pindex->nHeight, recoveredSig->getId().ToString());
                peerman.ForgetTxHash(nodeId, hash);
                if(peer) peerman.Misbehaving(*peer, 10, "invalid recovered signature block height");
                return;
            }
        } else if (is_btccheck_request) {
            if ((pindex->nHeight % BTCCHECK_PERIOD) != BTCCHECK_SIGN_OFFSET) {
                peerman.ForgetTxHash(nodeId, hash);
                if (peer) peerman.Misbehaving(*peer, 10, "invalid btcc recovered signature height");
                return;
            }
        } else {
            // A recovered sig referring to a known block hash but not matching any known block-bound request ID
            // is unexpected. Drop it early to avoid pollution/DoS via arbitrary block-based signing sessions.
            LogPrint(BCLog::LLMQ, "CSigningManager::%s -- unexpected block-bound recovered signature id=%s height=%d msgHash=%s\n",
                     __func__, recoveredSig->getId().ToString(), pindex->nHeight, recoveredSig->getMsgHash().ToString());
            peerman.ForgetTxHash(nodeId, hash);
            if (peer) peerman.Misbehaving(*peer, 10, "unexpected block-bound recovered signature");
            return;
        }
    }

    if (db.HasRecoveredSigForHash(hash))
    {
        LOCK(cs_main);
        peerman.ForgetTxHash(nodeId, hash);
        return;
    }

    auto signHash = recoveredSig->buildSignHash();

    LogPrint(BCLog::LLMQ, "CSigningManager::%s -- valid recSig. signHash=%s, id=%s, msgHash=%s\n", __func__,
            signHash.ToString(), recoveredSig->getId().ToString(), recoveredSig->getMsgHash().ToString());
    if (db.HasRecoveredSigForId(recoveredSig->getId())) {
        CRecoveredSig otherRecoveredSig;
        if (db.GetRecoveredSigById(recoveredSig->getId(), otherRecoveredSig)) {
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
    WITH_LOCK(cs_pending, pendingReconstructedRecoveredSigs.erase(recoveredSig->GetHash()));
    if (fMasternodeMode) {
        peerman.RelayRecoveredSig(recoveredSig->GetHash());
    }

    auto listeners = WITH_LOCK(cs_listeners, return recoveredSigsListeners);
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
    LOCK(cs_pending);
    pendingReconstructedRecoveredSigs.emplace(std::piecewise_construct, std::forward_as_tuple(recoveredSig->GetHash()), std::forward_as_tuple(recoveredSig));
}

void CSigningManager::TruncateRecoveredSig(const uint256& id)
{
    db.TruncateRecoveredSig(id);
}

void CSigningManager::Cleanup()
{
    int64_t now = TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now());
    if (now - lastCleanupTime < 5000) {
        return;
    }

    int64_t maxAge = gArgs.GetIntArg("-maxrecsigsage", DEFAULT_MAX_RECOVERED_SIGS_AGE);

    db.CleanupOldRecoveredSigs(maxAge);
    db.CleanupOldVotes(maxAge);

    lastCleanupTime = TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now());
}

void CSigningManager::RegisterRecoveredSigsListener(CRecoveredSigsListener* l)
{
    LOCK(cs_listeners);
    recoveredSigsListeners.emplace_back(l);
}

void CSigningManager::UnregisterRecoveredSigsListener(CRecoveredSigsListener* l)
{
    LOCK(cs_listeners);
    auto itRem = std::remove(recoveredSigsListeners.begin(), recoveredSigsListeners.end(), l);
    recoveredSigsListeners.erase(itRem, recoveredSigsListeners.end());
}

bool CSigningManager::AsyncSignIfMember(const uint256& id, const uint256& msgHash, const uint256& quorumHash, bool allowReSign)
{
    if (!fMasternodeMode || WITH_LOCK(activeMasternodeInfoCs, return activeMasternodeInfo.proTxHash.IsNull())) {
        return false;
    }
    const auto quorum = [&]() {
        if (quorumHash.IsNull()) {
            // This might end up giving different results on different members
            // This might happen when we are on the brink of confirming a new quorum
            // This gives a slight risk of not getting enough shares to recover a signature
            // But at least it shouldn't be possible to get conflicting recovered signatures
            // TODO fix this by re-signing when the next block arrives, but only when that block results in a change of the quorum list and no recovered signature has been created in the mean time
            return SelectQuorumForSigning(chainman, id);
        } else {
            return quorumManager->GetQuorum(quorumHash);
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
        bool hasVoted = db.HasVotedOnId(id);
        if (hasVoted) {
            uint256 prevMsgHash;
            db.GetVoteForId(id, prevMsgHash);
            if (msgHash != prevMsgHash) {
                LogPrintf("CSigningManager::%s -- already voted for id=%s and msgHash=%s. Not voting on conflicting msgHash=%s\n",
                            __func__, id.ToString(), prevMsgHash.ToString(), msgHash.ToString());
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

        if (db.HasRecoveredSigForId(id)) {
            // no need to sign it if we already have a recovered sig
            return true;
        }
        if (!hasVoted) {
            db.WriteVoteForId(id, msgHash);
        }
    }

    if (allowReSign) {
        // make us re-announce all known shares (other nodes might have run into a timeout)
        quorumSigSharesManager->ForceReAnnouncement(quorum, id, msgHash);
    }
    quorumSigSharesManager->AsyncSign(quorum, id, msgHash);

    return true;
}

bool CSigningManager::HasRecoveredSig(const uint256& id, const uint256& msgHash) const
{
    return db.HasRecoveredSig(id, msgHash);
}

bool CSigningManager::HasRecoveredSigForId(const uint256& id) const
{
    return db.HasRecoveredSigForId(id);
}

bool CSigningManager::HasRecoveredSigForSession(const uint256& signHash) const
{
    return db.HasRecoveredSigForSession(signHash);
}

bool CSigningManager::GetRecoveredSigForId(const uint256& id, llmq::CRecoveredSig& retRecSig) const
{
    if (!db.GetRecoveredSigById(id, retRecSig)) {
        return false;
    }
    return true;
}

bool CSigningManager::IsConflicting(const uint256& id, const uint256& msgHash) const
{
    if (!db.HasRecoveredSigForId(id)) {
        // no recovered sig present, so no conflict
        return false;
    }

    if (!db.HasRecoveredSig(id, msgHash)) {
        // recovered sig is present, but not for the given msgHash. That's a conflict!
        return true;
    }

    // all good
    return false;
}

bool CSigningManager::GetVoteForId(const uint256& id, uint256& msgHashRet) const
{
    return db.GetVoteForId(id, msgHashRet);
}

void CSigningManager::StartWorkerThread()
{
    // can't start new thread if we have one running already
    if (workThread.joinable()) {
        assert(false);
    }

    workThread = std::thread(&util::TraceThread, "signingman", [this] { WorkThreadMain(); });
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
    return BuildSignHash(quorumHash, id, msgHash);
}

uint256 BuildSignHash(const uint256& quorumHash, const uint256& id, const uint256& msgHash)
{
    CHashWriter h(SER_GETHASH, 0);
    h << quorumHash;
    h << id;
    h << msgHash;
    return h.GetHash();
}

bool IsQuorumActive(const uint256& quorumHash)
{
    // sig shares and recovered sigs are only accepted from recent/active quorums
    // we allow one more active quorum as specified in consensus, as otherwise there is a small window where things could
    // fail while we are on the brink of a new quorum
    auto& params = Params().GetConsensus().llmqTypeChainLocks;
    auto quorums = quorumManager->ScanQuorums(params.keepOldConnections);
    return ranges::any_of(quorums, [&quorumHash](const auto& q){ return q->qc->quorumHash == quorumHash; });
}

} // namespace llmq
