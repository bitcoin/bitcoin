// Copyright (c) 2018-2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/blockprocessor.h>
#include <llmq/commitment.h>
#include <llmq/dkgsessionmgr.h>
#include <llmq/options.h>
#include <llmq/params.h>
#include <llmq/quorums.h>
#include <llmq/signhash.h>
#include <llmq/utils.h>

#include <bls/bls.h>
#include <bls/bls_ies.h>
#include <chainparams.h>
#include <dbwrapper.h>
#include <evo/deterministicmns.h>
#include <evo/evodb.h>
#include <masternode/node.h>
#include <masternode/sync.h>
#include <net.h>
#include <netmessagemaker.h>
#include <util/irange.h>
#include <util/time.h>
#include <util/underlying.h>
#include <validation.h>

#include <cxxtimer.hpp>

namespace llmq
{

static const std::string DB_QUORUM_SK_SHARE = "q_Qsk";
static const std::string DB_QUORUM_QUORUM_VVEC = "q_Qqvvec";

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

void CQuorum::Init(CFinalCommitmentPtr _qc, const CBlockIndex* _pQuorumBaseBlockIndex, const uint256& _minedBlockHash, Span<CDeterministicMNCPtr> _members)
{
    qc = std::move(_qc);
    m_quorum_base_block_index = _pQuorumBaseBlockIndex;
    members = std::vector(_members.begin(), _members.end());
    minedBlockHash = _minedBlockHash;
}

bool CQuorum::SetVerificationVector(const std::vector<CBLSPublicKey>& quorumVecIn)
{
    const auto quorumVecInSerialized = ::SerializeHash(quorumVecIn);

    LOCK(cs_vvec_shShare);
    if (quorumVecInSerialized != qc->quorumVvecHash) {
        return false;
    }
    quorumVvec = std::make_shared<std::vector<CBLSPublicKey>>(quorumVecIn);
    return true;
}

bool CQuorum::SetSecretKeyShare(const CBLSSecretKey& secretKeyShare, const CActiveMasternodeManager& mn_activeman)
{
    if (!secretKeyShare.IsValid() || (secretKeyShare.GetPublicKey() != GetPubKeyShare(GetMemberIndex(mn_activeman.GetProTxHash())))) {
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
    for (const auto i : irange::range(members.size())) {
        // cppcheck-suppress useStlAlgorithm
        if (members[i]->proTxHash == proTxHash) {
            return int(i);
        }
    }
    return -1;
}

void CQuorum::WriteContributions(CDBWrapper& db) const
{
    uint256 dbKey = MakeQuorumKey(*this);

    LOCK(cs_vvec_shShare);
    if (HasVerificationVectorInternal()) {
        CDataStream s(SER_DISK, CLIENT_VERSION);
        WriteCompactSize(s, quorumVvec->size());
        for (auto& pubkey : *quorumVvec) {
            s << CBLSPublicKeyVersionWrapper(pubkey, false);
        }
        db.Write(std::make_pair(DB_QUORUM_QUORUM_VVEC, dbKey), s);
    }
    if (skShare.IsValid()) {
        db.Write(std::make_pair(DB_QUORUM_SK_SHARE, dbKey), skShare);
    }
}

bool CQuorum::ReadContributions(const CDBWrapper& db)
{
    uint256 dbKey = MakeQuorumKey(*this);
    CDataStream s(SER_DISK, CLIENT_VERSION);

    if (!db.ReadDataStream(std::make_pair(DB_QUORUM_QUORUM_VVEC, dbKey), s)) {
        return false;
    }

    size_t vvec_size = ReadCompactSize(s);
    CBLSPublicKey pubkey;
    std::vector<CBLSPublicKey> qv;
    for ([[maybe_unused]] size_t _ : irange::range(vvec_size)) {
        s >> CBLSPublicKeyVersionWrapper(pubkey, false);
        qv.emplace_back(pubkey);
    }

    LOCK(cs_vvec_shShare);
    quorumVvec = std::make_shared<std::vector<CBLSPublicKey>>(std::move(qv));
    // We ignore the return value here as it is ok if this fails. If it fails, it usually means that we are not a
    // member of the quorum but observed the whole DKG process to have the quorum verification vector.
    db.Read(std::make_pair(DB_QUORUM_SK_SHARE, dbKey), skShare);

    return true;
}

CQuorumManager::CQuorumManager(CBLSWorker& _blsWorker, CChainState& chainstate, CDeterministicMNManager& dmnman,
                               CDKGSessionManager& _dkgManager, CEvoDB& _evoDb,
                               CQuorumBlockProcessor& _quorumBlockProcessor, CQuorumSnapshotManager& qsnapman,
                               const CActiveMasternodeManager* const mn_activeman, const CMasternodeSync& mn_sync,
                               const CSporkManager& sporkman, bool unit_tests, bool wipe) :
    db(std::make_unique<CDBWrapper>(unit_tests ? "" : (gArgs.GetDataDirNet() / "llmq" / "quorumdb"), 1 << 20,
                                    unit_tests, wipe)),
    blsWorker(_blsWorker),
    m_chainstate(chainstate),
    m_dmnman(dmnman),
    dkgManager(_dkgManager),
    quorumBlockProcessor(_quorumBlockProcessor),
    m_qsnapman(qsnapman),
    m_mn_activeman(mn_activeman),
    m_mn_sync(mn_sync),
    m_sporkman(sporkman)
{
    utils::InitQuorumsCache(mapQuorumsCache, false);
    quorumThreadInterrupt.reset();
    MigrateOldQuorumDB(_evoDb);
}

CQuorumManager::~CQuorumManager() { Stop(); }

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

void CQuorumManager::TriggerQuorumDataRecoveryThreads(CConnman& connman, const CBlockIndex* pIndex) const
{
    if ((m_mn_activeman == nullptr && !IsWatchQuorumsEnabled()) || !QuorumDataRecoveryEnabled() || pIndex == nullptr) {
        return;
    }

    const std::map<Consensus::LLMQType, QvvecSyncMode> mapQuorumVvecSync = GetEnabledQuorumVvecSyncEntries();

    LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- Process block %s\n", __func__, pIndex->GetBlockHash().ToString());

    for (const auto& params : Params().GetConsensus().llmqs) {
        const auto vecQuorums = ScanQuorums(params.type, pIndex, params.keepOldConnections);

        // First check if we are member of any quorum of this type
        const uint256 proTxHash = m_mn_activeman != nullptr ? m_mn_activeman->GetProTxHash() : uint256();

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
            StartQuorumDataRecoveryThread(connman, pQuorum, pIndex, nDataMask);
        }
    }
}

void CQuorumManager::UpdatedBlockTip(const CBlockIndex* pindexNew, CConnman& connman, bool fInitialDownload) const
{
    if (!m_mn_sync.IsBlockchainSynced()) return;

    for (const auto& params : Params().GetConsensus().llmqs) {
        CheckQuorumConnections(connman, params, pindexNew);
    }

    if (m_mn_activeman != nullptr || IsWatchQuorumsEnabled()) {
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

    TriggerQuorumDataRecoveryThreads(connman, pindexNew);
    StartCleanupOldQuorumDataThread(pindexNew);
}

void CQuorumManager::CheckQuorumConnections(CConnman& connman, const Consensus::LLMQParams& llmqParams,
                                            const CBlockIndex* pindexNew) const
{
    if (m_mn_activeman == nullptr && !IsWatchQuorumsEnabled()) return;

    auto lastQuorums = ScanQuorums(llmqParams.type, pindexNew, (size_t)llmqParams.keepOldConnections);

    auto connmanQuorumsToDelete = connman.GetMasternodeQuorums(llmqParams.type);

    // don't remove connections for the currently in-progress DKG round
    if (IsQuorumRotationEnabled(llmqParams, pindexNew)) {
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

    const uint256 myProTxHash = m_mn_activeman != nullptr ? m_mn_activeman->GetProTxHash() : uint256();

    bool isISType = llmqParams.type == Params().GetConsensus().llmqTypeDIP0024InstantSend;

    bool watchOtherISQuorums = isISType && !myProTxHash.IsNull() &&
                    ranges::any_of(lastQuorums, [&myProTxHash](const auto& old_quorum){
                        return old_quorum->IsMember(myProTxHash);
                    });

    for (const auto& quorum : lastQuorums) {
        if (utils::EnsureQuorumConnections(llmqParams, connman, m_dmnman, m_sporkman, m_qsnapman,
                                           m_dmnman.GetListAtChainTip(), quorum->m_quorum_base_block_index, myProTxHash,
                                           /* is_masternode = */ m_mn_activeman != nullptr)) {
            if (connmanQuorumsToDelete.erase(quorum->qc->quorumHash) > 0) {
                LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- llmqType[%d] h[%d] keeping mn quorum connections for quorum: [%d:%s]\n", __func__, ToUnderlying(llmqParams.type), pindexNew->nHeight, quorum->m_quorum_base_block_index->nHeight, quorum->m_quorum_base_block_index->GetBlockHash().ToString());
            }
        } else if (watchOtherISQuorums && !quorum->IsMember(myProTxHash)) {
            std::unordered_set<uint256, StaticSaltedHasher> connections;
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

CQuorumPtr CQuorumManager::BuildQuorumFromCommitment(const Consensus::LLMQType llmqType, gsl::not_null<const CBlockIndex*> pQuorumBaseBlockIndex, bool populate_cache) const
{
    const uint256& quorumHash{pQuorumBaseBlockIndex->GetBlockHash()};

    auto [qc, minedBlockHash] = quorumBlockProcessor.GetMinedCommitment(llmqType, quorumHash);
    if (minedBlockHash == uint256::ZERO) {
        LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- No mined commitment for llmqType[%d] nHeight[%d] quorumHash[%s]\n", __func__, ToUnderlying(llmqType), pQuorumBaseBlockIndex->nHeight, pQuorumBaseBlockIndex->GetBlockHash().ToString());
        return nullptr;
    }
    assert(qc.quorumHash == pQuorumBaseBlockIndex->GetBlockHash());

    const auto& llmq_params_opt = Params().GetLLMQ(llmqType);
    assert(llmq_params_opt.has_value());
    auto quorum = std::make_shared<CQuorum>(llmq_params_opt.value(), blsWorker);
    auto members = utils::GetAllQuorumMembers(qc.llmqType, m_dmnman, m_qsnapman, pQuorumBaseBlockIndex);

    quorum->Init(std::make_unique<CFinalCommitment>(std::move(qc)), pQuorumBaseBlockIndex, minedBlockHash, members);

    if (populate_cache && llmq_params_opt->size == 1) {
        WITH_LOCK(cs_map_quorums, mapQuorumsCache[llmqType].insert(quorumHash, quorum));

        return quorum;
    }

    bool hasValidVvec = false;
    if (WITH_LOCK(cs_db, return quorum->ReadContributions(*db))) {
        hasValidVvec = true;
    } else {
        if (BuildQuorumContributions(quorum->qc, quorum)) {
            WITH_LOCK(cs_db, quorum->WriteContributions(*db));
            hasValidVvec = true;
        } else {
            LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- llmqType[%d] quorumIndex[%d] quorum.ReadContributions and BuildQuorumContributions for quorumHash[%s] failed\n", __func__, ToUnderlying(llmqType), quorum->qc->quorumIndex, quorum->qc->quorumHash.ToString());
        }
    }

    if (hasValidVvec && populate_cache) {
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
    std::vector<CBLSSecretKey> skContributions;
    if (!dkgManager.GetVerifiedContributions((Consensus::LLMQType)fqc->llmqType, quorum->m_quorum_base_block_index, fqc->validMembers, memberIndexes, vvecs, skContributions)) {
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
    if (!quorum->SetSecretKeyShare(blsWorker.AggregateSecretKeys(skContributions), *m_mn_activeman)) {
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

bool CQuorumManager::RequestQuorumData(CNode* pfrom, CConnman& connman, CQuorumCPtr pQuorum, uint16_t nDataMask,
                                       const uint256& proTxHash) const
{
    if (pfrom == nullptr) {
        LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- Invalid pfrom: nullptr\n", __func__);
        return false;
    }
    if (pfrom->nVersion < LLMQ_DATA_MESSAGES_VERSION) {
        LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- Version must be %d or greater.\n", __func__, LLMQ_DATA_MESSAGES_VERSION);
        return false;
    }
    if (pfrom->GetVerifiedProRegTxHash().IsNull()) {
        LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- pfrom is not a verified masternode\n", __func__);
        return false;
    }
    const Consensus::LLMQType llmqType = pQuorum->qc->llmqType;
    if (!Params().GetLLMQ(llmqType).has_value()) {
        LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- Invalid llmqType: %d\n", __func__, ToUnderlying(llmqType));
        return false;
    }
    const CBlockIndex* pindex{pQuorum->m_quorum_base_block_index};
    if (pindex == nullptr) {
        LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- Invalid m_quorum_base_block_index : nullptr\n", __func__);
        return false;
    }

    LOCK(cs_data_requests);
    const CQuorumDataRequestKey key(pfrom->GetVerifiedProRegTxHash(), true, pindex->GetBlockHash(), llmqType);
    const CQuorumDataRequest request(llmqType, pindex->GetBlockHash(), nDataMask, proTxHash);
    auto [old_pair, inserted] = mapQuorumDataRequests.emplace(key, request);
    if (!inserted) {
        if (old_pair->second.IsExpired(/*add_bias=*/true)) {
            old_pair->second = request;
        } else {
            LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- Already requested\n", __func__);
            return false;
        }
    }
    LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- sending QGETDATA quorumHash[%s] llmqType[%d] proRegTx[%s]\n", __func__, key.quorumHash.ToString(),
             ToUnderlying(key.llmqType), key.proRegTx.ToString());

    CNetMsgMaker msgMaker(pfrom->GetCommonVersion());
    connman.PushMessage(pfrom, msgMaker.Make(NetMsgType::QGETDATA, request));

    return true;
}

std::vector<CQuorumCPtr> CQuorumManager::ScanQuorums(Consensus::LLMQType llmqType, size_t nCountRequested) const
{
    const CBlockIndex* pindex = WITH_LOCK(::cs_main, return m_chainstate.m_chain.Tip());
    return ScanQuorums(llmqType, pindex, nCountRequested);
}

std::vector<CQuorumCPtr> CQuorumManager::ScanQuorums(Consensus::LLMQType llmqType, const CBlockIndex* pindexStart, size_t nCountRequested) const
{
    if (pindexStart == nullptr || nCountRequested == 0 || !IsQuorumTypeEnabled(llmqType, pindexStart)) {
        return {};
    }

    gsl::not_null<const CBlockIndex*> pindexStore{pindexStart};
    const auto& llmq_params_opt = Params().GetLLMQ(llmqType);
    assert(llmq_params_opt.has_value());

    // Quorum sets can only change during the mining phase of DKG.
    // Find the closest known block index.
    const int quorumCycleStartHeight = pindexStart->nHeight - (pindexStart->nHeight % llmq_params_opt->dkgInterval);
    const int quorumCycleMiningStartHeight = quorumCycleStartHeight + llmq_params_opt->dkgMiningWindowStart;
    const int quorumCycleMiningEndHeight = quorumCycleStartHeight + llmq_params_opt->dkgMiningWindowEnd;

    if (pindexStart->nHeight < quorumCycleMiningStartHeight) {
        // too early for this cycle, use the previous one
        // bail out if it's below genesis block
        if (quorumCycleMiningEndHeight < llmq_params_opt->dkgInterval) return {};
        pindexStore = pindexStart->GetAncestor(quorumCycleMiningEndHeight - llmq_params_opt->dkgInterval);
    } else if (pindexStart->nHeight > quorumCycleMiningEndHeight) {
        // we are past the mining phase of this cycle, use it
        pindexStore = pindexStart->GetAncestor(quorumCycleMiningEndHeight);
    }
    // everything else is inside the mining phase of this cycle, no pindexStore adjustment needed

    gsl::not_null<const CBlockIndex*> pIndexScanCommitments{pindexStore};
    size_t nScanCommitments{nCountRequested};
    std::vector<CQuorumCPtr> vecResultQuorums;

    {
        LOCK(cs_scan_quorums);
        if (scanQuorumsCache.empty()) {
            for (const auto& llmq : Params().GetConsensus().llmqs) {
                // NOTE: We store it for each block hash in the DKG mining phase here
                // and not for a single quorum hash per quorum like we do for other caches.
                // And we only do this for max_cycles() of the most recent quorums
                // because signing by old quorums requires the exact quorum hash to be specified
                // and quorum scanning isn't needed there.
                scanQuorumsCache.try_emplace(llmq.type, llmq.max_cycles(llmq.keepOldConnections) * (llmq.dkgMiningWindowEnd - llmq.dkgMiningWindowStart));
            }
        }
        auto& cache = scanQuorumsCache[llmqType];
        bool fCacheExists = cache.get(pindexStore->GetBlockHash(), vecResultQuorums);
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
                // bail out if it's below genesis block
                if (vecResultQuorums.back()->m_quorum_base_block_index->pprev == nullptr) return {};
                pIndexScanCommitments = vecResultQuorums.back()->m_quorum_base_block_index->pprev;
            }
        } else {
            // If there is nothing in cache request at least keepOldConnections because this gets cached then later
            nScanCommitments = std::max(nCountRequested, static_cast<size_t>(llmq_params_opt->keepOldConnections));
        }
    }

    // Get the block indexes of the mined commitments to build the required quorums from
    std::vector<const CBlockIndex*> pQuorumBaseBlockIndexes{ llmq_params_opt->useRotation ?
            quorumBlockProcessor.GetMinedCommitmentsIndexedUntilBlock(llmqType, pIndexScanCommitments, nScanCommitments) :
            quorumBlockProcessor.GetMinedCommitmentsUntilBlock(llmqType, pIndexScanCommitments, nScanCommitments)
    };
    vecResultQuorums.reserve(vecResultQuorums.size() + pQuorumBaseBlockIndexes.size());

    for (auto& pQuorumBaseBlockIndex : pQuorumBaseBlockIndexes) {
        assert(pQuorumBaseBlockIndex);
        // populate cache for keepOldConnections most recent quorums only
        bool populate_cache = vecResultQuorums.size() < static_cast<size_t>(llmq_params_opt->keepOldConnections);

        // We assume that every quorum asked for is available to us on hand, if this
        // fails then we can assume that something has gone wrong and we should stop
        // trying to process any further and return a blank.
        auto quorum = GetQuorum(llmqType, pQuorumBaseBlockIndex, populate_cache);
        if (!quorum) {
            LogPrintf("%s: ERROR! Unexpected missing quorum with llmqType=%d, blockHash=%s, populate_cache=%s\n",
                      __func__, ToUnderlying(llmqType), pQuorumBaseBlockIndex->GetBlockHash().ToString(),
                      populate_cache ? "true" : "false");
            return {};
        }
        vecResultQuorums.emplace_back(quorum);
    }

    const size_t nCountResult{vecResultQuorums.size()};
    if (nCountResult > 0) {
        LOCK(cs_scan_quorums);
        // Don't cache more than keepOldConnections elements
        // because signing by old quorums requires the exact quorum hash
        // to be specified and quorum scanning isn't needed there.
        auto& cache = scanQuorumsCache[llmqType];
        const size_t nCacheEndIndex = std::min(nCountResult, static_cast<size_t>(llmq_params_opt->keepOldConnections));
        cache.emplace(pindexStore->GetBlockHash(), {vecResultQuorums.begin(), vecResultQuorums.begin() + nCacheEndIndex});
    }
    // Don't return more than nCountRequested elements
    const size_t nResultEndIndex = std::min(nCountResult, nCountRequested);
    return {vecResultQuorums.begin(), vecResultQuorums.begin() + nResultEndIndex};
}

CQuorumCPtr CQuorumManager::GetQuorum(Consensus::LLMQType llmqType, const uint256& quorumHash) const
{
    const CBlockIndex* pQuorumBaseBlockIndex = [&]() {
        // Lock contention may still be high here; consider using a shared lock
        // We cannot hold cs_quorumBaseBlockIndexCache the whole time as that creates lock-order inversion with cs_main;
        // We cannot acquire cs_main if we have cs_quorumBaseBlockIndexCache held
        const CBlockIndex* pindex;
        if (!WITH_LOCK(cs_quorumBaseBlockIndexCache, return quorumBaseBlockIndexCache.get(quorumHash, pindex))) {
            pindex = WITH_LOCK(::cs_main, return m_chainstate.m_blockman.LookupBlockIndex(quorumHash));
            if (pindex) {
                LOCK(cs_quorumBaseBlockIndexCache);
                quorumBaseBlockIndexCache.insert(quorumHash, pindex);
            }
        }
        return pindex;
    }();
    if (!pQuorumBaseBlockIndex) {
        LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- block %s not found\n", __func__, quorumHash.ToString());
        return nullptr;
    }
    return GetQuorum(llmqType, pQuorumBaseBlockIndex);
}

CQuorumCPtr CQuorumManager::GetQuorum(Consensus::LLMQType llmqType, gsl::not_null<const CBlockIndex*> pQuorumBaseBlockIndex, bool populate_cache) const
{
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

    return BuildQuorumFromCommitment(llmqType, pQuorumBaseBlockIndex, populate_cache);
}

size_t CQuorumManager::GetQuorumRecoveryStartOffset(const CQuorumCPtr pQuorum, const CBlockIndex* pIndex) const
{
    assert(m_mn_activeman);

    auto mns = m_dmnman.GetListForBlock(pIndex);
    std::vector<uint256> vecProTxHashes;
    vecProTxHashes.reserve(mns.GetValidMNsCount());
    mns.ForEachMN(true, [&](auto& pMasternode) {
        vecProTxHashes.emplace_back(pMasternode.proTxHash);
    });
    std::sort(vecProTxHashes.begin(), vecProTxHashes.end());
    size_t nIndex{0};
    {
        auto my_protx_hash = m_mn_activeman->GetProTxHash();
        for (const auto i : irange::range(vecProTxHashes.size())) {
            // cppcheck-suppress useStlAlgorithm
            if (my_protx_hash == vecProTxHashes[i]) {
                nIndex = i;
                break;
            }
        }
    }
    return nIndex % pQuorum->qc->validMembers.size();
}

MessageProcessingResult CQuorumManager::ProcessMessage(CNode& pfrom, CConnman& connman, std::string_view msg_type, CDataStream& vRecv)
{
    auto strFunc = __func__;
    auto errorHandler = [&](const std::string& strError, int nScore = 10) -> MessageProcessingResult {
        LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- %s: %s, from peer=%d\n", strFunc, msg_type, strError, pfrom.GetId());
        if (nScore > 0) {
            return MisbehavingError{nScore};
        }
        return {};
    };

    if (msg_type == NetMsgType::QGETDATA) {
        if (m_mn_activeman == nullptr || (pfrom.GetVerifiedProRegTxHash().IsNull() && !pfrom.qwatch)) {
            return errorHandler("Not a verified masternode or a qwatch connection");
        }

        CQuorumDataRequest request;
        vRecv >> request;

        auto sendQDATA = [&](CQuorumDataRequest::Errors nError,
                             bool request_limit_exceeded,
                             const CDataStream& body = CDataStream(SER_NETWORK, PROTOCOL_VERSION)) -> MessageProcessingResult {
            MessageProcessingResult ret{};
            switch (nError) {
                case (CQuorumDataRequest::Errors::NONE):
                case (CQuorumDataRequest::Errors::QUORUM_TYPE_INVALID):
                case (CQuorumDataRequest::Errors::QUORUM_BLOCK_NOT_FOUND):
                case (CQuorumDataRequest::Errors::QUORUM_NOT_FOUND):
                case (CQuorumDataRequest::Errors::MASTERNODE_IS_NO_MEMBER):
                case (CQuorumDataRequest::Errors::UNDEFINED):
                    if (request_limit_exceeded) ret = errorHandler("Request limit exceeded", 25);
                    break;
                case (CQuorumDataRequest::Errors::QUORUM_VERIFICATION_VECTOR_MISSING):
                case (CQuorumDataRequest::Errors::ENCRYPTED_CONTRIBUTIONS_MISSING):
                    // Do not punish limit exceed if we don't have the requested data
                    break;
            }
            request.SetError(nError);
            CDataStream ssResponse{SER_NETWORK, pfrom.GetCommonVersion()};
            ssResponse << request << body;
            connman.PushMessage(&pfrom, CNetMsgMaker(pfrom.GetCommonVersion()).Make(NetMsgType::QDATA, ssResponse));
            return ret;
        };

        bool request_limit_exceeded(false);
        {
            LOCK2(::cs_main, cs_data_requests);
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

        if (!Params().GetLLMQ(request.GetLLMQType()).has_value()) {
            return sendQDATA(CQuorumDataRequest::Errors::QUORUM_TYPE_INVALID, request_limit_exceeded);
        }

        const CBlockIndex* pQuorumBaseBlockIndex = WITH_LOCK(::cs_main, return m_chainstate.m_blockman.LookupBlockIndex(request.GetQuorumHash()));
        if (pQuorumBaseBlockIndex == nullptr) {
            return sendQDATA(CQuorumDataRequest::Errors::QUORUM_BLOCK_NOT_FOUND, request_limit_exceeded);
        }

        const auto pQuorum = GetQuorum(request.GetLLMQType(), pQuorumBaseBlockIndex);
        if (pQuorum == nullptr) {
            return sendQDATA(CQuorumDataRequest::Errors::QUORUM_NOT_FOUND, request_limit_exceeded);
        }

        CDataStream ssResponseData(SER_NETWORK, pfrom.GetCommonVersion());

        // Check if request wants QUORUM_VERIFICATION_VECTOR data
        if (request.GetDataMask() & CQuorumDataRequest::QUORUM_VERIFICATION_VECTOR) {
            if (!pQuorum->HasVerificationVector()) {
                return sendQDATA(CQuorumDataRequest::Errors::QUORUM_VERIFICATION_VECTOR_MISSING, request_limit_exceeded);
            }

            WITH_LOCK(pQuorum->cs_vvec_shShare, ssResponseData << *pQuorum->quorumVvec);
        }

        // Check if request wants ENCRYPTED_CONTRIBUTIONS data
        if (request.GetDataMask() & CQuorumDataRequest::ENCRYPTED_CONTRIBUTIONS) {

            int memberIdx = pQuorum->GetMemberIndex(request.GetProTxHash());
            if (memberIdx == -1) {
                return sendQDATA(CQuorumDataRequest::Errors::MASTERNODE_IS_NO_MEMBER, request_limit_exceeded);
            }

            std::vector<CBLSIESEncryptedObject<CBLSSecretKey>> vecEncrypted;
            if (!dkgManager.GetEncryptedContributions(request.GetLLMQType(), pQuorumBaseBlockIndex, pQuorum->qc->validMembers, request.GetProTxHash(), vecEncrypted)) {
                return sendQDATA(CQuorumDataRequest::Errors::ENCRYPTED_CONTRIBUTIONS_MISSING, request_limit_exceeded);
            }

            ssResponseData << vecEncrypted;
        }

        return sendQDATA(CQuorumDataRequest::Errors::NONE, request_limit_exceeded, ssResponseData);
    }

    if (msg_type == NetMsgType::QDATA) {
        if ((m_mn_activeman == nullptr && !IsWatchQuorumsEnabled()) || pfrom.GetVerifiedProRegTxHash().IsNull()) {
            return errorHandler("Not a verified masternode and -watchquorums is not enabled");
        }

        CQuorumDataRequest request;
        vRecv >> request;

        {
            LOCK2(::cs_main, cs_data_requests);
            const CQuorumDataRequestKey key(pfrom.GetVerifiedProRegTxHash(), true, request.GetQuorumHash(), request.GetLLMQType());
            auto it = mapQuorumDataRequests.find(key);
            if (it == mapQuorumDataRequests.end()) {
                return errorHandler("Not requested");
            }
            if (it->second.IsProcessed()) {
                return errorHandler("Already received");
            }
            if (request != it->second) {
                return errorHandler("Not like requested");
            }
            it->second.SetProcessed();
        }

        if (request.GetError() != CQuorumDataRequest::Errors::NONE) {
            return errorHandler(strprintf("Error %d (%s)", request.GetError(), request.GetErrorString()), 0);
        }

        CQuorumPtr pQuorum;
        {
            if (LOCK(cs_map_quorums); !mapQuorumsCache[request.GetLLMQType()].get(request.GetQuorumHash(), pQuorum)) {
                return errorHandler("Quorum not found", 0); // Don't bump score because we asked for it
            }
        }

        // Check if request has QUORUM_VERIFICATION_VECTOR data
        if (request.GetDataMask() & CQuorumDataRequest::QUORUM_VERIFICATION_VECTOR) {

            std::vector<CBLSPublicKey> verificationVector;
            vRecv >> verificationVector;

            if (pQuorum->SetVerificationVector(verificationVector)) {
                StartCachePopulatorThread(pQuorum);
            } else {
                return errorHandler("Invalid quorum verification vector");
            }
        }

        // Check if request has ENCRYPTED_CONTRIBUTIONS data
        if (request.GetDataMask() & CQuorumDataRequest::ENCRYPTED_CONTRIBUTIONS) {
            assert(m_mn_activeman);

            if (WITH_LOCK(pQuorum->cs_vvec_shShare, return pQuorum->quorumVvec->size() != size_t(pQuorum->params.threshold))) {
                return errorHandler("No valid quorum verification vector available", 0); // Don't bump score because we asked for it
            }

            int memberIdx = pQuorum->GetMemberIndex(request.GetProTxHash());
            if (memberIdx == -1) {
                return errorHandler("Not a member of the quorum", 0); // Don't bump score because we asked for it
            }

            std::vector<CBLSIESEncryptedObject<CBLSSecretKey>> vecEncrypted;
            vRecv >> vecEncrypted;

            std::vector<CBLSSecretKey> vecSecretKeys;
            vecSecretKeys.resize(vecEncrypted.size());
            for (const auto i : irange::range(vecEncrypted.size())) {
                if (!m_mn_activeman->Decrypt(vecEncrypted[i], memberIdx, vecSecretKeys[i], PROTOCOL_VERSION)) {
                    return errorHandler("Failed to decrypt");
                }
            }

            CBLSSecretKey secretKeyShare = blsWorker.AggregateSecretKeys(vecSecretKeys);
            if (!pQuorum->SetSecretKeyShare(secretKeyShare, *m_mn_activeman)) {
                return errorHandler("Invalid secret key share received");
            }
        }
        WITH_LOCK(cs_db, pQuorum->WriteContributions(*db));
        return {};
    }
    return {};
}

void CQuorumManager::StartCachePopulatorThread(const CQuorumCPtr pQuorum) const
{
    if (!pQuorum->HasVerificationVector()) {
        return;
    }

    cxxtimer::Timer t(true);
    LogPrint(BCLog::LLMQ, "CQuorumManager::StartCachePopulatorThread -- type=%d height=%d hash=%s start\n",
            ToUnderlying(pQuorum->params.type),
            pQuorum->m_quorum_base_block_index->nHeight,
            pQuorum->m_quorum_base_block_index->GetBlockHash().ToString());

    // when then later some other thread tries to get keys, it will be much faster
    workerPool.push([pQuorum, t, this](int threadId) {
        for (const auto i : irange::range(pQuorum->members.size())) {
            if (quorumThreadInterrupt) {
                break;
            }
            if (pQuorum->qc->validMembers[i]) {
                pQuorum->GetPubKeyShare(i);
            }
        }
        LogPrint(BCLog::LLMQ, "CQuorumManager::StartCachePopulatorThread -- type=%d height=%d hash=%s done. time=%d\n",
                ToUnderlying(pQuorum->params.type),
                pQuorum->m_quorum_base_block_index->nHeight,
                pQuorum->m_quorum_base_block_index->GetBlockHash().ToString(),
                t.count());
    });
}

void CQuorumManager::StartQuorumDataRecoveryThread(CConnman& connman, const CQuorumCPtr pQuorum,
                                                   const CBlockIndex* pIndex, uint16_t nDataMaskIn) const
{
    assert(m_mn_activeman);

    bool expected = false;
    if (!pQuorum->fQuorumDataRecoveryThreadRunning.compare_exchange_strong(expected, true)) {
        LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- Already running\n", __func__);
        return;
    }

    workerPool.push([&connman, pQuorum, pIndex, nDataMaskIn, this](int threadId) {
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

        while (!m_mn_sync.IsBlockchainSynced() && !quorumThreadInterrupt) {
            quorumThreadInterrupt.sleep_for(std::chrono::seconds(nRequestTimeout));
        }

        if (quorumThreadInterrupt) {
            printLog("Aborted");
            return;
        }

        vecMemberHashes.reserve(pQuorum->qc->validMembers.size());
        for (auto& member : pQuorum->members) {
            if (pQuorum->IsValidMember(member->proTxHash) && member->proTxHash != m_mn_activeman->GetProTxHash()) {
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

            if ((GetTime<std::chrono::seconds>().count() - nTimeLastSuccess) > nRequestTimeout) {
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
                nTimeLastSuccess = GetTime<std::chrono::seconds>().count();
                connman.AddPendingMasternode(*pCurrentMemberHash);
                printLog("Connect");
            }

            auto proTxHash = m_mn_activeman->GetProTxHash();
            connman.ForEachNode([&](CNode* pNode) {
                auto verifiedProRegTxHash = pNode->GetVerifiedProRegTxHash();
                if (pCurrentMemberHash == nullptr || verifiedProRegTxHash != *pCurrentMemberHash) {
                    return;
                }

                if (RequestQuorumData(pNode, connman, pQuorum, nDataMask, proTxHash)) {
                    nTimeLastSuccess = GetTime<std::chrono::seconds>().count();
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

static void DataCleanupHelper(CDBWrapper& db, std::set<uint256> skip_list, bool compact = false)
{
    const auto prefixes = {DB_QUORUM_QUORUM_VVEC, DB_QUORUM_SK_SHARE};

    CDBBatch batch(db);
    std::unique_ptr<CDBIterator> pcursor(db.NewIterator());

    for (const auto& prefix : prefixes) {
        auto start = std::make_tuple(prefix, uint256());
        pcursor->Seek(start);

        int count{0};
        while (pcursor->Valid()) {
            decltype(start) k;

            if (!pcursor->GetKey(k) || std::get<0>(k) != prefix) {
                break;
            }

            pcursor->Next();

            if (skip_list.find(std::get<1>(k)) != skip_list.end()) continue;

            ++count;
            batch.Erase(k);

            if (batch.SizeEstimate() >= (1 << 24)) {
                db.WriteBatch(batch);
                batch.Clear();
            }
        }

        db.WriteBatch(batch);

        LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- %s removed %d\n", __func__, prefix, count);
    }

    pcursor.reset();

    if (compact) {
        // Avoid using this on regular cleanups, use on db migrations only
        LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- compact start\n", __func__);
        db.CompactFull();
        LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- compact end\n", __func__);
    }
}

void CQuorumManager::StartCleanupOldQuorumDataThread(const CBlockIndex* pIndex) const
{
    // Note: this function is CPU heavy and we don't want it to be running during DKGs.
    // The largest dkgMiningWindowStart for a related quorum type is 42 (LLMQ_60_75).
    // At the same time most quorums use dkgInterval = 24 so the next DKG for them
    // (after block 576 + 42) will start at block 576 + 24 * 2. That's only a 6 blocks
    // window and it's better to have more room so we pick next cycle.
    // dkgMiningWindowStart for small quorums is 10 i.e. a safe block to start
    // these calculations is at height 576 + 24 * 2 + 10 = 576 + 58.
    if ((m_mn_activeman == nullptr && !IsWatchQuorumsEnabled()) || pIndex == nullptr || (pIndex->nHeight % 576 != 58)) {
        return;
    }

    cxxtimer::Timer t(/*start=*/ true);
    LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- start\n", __func__);

    // do not block the caller thread
    workerPool.push([pIndex, t, this](int threadId) {
        std::set<uint256> dbKeysToSkip;

        if (LOCK(cs_cleanup); cleanupQuorumsCache.empty()) {
            utils::InitQuorumsCache(cleanupQuorumsCache, false);
        }
        for (const auto& params : Params().GetConsensus().llmqs) {
            if (quorumThreadInterrupt) {
                break;
            }
            LOCK(cs_cleanup);
            auto& cache = cleanupQuorumsCache[params.type];
            const CBlockIndex* pindex_loop{pIndex};
            std::set<uint256> quorum_keys;
            while (pindex_loop != nullptr && pIndex->nHeight - pindex_loop->nHeight < params.max_store_depth()) {
                uint256 quorum_key;
                if (cache.get(pindex_loop->GetBlockHash(), quorum_key)) {
                    quorum_keys.insert(quorum_key);
                    if (quorum_keys.size() >= static_cast<size_t>(params.keepOldKeys)) break; // extra safety belt
                }
                pindex_loop = pindex_loop->pprev;
            }
            for (const auto& pQuorum : ScanQuorums(params.type, pIndex, params.keepOldKeys - quorum_keys.size())) {
                const uint256 quorum_key = MakeQuorumKey(*pQuorum);
                quorum_keys.insert(quorum_key);
                cache.insert(pQuorum->m_quorum_base_block_index->GetBlockHash(), quorum_key);
            }
            dbKeysToSkip.merge(quorum_keys);
        }

        if (!quorumThreadInterrupt) {
            WITH_LOCK(cs_db, DataCleanupHelper(*db, dbKeysToSkip));
        }

        LogPrint(BCLog::LLMQ, "CQuorumManager::StartCleanupOldQuorumDataThread -- done. time=%d\n", t.count());
    });
}

// TODO: remove in v23
void CQuorumManager::MigrateOldQuorumDB(CEvoDB& evoDb) const
{
    LOCK(cs_db);
    if (!db->IsEmpty()) return;

    const auto prefixes = {DB_QUORUM_QUORUM_VVEC, DB_QUORUM_SK_SHARE};

    LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- start\n", __func__);

    CDBBatch batch(*db);
    std::unique_ptr<CDBIterator> pcursor(evoDb.GetRawDB().NewIterator());

    for (const auto& prefix : prefixes) {
        auto start = std::make_tuple(prefix, uint256());
        pcursor->Seek(start);

        int count{0};
        while (pcursor->Valid()) {
            decltype(start) k;
            CDataStream s(SER_DISK, CLIENT_VERSION);
            CBLSSecretKey sk;

            if (!pcursor->GetKey(k) || std::get<0>(k) != prefix) {
                break;
            }

            if (prefix == DB_QUORUM_QUORUM_VVEC) {
                if (!evoDb.GetRawDB().ReadDataStream(k, s)) {
                    break;
                }
                batch.Write(k, s);
            }
            if (prefix == DB_QUORUM_SK_SHARE) {
                if (!pcursor->GetValue(sk)) {
                    break;
                }
                batch.Write(k, sk);
            }

            if (batch.SizeEstimate() >= (1 << 24)) {
                db->WriteBatch(batch);
                batch.Clear();
            }

            ++count;
            pcursor->Next();
        }

        db->WriteBatch(batch);

        LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- %s moved %d\n", __func__, prefix, count);
    }

    pcursor.reset();
    db->CompactFull();

    DataCleanupHelper(evoDb.GetRawDB(), {});
    evoDb.CommitRootTransaction();

    LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- done\n", __func__);
}

CQuorumCPtr SelectQuorumForSigning(const Consensus::LLMQParams& llmq_params, const CChain& active_chain, const CQuorumManager& qman,
                                   const uint256& selectionHash, int signHeight, int signOffset)
{
    size_t poolSize = llmq_params.signingActiveQuorumCount;

    CBlockIndex* pindexStart;
    {
        LOCK(::cs_main);
        if (signHeight == -1) {
            signHeight = active_chain.Height();
        }
        int startBlockHeight = signHeight - signOffset;
        if (startBlockHeight > active_chain.Height() || startBlockHeight < 0) {
            return {};
        }
        pindexStart = active_chain[startBlockHeight];
    }

    if (IsQuorumRotationEnabled(llmq_params, pindexStart)) {
        auto quorums = qman.ScanQuorums(llmq_params.type, pindexStart, poolSize);
        if (quorums.empty()) {
            return nullptr;
        }
        //log2 int
        int n = std::log2(llmq_params.signingActiveQuorumCount);
        //Extract last 64 bits of selectionHash
        uint64_t b = selectionHash.GetUint64(3);
        //Take last n bits of b
        uint64_t signer = (((1ull << n) - 1) & (b >> (64 - n - 1)));

        if (signer > quorums.size()) {
            return nullptr;
        }
        auto itQuorum = std::find_if(quorums.begin(),
                                     quorums.end(),
                                     [signer](const CQuorumCPtr& obj) {
                                         return uint64_t(obj->qc->quorumIndex) == signer;
                                     });
        if (itQuorum == quorums.end()) {
            return nullptr;
        }
        return *itQuorum;
    } else {
        auto quorums = qman.ScanQuorums(llmq_params.type, pindexStart, poolSize);
        if (quorums.empty()) {
            return nullptr;
        }

        std::vector<std::pair<uint256, size_t>> scores;
        scores.reserve(quorums.size());
        for (const auto i : irange::range(quorums.size())) {
            CHashWriter h(SER_NETWORK, 0);
            h << llmq_params.type;
            h << quorums[i]->qc->quorumHash;
            h << selectionHash;
            scores.emplace_back(h.GetHash(), i);
        }
        std::sort(scores.begin(), scores.end());
        return quorums[scores.front().second];
    }
}

VerifyRecSigStatus VerifyRecoveredSig(Consensus::LLMQType llmqType, const CChain& active_chain, const CQuorumManager& qman,
                        int signedAtHeight, const uint256& id, const uint256& msgHash, const CBLSSignature& sig,
                        const int signOffset)
{
    const auto& llmq_params_opt = Params().GetLLMQ(llmqType);
    assert(llmq_params_opt.has_value());
    auto quorum = SelectQuorumForSigning(llmq_params_opt.value(), active_chain, qman, id, signedAtHeight, signOffset);
    if (!quorum) {
        return VerifyRecSigStatus::NoQuorum;
    }

    SignHash signHash(llmqType, quorum->qc->quorumHash, id, msgHash);
    const bool ret = sig.VerifyInsecure(quorum->qc->quorumPublicKey, signHash.Get());
    return ret ? VerifyRecSigStatus::Valid : VerifyRecSigStatus::Invalid;
}
} // namespace llmq
