// Copyright (c) 2018-2019 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "quorums.h"
#include "quorums_blockprocessor.h"
#include "quorums_commitment.h"
#include "quorums_dkgsession.h"
#include "quorums_dkgsessionmgr.h"
#include "quorums_init.h"
#include "quorums_utils.h"

#include "evo/specialtx.h"

#include "activemasternode.h"
#include "chainparams.h"
#include "init.h"
#include "univalue.h"
#include "validation.h"

#include "cxxtimer.hpp"

namespace llmq
{

static const std::string DB_QUORUM_SK_SHARE = "q_Qsk";
static const std::string DB_QUORUM_QUORUM_VVEC = "q_Qqvvec";

CQuorumManager* quorumManager;

static uint256 MakeQuorumKey(const CQuorum& q)
{
    CHashWriter hw(SER_NETWORK, 0);
    hw << (uint8_t)q.params.type;
    hw << q.quorumHash;
    for (const auto& dmn : q.members) {
        hw << dmn->proTxHash;
    }
    return hw.GetHash();
}

CQuorum::~CQuorum()
{
    // most likely the thread is already done
    stopCachePopulatorThread = true;
    if (cachePopulatorThread.joinable()) {
        cachePopulatorThread.join();
    }
}

void CQuorum::Init(const uint256& _quorumHash, int _height, const std::vector<CDeterministicMNCPtr>& _members, const std::vector<bool>& _validMembers, const CBLSPublicKey& _quorumPublicKey)
{
    quorumHash = _quorumHash;
    height = _height;
    members = _members;
    validMembers = _validMembers;
    quorumPublicKey = _quorumPublicKey;
}

bool CQuorum::IsMember(const uint256& proTxHash) const
{
    for (auto& dmn : members) {
        if (dmn->proTxHash == proTxHash) {
            return true;
        }
    }
    return false;
}

bool CQuorum::IsValidMember(const uint256& proTxHash) const
{
    for (size_t i = 0; i < members.size(); i++) {
        if (members[i]->proTxHash == proTxHash) {
            return validMembers[i];
        }
    }
    return false;
}

CBLSPublicKey CQuorum::GetPubKeyShare(size_t memberIdx) const
{
    if (quorumVvec == nullptr || memberIdx >= members.size() || !validMembers[memberIdx]) {
        return CBLSPublicKey();
    }
    auto& m = members[memberIdx];
    return blsCache.BuildPubKeyShare(m->proTxHash, quorumVvec, CBLSId::FromHash(m->proTxHash));
}

CBLSSecretKey CQuorum::GetSkShare() const
{
    return skShare;
}

int CQuorum::GetMemberIndex(const uint256& proTxHash) const
{
    for (size_t i = 0; i < members.size(); i++) {
        if (members[i]->proTxHash == proTxHash) {
            return (int)i;
        }
    }
    return -1;
}

void CQuorum::WriteContributions(CEvoDB& evoDb)
{
    uint256 dbKey = MakeQuorumKey(*this);

    if (quorumVvec != nullptr) {
        evoDb.GetRawDB().Write(std::make_pair(DB_QUORUM_QUORUM_VVEC, dbKey), *quorumVvec);
    }
    if (skShare.IsValid()) {
        evoDb.GetRawDB().Write(std::make_pair(DB_QUORUM_SK_SHARE, dbKey), skShare);
    }
}

bool CQuorum::ReadContributions(CEvoDB& evoDb)
{
    uint256 dbKey = MakeQuorumKey(*this);

    BLSVerificationVector qv;
    if (evoDb.Read(std::make_pair(DB_QUORUM_QUORUM_VVEC, dbKey), qv)) {
        quorumVvec = std::make_shared<BLSVerificationVector>(std::move(qv));
    } else {
        return false;
    }

    // We ignore the return value here as it is ok if this fails. If it fails, it usually means that we are not a
    // member of the quorum but observed the whole DKG process to have the quorum verification vector.
    evoDb.Read(std::make_pair(DB_QUORUM_SK_SHARE, dbKey), skShare);

    return true;
}

void CQuorum::StartCachePopulatorThread(std::shared_ptr<CQuorum> _this)
{
    if (_this->quorumVvec == nullptr) {
        return;
    }

    cxxtimer::Timer t(true);
    LogPrint("llmq", "CQuorum::StartCachePopulatorThread -- start\n");

    // this thread will exit after some time
    // when then later some other thread tries to get keys, it will be much faster
    _this->cachePopulatorThread = std::thread([_this, t]() {
        RenameThread("quorum-cachepop");
        for (size_t i = 0; i < _this->members.size() && !_this->stopCachePopulatorThread && !ShutdownRequested(); i++) {
            if (_this->validMembers[i]) {
                _this->GetPubKeyShare(i);
            }
        }
        LogPrint("llmq", "CQuorum::StartCachePopulatorThread -- done. time=%d\n", t.count());
    });
}

CQuorumManager::CQuorumManager(CEvoDB& _evoDb, CBLSWorker& _blsWorker, CDKGSessionManager& _dkgManager) :
    evoDb(_evoDb),
    blsWorker(_blsWorker),
    dkgManager(_dkgManager)
{
}

void CQuorumManager::UpdatedBlockTip(const CBlockIndex* pindexNew, const CBlockIndex* pindexFork, bool fInitialDownload)
{
    if (fInitialDownload) {
        return;
    }

    for (auto& p : Params().GetConsensus().llmqs) {
        EnsureQuorumConnections(p.first, pindexNew);
    }
}

void CQuorumManager::EnsureQuorumConnections(Consensus::LLMQType llmqType, const CBlockIndex* pindexNew)
{
    const auto& params = Params().GetConsensus().llmqs.at(llmqType);

    auto myProTxHash = activeMasternodeInfo.proTxHash;
    auto lastQuorums = ScanQuorums(llmqType, pindexNew, (size_t)params.keepOldConnections);

    auto connmanQuorumsToDelete = g_connman->GetMasternodeQuorums(llmqType);

    // don't remove connections for the currently in-progress DKG round
    int curDkgHeight = pindexNew->nHeight - (pindexNew->nHeight % params.dkgInterval);
    auto curDkgBlock = pindexNew->GetAncestor(curDkgHeight)->GetBlockHash();
    connmanQuorumsToDelete.erase(curDkgBlock);

    for (auto& quorum : lastQuorums) {
        if (!quorum->IsMember(myProTxHash) && !GetBoolArg("-watchquorums", DEFAULT_WATCH_QUORUMS)) {
            continue;
        }

        if (!g_connman->HasMasternodeQuorumNodes(llmqType, quorum->quorumHash)) {
            std::set<CService> connections;
            if (quorum->IsMember(myProTxHash)) {
                connections = CLLMQUtils::GetQuorumConnections(llmqType, quorum->quorumHash, myProTxHash);
            } else {
                auto cindexes = CLLMQUtils::CalcDeterministicWatchConnections(llmqType, quorum->quorumHash, quorum->members.size(), 1);
                for (auto idx : cindexes) {
                    connections.emplace(quorum->members[idx]->pdmnState->addr);
                }
            }
            if (!connections.empty()) {
                std::string debugMsg = strprintf("CQuorumManager::%s -- adding masternodes quorum connections for quorum %s:\n", __func__, quorum->quorumHash.ToString());
                for (auto& c : connections) {
                    debugMsg += strprintf("  %s\n", c.ToString(false));
                }
                LogPrint("llmq", debugMsg);
                g_connman->AddMasternodeQuorumNodes(llmqType, quorum->quorumHash, connections);
            }
        }
        connmanQuorumsToDelete.erase(quorum->quorumHash);
    }

    for (auto& qh : connmanQuorumsToDelete) {
        LogPrint("llmq", "CQuorumManager::%s -- removing masternodes quorum connections for quorum %s:\n", __func__, qh.ToString());
        g_connman->RemoveMasternodeQuorumNodes(llmqType, qh);
    }
}

bool CQuorumManager::BuildQuorumFromCommitment(const CFinalCommitment& qc, const CBlockIndex* pindexQuorum, std::shared_ptr<CQuorum>& quorum) const
{
    assert(pindexQuorum);
    assert(qc.quorumHash == pindexQuorum->GetBlockHash());

    auto members = CLLMQUtils::GetAllQuorumMembers((Consensus::LLMQType)qc.llmqType, qc.quorumHash);

    quorum->Init(qc.quorumHash, pindexQuorum->nHeight, members, qc.validMembers, qc.quorumPublicKey);

    bool hasValidVvec = false;
    if (quorum->ReadContributions(evoDb)) {
        hasValidVvec = true;
    } else {
        if (BuildQuorumContributions(qc, quorum)) {
            quorum->WriteContributions(evoDb);
            hasValidVvec = true;
        } else {
            LogPrint("llmq", "CQuorumManager::%s -- quorum.ReadContributions and BuildQuorumContributions for block %s failed\n", __func__, qc.quorumHash.ToString());
        }
    }

    if (hasValidVvec) {
        // pre-populate caches in the background
        // recovering public key shares is quite expensive and would result in serious lags for the first few signing
        // sessions if the shares would be calculated on-demand
        CQuorum::StartCachePopulatorThread(quorum);
    }

    return true;
}

bool CQuorumManager::BuildQuorumContributions(const CFinalCommitment& fqc, std::shared_ptr<CQuorum>& quorum) const
{
    std::vector<uint16_t> memberIndexes;
    std::vector<BLSVerificationVectorPtr> vvecs;
    BLSSecretKeyVector skContributions;
    if (!dkgManager.GetVerifiedContributions((Consensus::LLMQType)fqc.llmqType, fqc.quorumHash, fqc.validMembers, memberIndexes, vvecs, skContributions)) {
        return false;
    }

    BLSVerificationVectorPtr quorumVvec;
    CBLSSecretKey skShare;

    cxxtimer::Timer t2(true);
    quorumVvec = blsWorker.BuildQuorumVerificationVector(vvecs);
    if (quorumVvec == nullptr) {
        LogPrint("llmq", "CQuorumManager::%s -- failed to build quorumVvec\n", __func__);
        // without the quorum vvec, there can't be a skShare, so we fail here. Failure is not fatal here, as it still
        // allows to use the quorum as a non-member (verification through the quorum pub key)
        return false;
    }
    skShare = blsWorker.AggregateSecretKeys(skContributions);
    if (!skShare.IsValid()) {
        LogPrint("llmq", "CQuorumManager::%s -- failed to build skShare\n", __func__);
        // We don't bail out here as this is not a fatal error and still allows us to recover public key shares (as we
        // have a valid quorum vvec at this point)
    }
    t2.stop();

    LogPrint("llmq", "CQuorumManager::%s -- built quorum vvec and skShare. time=%d\n", __func__, t2.count());

    quorum->quorumVvec = quorumVvec;
    quorum->skShare = skShare;

    return true;
}

bool CQuorumManager::HasQuorum(Consensus::LLMQType llmqType, const uint256& quorumHash)
{
    return quorumBlockProcessor->HasMinedCommitment(llmqType, quorumHash);
}

std::vector<CQuorumCPtr> CQuorumManager::ScanQuorums(Consensus::LLMQType llmqType, size_t maxCount)
{
    const CBlockIndex* pindex;
    {
        LOCK(cs_main);
        pindex = chainActive.Tip();
    }
    return ScanQuorums(llmqType, pindex, maxCount);
}

std::vector<CQuorumCPtr> CQuorumManager::ScanQuorums(Consensus::LLMQType llmqType, const CBlockIndex* pindexStart, size_t maxCount)
{
    std::vector<CQuorumCPtr> result;
    result.reserve(maxCount);

    auto& params = Params().GetConsensus().llmqs.at(llmqType);

    auto firstQuorumHash = quorumBlockProcessor->GetFirstMinedQuorumHash(llmqType);
    if (firstQuorumHash.IsNull()) {
        // no quorum mined yet, avoid scanning the whole chain down to genesis
        return result;
    }

    auto pindex = pindexStart->GetAncestor(pindexStart->nHeight - (pindexStart->nHeight % params.dkgInterval));

    while (pindex != nullptr
            && pindex->nHeight >= params.dkgInterval
            && result.size() < maxCount
            && deterministicMNManager->IsDIP3Enforced(pindex->nHeight)) {
        auto quorum = GetQuorum(llmqType, pindex);
        if (quorum) {
            result.emplace_back(quorum);
        }

        if (pindex->GetBlockHash() == firstQuorumHash) {
            // no need to scan further if we know that there are no quorums below this block
            break;
        }

        pindex = pindex->GetAncestor(pindex->nHeight - params.dkgInterval);
    }

    return result;
}

CQuorumCPtr CQuorumManager::GetQuorum(Consensus::LLMQType llmqType, const uint256& quorumHash)
{
    CBlockIndex* pindexQuorum;
    {
        LOCK(cs_main);
        auto quorumIt = mapBlockIndex.find(quorumHash);

        if (quorumIt == mapBlockIndex.end()) {
            LogPrint("llmq", "CQuorumManager::%s -- block %s not found", __func__, quorumHash.ToString());
            return nullptr;
        }
        pindexQuorum = quorumIt->second;
    }
    return GetQuorum(llmqType, pindexQuorum);
}

CQuorumCPtr CQuorumManager::GetQuorum(Consensus::LLMQType llmqType, const CBlockIndex* pindexQuorum)
{
    assert(pindexQuorum);

    auto quorumHash = pindexQuorum->GetBlockHash();

    // we must check this before we look into the cache. Reorgs might have happened which would mean we might have
    // cached quorums which are not in the active chain anymore
    if (!HasQuorum(llmqType, quorumHash)) {
        return nullptr;
    }

    LOCK(quorumsCacheCs);

    auto it = quorumsCache.find(std::make_pair(llmqType, quorumHash));
    if (it != quorumsCache.end()) {
        return it->second;
    }

    CFinalCommitment qc;
    if (!quorumBlockProcessor->GetMinedCommitment(llmqType, quorumHash, qc)) {
        return nullptr;
    }

    auto& params = Params().GetConsensus().llmqs.at(llmqType);

    auto quorum = std::make_shared<CQuorum>(params, blsWorker);

    if (!BuildQuorumFromCommitment(qc, pindexQuorum, quorum)) {
        return nullptr;
    }

    quorumsCache.emplace(std::make_pair(llmqType, quorumHash), quorum);

    return quorum;
}

CQuorumCPtr CQuorumManager::GetNewestQuorum(Consensus::LLMQType llmqType)
{
    auto quorums = ScanQuorums(llmqType, 1);
    if (quorums.empty()) {
        return nullptr;
    }
    return quorums.front();
}

}
