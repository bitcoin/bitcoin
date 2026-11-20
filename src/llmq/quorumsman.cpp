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
#include <util/check.h>
#include <util/helpers.h>
#include <util/std23.h>

#include <chainparams.h>
#include <dbwrapper.h>
#include <logging.h>
#include <util/thread.h>
#include <util/time.h>
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
    AssertLockHeld(::cs_main);
    mapQuorumsCache.Init(m_chainman.GetConsensus(), /*limit_by_connections=*/false);
    MigrateOldQuorumDB(_evoDb);
    m_cache_interrupt.reset();
    m_cache_thread = std::thread(&util::TraceThread, "q-cache", [this] { CacheWarmingThreadMain(); });
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
        LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- No mined commitment for llmqType[%d] nHeight[%d] quorumHash[%s]\n", __func__, std23::to_underlying(llmqType), pQuorumBaseBlockIndex->nHeight, pQuorumBaseBlockIndex->GetBlockHash().ToString());
        return nullptr;
    }
    assert(qc.quorumHash == pQuorumBaseBlockIndex->GetBlockHash());

    const auto& llmq_params_opt = Params().GetLLMQ(llmqType);
    assert(llmq_params_opt.has_value());
    auto members = utils::GetAllQuorumMembers(qc.llmqType, {m_dmnman, m_qsnapman, m_chainman, pQuorumBaseBlockIndex});

    auto quorum = std::make_shared<CQuorum>(llmq_params_opt.value(), blsWorker,
        std::make_unique<CFinalCommitment>(std::move(qc)), pQuorumBaseBlockIndex, minedBlockHash, members);

    if (populate_cache && llmq_params_opt->size == 1) {
        WITH_LOCK(m_cs_maps, mapQuorumsCache.insert(llmqType, quorumHash, quorum));

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
            LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- llmqType[%d] quorumIndex[%d] quorum.ReadContributions and BuildQuorumContributions for quorumHash[%s] failed\n", __func__, std23::to_underlying(llmqType), quorum->qc->quorumIndex, quorum->qc->quorumHash.ToString());
        }
    }

    if (hasValidVvec && populate_cache) {
        // pre-populate caches in the background
        // recovering public key shares is quite expensive and would result in serious lags for the first few signing
        // sessions if the shares would be calculated on-demand
        QueueQuorumForWarming(quorum);
    }

    WITH_LOCK(m_cs_maps, mapQuorumsCache.insert(llmqType, quorumHash, quorum));

    return quorum;
}

bool CQuorumManager::BuildQuorumContributions(const CFinalCommitmentPtr& fqc, const std::shared_ptr<CQuorum>& quorum) const
{
    std::vector<uint16_t> memberIndexes;
    std::vector<BLSVerificationVectorPtr> vvecs;
    std::vector<CBLSSecretKey> skContributions;
    if (!m_qdkgsman ||
        !m_qdkgsman->GetVerifiedContributions(fqc->llmqType, quorum->m_quorum_base_block_index,
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

bool CQuorumManager::HasQuorum(Consensus::LLMQType llmqType, const CQuorumBlockProcessor& quorum_block_processor,
                               const uint256& quorumHash, const CChain& chain)
{
    AssertLockHeld(::cs_main);
    return quorum_block_processor.HasMinedCommitment(llmqType, quorumHash, chain);
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
    return ScanQuorums(llmqType, pindexStart, nCountRequested, nullptr);
}

std::vector<CQuorumCPtr> CQuorumManager::ScanQuorums(Consensus::LLMQType llmqType,
                                                     gsl::not_null<const CBlockIndex*> pindexStart,
                                                     size_t nCountRequested, const CChain& chain) const
{
    AssertLockHeld(::cs_main);
    return ScanQuorums(llmqType, pindexStart, nCountRequested, &chain);
}

std::vector<CQuorumCPtr> CQuorumManager::ScanQuorums(Consensus::LLMQType llmqType,
                                                     gsl::not_null<const CBlockIndex*> pindexStart,
                                                     size_t nCountRequested, const CChain* chain) const
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

    if (chain == nullptr) {
        LOCK(m_cs_maps);
        if (!scanQuorumsCache.IsInitialized()) {
            // NOTE: We store it for each block hash in the DKG mining phase here
            // and not for a single quorum hash per quorum like we do for other caches.
            // And we only do this for max_cycles() of the most recent quorums
            // because signing by old quorums requires the exact quorum hash to be specified
            // and quorum scanning isn't needed there.
            scanQuorumsCache.Init(Params().GetConsensus(), [](const Consensus::LLMQParams& llmq) {
                return llmq.max_cycles(llmq.keepOldConnections) * (llmq.dkgMiningWindowEnd - llmq.dkgMiningWindowStart);
            });
        }
        bool fCacheExists = scanQuorumsCache.get(llmqType, pindexStore->GetBlockHash(), vecResultQuorums);
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
    } else {
        nScanCommitments = std::max(nCountRequested, static_cast<size_t>(llmq_params_opt->keepOldConnections));
    }

    // Get the block indexes of the mined commitments to build the required quorums from.
    // The mined commitment index follows the active chain, so a scan from a block that is
    // not on it (e.g. a tip notification processed after a reorg) reads another branch.
    // Check that under the same cs_main hold so a reorg cannot slip in between.
    bool scan_on_active_chain{false};
    std::vector<const CBlockIndex*> pQuorumBaseBlockIndexes;
    {
        LOCK(::cs_main);
        const CChain& active_chain{m_chainman.ActiveChain()};
        scan_on_active_chain = active_chain.Contains(pindexStore) && active_chain.Contains(pIndexScanCommitments);
        pQuorumBaseBlockIndexes = llmq_params_opt->useRotation
                                      ? quorumBlockProcessor.GetMinedCommitmentsIndexedUntilBlock(llmqType,
                                                                                                  pIndexScanCommitments,
                                                                                                  nScanCommitments)
                                      : quorumBlockProcessor.GetMinedCommitmentsUntilBlock(llmqType, pIndexScanCommitments,
                                                                                           nScanCommitments);
    }
    vecResultQuorums.reserve(vecResultQuorums.size() + pQuorumBaseBlockIndexes.size());

    for (auto& pQuorumBaseBlockIndex : pQuorumBaseBlockIndexes) {
        assert(pQuorumBaseBlockIndex);
        // populate cache for keepOldConnections most recent quorums only
        bool populate_cache = vecResultQuorums.size() < static_cast<size_t>(llmq_params_opt->keepOldConnections);

        // We assume that every quorum asked for is available to us on hand, if this
        // fails then we can assume that something has gone wrong and we should stop
        // trying to process any further and return a blank.
        CQuorumCPtr quorum;
        if (chain) {
            quorum = [&]() NO_THREAD_SAFETY_ANALYSIS {
                return GetQuorum(llmqType, pQuorumBaseBlockIndex, *chain, populate_cache);
            }();
        } else {
            quorum = GetQuorum(llmqType, pQuorumBaseBlockIndex, populate_cache);
        }
        if (!quorum) {
            LogPrintf("%s: ERROR! Unexpected missing quorum with llmqType=%d, blockHash=%s, populate_cache=%s\n",
                      __func__, std23::to_underlying(llmqType), pQuorumBaseBlockIndex->GetBlockHash().ToString(),
                      util::to_string(populate_cache));
            return {};
        }
        vecResultQuorums.emplace_back(quorum);
    }

    const size_t nCountResult{vecResultQuorums.size()};
    if (nCountResult > 0 && chain == nullptr && scan_on_active_chain) {
        LOCK2(::cs_main, m_cs_maps);
        // GetQuorum() read commitments by quorum hash without cs_main; if pindexStore was disconnected
        // meanwhile they may be another branch's commitments for the same quorums.
        if (m_chainman.ActiveChain().Contains(pindexStore)) {
            // Don't cache more than keepOldConnections elements
            // because signing by old quorums requires the exact quorum hash
            // to be specified and quorum scanning isn't needed there.
            const size_t nCacheEndIndex = std::min(nCountResult, static_cast<size_t>(llmq_params_opt->keepOldConnections));
            scanQuorumsCache.emplace(llmqType, pindexStore->GetBlockHash(),
                                     {vecResultQuorums.begin(), vecResultQuorums.begin() + nCacheEndIndex});
        }
    }
    // Don't return more than nCountRequested elements
    const size_t nResultEndIndex = std::min(nCountResult, nCountRequested);
    return {vecResultQuorums.begin(), vecResultQuorums.begin() + nResultEndIndex};
}

std::vector<CFinalCommitment> CQuorumManager::ScanCommitments(Consensus::LLMQType llmqType, size_t nCountRequested) const
{
    const CBlockIndex* pindex = WITH_LOCK(::cs_main, return m_chainman.ActiveTip());
    return ScanCommitments(llmqType, pindex, nCountRequested);
}

std::vector<CFinalCommitment> CQuorumManager::ScanCommitments(Consensus::LLMQType llmqType,
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
    std::vector<CFinalCommitment> vecResultCommitments;
    std::optional<uint256> continuationQuorumHash;

    {
        LOCK(m_cs_maps);
        if (scanCommitmentsCache.empty()) {
            for (const auto& llmq : Params().GetConsensus().llmqs) {
                scanCommitmentsCache.try_emplace(llmq.type, llmq.max_cycles(llmq.keepOldConnections) * (llmq.dkgMiningWindowEnd - llmq.dkgMiningWindowStart));
            }
        }
        auto& cache = scanCommitmentsCache[llmqType];
        bool fCacheExists = cache.get(pindexStore->GetBlockHash(), vecResultCommitments);
        if (fCacheExists) {
            // We have exactly what requested so just return it
            if (vecResultCommitments.size() == nCountRequested) {
                return vecResultCommitments;
            }
            // If we have more cached than requested return only a subvector
            if (vecResultCommitments.size() > nCountRequested) {
                return {vecResultCommitments.begin(), vecResultCommitments.begin() + nCountRequested};
            }
            // If we have cached quorums but not enough, subtract what we have from the count and the set correct index where to start
            // scanning for the rests
            if (!vecResultCommitments.empty()) {
                nScanCommitments -= vecResultCommitments.size();
                // Resolve the continuation point after releasing m_cs_maps. Both the
                // mined-commitment lookup and block-index lookup take locks whose
                // order is incompatible with m_cs_maps.
                continuationQuorumHash = vecResultCommitments.back().quorumHash;
            }
        } else {
            // If there is nothing in cache request at least keepOldConnections because this gets cached then later
            nScanCommitments = std::max(nCountRequested, static_cast<size_t>(llmq_params_opt->keepOldConnections));
        }
    }

    if (continuationQuorumHash) {
        uint256 continuationBlockHash;
        if (llmq_params_opt->useRotation) {
            // Rotating scans are keyed by quorum base blocks. Resuming from the
            // mined block would revisit the other commitments from the same cycle.
            continuationBlockHash = *continuationQuorumHash;
        } else {
            continuationBlockHash = quorumBlockProcessor.GetMinedCommitment(llmqType, *continuationQuorumHash).second;
        }
        const CBlockIndex* pLastIndex = WITH_LOCK(::cs_main, return m_chainman.m_blockman.LookupBlockIndex(continuationBlockHash));
        if (!pLastIndex || pLastIndex->pprev == nullptr) return {};
        pIndexScanCommitments = pLastIndex->pprev;
    }

    // Get the block indexes of the mined commitments to build the required quorums from.
    // The mined commitment index follows the active chain, so a scan from a block that is
    // not on it (e.g. a tip notification processed after a reorg) reads another branch.
    // Check that under the same cs_main hold so a reorg cannot slip in between.
    bool scan_on_active_chain{false};
    std::vector<const CBlockIndex*> pQuorumBaseBlockIndexes;
    {
        LOCK(::cs_main);
        const CChain& active_chain{m_chainman.ActiveChain()};
        scan_on_active_chain = active_chain.Contains(pindexStore) && active_chain.Contains(pIndexScanCommitments);
        pQuorumBaseBlockIndexes = llmq_params_opt->useRotation
                                      ? quorumBlockProcessor.GetMinedCommitmentsIndexedUntilBlock(llmqType,
                                                                                                  pIndexScanCommitments,
                                                                                                  nScanCommitments)
                                      : quorumBlockProcessor.GetMinedCommitmentsUntilBlock(llmqType, pIndexScanCommitments,
                                                                                           nScanCommitments);
    }
    vecResultCommitments.reserve(vecResultCommitments.size() + pQuorumBaseBlockIndexes.size());

    for (auto& pQuorumBaseBlockIndex : pQuorumBaseBlockIndexes) {
        assert(pQuorumBaseBlockIndex);
        // We assume that every quorum asked for is available to us on hand, if this
        // fails then we can assume that something has gone wrong and we should stop
        // trying to process any further and return a blank.
        auto [qc, _] = quorumBlockProcessor.GetMinedCommitment(llmqType, pQuorumBaseBlockIndex->GetBlockHash());
        if (qc.IsNull()) {
            LogPrintf("%s: ERROR! Unexpected missing commitment with llmqType=%d, blockHash=%s\n",
                      __func__, std23::to_underlying(llmqType), pQuorumBaseBlockIndex->GetBlockHash().ToString());
            return {};
        }
        vecResultCommitments.emplace_back(std::move(qc));
    }

    const size_t nCountResult{vecResultCommitments.size()};
    if (nCountResult > 0 && scan_on_active_chain) {
        LOCK2(::cs_main, m_cs_maps);
        // The commitments were read by quorum hash without cs_main; if pindexStore was disconnected
        // meanwhile they may be another branch's commitments for the same quorums.
        if (m_chainman.ActiveChain().Contains(pindexStore)) {
            // Don't cache more than keepOldConnections elements
            // because signing by old quorums requires the exact quorum hash
            // to be specified and quorum scanning isn't needed there.
            auto& cache = scanCommitmentsCache[llmqType];
            const size_t nCacheEndIndex = std::min(nCountResult, static_cast<size_t>(llmq_params_opt->keepOldConnections));
            cache.emplace(pindexStore->GetBlockHash(),
                          {vecResultCommitments.begin(), vecResultCommitments.begin() + nCacheEndIndex});
        }
    }
    // Don't return more than nCountRequested elements
    const size_t nResultEndIndex = std::min(nCountResult, nCountRequested);
    return {vecResultCommitments.begin(), vecResultCommitments.begin() + nResultEndIndex};
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
            if (!it->first.m_we_requested) {
                // On counter desync, skip the decrement: over-counting fails toward stricter
                // limits, while an underflow would disable the caps entirely.
                auto count_it = m_inbound_request_counts.find(it->first.proRegTx);
                if (Assume(count_it != m_inbound_request_counts.end() && count_it->second > 0) &&
                    --count_it->second == 0) {
                    m_inbound_request_counts.erase(count_it);
                }
                if (Assume(m_inbound_request_count > 0)) {
                    --m_inbound_request_count;
                }
            }
            it = mapQuorumDataRequests.erase(it);
        } else {
            ++it;
        }
    }
}

void CQuorumManager::CleanupOldQuorumData(const Uint256HashSet& dbKeysToSkip) const
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

CQuorumCPtr CQuorumManager::GetQuorum(Consensus::LLMQType llmqType, const uint256& quorumHash,
                                      const CChain& chain) const
{
    AssertLockHeld(::cs_main);

    const auto* pQuorumBaseBlockIndex = m_chainman.m_blockman.LookupBlockIndex(quorumHash);
    if (!pQuorumBaseBlockIndex) {
        LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- block %s not found\n", __func__, quorumHash.ToString());
        return nullptr;
    }
    return GetQuorum(llmqType, pQuorumBaseBlockIndex, chain);
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
    {
        LOCK(m_cs_maps);
        if (mapQuorumsCache.get(llmqType, quorumHash, pQuorum)) {
            return pQuorum;
        }
    }

    return BuildQuorumFromCommitment(llmqType, pQuorumBaseBlockIndex, populate_cache);
}

CQuorumCPtr CQuorumManager::GetQuorum(Consensus::LLMQType llmqType,
                                      gsl::not_null<const CBlockIndex*> pQuorumBaseBlockIndex,
                                      const CChain& chain, bool populate_cache) const
{
    AssertLockHeld(::cs_main);

    const auto quorumHash = pQuorumBaseBlockIndex->GetBlockHash();
    if (!HasQuorum(llmqType, quorumBlockProcessor, quorumHash, chain)) {
        return nullptr;
    }

    CQuorumPtr pQuorum;
    if (LOCK(m_cs_maps); mapQuorumsCache.get(llmqType, quorumHash, pQuorum)) {
        return pQuorum;
    }

    return BuildQuorumFromCommitment(llmqType, pQuorumBaseBlockIndex, populate_cache);
}

DataRequestRegistration CQuorumManager::RegisterDataRequest(const CQuorumDataRequestKey& key,
                                                            const CQuorumDataRequest& request, bool add_expiry_bias) const
{
    LOCK(cs_data_requests);
    if (auto it = mapQuorumDataRequests.find(key); it != mapQuorumDataRequests.end()) {
        if (!it->second.IsExpired(add_expiry_bias)) {
            return DataRequestRegistration::RateLimited;
        }
        it->second = request;
        return DataRequestRegistration::Accepted;
    }

    if (!key.m_we_requested) {
        if (m_inbound_request_count >= MAX_INBOUND_DATA_REQUESTS) {
            return DataRequestRegistration::CapacityExhausted;
        }
        const auto count_it = m_inbound_request_counts.find(key.proRegTx);
        if (count_it != m_inbound_request_counts.end() && count_it->second >= MAX_INBOUND_DATA_REQUESTS_PER_REQUESTER) {
            // All unauthenticated qwatch peers share the null identity, so exhaustion of that
            // budget is not attributable to the connection that happened to arrive last.
            return key.proRegTx.IsNull() ? DataRequestRegistration::CapacityExhausted
                                         : DataRequestRegistration::RequesterLimitExceeded;
        }
    }

    mapQuorumDataRequests.emplace(key, request);
    if (!key.m_we_requested) {
        ++m_inbound_request_counts[key.proRegTx];
        ++m_inbound_request_count;
    }
    return DataRequestRegistration::Accepted;
}

CQuorumManager::DataResponseValidation CQuorumManager::ValidateDataResponse(
    const CQuorumDataRequestKey& key, const CQuorumDataRequest& response) const
{
    LOCK(cs_data_requests);
    auto it = mapQuorumDataRequests.find(key);
    if (it == mapQuorumDataRequests.end()) {
        return DataResponseValidation::NotRequested;
    }
    if (it->second.IsProcessed()) {
        return DataResponseValidation::AlreadyReceived;
    }
    if (response != it->second) {
        return DataResponseValidation::Mismatch;
    }
    it->second.SetProcessed();
    return DataResponseValidation::OK;
}

CQuorumPtr CQuorumManager::GetCachedMutableQuorum(Consensus::LLMQType llmqType, const uint256& quorumHash) const
{
    CQuorumPtr pQuorum;
    LOCK(m_cs_maps);
    mapQuorumsCache.get(llmqType, quorumHash, pQuorum);
    return pQuorum;
}

void CQuorumManager::WriteContributions(const CQuorumPtr& quorum) const
{
    LOCK(cs_db);
    quorum->WriteContributions(*db);
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
                 std23::to_underlying(pQuorum->params.type), pQuorum->m_quorum_base_block_index->nHeight,
                 pQuorum->m_quorum_base_block_index->GetBlockHash().ToString());

        // when then later some other thread tries to get keys, it will be much faster
        for (const auto i : util::irange(pQuorum->members.size())) {
            if (m_cache_interrupt) {
                break;
            }
            if (pQuorum->qc->validMembers[i]) {
                pQuorum->GetPubKeyShare(i);
            }
        }

        LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- type=%d height=%d hash=%s done. time=%d\n", __func__,
                 std23::to_underlying(pQuorum->params.type), pQuorum->m_quorum_base_block_index->nHeight,
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
                batch.Write(k, Span{s});
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

CBlockIndex* SelectQuorumForSigningStartBlock(const CChain& active_chain, int signHeight, int signOffset)
{
    AssertLockHeld(::cs_main);
    if (signHeight == -1) {
        signHeight = active_chain.Height();
    }
    const int startBlockHeight = signHeight - signOffset;
    if (startBlockHeight > active_chain.Height() || startBlockHeight < 0) {
        return nullptr;
    }
    return active_chain[startBlockHeight];
}

CQuorumCPtr SelectQuorumForSigning(const Consensus::LLMQParams& llmq_params, const CQuorumManager& qman,
                                   const uint256& selectionHash, const CBlockIndex* pindexStart)
{
    if (pindexStart == nullptr) {
        return nullptr;
    }
    size_t poolSize = llmq_params.signingActiveQuorumCount;

    // don't remove connections for the currently in-progress DKG round
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
        for (const auto i : util::irange(quorums.size())) {
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

CQuorumCPtr SelectQuorumForSigning(const Consensus::LLMQParams& llmq_params, const CChain& active_chain, const CQuorumManager& qman,
                                   const uint256& selectionHash, int signHeight, int signOffset)
{
    const CBlockIndex* pindexStart = WITH_LOCK(::cs_main, return SelectQuorumForSigningStartBlock(active_chain, signHeight, signOffset));
    return SelectQuorumForSigning(llmq_params, qman, selectionHash, pindexStart);
}

VerifyRecSigStatus VerifyRecoveredSig(Consensus::LLMQType llmqType, const CQuorumManager& qman,
                                      const CBlockIndex* pindexStart, const uint256& id, const uint256& msgHash,
                                      const CBLSSignature& sig)
{
    const auto& llmq_params_opt = Params().GetLLMQ(llmqType);
    assert(llmq_params_opt.has_value());
    auto quorum = SelectQuorumForSigning(llmq_params_opt.value(), qman, id, pindexStart);
    if (!quorum) {
        return VerifyRecSigStatus::NoQuorum;
    }

    SignHash signHash{llmqType, quorum->qc->quorumHash, id, msgHash};
    const bool ret = sig.VerifyInsecure(quorum->qc->quorumPublicKey, signHash.Get());
    return ret ? VerifyRecSigStatus::Valid : VerifyRecSigStatus::Invalid;
}

std::optional<CFinalCommitment> SelectCommitmentForSigning(const Consensus::LLMQParams& llmq_params, const CQuorumManager& qman,
                                                           const uint256& selectionHash, const CBlockIndex* pindexStart)
{
    size_t poolSize = llmq_params.signingActiveQuorumCount;

    if (IsQuorumRotationEnabled(llmq_params, pindexStart)) {
        auto commitments = qman.ScanCommitments(llmq_params.type, pindexStart, poolSize);
        if (commitments.empty()) {
            return std::nullopt;
        }
        //log2 int
        int n = std::log2(llmq_params.signingActiveQuorumCount);
        //Extract last 64 bits of selectionHash
        uint64_t b = selectionHash.GetUint64(3);
        //Take last n bits of b
        uint64_t signer = (((1ull << n) - 1) & (b >> (64 - n - 1)));

        if (signer > commitments.size()) {
            return std::nullopt;
        }
        auto it = std::find_if(commitments.begin(),
                                     commitments.end(),
                                     [signer](const CFinalCommitment& obj) {
                                         return uint64_t(obj.quorumIndex) == signer;
                                     });
        if (it == commitments.end()) {
            return std::nullopt;
        }
        return *it;
    } else {
        auto commitments = qman.ScanCommitments(llmq_params.type, pindexStart, poolSize);
        if (commitments.empty()) {
            return std::nullopt;
        }

        std::vector<std::pair<uint256, size_t>> scores;
        scores.reserve(commitments.size());
        for (const auto i : util::irange(commitments.size())) {
            CHashWriter h(SER_NETWORK, 0);
            h << llmq_params.type;
            h << commitments[i].quorumHash;
            h << selectionHash;
            scores.emplace_back(h.GetHash(), i);
        }
        std::sort(scores.begin(), scores.end());
        return commitments[scores.front().second];
    }
}

VerifyRecSigStatus VerifyRecoveredSig(Consensus::LLMQType llmqType, const CChain& active_chain, const CQuorumManager& qman,
                        int signedAtHeight, const uint256& id, const uint256& msgHash, const CBLSSignature& sig,
                        const int signOffset)
{
    const CBlockIndex* pindexStart = WITH_LOCK(::cs_main, return SelectQuorumForSigningStartBlock(active_chain, signedAtHeight, signOffset));
    return VerifyRecoveredSig(llmqType, qman, pindexStart, id, msgHash, sig);
}
} // namespace llmq
