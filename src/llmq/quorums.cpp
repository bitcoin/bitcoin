// Copyright (c) 2018-2021 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <llmq/quorums.h>
#include <llmq/quorums_blockprocessor.h>
#include <llmq/quorums_commitment.h>
#include <llmq/quorums_dkgsession.h>
#include <llmq/quorums_dkgsessionmgr.h>
#include <llmq/quorums_init.h>
#include <llmq/quorums_utils.h>

#include <evo/specialtx.h>
#include <evo/deterministicmns.h>

#include <masternode/activemasternode.h>
#include <chainparams.h>
#include <init.h>
#include <masternode/masternodesync.h>
#include <univalue.h>
#include <validation.h>
#include <shutdown.h>

#include <cxxtimer.hpp>
#include <net.h>
namespace llmq
{

// forward declaration to avoid circular dependency
uint256 BuildSignHash(const uint256& quorumHash, const uint256& id, const uint256& msgHash);

CQuorumManager* quorumManager;

static uint256 MakeQuorumKey(const CQuorum& q)
{
    CHashWriter hw(SER_NETWORK, 0);
    hw << q.qc->quorumHash;
    for (const auto& dmn : q.members) {
        hw << dmn->proTxHash;
    }
    return hw.GetHash();
}

CQuorum::CQuorum(CBLSWorker& _blsWorker) : blsCache(_blsWorker)
{
}

void CQuorum::Init(CFinalCommitmentPtr _qc, const CBlockIndex* _pQuorumBaseBlockIndex, const uint256& _minedBlockHash, Span<CDeterministicMNCPtr> _members)
{
    qc = std::move(_qc);
    m_quorum_base_block_index = _pQuorumBaseBlockIndex;
    members = std::vector(_members.begin(), _members.end());
    minedBlockHash = _minedBlockHash;
}

bool CQuorum::SetSecretKeyShare(const CBLSSecretKey& secretKeyShare)
{
    const auto proTxHash = WITH_LOCK(activeMasternodeInfoCs, return activeMasternodeInfo.proTxHash);
    if (!secretKeyShare.IsValid() || (secretKeyShare.GetPublicKey() != GetPubKeyShare(GetMemberIndex(proTxHash)))) {
        return false;
    }
    LOCK(cs_vvec_shShare);
    skShare = secretKeyShare;
    return true;
}
bool CQuorum::IsMember(const uint256& proTxHash) const
{
    return ranges::any_of(members, [&proTxHash](const auto& dmn){
        return dmn->proTxHash == proTxHash;
    });
}

bool CQuorum::IsValidMember(const uint256& proTxHash) const
{
    for (size_t i = 0; i < members.size(); i++) {
        if (members[i]->proTxHash == proTxHash) {
            return qc->validMembers[i];
        }
    }
    return false;
}

CBLSPublicKey CQuorum::GetPubKeyShare(size_t memberIdx) const
{
    LOCK(cs_vvec_shShare);
    if (!HasVerificationVectorInternal() || memberIdx >= members.size() || !qc->validMembers[memberIdx]) {
        return CBLSPublicKey();
    }
    const auto& m = members[memberIdx];
    return blsCache.BuildPubKeyShare(m->proTxHash, quorumVvec, CBLSId(m->proTxHash));
}

bool CQuorum::HasVerificationVector() const {
    LOCK(cs_vvec_shShare);
    return HasVerificationVectorInternal();
}

bool CQuorum::HasVerificationVectorInternal() const {
    AssertLockHeld(cs_vvec_shShare);
    return quorumVvec != nullptr;
}
CBLSSecretKey CQuorum::GetSkShare() const
{
    LOCK(cs_vvec_shShare);
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

void CQuorum::WriteContributions(std::unique_ptr<CEvoDB<uint256, std::vector<CBLSPublicKey>, StaticSaltedHasher>>& evoDb_vvec, std::unique_ptr<CEvoDB<uint256, CBLSSecretKey, StaticSaltedHasher>>& evoDb_sk)
{
    uint256 dbKey = MakeQuorumKey(*this);
    LOCK(cs_vvec_shShare);
    if (HasVerificationVectorInternal()) {
        evoDb_vvec->WriteCache(dbKey, *quorumVvec);
    }
    if (skShare.IsValid()) {
        evoDb_sk->WriteCache(dbKey, skShare);
    }
}

bool CQuorum::ReadContributions(std::unique_ptr<CEvoDB<uint256, std::vector<CBLSPublicKey>, StaticSaltedHasher>>& evoDb_vvec, std::unique_ptr<CEvoDB<uint256, CBLSSecretKey, StaticSaltedHasher>>& evoDb_sk)
{
    uint256 dbKey = MakeQuorumKey(*this);
    std::vector<CBLSPublicKey> qv;
    if (evoDb_vvec->ReadCache(dbKey, qv)) {
        WITH_LOCK(cs_vvec_shShare, quorumVvec = std::make_shared<std::vector<CBLSPublicKey>>(std::move(qv)));
    } else {
        return false;
    }

    // We ignore the return value here as it is ok if this fails. If it fails, it usually means that we are not a
    // member of the quorum but observed the whole DKG process to have the quorum verification vector.
    WITH_LOCK(cs_vvec_shShare, evoDb_sk->ReadCache(dbKey, skShare));
    return true;
}

CQuorumManager::CQuorumManager(const DBParams& db_params_vvecs, const DBParams& db_params_sk, CBLSWorker& _blsWorker, CDKGSessionManager& _dkgManager, ChainstateManager& _chainman) :
    blsWorker(_blsWorker),
    dkgManager(_dkgManager),
    chainman(_chainman),
    evoDb_vvec(std::make_unique<CEvoDB<uint256, std::vector<CBLSPublicKey>, StaticSaltedHasher>>(db_params_vvecs, QUORUM_CACHE_SIZE)),
    evoDb_sk(std::make_unique<CEvoDB<uint256, CBLSSecretKey, StaticSaltedHasher>>(db_params_sk, QUORUM_CACHE_SIZE))
{
    quorumThreadInterrupt.reset();
    vecQuorumsCache.reserve(QUORUM_CACHE_SIZE);
}

CQuorumManager::~CQuorumManager() { Stop(); }

void CQuorumManager::Start()
{
    int workerCount = std::thread::hardware_concurrency() / 2;
    workerCount = std::max(std::min(1, workerCount), 4);
    workerPool.resize(workerCount);
}

void CQuorumManager::Stop()
{
    quorumThreadInterrupt();
    workerPool.clear_queue();
    workerPool.stop(true);
}

void CQuorumManager::UpdatedBlockTip(const CBlockIndex* pindexNew, bool fInitialDownload)
{
    if (!masternodeSync.IsBlockchainSynced()) {
        return;
    }
    EnsureQuorumConnections(pindexNew);
}

void CQuorumManager::EnsureQuorumConnections(const CBlockIndex* pindexNew)
{
    if (!fMasternodeMode && !CLLMQUtils::IsWatchQuorumsEnabled()) return;
    const Consensus::LLMQParams& llmqParams = Params().GetConsensus().llmqTypeChainLocks;
    auto lastQuorums = ScanQuorums(pindexNew, (size_t)llmqParams.keepOldConnections);

    auto connmanQuorumsToDelete = dkgManager.connman.GetMasternodeQuorums();

    // don't remove connections for the currently in-progress DKG round
    int curDkgHeight = pindexNew->nHeight - (pindexNew->nHeight % llmqParams.dkgInterval);
    auto curDkgBlock = pindexNew->GetAncestor(curDkgHeight)->GetBlockHash();
    connmanQuorumsToDelete.erase(curDkgBlock);
    LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- h[%d] keeping mn quorum connections for quorum: [%d:%s]\n", __func__,  pindexNew->nHeight, curDkgHeight, curDkgBlock.ToString());

    for (const auto& quorum : lastQuorums) {
        if (CLLMQUtils::EnsureQuorumConnections(quorum->m_quorum_base_block_index, WITH_LOCK(activeMasternodeInfoCs, return activeMasternodeInfo.proTxHash), dkgManager.connman)) {
            if (connmanQuorumsToDelete.erase(quorum->qc->quorumHash) > 0) {
                LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- h[%d] keeping mn quorum connections for quorum: [%d:%s]\n", __func__, pindexNew->nHeight, quorum->m_quorum_base_block_index->nHeight, quorum->m_quorum_base_block_index->GetBlockHash().ToString());
            }
        }
        if (connmanQuorumsToDelete.count(quorum->qc->quorumHash) > 0) {
            LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- removing masternodes quorum connections for quorum %s:\n", __func__, quorum->qc->quorumHash.ToString());
            dkgManager.connman.RemoveMasternodeQuorumNodes(quorum->qc->quorumHash);
        }
    }
    for (const auto& quorumHash : connmanQuorumsToDelete) {
        LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- removing masternodes quorum connections for quorum %s:\n", __func__, quorumHash.ToString());
        dkgManager.connman.RemoveMasternodeQuorumNodes(quorumHash);
    }
}

CQuorumPtr CQuorumManager::BuildQuorumFromCommitment(const CBlockIndex* pQuorumBaseBlockIndex)
{
    assert(pQuorumBaseBlockIndex);

    const uint256& quorumHash{pQuorumBaseBlockIndex->GetBlockHash()};
    uint256 minedBlockHash;
    CFinalCommitmentPtr qc = quorumBlockProcessor->GetMinedCommitment(quorumHash, minedBlockHash);
    if (qc == nullptr) {
        LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- No mined commitment for nHeight[%d] quorumHash[%s]\n", __func__, pQuorumBaseBlockIndex->nHeight, pQuorumBaseBlockIndex->GetBlockHash().ToString());
        return nullptr;
    }
    assert(qc->quorumHash == pQuorumBaseBlockIndex->GetBlockHash());

    auto quorum = std::make_shared<CQuorum>(blsWorker);
    auto members = CLLMQUtils::GetAllQuorumMembers(pQuorumBaseBlockIndex);

    quorum->Init(std::move(qc), pQuorumBaseBlockIndex, minedBlockHash, members);

    bool hasValidVvec = false;
    if (WITH_LOCK(cs_db, return quorum->ReadContributions(evoDb_vvec, evoDb_sk))) {
        hasValidVvec = true;
    } else {
        if (BuildQuorumContributions(quorum->qc, quorum)) {
            WITH_LOCK(cs_db, quorum->WriteContributions(evoDb_vvec, evoDb_sk));
            hasValidVvec = true;
        } else {
            LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- quorum.ReadContributions and BuildQuorumContributions for quorumHash[%s] failed\n", __func__, quorum->qc->quorumHash.ToString());
        }
    }

    if (hasValidVvec) {
        // pre-populate caches in the background
        // recovering public key shares is quite expensive and would result in serious lags for the first few signing
        // sessions if the shares would be calculated on-demand
        StartCachePopulatorThread(quorum);
    }
    {
        LOCK(cs_quorums);
        if(vecQuorumsCache.size() >= QUORUM_CACHE_SIZE) {
            vecQuorumsCache.erase(vecQuorumsCache.begin());
        }
        vecQuorumsCache.emplace_back(quorum);
    }

    return quorum;
}

bool CQuorumManager::BuildQuorumContributions(const CFinalCommitmentPtr& fqc, const std::shared_ptr<CQuorum>& quorum) const
{
    std::vector<uint16_t> memberIndexes;
    std::vector<BLSVerificationVectorPtr> vvecs;
    std::vector<CBLSSecretKey> skContributions;
    if (!dkgManager.GetVerifiedContributions(quorum->m_quorum_base_block_index, fqc->validMembers, memberIndexes, vvecs, skContributions)) {
        return false;
    }

    cxxtimer::Timer t2(true);
    quorum->SetVerificationVector(blsWorker.BuildQuorumVerificationVector(vvecs));
    if (!quorum->HasVerificationVector()) {
        LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- failed to build quorumVvec\n", __func__);
        // without the quorum vvec, there can't be a skShare, so we fail here. Failure is not fatal here, as it still
        // allows to use the quorum as a non-member (verification through the quorum pub key)
        return false;
    }
    if (!quorum->SetSecretKeyShare(blsWorker.AggregateSecretKeys(skContributions))) {
        LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- failed to build skShare\n", __func__);
        // We don't bail out here as this is not a fatal error and still allows us to recover public key shares (as we
        // have a valid quorum vvec at this point)
    }
    t2.stop();

    LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- built quorum vvec and skShare. time=%d\n", __func__, t2.count());

    return true;
}

bool CQuorumManager::HasQuorum(const uint256& quorumHash)
{
    return quorumBlockProcessor->HasMinedCommitment(quorumHash);
}

std::vector<CQuorumCPtr> CQuorumManager::ScanQuorums(size_t nCountRequested)
{
    const CBlockIndex* pindex = WITH_LOCK(cs_main, return chainman.ActiveTip());
    return ScanQuorums(pindex, nCountRequested);
}

std::vector<CQuorumCPtr> CQuorumManager::ScanQuorums(const CBlockIndex* pindexStart, size_t nCountRequested)
{
    if (pindexStart == nullptr || nCountRequested == 0) {
        return {};
    }

    const auto &nDKGInterval = Params().GetConsensus().llmqTypeChainLocks.dkgInterval;
    std::vector<CQuorumCPtr> vecResultQuorums;
    vecResultQuorums.reserve(nCountRequested);

    const CBlockIndex* pIndex = pindexStart;
    // Adjust pindexStart to the nearest previous block that aligns with the dkgInterval boundary
    int offset = pIndex->nHeight % nDKGInterval;
    if (offset != 0) {
        pIndex = pIndex->GetAncestor(pIndex->nHeight - offset);
    }

    // Iterate through blocks using dkgInterval to gather the required number of quorums
    while (vecResultQuorums.size() < nCountRequested && pIndex != nullptr) {
        CQuorumCPtr quorum = GetQuorum(pIndex);
        if (quorum != nullptr) {
            vecResultQuorums.emplace_back(quorum);
        }
        // Move to the previous block at the interval of nDKGInterval
        pIndex = pIndex->GetAncestor(pIndex->nHeight - nDKGInterval);
    }
    
    return vecResultQuorums;
}



CQuorumCPtr CQuorumManager::GetQuorum(const uint256& quorumHash)
{
    CBlockIndex* pQuorumBaseBlockIndex = WITH_LOCK(cs_main, return chainman.m_blockman.LookupBlockIndex(quorumHash));
    if (!pQuorumBaseBlockIndex) {
        LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- block %s not found\n", __func__, quorumHash.ToString());
        return nullptr;
    }
    return GetQuorum(pQuorumBaseBlockIndex);
}

std::vector<CQuorumCPtr>::iterator CQuorumManager::FindQuorumByHash(const uint256& blockHash) {
    AssertLockHeld(cs_quorums);
    return std::find_if(vecQuorumsCache.begin(), vecQuorumsCache.end(), [&blockHash](const CQuorumCPtr& quorum) {
        return quorum->m_quorum_base_block_index->GetBlockHash() == blockHash;
    });
}

CQuorumCPtr CQuorumManager::GetQuorum(const CBlockIndex* pQuorumBaseBlockIndex)
{
    assert(pQuorumBaseBlockIndex);

    auto quorumHash = pQuorumBaseBlockIndex->GetBlockHash();

    // we must check this before we look into the cache. Reorgs might have happened which would mean we might have
    // cached quorums which are not in the active chain anymore
    if (!HasQuorum(quorumHash)) {
        return nullptr;
    }
    {
        LOCK(cs_quorums);
        auto it = FindQuorumByHash(quorumHash);
        if (it != vecQuorumsCache.end()) {
            return *it;
        }
    }
    return BuildQuorumFromCommitment(pQuorumBaseBlockIndex);
}

void CQuorumManager::StartCachePopulatorThread(const CQuorumCPtr pQuorum) const
{
    if (!pQuorum->HasVerificationVector()) {
        return;
    }

    cxxtimer::Timer t(true);
    LogPrint(BCLog::LLMQ, "CQuorumManager::StartCachePopulatorThread -- start\n");

    // when then later some other thread tries to get keys, it will be much faster
    workerPool.push([pQuorum, t, this](int threadId) {
        for (size_t i = 0; i < pQuorum->members.size(); i++) {
            if (quorumThreadInterrupt) {
                break;
            }
            if (pQuorum->qc->validMembers[i]) {
                pQuorum->GetPubKeyShare(i);
            }
        }
        LogPrint(BCLog::LLMQ, "CQuorumManager::StartCachePopulatorThread -- done. time=%d\n", t.count());
    });
}

bool CQuorumManager::DoMaintenance(bool bForceFlush, bool fSync) {
    {
        LOCK(evoDb_vvec->cs);
        if (evoDb_vvec->IsCacheFull()) {
            evoDb_vvec->ResetDB();
            bForceFlush = true;
            LogPrint(BCLog::SYS, "CQuorumManager::DoMaintenance evoDb_vvec Database successfully wiped and recreated.\n");
        }
        if(bForceFlush) {
            if(!evoDb_vvec->FlushCacheToDisk(/*CHUNK_ITEMS=*/256, fSync)) {
                return false;
            }
        }
    }
    {
        LOCK(evoDb_sk->cs);
        if (evoDb_sk->IsCacheFull()) {
            evoDb_sk->ResetDB();
            bForceFlush = true;
            LogPrint(BCLog::SYS, "CQuorumManager::DoMaintenance evoDb_sk Database successfully wiped and recreated.\n");
        }
        if(bForceFlush) {
            if(!evoDb_sk->FlushCacheToDisk(/*CHUNK_ITEMS=*/256, fSync)) {
                return false;
            }
        }
    }
    return true;
}
bool CQuorumManager::FlushCacheToDisk(bool bForceFlush, bool fSync) {
    return DoMaintenance(bForceFlush, fSync);
}

CQuorumCPtr SelectQuorumForSigning(ChainstateManager& chainman,
                                   const uint256& selectionHash, int signHeight, int signOffset)
{
    const auto& llmqParams = Params().GetConsensus().llmqTypeChainLocks;
    size_t poolSize = llmqParams.signingActiveQuorumCount;

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

   
    auto quorums = quorumManager->ScanQuorums(pindexStart, poolSize);
    if (quorums.empty()) {
        return nullptr;
    }

    std::vector<std::pair<uint256, size_t>> scores;
    scores.reserve(quorums.size());
    for (size_t i = 0; i < quorums.size(); i++) {
        CHashWriter h(SER_NETWORK, 0);
        h << quorums[i]->qc->quorumHash;
        h << selectionHash;
        scores.emplace_back(h.GetHash(), i);
    }
    std::sort(scores.begin(), scores.end());
    return quorums[scores.front().second];
    
}

VerifyRecSigStatus VerifyRecoveredSig(ChainstateManager& chainman,
                        int signedAtHeight, const uint256& id, const uint256& msgHash, const CBLSSignature& sig,
                        const int signOffset)
{
    auto quorum = SelectQuorumForSigning(chainman, id, signedAtHeight, signOffset);
    if (!quorum) {
        return VerifyRecSigStatus::NoQuorum;
    }

    uint256 signHash = BuildSignHash(quorum->qc->quorumHash, id, msgHash);
    const bool ret = sig.VerifyInsecure(quorum->qc->quorumPublicKey, signHash);
    return ret ? VerifyRecSigStatus::Valid : VerifyRecSigStatus::Invalid;
}
} // namespace llmq
