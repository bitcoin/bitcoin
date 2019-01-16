// Copyright (c) 2018 The Dash Core developers
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

bool CQuorum::WriteContributions(CEvoDB& evoDb)
{
    uint256 dbKey = MakeQuorumKey(*this);

    if (quorumVvec != nullptr) {
        evoDb.GetRawDB().Write(std::make_pair(DB_QUORUM_QUORUM_VVEC, dbKey), *quorumVvec);
    }
    if (skShare.IsValid()) {
        evoDb.GetRawDB().Write(std::make_pair(DB_QUORUM_SK_SHARE, dbKey), skShare);
    }
    return true;
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
    LogPrintf("CQuorum::StartCachePopulatorThread -- start\n");

    // this thread will exit after some time
    // when then later some other thread tries to get keys, it will be much faster
    _this->cachePopulatorThread = std::thread([_this, t]() {
        RenameThread("quorum-cachepop");
        for (size_t i = 0; i < _this->members.size() && !_this->stopCachePopulatorThread && !ShutdownRequested(); i++) {
            if (_this->validMembers[i]) {
                _this->GetPubKeyShare(i);
            }
        }
        LogPrintf("CQuorum::StartCachePopulatorThread -- done. time=%d\n", t.count());
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

    LOCK(cs_main);

    for (auto& p : Params().GetConsensus().llmqs) {
        EnsureQuorumConnections(p.first, pindexNew);
    }
}

void CQuorumManager::EnsureQuorumConnections(Consensus::LLMQType llmqType, const CBlockIndex* pindexNew)
{
    AssertLockHeld(cs_main);

    const auto& params = Params().GetConsensus().llmqs.at(llmqType);

    auto myProTxHash = activeMasternodeInfo.proTxHash;
    auto lastQuorums = ScanQuorums(llmqType, (size_t)params.keepOldConnections);

    auto connmanQuorumsToDelete = g_connman->GetMasternodeQuorums(llmqType);

    // don't remove connections for the currently in-progress DKG round
    int curDkgHeight = pindexNew->nHeight - (pindexNew->nHeight % params.dkgInterval);
    auto curDkgBlock = chainActive[curDkgHeight]->GetBlockHash();
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
                LogPrintf(debugMsg);
                g_connman->AddMasternodeQuorumNodes(llmqType, quorum->quorumHash, connections);
            }
        }
        connmanQuorumsToDelete.erase(quorum->quorumHash);
    }

    for (auto& qh : connmanQuorumsToDelete) {
        LogPrintf("CQuorumManager::%s -- removing masternodes quorum connections for quorum %s:\n", __func__, qh.ToString());
        g_connman->RemoveMasternodeQuorumNodes(llmqType, qh);
    }
}

bool CQuorumManager::BuildQuorumFromCommitment(const CFinalCommitment& qc, std::shared_ptr<CQuorum>& quorum) const
{
    AssertLockHeld(cs_main);

    if (!mapBlockIndex.count(qc.quorumHash)) {
        LogPrintf("CQuorumManager::%s -- block %s not found", __func__, qc.quorumHash.ToString());
        return false;
    }
    auto& params = Params().GetConsensus().llmqs.at((Consensus::LLMQType)qc.llmqType);
    auto quorumIndex = mapBlockIndex[qc.quorumHash];
    auto members = CLLMQUtils::GetAllQuorumMembers((Consensus::LLMQType)qc.llmqType, qc.quorumHash);

    quorum->Init(qc.quorumHash, quorumIndex->nHeight, members, qc.validMembers, qc.quorumPublicKey);

    if (!quorum->ReadContributions(evoDb)) {
        if (BuildQuorumContributions(qc, quorum)) {
            quorum->WriteContributions(evoDb);
        } else {
            LogPrintf("CQuorumManager::%s -- quorum.ReadContributions and BuildQuorumContributions for block %s failed", __func__, qc.quorumHash.ToString());
        }
    }

    // pre-populate caches in the background
    // recovering public key shares is quite expensive and would result in serious lags for the first few signing
    // sessions if the shares would be calculated on-demand
    CQuorum::StartCachePopulatorThread(quorum);

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
        LogPrintf("CQuorumManager::%s -- failed to build quorumVvec\n", __func__);
        // without the quorum vvec, there can't be a skShare, so we fail here. Failure is not fatal here, as it still
        // allows to use the quorum as a non-member (verification through the quorum pub key)
        return false;
    }
    skShare = blsWorker.AggregateSecretKeys(skContributions);
    if (!skShare.IsValid()) {
        LogPrintf("CQuorumManager::%s -- failed to build skShare\n", __func__);
        // We don't bail out here as this is not a fatal error and still allows us to recover public key shares (as we
        // have a valid quorum vvec at this point)
    }
    t2.stop();

    LogPrintf("CQuorumManager::%s -- built quorum vvec and skShare. time=%d\n", __func__, t2.count());

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
    LOCK(cs_main);
    return ScanQuorums(llmqType, chainActive.Tip()->GetBlockHash(), maxCount);
}

std::vector<CQuorumCPtr> CQuorumManager::ScanQuorums(Consensus::LLMQType llmqType, const uint256& startBlock, size_t maxCount)
{
    std::vector<CQuorumCPtr> result;

    LOCK(cs_main);
    if (!mapBlockIndex.count(startBlock)) {
        return result;
    }

    result.reserve(maxCount);

    CBlockIndex* pindex = mapBlockIndex[startBlock];

    while (pindex != NULL && result.size() < maxCount && deterministicMNManager->IsDIP3Active(pindex->nHeight)) {
        if (HasQuorum(llmqType, pindex->GetBlockHash())) {
            auto quorum = GetQuorum(llmqType, pindex->GetBlockHash());
            if (quorum) {
                result.emplace_back(quorum);
            }
        }

        // TODO speedup (skip blocks where no quorums could have been mined)
        pindex = pindex->pprev;
    }

    return result;
}

CQuorumCPtr CQuorumManager::SelectQuorum(Consensus::LLMQType llmqType, const uint256& selectionHash, size_t poolSize)
{
    LOCK(cs_main);
    return SelectQuorum(llmqType, chainActive.Tip()->GetBlockHash(), selectionHash, poolSize);
}

CQuorumCPtr CQuorumManager::SelectQuorum(Consensus::LLMQType llmqType, const uint256& startBlock, const uint256& selectionHash, size_t poolSize)
{
    auto quorums = ScanQuorums(llmqType, startBlock, poolSize);
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

CQuorumCPtr CQuorumManager::GetQuorum(Consensus::LLMQType llmqType, const uint256& quorumHash)
{
    AssertLockHeld(cs_main);

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
    if (!BuildQuorumFromCommitment(qc, quorum)) {
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
