// Copyright (c) 2018-2023 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/quorums.h>
#include <llmq/commitment.h>
#include <llmq/blockprocessor.h>
#include <llmq/dkgsession.h>
#include <llmq/dkgsessionmgr.h>
#include <llmq/utils.h>

#include <evo/specialtx.h>
#include <evo/deterministicmns.h>

#include <chainparams.h>
#include <masternode/node.h>
#include <masternode/sync.h>
#include <net.h>
#include <net_processing.h>
#include <netmessagemaker.h>
#include <univalue.h>
#include <util/irange.h>
#include <util/underlying.h>
#include <validation.h>

#include <cxxtimer.hpp>

namespace llmq
{

static const std::string DB_QUORUM_SK_SHARE = "q_Qsk";
static const std::string DB_QUORUM_QUORUM_VVEC = "q_Qqvvec";

std::unique_ptr<CQuorumManager> quorumManager;

RecursiveMutex cs_data_requests;
static std::unordered_map<CQuorumDataRequestKey, CQuorumDataRequest, StaticSaltedHasher> mapQuorumDataRequests GUARDED_BY(cs_data_requests);

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

std::string CQuorumDataRequest::GetErrorString() const
{
    switch (nError) {
        case (Errors::NONE):
            return "NONE";
        case (Errors::QUORUM_TYPE_INVALID):
            return "QUORUM_TYPE_INVALID";
        case (Errors::QUORUM_BLOCK_NOT_FOUND):
            return "QUORUM_BLOCK_NOT_FOUND";
        case (Errors::QUORUM_NOT_FOUND):
            return "QUORUM_NOT_FOUND";
        case (Errors::MASTERNODE_IS_NO_MEMBER):
            return "MASTERNODE_IS_NO_MEMBER";
        case (Errors::QUORUM_VERIFICATION_VECTOR_MISSING):
            return "QUORUM_VERIFICATION_VECTOR_MISSING";
        case (Errors::ENCRYPTED_CONTRIBUTIONS_MISSING):
            return "ENCRYPTED_CONTRIBUTIONS_MISSING";
        case (Errors::UNDEFINED):
            return "UNDEFINED";
        default:
            return "UNDEFINED";
    }
    return "UNDEFINED";
}

CQuorum::CQuorum(const Consensus::LLMQParams& _params, CBLSWorker& _blsWorker) : params(_params), blsCache(_blsWorker)
{
}

void CQuorum::Init(CFinalCommitmentPtr _qc, const CBlockIndex* _pQuorumBaseBlockIndex, const uint256& _minedBlockHash, const std::vector<CDeterministicMNCPtr>& _members)
{
    qc = std::move(_qc);
    m_quorum_base_block_index = _pQuorumBaseBlockIndex;
    members = _members;
    minedBlockHash = _minedBlockHash;
}

bool CQuorum::SetVerificationVector(const BLSVerificationVector& quorumVecIn)
{
    const auto quorumVecInSerialized = ::SerializeHash(quorumVecIn);

    LOCK(cs);
    if (quorumVecInSerialized != qc->quorumVvecHash) {
        return false;
    }
    quorumVvec = std::make_shared<BLSVerificationVector>(quorumVecIn);
    return true;
}

bool CQuorum::SetSecretKeyShare(const CBLSSecretKey& secretKeyShare)
{
    if (!secretKeyShare.IsValid() || (secretKeyShare.GetPublicKey() != GetPubKeyShare(WITH_LOCK(activeMasternodeInfoCs, return GetMemberIndex(activeMasternodeInfo.proTxHash))))) {
        return false;
    }
    LOCK(cs);
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
    for (const auto i : irange::range(members.size())) {
        // cppcheck-suppress useStlAlgorithm
        if (members[i]->proTxHash == proTxHash) {
            return qc->validMembers[i];
        }
    }
    return false;
}

CBLSPublicKey CQuorum::GetPubKeyShare(size_t memberIdx) const
{
    LOCK(cs);
    if (!HasVerificationVector() || memberIdx >= members.size() || !qc->validMembers[memberIdx]) {
        return CBLSPublicKey();
    }
    const auto& m = members[memberIdx];
    return blsCache.BuildPubKeyShare(m->proTxHash, quorumVvec, CBLSId(m->proTxHash));
}

bool CQuorum::HasVerificationVector() const {
    LOCK(cs);
    return quorumVvec != nullptr;
}

CBLSSecretKey CQuorum::GetSkShare() const
{
    LOCK(cs);
    return skShare;
}

int CQuorum::GetMemberIndex(const uint256& proTxHash) const
{
    for (const auto i : irange::range(members.size())) {
        // cppcheck-suppress useStlAlgorithm
        if (members[i]->proTxHash == proTxHash) {
            return int(i);
        }
    }
    return -1;
}

void CQuorum::WriteContributions(CEvoDB& evoDb) const
{
    uint256 dbKey = MakeQuorumKey(*this);

    LOCK(cs);
    if (HasVerificationVector()) {
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
        WITH_LOCK(cs, quorumVvec = std::make_shared<BLSVerificationVector>(std::move(qv)));
    } else {
        return false;
    }

    // We ignore the return value here as it is ok if this fails. If it fails, it usually means that we are not a
    // member of the quorum but observed the whole DKG process to have the quorum verification vector.
    WITH_LOCK(cs, evoDb.Read(std::make_pair(DB_QUORUM_SK_SHARE, dbKey), skShare));

    return true;
}

CQuorumManager::CQuorumManager(CEvoDB& _evoDb, CConnman& _connman, CBLSWorker& _blsWorker, CQuorumBlockProcessor& _quorumBlockProcessor,
                               CDKGSessionManager& _dkgManager, const std::unique_ptr<CMasternodeSync>& mn_sync,
                               const std::unique_ptr<PeerManager>& peerman) :
    m_evoDb(_evoDb),
    connman(_connman),
    blsWorker(_blsWorker),
    dkgManager(_dkgManager),
    quorumBlockProcessor(_quorumBlockProcessor),
    m_mn_sync(mn_sync),
    m_peerman(peerman)
{
    utils::InitQuorumsCache(mapQuorumsCache);
    utils::InitQuorumsCache(scanQuorumsCache);

    quorumThreadInterrupt.reset();
}

void CQuorumManager::Start()
{
    int workerCount = std::thread::hardware_concurrency() / 2;
    workerCount = std::max(std::min(1, workerCount), 4);
    workerPool.resize(workerCount);
    RenameThreadPool(workerPool, "q-mngr");
}

void CQuorumManager::Stop()
{
    quorumThreadInterrupt();
    workerPool.clear_queue();
    workerPool.stop(true);
}

void CQuorumManager::TriggerQuorumDataRecoveryThreads(const CBlockIndex* pIndex) const
{
    if (!fMasternodeMode || !utils::QuorumDataRecoveryEnabled() || pIndex == nullptr) {
        return;
    }

    const std::map<Consensus::LLMQType, QvvecSyncMode> mapQuorumVvecSync = utils::GetEnabledQuorumVvecSyncEntries();

    LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- Process block %s\n", __func__, pIndex->GetBlockHash().ToString());

    for (const auto& params : Params().GetConsensus().llmqs) {
        const auto vecQuorums = ScanQuorums(params.type, pIndex, params.keepOldConnections);

        // First check if we are member of any quorum of this type
        auto proTxHash = WITH_LOCK(activeMasternodeInfoCs, return activeMasternodeInfo.proTxHash);
        bool fWeAreQuorumTypeMember = ranges::any_of(vecQuorums, [&proTxHash](const auto& pQuorum) {
            return pQuorum->IsValidMember(proTxHash);
        });

        for (const auto& pQuorum : vecQuorums) {
            // If there is already a thread running for this specific quorum skip it
            if (pQuorum->fQuorumDataRecoveryThreadRunning) {
                continue;
            }

            uint16_t nDataMask{0};
            const bool fWeAreQuorumMember = pQuorum->IsValidMember(proTxHash);
            const bool fSyncForTypeEnabled = mapQuorumVvecSync.count(pQuorum->qc->llmqType) > 0;
            const QvvecSyncMode syncMode = fSyncForTypeEnabled ? mapQuorumVvecSync.at(pQuorum->qc->llmqType) : QvvecSyncMode::Invalid;
            const bool fSyncCurrent = syncMode == QvvecSyncMode::Always || (syncMode == QvvecSyncMode::OnlyIfTypeMember && fWeAreQuorumTypeMember);

            if ((fWeAreQuorumMember || (fSyncForTypeEnabled && fSyncCurrent)) && !pQuorum->HasVerificationVector()) {
                nDataMask |= llmq::CQuorumDataRequest::QUORUM_VERIFICATION_VECTOR;
            }

            if (fWeAreQuorumMember && !pQuorum->GetSkShare().IsValid()) {
                nDataMask |= llmq::CQuorumDataRequest::ENCRYPTED_CONTRIBUTIONS;
            }

            if (nDataMask == 0) {
                LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- No data needed from (%d, %s) at height %d\n",
                    __func__, ToUnderlying(pQuorum->qc->llmqType), pQuorum->qc->quorumHash.ToString(), pIndex->nHeight);
                continue;
            }

            // Finally start the thread which triggers the requests for this quorum
            StartQuorumDataRecoveryThread(pQuorum, pIndex, nDataMask);
        }
    }
}

void CQuorumManager::UpdatedBlockTip(const CBlockIndex* pindexNew, bool fInitialDownload) const
{
    if (!m_mn_sync->IsBlockchainSynced()) {
        return;
    }

    for (const auto& params : Params().GetConsensus().llmqs) {
        CheckQuorumConnections(params, pindexNew);
    }

    {
        // Cleanup expired data requests
        LOCK(cs_data_requests);
        auto it = mapQuorumDataRequests.begin();
        while (it != mapQuorumDataRequests.end()) {
            if (it->second.IsExpired(/*add_bias=*/true)) {
                it = mapQuorumDataRequests.erase(it);
            } else {
                ++it;
            }
        }
    }

    TriggerQuorumDataRecoveryThreads(pindexNew);
}

void CQuorumManager::CheckQuorumConnections(const Consensus::LLMQParams& llmqParams, const CBlockIndex* pindexNew) const
{
    auto lastQuorums = ScanQuorums(llmqParams.type, pindexNew, (size_t)llmqParams.keepOldConnections);

    auto connmanQuorumsToDelete = connman.GetMasternodeQuorums(llmqParams.type);

    // don't remove connections for the currently in-progress DKG round
    if (utils::IsQuorumRotationEnabled(llmqParams, pindexNew)) {
        int cycleIndexTipHeight = pindexNew->nHeight % llmqParams.dkgInterval;
        int cycleQuorumBaseHeight = pindexNew->nHeight - cycleIndexTipHeight;
        std::stringstream ss;
        for (const auto quorumIndex : irange::range(llmqParams.signingActiveQuorumCount)) {
            if (quorumIndex <= cycleIndexTipHeight) {
                int curDkgHeight = cycleQuorumBaseHeight + quorumIndex;
                auto curDkgBlock = pindexNew->GetAncestor(curDkgHeight)->GetBlockHash();
                ss << curDkgHeight << ":" << curDkgBlock.ToString() << " | ";
                connmanQuorumsToDelete.erase(curDkgBlock);
            }
        }
        LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- llmqType[%d] h[%d] keeping mn quorum connections for rotated quorums: [%s]\n", __func__, ToUnderlying(llmqParams.type), pindexNew->nHeight, ss.str());
    } else {
        int curDkgHeight = pindexNew->nHeight - (pindexNew->nHeight % llmqParams.dkgInterval);
        auto curDkgBlock = pindexNew->GetAncestor(curDkgHeight)->GetBlockHash();
        connmanQuorumsToDelete.erase(curDkgBlock);
        LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- llmqType[%d] h[%d] keeping mn quorum connections for quorum: [%d:%s]\n", __func__, ToUnderlying(llmqParams.type), pindexNew->nHeight, curDkgHeight, curDkgBlock.ToString());
    }

    const auto myProTxHash = WITH_LOCK(activeMasternodeInfoCs, return activeMasternodeInfo.proTxHash);
    bool isISType = llmqParams.type == Params().GetConsensus().llmqTypeInstantSend ||
                    llmqParams.type == Params().GetConsensus().llmqTypeDIP0024InstantSend;

    bool watchOtherISQuorums = isISType && !myProTxHash.IsNull() &&
                    ranges::any_of(lastQuorums, [&myProTxHash](const auto& old_quorum){
                        return old_quorum->IsMember(myProTxHash);
                    });

    for (const auto& quorum : lastQuorums) {
        if (utils::EnsureQuorumConnections(llmqParams, quorum->m_quorum_base_block_index, connman, myProTxHash)) {
            if (connmanQuorumsToDelete.erase(quorum->qc->quorumHash) > 0) {
                LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- llmqType[%d] h[%d] keeping mn quorum connections for quorum: [%d:%s]\n", __func__, ToUnderlying(llmqParams.type), pindexNew->nHeight, quorum->m_quorum_base_block_index->nHeight, quorum->m_quorum_base_block_index->GetBlockHash().ToString());
            }
        } else if (watchOtherISQuorums && !quorum->IsMember(myProTxHash)) {
            std::set<uint256> connections;
            const auto& cindexes = utils::CalcDeterministicWatchConnections(llmqParams.type, quorum->m_quorum_base_block_index, quorum->members.size(), 1);
            for (auto idx : cindexes) {
                connections.emplace(quorum->members[idx]->proTxHash);
            }
            if (!connections.empty()) {
                if (!connman.HasMasternodeQuorumNodes(llmqParams.type, quorum->m_quorum_base_block_index->GetBlockHash())) {
                    LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- llmqType[%d] h[%d] adding mn inter-quorum connections for quorum: [%d:%s]\n", __func__, ToUnderlying(llmqParams.type), pindexNew->nHeight, quorum->m_quorum_base_block_index->nHeight, quorum->m_quorum_base_block_index->GetBlockHash().ToString());
                    connman.SetMasternodeQuorumNodes(llmqParams.type, quorum->m_quorum_base_block_index->GetBlockHash(), connections);
                    connman.SetMasternodeQuorumRelayMembers(llmqParams.type, quorum->m_quorum_base_block_index->GetBlockHash(), connections);
                }
                if (connmanQuorumsToDelete.erase(quorum->qc->quorumHash) > 0) {
                    LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- llmqType[%d] h[%d] keeping mn inter-quorum connections for quorum: [%d:%s]\n", __func__, ToUnderlying(llmqParams.type), pindexNew->nHeight, quorum->m_quorum_base_block_index->nHeight, quorum->m_quorum_base_block_index->GetBlockHash().ToString());
                }
            }
        }
    }
    for (const auto& quorumHash : connmanQuorumsToDelete) {
        LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- removing masternodes quorum connections for quorum %s:\n", __func__, quorumHash.ToString());
        connman.RemoveMasternodeQuorumNodes(llmqParams.type, quorumHash);
    }
}

CQuorumPtr CQuorumManager::BuildQuorumFromCommitment(const Consensus::LLMQType llmqType, const CBlockIndex* pQuorumBaseBlockIndex) const
{
    assert(pQuorumBaseBlockIndex);

    const uint256& quorumHash{pQuorumBaseBlockIndex->GetBlockHash()};
    uint256 minedBlockHash;
    CFinalCommitmentPtr qc = quorumBlockProcessor.GetMinedCommitment(llmqType, quorumHash, minedBlockHash);
    if (qc == nullptr) {
        LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- No mined commitment for llmqType[%d] nHeight[%d] quorumHash[%s]\n", __func__, ToUnderlying(llmqType), pQuorumBaseBlockIndex->nHeight, pQuorumBaseBlockIndex->GetBlockHash().ToString());
        return nullptr;
    }
    assert(qc->quorumHash == pQuorumBaseBlockIndex->GetBlockHash());

    const auto& llmq_params_opt = GetLLMQParams(llmqType);
    assert(llmq_params_opt.has_value());
    auto quorum = std::make_shared<CQuorum>(llmq_params_opt.value(), blsWorker);
    auto members = utils::GetAllQuorumMembers(qc->llmqType, pQuorumBaseBlockIndex);

    quorum->Init(std::move(qc), pQuorumBaseBlockIndex, minedBlockHash, members);

    bool hasValidVvec = false;
    if (quorum->ReadContributions(m_evoDb)) {
        hasValidVvec = true;
    } else {
        if (BuildQuorumContributions(quorum->qc, quorum)) {
            quorum->WriteContributions(m_evoDb);
            hasValidVvec = true;
        } else {
            LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- llmqType[%d] quorumIndex[%d] quorum.ReadContributions and BuildQuorumContributions for quorumHash[%s] failed\n", __func__, ToUnderlying(llmqType), quorum->qc->quorumIndex, quorum->qc->quorumHash.ToString());
        }
    }

    if (hasValidVvec) {
        // pre-populate caches in the background
        // recovering public key shares is quite expensive and would result in serious lags for the first few signing
        // sessions if the shares would be calculated on-demand
        StartCachePopulatorThread(quorum);
    }

    WITH_LOCK(cs_map_quorums, mapQuorumsCache[llmqType].insert(quorumHash, quorum));

    return quorum;
}

bool CQuorumManager::BuildQuorumContributions(const CFinalCommitmentPtr& fqc, const std::shared_ptr<CQuorum>& quorum) const
{
    std::vector<uint16_t> memberIndexes;
    std::vector<BLSVerificationVectorPtr> vvecs;
    BLSSecretKeyVector skContributions;
    if (!dkgManager.GetVerifiedContributions((Consensus::LLMQType)fqc->llmqType, quorum->m_quorum_base_block_index, fqc->validMembers, memberIndexes, vvecs, skContributions)) {
        return false;
    }

    cxxtimer::Timer t2(true);
    LOCK(quorum->cs);
    quorum->quorumVvec = blsWorker.BuildQuorumVerificationVector(vvecs);
    if (!quorum->HasVerificationVector()) {
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

bool CQuorumManager::HasQuorum(Consensus::LLMQType llmqType, const CQuorumBlockProcessor& quorum_block_processor, const uint256& quorumHash)
{
    return quorum_block_processor.HasMinedCommitment(llmqType, quorumHash);
}

bool CQuorumManager::RequestQuorumData(CNode* pfrom, Consensus::LLMQType llmqType, const CBlockIndex* pQuorumBaseBlockIndex, uint16_t nDataMask, const uint256& proTxHash) const
{
    if (pfrom == nullptr) {
        LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- Invalid pfrom: nullptr\n", __func__);
        return false;
    }
    if (pfrom->nVersion < LLMQ_DATA_MESSAGES_VERSION) {
        LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- Version must be %d or greater.\n", __func__, LLMQ_DATA_MESSAGES_VERSION);
        return false;
    }
    if (pfrom->GetVerifiedProRegTxHash().IsNull() && !pfrom->qwatch) {
        LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- pfrom is neither a verified masternode nor a qwatch connection\n", __func__);
        return false;
    }
    if (!GetLLMQParams(llmqType).has_value()) {
        LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- Invalid llmqType: %d\n", __func__, ToUnderlying(llmqType));
        return false;
    }
    if (pQuorumBaseBlockIndex == nullptr) {
        LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- Invalid pQuorumBaseBlockIndex: nullptr\n", __func__);
        return false;
    }
    if (GetQuorum(llmqType, pQuorumBaseBlockIndex) == nullptr) {
        LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- Quorum not found: %s, %d\n", __func__, pQuorumBaseBlockIndex->GetBlockHash().ToString(), ToUnderlying(llmqType));
        return false;
    }

    LOCK(cs_data_requests);
    const CQuorumDataRequestKey key(pfrom->GetVerifiedProRegTxHash(), true, pQuorumBaseBlockIndex->GetBlockHash(), llmqType);
    const CQuorumDataRequest request(llmqType, pQuorumBaseBlockIndex->GetBlockHash(), nDataMask, proTxHash);
    auto [old_pair, exists] = mapQuorumDataRequests.emplace(key, request);
    if (!exists) {
        if (old_pair->second.IsExpired(/*add_bias=*/true)) {
            old_pair->second = request;
        } else {
            LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- Already requested\n", __func__);
            return false;
        }
    }
    LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- sending QGETDATA quorumHash[%s] llmqType[%d] proRegTx[%s]\n", __func__, key.quorumHash.ToString(),
             ToUnderlying(key.llmqType), key.proRegTx.ToString());

    CNetMsgMaker msgMaker(pfrom->GetSendVersion());
    connman.PushMessage(pfrom, msgMaker.Make(NetMsgType::QGETDATA, request));

    return true;
}

std::vector<CQuorumCPtr> CQuorumManager::ScanQuorums(Consensus::LLMQType llmqType, size_t nCountRequested) const
{
    const CBlockIndex* pindex = WITH_LOCK(cs_main, return ::ChainActive().Tip());
    return ScanQuorums(llmqType, pindex, nCountRequested);
}

std::vector<CQuorumCPtr> CQuorumManager::ScanQuorums(Consensus::LLMQType llmqType, const CBlockIndex* pindexStart, size_t nCountRequested) const
{
    if (pindexStart == nullptr || nCountRequested == 0 || !utils::IsQuorumTypeEnabled(llmqType, *this, pindexStart)) {
        return {};
    }

    const CBlockIndex* pIndexScanCommitments{pindexStart};
    size_t nScanCommitments{nCountRequested};
    std::vector<CQuorumCPtr> vecResultQuorums;

    {
        LOCK(cs_scan_quorums);
        auto& cache = scanQuorumsCache[llmqType];
        bool fCacheExists = cache.get(pindexStart->GetBlockHash(), vecResultQuorums);
        if (fCacheExists) {
            // We have exactly what requested so just return it
            if (vecResultQuorums.size() == nCountRequested) {
                return vecResultQuorums;
            }
            // If we have more cached than requested return only a subvector
            if (vecResultQuorums.size() > nCountRequested) {
                return {vecResultQuorums.begin(), vecResultQuorums.begin() + nCountRequested};
            }
            // If we have cached quorums but not enough, subtract what we have from the count and the set correct index where to start
            // scanning for the rests
            if (!vecResultQuorums.empty()) {
                nScanCommitments -= vecResultQuorums.size();
                pIndexScanCommitments = vecResultQuorums.back()->m_quorum_base_block_index->pprev;
            }
        } else {
            // If there is nothing in cache request at least cache.max_size() because this gets cached then later
            nScanCommitments = std::max(nCountRequested, cache.max_size());
        }
    }

    // Get the block indexes of the mined commitments to build the required quorums from
    const auto& llmq_params_opt = GetLLMQParams(llmqType);
    assert(llmq_params_opt.has_value());
    std::vector<const CBlockIndex*> pQuorumBaseBlockIndexes{ llmq_params_opt->useRotation ?
            quorumBlockProcessor.GetMinedCommitmentsIndexedUntilBlock(llmqType, pIndexScanCommitments, nScanCommitments) :
            quorumBlockProcessor.GetMinedCommitmentsUntilBlock(llmqType, pIndexScanCommitments, nScanCommitments)
    };
    vecResultQuorums.reserve(vecResultQuorums.size() + pQuorumBaseBlockIndexes.size());

    for (auto& pQuorumBaseBlockIndex : pQuorumBaseBlockIndexes) {
        assert(pQuorumBaseBlockIndex);
        auto quorum = GetQuorum(llmqType, pQuorumBaseBlockIndex);
        assert(quorum != nullptr);
        vecResultQuorums.emplace_back(quorum);
    }

    const size_t nCountResult{vecResultQuorums.size()};
    if (nCountResult > 0) {
        LOCK(cs_scan_quorums);
        // Don't cache more than cache.max_size() elements
        auto& cache = scanQuorumsCache[llmqType];
        const size_t nCacheEndIndex = std::min(nCountResult, cache.max_size());
        cache.emplace(pindexStart->GetBlockHash(), {vecResultQuorums.begin(), vecResultQuorums.begin() + nCacheEndIndex});
    }
    // Don't return more than nCountRequested elements
    const size_t nResultEndIndex = std::min(nCountResult, nCountRequested);
    return {vecResultQuorums.begin(), vecResultQuorums.begin() + nResultEndIndex};
}

CQuorumCPtr CQuorumManager::GetQuorum(Consensus::LLMQType llmqType, const uint256& quorumHash) const
{
    const CBlockIndex* pQuorumBaseBlockIndex = WITH_LOCK(cs_main, return g_chainman.m_blockman.LookupBlockIndex(quorumHash));
    if (!pQuorumBaseBlockIndex) {
        LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- block %s not found\n", __func__, quorumHash.ToString());
        return nullptr;
    }
    return GetQuorum(llmqType, pQuorumBaseBlockIndex);
}

CQuorumCPtr CQuorumManager::GetQuorum(Consensus::LLMQType llmqType, const CBlockIndex* pQuorumBaseBlockIndex) const
{
    assert(pQuorumBaseBlockIndex);

    auto quorumHash = pQuorumBaseBlockIndex->GetBlockHash();

    // we must check this before we look into the cache. Reorgs might have happened which would mean we might have
    // cached quorums which are not in the active chain anymore
    if (!HasQuorum(llmqType, quorumBlockProcessor, quorumHash)) {
        return nullptr;
    }

    CQuorumPtr pQuorum;
    if (LOCK(cs_map_quorums); mapQuorumsCache[llmqType].get(quorumHash, pQuorum)) {
        return pQuorum;
    }

    return BuildQuorumFromCommitment(llmqType, pQuorumBaseBlockIndex);
}

size_t CQuorumManager::GetQuorumRecoveryStartOffset(const CQuorumCPtr pQuorum, const CBlockIndex* pIndex) const
{
    auto mns = deterministicMNManager->GetListForBlock(pIndex);
    std::vector<uint256> vecProTxHashes;
    vecProTxHashes.reserve(mns.GetValidMNsCount());
    mns.ForEachMN(true, [&](auto& pMasternode) {
        vecProTxHashes.emplace_back(pMasternode.proTxHash);
    });
    std::sort(vecProTxHashes.begin(), vecProTxHashes.end());
    size_t nIndex{0};
    {
        LOCK(activeMasternodeInfoCs);
        for (const auto i : irange::range(vecProTxHashes.size())) {
            // cppcheck-suppress useStlAlgorithm
            if (activeMasternodeInfo.proTxHash == vecProTxHashes[i]) {
                nIndex = i;
                break;
            }
        }
    }
    return nIndex % pQuorum->qc->validMembers.size();
}

void CQuorumManager::ProcessMessage(CNode& pfrom, const std::string& msg_type, CDataStream& vRecv)
{
    auto strFunc = __func__;
    auto errorHandler = [&](const std::string& strError, int nScore = 10) {
        LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- %s: %s, from peer=%d\n", strFunc, msg_type, strError, pfrom.GetId());
        if (nScore > 0) {
            m_peerman->Misbehaving(pfrom.GetId(), nScore);
        }
    };

    if (msg_type == NetMsgType::QGETDATA) {

        if (!fMasternodeMode || (pfrom.GetVerifiedProRegTxHash().IsNull() && !pfrom.qwatch)) {
            errorHandler("Not a verified masternode or a qwatch connection");
            return;
        }

        CQuorumDataRequest request;
        vRecv >> request;

        auto sendQDATA = [&](CQuorumDataRequest::Errors nError,
                             bool request_limit_exceeded,
                             const CDataStream& body = CDataStream(SER_NETWORK, PROTOCOL_VERSION)) {
            switch (nError) {
                case (CQuorumDataRequest::Errors::NONE):
                case (CQuorumDataRequest::Errors::QUORUM_TYPE_INVALID):
                case (CQuorumDataRequest::Errors::QUORUM_BLOCK_NOT_FOUND):
                case (CQuorumDataRequest::Errors::QUORUM_NOT_FOUND):
                case (CQuorumDataRequest::Errors::MASTERNODE_IS_NO_MEMBER):
                case (CQuorumDataRequest::Errors::UNDEFINED):
                    if (request_limit_exceeded) errorHandler("Request limit exceeded", 25);
                    break;
                case (CQuorumDataRequest::Errors::QUORUM_VERIFICATION_VECTOR_MISSING):
                case (CQuorumDataRequest::Errors::ENCRYPTED_CONTRIBUTIONS_MISSING):
                    // Do not punish limit exceed if we don't have the requested data
                    break;
            }
            request.SetError(nError);
            CDataStream ssResponse(SER_NETWORK, pfrom.GetSendVersion(), request, body);
            connman.PushMessage(&pfrom, CNetMsgMaker(pfrom.GetSendVersion()).Make(NetMsgType::QDATA, ssResponse));
        };

        bool request_limit_exceeded(false);
        {
            LOCK2(cs_main, cs_data_requests);
            const CQuorumDataRequestKey key(pfrom.GetVerifiedProRegTxHash(), false, request.GetQuorumHash(), request.GetLLMQType());
            auto it = mapQuorumDataRequests.find(key);
            if (it == mapQuorumDataRequests.end()) {
                it = mapQuorumDataRequests.emplace(key, request).first;
            } else if (it->second.IsExpired(/*add_bias=*/false)) {
                it->second = request;
            } else {
                request_limit_exceeded = true;
            }
        }

        if (!GetLLMQParams(request.GetLLMQType()).has_value()) {
            sendQDATA(CQuorumDataRequest::Errors::QUORUM_TYPE_INVALID, request_limit_exceeded);
            return;
        }

        const CBlockIndex* pQuorumBaseBlockIndex = WITH_LOCK(cs_main, return g_chainman.m_blockman.LookupBlockIndex(request.GetQuorumHash()));
        if (pQuorumBaseBlockIndex == nullptr) {
            sendQDATA(CQuorumDataRequest::Errors::QUORUM_BLOCK_NOT_FOUND, request_limit_exceeded);
            return;
        }

        const CQuorumCPtr pQuorum = GetQuorum(request.GetLLMQType(), pQuorumBaseBlockIndex);
        if (pQuorum == nullptr) {
            sendQDATA(CQuorumDataRequest::Errors::QUORUM_NOT_FOUND, request_limit_exceeded);
            return;
        }

        CDataStream ssResponseData(SER_NETWORK, pfrom.GetSendVersion());

        // Check if request wants QUORUM_VERIFICATION_VECTOR data
        if (request.GetDataMask() & CQuorumDataRequest::QUORUM_VERIFICATION_VECTOR) {
            if (!pQuorum->HasVerificationVector()) {
                sendQDATA(CQuorumDataRequest::Errors::QUORUM_VERIFICATION_VECTOR_MISSING, request_limit_exceeded);
                return;
            }

            WITH_LOCK(pQuorum->cs, ssResponseData << *pQuorum->quorumVvec);
        }

        // Check if request wants ENCRYPTED_CONTRIBUTIONS data
        if (request.GetDataMask() & CQuorumDataRequest::ENCRYPTED_CONTRIBUTIONS) {

            int memberIdx = pQuorum->GetMemberIndex(request.GetProTxHash());
            if (memberIdx == -1) {
                sendQDATA(CQuorumDataRequest::Errors::MASTERNODE_IS_NO_MEMBER, request_limit_exceeded);
                return;
            }

            std::vector<CBLSIESEncryptedObject<CBLSSecretKey>> vecEncrypted;
            if (!dkgManager.GetEncryptedContributions(request.GetLLMQType(), pQuorumBaseBlockIndex, pQuorum->qc->validMembers, request.GetProTxHash(), vecEncrypted)) {
                sendQDATA(CQuorumDataRequest::Errors::ENCRYPTED_CONTRIBUTIONS_MISSING, request_limit_exceeded);
                return;
            }

            ssResponseData << vecEncrypted;
        }

        sendQDATA(CQuorumDataRequest::Errors::NONE, request_limit_exceeded, ssResponseData);
        return;
    }

    if (msg_type == NetMsgType::QDATA) {
        if ((!fMasternodeMode && !utils::IsWatchQuorumsEnabled()) || (pfrom.GetVerifiedProRegTxHash().IsNull() && !pfrom.qwatch)) {
            errorHandler("Not a verified masternode or a qwatch connection");
            return;
        }

        CQuorumDataRequest request;
        vRecv >> request;

        {
            LOCK2(cs_main, cs_data_requests);
            const CQuorumDataRequestKey key(pfrom.GetVerifiedProRegTxHash(), true, request.GetQuorumHash(), request.GetLLMQType());
            auto it = mapQuorumDataRequests.find(key);
            if (it == mapQuorumDataRequests.end()) {
                errorHandler("Not requested");
                return;
            }
            if (it->second.IsProcessed()) {
                errorHandler("Already received");
                return;
            }
            if (request != it->second) {
                errorHandler("Not like requested");
                return;
            }
            it->second.SetProcessed();
        }

        if (request.GetError() != CQuorumDataRequest::Errors::NONE) {
            errorHandler(strprintf("Error %d (%s)", request.GetError(), request.GetErrorString()), 0);
            return;
        }

        CQuorumPtr pQuorum;
        {
            if (LOCK(cs_map_quorums); !mapQuorumsCache[request.GetLLMQType()].get(request.GetQuorumHash(), pQuorum)) {
                errorHandler("Quorum not found", 0); // Don't bump score because we asked for it
                return;
            }
        }

        // Check if request has QUORUM_VERIFICATION_VECTOR data
        if (request.GetDataMask() & CQuorumDataRequest::QUORUM_VERIFICATION_VECTOR) {

            BLSVerificationVector verificationVector;
            vRecv >> verificationVector;

            if (pQuorum->SetVerificationVector(verificationVector)) {
                StartCachePopulatorThread(pQuorum);
            } else {
                errorHandler("Invalid quorum verification vector");
                return;
            }
        }

        // Check if request has ENCRYPTED_CONTRIBUTIONS data
        if (request.GetDataMask() & CQuorumDataRequest::ENCRYPTED_CONTRIBUTIONS) {

            if (WITH_LOCK(pQuorum->cs, return pQuorum->quorumVvec->size() != size_t(pQuorum->params.threshold))) {
                errorHandler("No valid quorum verification vector available", 0); // Don't bump score because we asked for it
                return;
            }

            int memberIdx = pQuorum->GetMemberIndex(request.GetProTxHash());
            if (memberIdx == -1) {
                errorHandler("Not a member of the quorum", 0); // Don't bump score because we asked for it
                return;
            }

            std::vector<CBLSIESEncryptedObject<CBLSSecretKey>> vecEncrypted;
            vRecv >> vecEncrypted;

            BLSSecretKeyVector vecSecretKeys;
            vecSecretKeys.resize(vecEncrypted.size());
            auto secret = WITH_LOCK(activeMasternodeInfoCs, return *activeMasternodeInfo.blsKeyOperator);
            for (const auto i : irange::range(vecEncrypted.size())) {
                if (!vecEncrypted[i].Decrypt(memberIdx, secret, vecSecretKeys[i], PROTOCOL_VERSION)) {
                    errorHandler("Failed to decrypt");
                    return;
                }
            }

            CBLSSecretKey secretKeyShare = blsWorker.AggregateSecretKeys(vecSecretKeys);
            if (!pQuorum->SetSecretKeyShare(secretKeyShare)) {
                errorHandler("Invalid secret key share received");
                return;
            }
        }
        pQuorum->WriteContributions(m_evoDb);
        return;
    }
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
        for (const auto i : irange::range(pQuorum->members.size())) {
            if (!quorumThreadInterrupt) {
                break;
            }
            if (pQuorum->qc->validMembers[i]) {
                pQuorum->GetPubKeyShare(i);
            }
        }
        LogPrint(BCLog::LLMQ, "CQuorumManager::StartCachePopulatorThread -- done. time=%d\n", t.count());
    });
}

void CQuorumManager::StartQuorumDataRecoveryThread(const CQuorumCPtr pQuorum, const CBlockIndex* pIndex, uint16_t nDataMaskIn) const
{
    if (pQuorum->fQuorumDataRecoveryThreadRunning) {
        LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- Already running\n", __func__);
        return;
    }
    pQuorum->fQuorumDataRecoveryThreadRunning = true;

    workerPool.push([pQuorum, pIndex, nDataMaskIn, this](int threadId) {
        size_t nTries{0};
        uint16_t nDataMask{nDataMaskIn};
        int64_t nTimeLastSuccess{0};
        uint256* pCurrentMemberHash{nullptr};
        std::vector<uint256> vecMemberHashes;
        const size_t nMyStartOffset{GetQuorumRecoveryStartOffset(pQuorum, pIndex)};
        const int64_t nRequestTimeout{10};

        auto printLog = [&](const std::string& strMessage) {
            const std::string strMember{pCurrentMemberHash == nullptr ? "nullptr" : pCurrentMemberHash->ToString()};
            LogPrint(BCLog::LLMQ, "CQuorumManager::StartQuorumDataRecoveryThread -- %s - for llmqType %d, quorumHash %s, nDataMask (%d/%d), pCurrentMemberHash %s, nTries %d\n",
                strMessage, ToUnderlying(pQuorum->qc->llmqType), pQuorum->qc->quorumHash.ToString(), nDataMask, nDataMaskIn, strMember, nTries);
        };
        printLog("Start");

        while (!m_mn_sync->IsBlockchainSynced() && !quorumThreadInterrupt) {
            quorumThreadInterrupt.sleep_for(std::chrono::seconds(nRequestTimeout));
        }

        if (quorumThreadInterrupt) {
            printLog("Aborted");
            return;
        }

        vecMemberHashes.reserve(pQuorum->qc->validMembers.size());
        for (auto& member : pQuorum->members) {
            if (pQuorum->IsValidMember(member->proTxHash) && member->proTxHash != WITH_LOCK(activeMasternodeInfoCs, return activeMasternodeInfo.proTxHash)) {
                vecMemberHashes.push_back(member->proTxHash);
            }
        }
        std::sort(vecMemberHashes.begin(), vecMemberHashes.end());

        printLog("Try to request");

        while (nDataMask > 0 && !quorumThreadInterrupt) {

            if (nDataMask & llmq::CQuorumDataRequest::QUORUM_VERIFICATION_VECTOR &&
                pQuorum->HasVerificationVector()) {
                nDataMask &= ~llmq::CQuorumDataRequest::QUORUM_VERIFICATION_VECTOR;
                printLog("Received quorumVvec");
            }

            if (nDataMask & llmq::CQuorumDataRequest::ENCRYPTED_CONTRIBUTIONS && pQuorum->GetSkShare().IsValid()) {
                nDataMask &= ~llmq::CQuorumDataRequest::ENCRYPTED_CONTRIBUTIONS;
                printLog("Received skShare");
            }

            if (nDataMask == 0) {
                printLog("Success");
                break;
            }

            if ((GetAdjustedTime() - nTimeLastSuccess) > nRequestTimeout) {
                if (nTries >= vecMemberHashes.size()) {
                    printLog("All tried but failed");
                    break;
                }
                // Access the member list of the quorum with the calculated offset applied to balance the load equally
                pCurrentMemberHash = &vecMemberHashes[(nMyStartOffset + nTries++) % vecMemberHashes.size()];
                {
                    LOCK(cs_data_requests);
                    const CQuorumDataRequestKey key(*pCurrentMemberHash, true, pQuorum->qc->quorumHash, pQuorum->qc->llmqType);
                    auto it = mapQuorumDataRequests.find(key);
                    if (it != mapQuorumDataRequests.end() && !it->second.IsExpired(/*add_bias=*/true)) {
                        printLog("Already asked");
                        continue;
                    }
                }
                // Sleep a bit depending on the start offset to balance out multiple requests to same masternode
                quorumThreadInterrupt.sleep_for(std::chrono::milliseconds(nMyStartOffset * 100));
                nTimeLastSuccess = GetAdjustedTime();
                connman.AddPendingMasternode(*pCurrentMemberHash);
                printLog("Connect");
            }

            auto proTxHash = WITH_LOCK(activeMasternodeInfoCs, return activeMasternodeInfo.proTxHash);
            connman.ForEachNode([&](CNode* pNode) {
                auto verifiedProRegTxHash = pNode->GetVerifiedProRegTxHash();
                if (pCurrentMemberHash == nullptr || verifiedProRegTxHash != *pCurrentMemberHash) {
                    return;
                }

                if (RequestQuorumData(pNode, pQuorum->qc->llmqType, pQuorum->m_quorum_base_block_index, nDataMask, proTxHash)) {
                    nTimeLastSuccess = GetAdjustedTime();
                    printLog("Requested");
                } else {
                    LOCK(cs_data_requests);
                    const CQuorumDataRequestKey key(*pCurrentMemberHash, true, pQuorum->qc->quorumHash, pQuorum->qc->llmqType);
                    auto it = mapQuorumDataRequests.find(key);
                    if (it == mapQuorumDataRequests.end()) {
                        printLog("Failed");
                        pNode->fDisconnect = true;
                        pCurrentMemberHash = nullptr;
                        return;
                    } else if (it->second.IsProcessed()) {
                        printLog("Processed");
                        pNode->fDisconnect = true;
                        pCurrentMemberHash = nullptr;
                        return;
                    } else {
                        printLog("Waiting");
                        return;
                    }
                }
            });
            quorumThreadInterrupt.sleep_for(std::chrono::seconds(1));
        }
        pQuorum->fQuorumDataRecoveryThreadRunning = false;
        printLog("Done");
    });
}

} // namespace llmq
