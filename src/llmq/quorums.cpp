// Copyright (c) 2018-2019 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/quorums.h>
#include <llmq/quorums_blockprocessor.h>
#include <llmq/quorums_dkgsession.h>
#include <llmq/quorums_dkgsessionmgr.h>
#include <llmq/quorums_init.h>
#include <llmq/quorums_utils.h>

#include <evo/specialtx.h>

#include <masternode/activemasternode.h>
#include <chainparams.h>
#include <init.h>
#include <masternode/masternode-sync.h>
#include <univalue.h>
#include <validation.h>

#include <cxxtimer.hpp>

namespace llmq
{

static const std::string DB_QUORUM_SK_SHARE = "q_Qsk";
static const std::string DB_QUORUM_QUORUM_VVEC = "q_Qqvvec";

CQuorumManager* quorumManager;

static uint256 MakeQuorumKey(const CQuorum& q)
{
    CHashWriter hw(SER_NETWORK, 0);
    hw << q.params.type;
    hw << q.qc.quorumHash;
    for (const auto& dmn : q.members) {
        hw << dmn->proTxHash;
    }
    return hw.GetHash();
}

CQuorum::~CQuorum()
{
    // most likely the thread is already done
    stopCachePopulatorThread = true;
    // watch out to not join the thread when we're called from inside the thread, which might happen on shutdown. This
    // is because on shutdown the thread is the last owner of the shared CQuorum instance and thus the destroyer of it.
    if (cachePopulatorThread.joinable() && cachePopulatorThread.get_id() != std::this_thread::get_id()) {
        cachePopulatorThread.join();
    }
}

void CQuorum::Init(const CFinalCommitment& _qc, const CBlockIndex* _pindexQuorum, const uint256& _minedBlockHash, const std::vector<CDeterministicMNCPtr>& _members)
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
            return qc.validMembers[i];
        }
    }
    return false;
}

CBLSPublicKey CQuorum::GetPubKeyShare(size_t memberIdx) const
{
    if (quorumVvec == nullptr || memberIdx >= members.size() || !qc.validMembers[memberIdx]) {
        return CBLSPublicKey();
    }
    auto& m = members[memberIdx];
    return blsCache.BuildPubKeyShare(m->proTxHash, quorumVvec, CBLSId(m->proTxHash));
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
    LogPrint(BCLog::LLMQ, "CQuorum::StartCachePopulatorThread -- start\n");

    // this thread will exit after some time
    // when then later some other thread tries to get keys, it will be much faster
    _this->cachePopulatorThread = std::thread([_this, t]() {
        RenameThread("dash-q-cachepop");
        for (size_t i = 0; i < _this->members.size() && !_this->stopCachePopulatorThread && !ShutdownRequested(); i++) {
            if (_this->qc.validMembers[i]) {
                _this->GetPubKeyShare(i);
            }
        }
        LogPrint(BCLog::LLMQ, "CQuorum::StartCachePopulatorThread -- done. time=%d\n", t.count());
    });
}

CQuorumManager::CQuorumManager(CEvoDB& _evoDb, CBLSWorker& _blsWorker, CDKGSessionManager& _dkgManager) :
    evoDb(_evoDb),
    blsWorker(_blsWorker),
    dkgManager(_dkgManager)
{
}

void CQuorumManager::UpdatedBlockTip(const CBlockIndex* pindexNew, bool fInitialDownload) const
{
    if (!masternodeSync.IsBlockchainSynced()) {
        return;
    }

    for (auto& p : Params().GetConsensus().llmqs) {
        EnsureQuorumConnections(p.first, pindexNew);
    }
}

void CQuorumManager::EnsureQuorumConnections(Consensus::LLMQType llmqType, const CBlockIndex* pindexNew) const
{
    const auto& params = Params().GetConsensus().llmqs.at(llmqType);

    auto myProTxHash = activeMasternodeInfo.proTxHash;
    auto lastQuorums = ScanQuorums(llmqType, pindexNew, (size_t)params.keepOldConnections);

    auto connmanQuorumsToDelete = g_connman->GetMasternodeQuorums(llmqType);

    // don't remove connections for the currently in-progress DKG round
    int curDkgHeight = pindexNew->nHeight - (pindexNew->nHeight % params.dkgInterval);
    auto curDkgBlock = pindexNew->GetAncestor(curDkgHeight)->GetBlockHash();
    connmanQuorumsToDelete.erase(curDkgBlock);

    bool allowWatch = gArgs.GetBoolArg("-watchquorums", DEFAULT_WATCH_QUORUMS);
    for (auto& quorum : lastQuorums) {
        if (!quorum->IsMember(myProTxHash) && !allowWatch) {
            continue;
        }

        CLLMQUtils::EnsureQuorumConnections(llmqType, quorum->pindexQuorum, myProTxHash, allowWatch);

        connmanQuorumsToDelete.erase(quorum->qc.quorumHash);
    }

    for (auto& qh : connmanQuorumsToDelete) {
        LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- removing masternodes quorum connections for quorum %s:\n", __func__, qh.ToString());
        g_connman->RemoveMasternodeQuorumNodes(llmqType, qh);
    }
}

CQuorumPtr CQuorumManager::BuildQuorumFromCommitment(const Consensus::LLMQType llmqType, const CBlockIndex* pindexQuorum) const
{
    AssertLockHeld(quorumsCacheCs);
    assert(pindexQuorum);

    CFinalCommitment qc;
    const uint256& quorumHash{pindexQuorum->GetBlockHash()};
    uint256 minedBlockHash;
    if (!quorumBlockProcessor->GetMinedCommitment(llmqType, quorumHash, qc, minedBlockHash)) {
        return nullptr;
    }
    assert(qc.quorumHash == pindexQuorum->GetBlockHash());

    auto quorum = std::make_shared<CQuorum>(llmq::GetLLMQParams(llmqType), blsWorker);
    auto members = CLLMQUtils::GetAllQuorumMembers((Consensus::LLMQType)qc.llmqType, pindexQuorum);

    quorum->Init(qc, pindexQuorum, minedBlockHash, members);

    bool hasValidVvec = false;
    if (quorum->ReadContributions(evoDb)) {
        hasValidVvec = true;
    } else {
        if (BuildQuorumContributions(qc, quorum)) {
            quorum->WriteContributions(evoDb);
            hasValidVvec = true;
        } else {
            LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- quorum.ReadContributions and BuildQuorumContributions for block %s failed\n", __func__, qc.quorumHash.ToString());
        }
    }

    if (hasValidVvec) {
        // pre-populate caches in the background
        // recovering public key shares is quite expensive and would result in serious lags for the first few signing
        // sessions if the shares would be calculated on-demand
        CQuorum::StartCachePopulatorThread(quorum);
    }

    quorumsCache.emplace(std::make_pair(llmqType, quorumHash), quorum);

    return quorum;
}

bool CQuorumManager::BuildQuorumContributions(const CFinalCommitment& fqc, std::shared_ptr<CQuorum>& quorum) const
{
    std::vector<uint16_t> memberIndexes;
    std::vector<BLSVerificationVectorPtr> vvecs;
    BLSSecretKeyVector skContributions;
    if (!dkgManager.GetVerifiedContributions((Consensus::LLMQType)fqc.llmqType, quorum->pindexQuorum, fqc.validMembers, memberIndexes, vvecs, skContributions)) {
        return false;
    }

    BLSVerificationVectorPtr quorumVvec;
    CBLSSecretKey skShare;

    cxxtimer::Timer t2(true);
    quorumVvec = blsWorker.BuildQuorumVerificationVector(vvecs);
    if (quorumVvec == nullptr) {
        LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- failed to build quorumVvec\n", __func__);
        // without the quorum vvec, there can't be a skShare, so we fail here. Failure is not fatal here, as it still
        // allows to use the quorum as a non-member (verification through the quorum pub key)
        return false;
    }
    skShare = blsWorker.AggregateSecretKeys(skContributions);
    if (!skShare.IsValid()) {
        LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- failed to build skShare\n", __func__);
        // We don't bail out here as this is not a fatal error and still allows us to recover public key shares (as we
        // have a valid quorum vvec at this point)
    }
    t2.stop();

    LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- built quorum vvec and skShare. time=%d\n", __func__, t2.count());

    quorum->quorumVvec = quorumVvec;
    quorum->skShare = skShare;

    return true;
}

bool CQuorumManager::HasQuorum(Consensus::LLMQType llmqType, const uint256& quorumHash)
{
    return quorumBlockProcessor->HasMinedCommitment(llmqType, quorumHash);
}

std::vector<CQuorumCPtr> CQuorumManager::ScanQuorums(Consensus::LLMQType llmqType, size_t maxCount) const
{
    const CBlockIndex* pindex;
    {
        LOCK(cs_main);
        pindex = chainActive.Tip();
    }
    return ScanQuorums(llmqType, pindex, maxCount);
}

std::vector<CQuorumCPtr> CQuorumManager::ScanQuorums(Consensus::LLMQType llmqType, const CBlockIndex* pindexStart, size_t maxCount) const
{
    auto& params = Params().GetConsensus().llmqs.at(llmqType);

    auto cacheKey = std::make_pair(llmqType, pindexStart->GetBlockHash());
    const size_t cacheMaxSize = params.signingActiveQuorumCount + 1;

    std::vector<CQuorumCPtr> result;

    if (maxCount <= cacheMaxSize) {
        LOCK(quorumsCacheCs);
        if (scanQuorumsCache.get(cacheKey, result)) {
            if (result.size() > maxCount) {
                result.resize(maxCount);
            }
            return result;
        }
    }

    bool storeCache = false;
    size_t maxCount2 = maxCount;
    if (maxCount2 <= cacheMaxSize) {
        maxCount2 = cacheMaxSize;
        storeCache = true;
    }

    auto quorumIndexes = quorumBlockProcessor->GetMinedCommitmentsUntilBlock(params.type, pindexStart, maxCount2);
    result.reserve(quorumIndexes.size());

    for (auto& quorumIndex : quorumIndexes) {
        assert(quorumIndex);
        auto quorum = GetQuorum(params.type, quorumIndex);
        assert(quorum != nullptr);
        result.emplace_back(quorum);
    }

    if (storeCache) {
        LOCK(quorumsCacheCs);
        scanQuorumsCache.insert(cacheKey, result);
    }

    if (result.size() > maxCount) {
        result.resize(maxCount);
    }

    return result;
}

CQuorumCPtr CQuorumManager::GetQuorum(Consensus::LLMQType llmqType, const uint256& quorumHash) const
{
    CBlockIndex* pindexQuorum;
    {
        LOCK(cs_main);

        pindexQuorum = LookupBlockIndex(quorumHash);
        if (!pindexQuorum) {
            LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- block %s not found\n", __func__, quorumHash.ToString());
            return nullptr;
        }
    }
    return GetQuorum(llmqType, pindexQuorum);
}

CQuorumCPtr CQuorumManager::GetQuorum(Consensus::LLMQType llmqType, const CBlockIndex* pindexQuorum) const
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

    return BuildQuorumFromCommitment(llmqType, pindexQuorum);
}

} // namespace llmq
