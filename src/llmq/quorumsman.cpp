// Copyright (c) 2018-2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/quorumsman.h>

#include <bls/bls.h>
#include <bls/bls_ies.h>
#include <evo/deterministicmns.h>
#include <evo/evodb.h>
#include <llmq/blockprocessor.h>
#include <llmq/commitment.h>
#include <llmq/dkgsessionmgr.h>
#include <llmq/options.h>
#include <llmq/params.h>
#include <llmq/signhash.h>
#include <llmq/utils.h>
#include <util/irange.h>

#include <chainparams.h>
#include <dbwrapper.h>
#include <net.h>
#include <netmessagemaker.h>
#include <util/thread.h>
#include <util/time.h>
#include <util/underlying.h>
#include <validation.h>

#include <cxxtimer.hpp>

namespace llmq {
CQuorumManager::CQuorumManager(CBLSWorker& _blsWorker, CDeterministicMNManager& dmnman, CEvoDB& _evoDb,
                               CQuorumBlockProcessor& _quorumBlockProcessor, CQuorumSnapshotManager& qsnapman,
                               const ChainstateManager& chainman, const util::DbWrapperParams& db_params) :
    blsWorker{_blsWorker},
    m_dmnman{dmnman},
    quorumBlockProcessor{_quorumBlockProcessor},
    m_qsnapman{qsnapman},
    m_chainman{chainman},
    db{util::MakeDbWrapper({db_params.path / "llmq" / "quorumdb", db_params.memory, db_params.wipe, /*cache_size=*/1 << 20})}
{
    utils::InitQuorumsCache(mapQuorumsCache, m_chainman.GetConsensus(), /*limit_by_connections=*/false);
    m_cache_interrupt.reset();
    m_cache_thread = std::thread(&util::TraceThread, "q-cache", [this] { CacheWarmingThreadMain(); });
    MigrateOldQuorumDB(_evoDb);
}

CQuorumManager::~CQuorumManager()
{
    if (m_cache_thread.joinable()) {
        m_cache_interrupt();
        m_cache_thread.join();
    }
}

bool CQuorumManager::GetEncryptedContributions(Consensus::LLMQType llmq_type, const CBlockIndex* block_index,
                                               const std::vector<bool>& valid_members, const uint256& protx_hash,
                                               std::vector<CBLSIESEncryptedObject<CBLSSecretKey>>& vec_enc) const
{
    if (m_qdkgsman) {
        return m_qdkgsman->GetEncryptedContributions(llmq_type, block_index, valid_members, protx_hash, vec_enc);
    }
    return false;
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
    auto members = utils::GetAllQuorumMembers(qc.llmqType, {m_dmnman, m_qsnapman, m_chainman, pQuorumBaseBlockIndex});

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
        QueueQuorumForWarming(quorum);
    }

    WITH_LOCK(cs_map_quorums, mapQuorumsCache[llmqType].insert(quorumHash, quorum));

    return quorum;
}

bool CQuorumManager::BuildQuorumContributions(const CFinalCommitmentPtr& fqc, const std::shared_ptr<CQuorum>& quorum) const
{
    std::vector<uint16_t> memberIndexes;
    std::vector<BLSVerificationVectorPtr> vvecs;
    std::vector<CBLSSecretKey> skContributions;
    if (!m_qdkgsman ||
        !m_qdkgsman->GetVerifiedContributions((Consensus::LLMQType)fqc->llmqType, quorum->m_quorum_base_block_index,
                                            fqc->validMembers, memberIndexes, vvecs, skContributions)) {
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
    if (!m_handler || !m_handler->SetQuorumSecretKeyShare(*quorum, skContributions)) {
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

bool CQuorumManager::RequestQuorumData(CNode* pfrom, CConnman& connman, const CQuorum& quorum, uint16_t nDataMask,
                                       const uint256& proTxHash) const
{
    if (pfrom == nullptr) {
        LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- Invalid pfrom: nullptr\n", __func__);
        return false;
    }
    if (pfrom->GetVerifiedProRegTxHash().IsNull()) {
        LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- pfrom is not a verified masternode\n", __func__);
        return false;
    }
    const Consensus::LLMQType llmqType = quorum.qc->llmqType;
    if (!Params().GetLLMQ(llmqType).has_value()) {
        LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- Invalid llmqType: %d\n", __func__, ToUnderlying(llmqType));
        return false;
    }
    const CBlockIndex* pindex{quorum.m_quorum_base_block_index};
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
    const CBlockIndex* pindex = WITH_LOCK(::cs_main, return m_chainman.ActiveTip());
    return ScanQuorums(llmqType, pindex, nCountRequested);
}

std::vector<CQuorumCPtr> CQuorumManager::ScanQuorums(Consensus::LLMQType llmqType,
                                                     gsl::not_null<const CBlockIndex*> pindexStart,
                                                     size_t nCountRequested) const
{
    if (nCountRequested == 0 || !m_chainman.IsQuorumTypeEnabled(llmqType, pindexStart)) {
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

bool CQuorumManager::IsMasternode() const
{
    if (m_handler) {
        return m_handler->IsMasternode();
    }
    return false;
}

bool CQuorumManager::IsWatching() const
{
    if (m_handler) {
        return m_handler->IsWatching();
    }
    return false;
}

bool CQuorumManager::IsDataRequestPending(const uint256& proRegTx, bool we_requested, const uint256& quorumHash,
                                          Consensus::LLMQType llmqType) const
{
    const CQuorumDataRequestKey key{proRegTx, we_requested, quorumHash, llmqType};
    LOCK(cs_data_requests);
    const auto it = mapQuorumDataRequests.find(key);
    return it != mapQuorumDataRequests.end() && !it->second.IsExpired(/*add_bias=*/true);
}

DataRequestStatus CQuorumManager::GetDataRequestStatus(const uint256& proRegTx, bool we_requested,
                                                       const uint256& quorumHash, Consensus::LLMQType llmqType) const
{
    const CQuorumDataRequestKey key{proRegTx, we_requested, quorumHash, llmqType};
    LOCK(cs_data_requests);
    const auto it = mapQuorumDataRequests.find(key);
    if (it == mapQuorumDataRequests.end()) {
        return DataRequestStatus::NotFound;
    }
    if (it->second.IsProcessed()) {
        return DataRequestStatus::Processed;
    }
    return DataRequestStatus::Pending;
}

void CQuorumManager::CleanupExpiredDataRequests() const
{
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

void CQuorumManager::CleanupOldQuorumData(const std::set<uint256>& dbKeysToSkip) const
{
    LOCK(cs_db);
    DataCleanupHelper(*db, dbKeysToSkip);
}

CQuorumCPtr CQuorumManager::GetQuorum(Consensus::LLMQType llmqType, const uint256& quorumHash) const
{
    const CBlockIndex* pQuorumBaseBlockIndex = [&]() {
        // Lock contention may still be high here; consider using a shared lock
        // We cannot hold cs_quorumBaseBlockIndexCache the whole time as that creates lock-order inversion with cs_main;
        // We cannot acquire cs_main if we have cs_quorumBaseBlockIndexCache held
        const CBlockIndex* pindex;
        if (!WITH_LOCK(cs_quorumBaseBlockIndexCache, return quorumBaseBlockIndexCache.get(quorumHash, pindex))) {
            pindex = WITH_LOCK(::cs_main, return m_chainman.m_blockman.LookupBlockIndex(quorumHash));
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

MessageProcessingResult CQuorumManager::ProcessMessage(CNode& pfrom, CConnman& connman, std::string_view msg_type, CDataStream& vRecv)
{
    if (msg_type == NetMsgType::QGETDATA) {
        if (!IsMasternode() || (pfrom.GetVerifiedProRegTxHash().IsNull() && !pfrom.qwatch)) {
            return MisbehavingError{10, "not a verified masternode or a qwatch connection"};
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
                    if (request_limit_exceeded) ret = MisbehavingError{25, "request limit exceeded"};
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

        const CBlockIndex* pQuorumBaseBlockIndex = WITH_LOCK(::cs_main, return m_chainman.m_blockman.LookupBlockIndex(request.GetQuorumHash()));
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
        CQuorumDataRequest::Errors ret_err{CQuorumDataRequest::Errors::NONE};
        MessageProcessingResult qdata_ret{}, ret{};
        if (m_handler) {
            ret = m_handler->ProcessContribQGETDATA(request_limit_exceeded, ssResponseData, *pQuorum, request, pQuorumBaseBlockIndex);
            if (auto request_err = request.GetError(); request_err != CQuorumDataRequest::Errors::NONE &&
                                                       request_err != CQuorumDataRequest::Errors::UNDEFINED) {
                ret_err = request_err;
            }
        }
        // sendQDATA also pushes a message independent of the returned value
        if (ret_err != CQuorumDataRequest::Errors::NONE) {
            qdata_ret = sendQDATA(ret_err, request_limit_exceeded);
        } else {
            qdata_ret = sendQDATA(CQuorumDataRequest::Errors::NONE, request_limit_exceeded, ssResponseData);
        }
        return ret.empty() ? qdata_ret : ret;
    }

    if (msg_type == NetMsgType::QDATA) {
        if ((!IsMasternode() && !IsWatching()) || pfrom.GetVerifiedProRegTxHash().IsNull()) {
            return MisbehavingError{10, "not a verified masternode and -watchquorums is not enabled"};
        }

        CQuorumDataRequest request;
        vRecv >> request;

        {
            LOCK2(::cs_main, cs_data_requests);
            const CQuorumDataRequestKey key(pfrom.GetVerifiedProRegTxHash(), true, request.GetQuorumHash(), request.GetLLMQType());
            auto it = mapQuorumDataRequests.find(key);
            if (it == mapQuorumDataRequests.end()) {
                return MisbehavingError{10, "not requested"};
            }
            if (it->second.IsProcessed()) {
                return MisbehavingError(10, "already received");
            }
            if (request != it->second) {
                return MisbehavingError(10, "not like requested");
            }
            it->second.SetProcessed();
        }

        if (request.GetError() != CQuorumDataRequest::Errors::NONE) {
            LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- %s: Error %d (%s), from peer=%d\n", __func__, msg_type, request.GetError(), request.GetErrorString(), pfrom.GetId());
            return {};
        }

        CQuorumPtr pQuorum;
        {
            if (LOCK(cs_map_quorums); !mapQuorumsCache[request.GetLLMQType()].get(request.GetQuorumHash(), pQuorum)) {
                // Don't bump score because we asked for it
                LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- %s: Quorum not found, from peer=%d\n", __func__, msg_type, pfrom.GetId());
                return {};
            }
        }

        // Check if request has QUORUM_VERIFICATION_VECTOR data
        if (request.GetDataMask() & CQuorumDataRequest::QUORUM_VERIFICATION_VECTOR) {

            std::vector<CBLSPublicKey> verificationVector;
            vRecv >> verificationVector;

            if (pQuorum->SetVerificationVector(verificationVector)) {
                QueueQuorumForWarming(pQuorum);
            } else {
                return MisbehavingError{10, "invalid quorum verification vector"};
            }
        }

        // Check if request has ENCRYPTED_CONTRIBUTIONS data
        if (m_handler) {
            if (auto ret = m_handler->ProcessContribQDATA(pfrom, vRecv, *pQuorum, request); !ret.empty()) {
                return ret;
            }
        }

        WITH_LOCK(cs_db, pQuorum->WriteContributions(*db));
        return {};
    }

    return {};
}

void CQuorumManager::CacheWarmingThreadMain() const
{
    while (!m_cache_interrupt) {
        CQuorumCPtr pQuorum;
        {
            LOCK(m_cache_cs);
            if (!m_cache_queue.empty()) {
                pQuorum = std::move(m_cache_queue.front());
                m_cache_queue.pop_front();
            };
        }

        if (!pQuorum) {
            m_cache_interrupt.sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        cxxtimer::Timer t(true);
        LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- type=%d height=%d hash=%s start\n", __func__,
                 ToUnderlying(pQuorum->params.type), pQuorum->m_quorum_base_block_index->nHeight,
                 pQuorum->m_quorum_base_block_index->GetBlockHash().ToString());

        // when then later some other thread tries to get keys, it will be much faster
        for (const auto i : irange::range(pQuorum->members.size())) {
            if (m_cache_interrupt) {
                break;
            }
            if (pQuorum->qc->validMembers[i]) {
                pQuorum->GetPubKeyShare(i);
            }
        }

        LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- type=%d height=%d hash=%s done. time=%d\n", __func__,
                 ToUnderlying(pQuorum->params.type), pQuorum->m_quorum_base_block_index->nHeight,
                 pQuorum->m_quorum_base_block_index->GetBlockHash().ToString(), t.count());
    }
}

void CQuorumManager::QueueQuorumForWarming(CQuorumCPtr pQuorum) const
{
    if (pQuorum->HasVerificationVector()) {
        LOCK(m_cache_cs);
        m_cache_queue.push_back(std::move(pQuorum));
    }
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

    SignHash signHash{llmqType, quorum->qc->quorumHash, id, msgHash};
    const bool ret = sig.VerifyInsecure(quorum->qc->quorumPublicKey, signHash.Get());
    return ret ? VerifyRecSigStatus::Valid : VerifyRecSigStatus::Invalid;
}
} // namespace llmq
