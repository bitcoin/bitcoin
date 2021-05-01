// Copyright (c) 2018-2019 The Dash Core developers
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

static const std::string DB_QUORUM_SK_SHARE = "q_Qsk";
static const std::string DB_QUORUM_QUORUM_VVEC = "q_Qqvvec";

CQuorumManager* quorumManager;

static uint256 MakeQuorumKey(const CQuorum& q)
{
    CHashWriter hw(SER_NETWORK, 0);
    hw << q.params.type;
    hw << q.qc->quorumHash;
    for (const auto& dmn : q.members) {
        hw << dmn->proTxHash;
    }
    return hw.GetHash();
}

CQuorum::CQuorum(const Consensus::LLMQParams& _params, CBLSWorker& _blsWorker) : params(_params), blsCache(_blsWorker)
{
}

CQuorum::~CQuorum() = default;

void CQuorum::Init(const CFinalCommitmentPtr& _qc, const CBlockIndex* _pindexQuorum, const uint256& _minedBlockHash, const std::vector<CDeterministicMNCPtr>& _members)
{
    qc = _qc;
    pindexQuorum = _pindexQuorum;
    members = _members;
    minedBlockHash = _minedBlockHash;
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
            return qc->validMembers[i];
        }
    }
    return false;
}

CBLSPublicKey CQuorum::GetPubKeyShare(size_t memberIdx) const
{
    if (quorumVvec == nullptr || memberIdx >= members.size() || !qc->validMembers[memberIdx]) {
        return CBLSPublicKey();
    }
    auto& m = members[memberIdx];
    return blsCache.BuildPubKeyShare(m->proTxHash, quorumVvec, CBLSId::FromHash(m->proTxHash));
}

const CBLSSecretKey &CQuorum::GetSkShare() const
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

void CQuorum::WriteContributions()
{
    if(!evoDb)
        return;
    uint256 dbKey = MakeQuorumKey(*this);

    if (quorumVvec != nullptr) {
        evoDb->GetRawDB().Write(std::make_pair(DB_QUORUM_QUORUM_VVEC, dbKey), *quorumVvec);
    }
    if (skShare.IsValid()) {
        evoDb->GetRawDB().Write(std::make_pair(DB_QUORUM_SK_SHARE, dbKey), skShare);
    }
}

bool CQuorum::ReadContributions()
{
    if(!evoDb)
        return false;
    uint256 dbKey = MakeQuorumKey(*this);

    BLSVerificationVector qv;
    if (evoDb->Read(std::make_pair(DB_QUORUM_QUORUM_VVEC, dbKey), qv)) {
        quorumVvec = std::make_shared<BLSVerificationVector>(std::move(qv));
    } else {
        return false;
    }

    // We ignore the return value here as it is ok if this fails. If it fails, it usually means that we are not a
    // member of the quorum but observed the whole DKG process to have the quorum verification vector.
    evoDb->Read(std::make_pair(DB_QUORUM_SK_SHARE, dbKey), skShare);

    return true;
}

CQuorumManager::CQuorumManager(CBLSWorker& _blsWorker, CDKGSessionManager& _dkgManager) :
    blsWorker(_blsWorker),
    dkgManager(_dkgManager)
{
    CLLMQUtils::InitQuorumsCache(mapQuorumsCache);
    CLLMQUtils::InitQuorumsCache(scanQuorumsCache);
    quorumThreadInterrupt.reset();
}

CQuorumManager::~CQuorumManager()
{
    Stop();
}

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

    for (auto& p : Params().GetConsensus().llmqs) {
        EnsureQuorumConnections(p.first, pindexNew);
    }
}

void CQuorumManager::EnsureQuorumConnections(uint8_t llmqType, const CBlockIndex* pindexNew)
{
    const auto& params = Params().GetConsensus().llmqs.at(llmqType);
    auto myProTxHash = activeMasternodeInfo.proTxHash;
    std::vector<CQuorumCPtr> lastQuorums;
    ScanQuorums(llmqType, pindexNew, (size_t)params.keepOldConnections, lastQuorums);
    std::set<uint256> connmanQuorumsToDelete;
    dkgManager.connman.GetMasternodeQuorums(llmqType, connmanQuorumsToDelete);
    // don't remove connections for the currently in-progress DKG round
    int curDkgHeight = pindexNew->nHeight - (pindexNew->nHeight % params.dkgInterval);
    auto curDkgBlock = pindexNew->GetAncestor(curDkgHeight)->GetBlockHash();
    connmanQuorumsToDelete.erase(curDkgBlock);
    bool allowWatch = gArgs.GetBoolArg("-watchquorums", DEFAULT_WATCH_QUORUMS);
    for (auto& quorum : lastQuorums) {
        if (!quorum->IsMember(myProTxHash) && !allowWatch) {
            continue;
        }
        CLLMQUtils::EnsureQuorumConnections(llmqType, quorum->pindexQuorum, myProTxHash, allowWatch, dkgManager.connman);
        connmanQuorumsToDelete.erase(quorum->qc->quorumHash);
    }
    for (auto& qh : connmanQuorumsToDelete) {
        LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- removing masternodes quorum connections for quorum %s:\n", __func__, qh.ToString());
        dkgManager.connman.RemoveMasternodeQuorumNodes(llmqType, qh);
    }
}

bool CQuorumManager::BuildQuorumFromCommitment(const uint8_t llmqType, const CBlockIndex* pindexQuorum, std::shared_ptr<CQuorum>& quorum) const
{
    assert(pindexQuorum);

    const uint256& quorumHash{pindexQuorum->GetBlockHash()};
    uint256 minedBlockHash;
    CFinalCommitmentPtr qc = quorumBlockProcessor->GetMinedCommitment(llmqType, quorumHash, minedBlockHash);
    if (qc == nullptr) {
        return false;
    }
    assert(qc->quorumHash == pindexQuorum->GetBlockHash());

    std::vector<CDeterministicMNCPtr> members;
    CLLMQUtils::GetAllQuorumMembers(qc->llmqType, pindexQuorum, members);

    quorum->Init(qc, pindexQuorum, minedBlockHash, members);

    bool hasValidVvec = false;
    if (quorum->ReadContributions()) {
        hasValidVvec = true;
    } else {
        if (BuildQuorumContributions(qc, quorum)) {
            quorum->WriteContributions();
            hasValidVvec = true;
        } else {
            LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- quorum.ReadContributions and BuildQuorumContributions for block %s failed\n", __func__, qc->quorumHash.ToString());
        }
    }

    if (hasValidVvec) {
        // pre-populate caches in the background
        // recovering public key shares is quite expensive and would result in serious lags for the first few signing
        // sessions if the shares would be calculated on-demand
        StartCachePopulatorThread(quorum);
    }

    mapQuorumsCache[llmqType].emplace(quorumHash, quorum);

    return true;
}
bool CQuorumManager::BuildQuorumContributions(const CFinalCommitmentPtr& fqc, std::shared_ptr<CQuorum>& quorum) const
{
    std::vector<uint16_t> memberIndexes;
    std::vector<BLSVerificationVectorPtr> vvecs;
    BLSSecretKeyVector skContributions;
    if (!dkgManager.GetVerifiedContributions(fqc->llmqType, quorum->pindexQuorum, fqc->validMembers, memberIndexes, vvecs, skContributions)) {
        return false;
    }


    cxxtimer::Timer t2(true);
    quorum->quorumVvec = blsWorker.BuildQuorumVerificationVector(vvecs);
    if (quorum->quorumVvec == nullptr) {
        LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- failed to build quorumVvec\n", __func__);
        // without the quorum vvec, there can't be a skShare, so we fail here. Failure is not fatal here, as it still
        // allows to use the quorum as a non-member (verification through the quorum pub key)
        return false;
    }
	quorum->skShare = blsWorker.AggregateSecretKeys(skContributions);
    if (!quorum->skShare.IsValid()) {
        quorum->skShare.Reset();
        LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- failed to build skShare\n", __func__);
        // We don't bail out here as this is not a fatal error and still allows us to recover public key shares (as we
        // have a valid quorum vvec at this point)
    }
    t2.stop();

    LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- built quorum vvec and skShare. time=%d\n", __func__, t2.count());

    return true;
}

bool CQuorumManager::HasQuorum(uint8_t llmqType, const uint256& quorumHash)
{
    return quorumBlockProcessor->HasMinedCommitment(llmqType, quorumHash);
}

void CQuorumManager::ScanQuorums(uint8_t llmqType, size_t nCountRequested, std::vector<CQuorumCPtr>& quorums) const
{
    const CBlockIndex* pindex;
    {
        LOCK(cs_main);
        pindex = ::ChainActive().Tip();
    }
    ScanQuorums(llmqType, pindex, nCountRequested, quorums);
}

void CQuorumManager::ScanQuorums(uint8_t llmqType, const CBlockIndex* pindexStart, size_t nCountRequested, std::vector<CQuorumCPtr>& vecResultQuorums) const
{
    if (pindexStart == nullptr || nCountRequested == 0) {
        return;
    }

    bool fCacheExists{false};
    void* pIndexScanCommitments{(void*)pindexStart};
    size_t nScanCommitments{nCountRequested};

    {
        LOCK(quorumsCacheCs);
        auto& cache = scanQuorumsCache[llmqType];
        fCacheExists = cache.get(pindexStart->GetBlockHash(), vecResultQuorums);
        if (fCacheExists) {
            // We have exactly what requested so just return it
            if (vecResultQuorums.size() == nCountRequested) {
                return;
            }
            // If we have more cached than requested return only a subvector
            if (vecResultQuorums.size() > nCountRequested) {
                vecResultQuorums.resize(nCountRequested);
                return;
            }
            // If we have cached quorums but not enough, subtract what we have from the count and the set correct index where to start
            // scanning for the rests
            if(vecResultQuorums.size() > 0) {
                nScanCommitments -= vecResultQuorums.size();
                pIndexScanCommitments = (void*)vecResultQuorums.back()->pindexQuorum->pprev;
            }
        } else {
            // If there is nothing in cache request at least cache.max_size() because this gets cached then later
            nScanCommitments = std::max(nCountRequested, cache.max_size());
        }
    }
    // Get the block indexes of the mined commitments to build the required quorums from
    std::vector<const CBlockIndex*> quorumIndexes;
    quorumBlockProcessor->GetMinedCommitmentsUntilBlock(llmqType, (const CBlockIndex*)pIndexScanCommitments, nScanCommitments, quorumIndexes);
    vecResultQuorums.reserve(vecResultQuorums.size() + quorumIndexes.size());

    for (auto& quorumIndex : quorumIndexes) {
        assert(quorumIndex);
        auto quorum = GetQuorum(llmqType, quorumIndex);
        assert(quorum != nullptr);
        vecResultQuorums.emplace_back(quorum);
    }

    size_t nCountResult{vecResultQuorums.size()};
    if (nCountResult > 0 && !fCacheExists) {
        LOCK(quorumsCacheCs);
        // Don't cache more than cache.max_size() elements
        auto& cache = scanQuorumsCache[llmqType];
        size_t nCacheEndIndex = std::min(nCountResult, cache.max_size());
        cache.emplace(pindexStart->GetBlockHash(), {vecResultQuorums.begin(), vecResultQuorums.begin() + nCacheEndIndex});
    }
    // Don't return more than nCountRequested elements
    size_t nResultEndIndex = std::min(nCountResult, nCountRequested);
    vecResultQuorums.resize(nResultEndIndex);
}

CQuorumCPtr CQuorumManager::GetQuorum(uint8_t llmqType, const uint256& quorumHash) const
{
    CBlockIndex* pindexQuorum;
    {
        LOCK(cs_main);

        pindexQuorum = g_chainman.m_blockman.LookupBlockIndex(quorumHash);
        if (!pindexQuorum) {
            LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- block %s not found\n", __func__, quorumHash.ToString());
            return nullptr;
        }
    }
    return GetQuorum(llmqType, pindexQuorum);
}


CQuorumCPtr CQuorumManager::GetQuorum(uint8_t llmqType, const CBlockIndex* pindexQuorum) const
{
    assert(pindexQuorum);

    auto quorumHash = pindexQuorum->GetBlockHash();

    // we must check this before we look into the cache. Reorgs might have happened which would mean we might have
    // cached quorums which are not in the active chain anymore
    if (!HasQuorum(llmqType, quorumHash)) {
        return nullptr;
    }

    LOCK(quorumsCacheCs);
    CQuorumCPtr pQuorum;
    if (mapQuorumsCache[llmqType].get(quorumHash, pQuorum)) {
        return pQuorum;
    }

    auto& params = Params().GetConsensus().llmqs.at(llmqType);

    auto quorum = std::make_shared<CQuorum>(params, blsWorker);
    if(!BuildQuorumFromCommitment(llmqType, pindexQuorum, quorum)) {
        return nullptr;
    }
    return quorum;
}

void CQuorumManager::StartCachePopulatorThread(const CQuorumCPtr pQuorum) const
{
    if (pQuorum->quorumVvec == nullptr) {
        return;
    }

    cxxtimer::Timer t(true);
    LogPrint(BCLog::LLMQ, "CQuorumManager::StartCachePopulatorThread -- start\n");

    // when then later some other thread tries to get keys, it will be much faster
    workerPool.push([pQuorum, t, this](int threadId) {
        for (size_t i = 0; i < pQuorum->members.size() && !quorumThreadInterrupt; i++) {
            if (pQuorum->qc->validMembers[i]) {
                pQuorum->GetPubKeyShare(i);
            }
        }
        LogPrint(BCLog::LLMQ, "CQuorumManager::StartCachePopulatorThread -- done. time=%d\n", t.count());
    });
}

} // namespace llmq
