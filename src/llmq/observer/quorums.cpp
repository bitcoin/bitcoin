// Copyright (c) 2018-2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/observer/quorums.h>

#include <evo/deterministicmns.h>
#include <llmq/commitment.h>
#include <llmq/options.h>
#include <llmq/quorums.h>
#include <llmq/utils.h>
#include <masternode/sync.h>
#include <msg_result.h>
#include <unordered_lru_cache.h>

#include <chain.h>
#include <chainparams.h>
#include <logging.h>
#include <net.h>
#include <validation.h>

#include <cxxtimer.hpp>

namespace llmq {
QuorumObserver::QuorumObserver(CConnman& connman, CDeterministicMNManager& dmnman, QuorumObserverParent& qman,
                               CQuorumSnapshotManager& qsnapman, const ChainstateManager& chainman,
                               const CMasternodeSync& mn_sync, const CSporkManager& sporkman,
                               const llmq::QvvecSyncModeMap& sync_map, bool quorums_recovery) :
    m_connman{connman},
    m_dmnman{dmnman},
    m_qman{qman},
    m_qsnapman{qsnapman},
    m_chainman{chainman},
    m_mn_sync{mn_sync},
    m_sporkman{sporkman},
    m_quorums_recovery{quorums_recovery},
    m_sync_map{sync_map}
{
    quorumThreadInterrupt.reset();
}

QuorumObserver::~QuorumObserver()
{
    Stop();
}

void QuorumObserver::Start()
{
    int workerCount = std::thread::hardware_concurrency() / 2;
    workerCount = std::clamp(workerCount, 1, 4);
    workerPool.resize(workerCount);
    RenameThreadPool(workerPool, "q-mngr");
}

void QuorumObserver::Stop()
{
    quorumThreadInterrupt();
    workerPool.clear_queue();
    workerPool.stop(true);
}

void QuorumObserver::UpdatedBlockTip(const CBlockIndex* pindexNew, bool fInitialDownload) const
{
    if (!pindexNew) return;
    if (!m_mn_sync.IsBlockchainSynced()) return;

    for (const auto& params : Params().GetConsensus().llmqs) {
        CheckQuorumConnections(params, pindexNew);
    }

    // Cleanup expired data requests
    m_qman.CleanupExpiredDataRequests();

    TriggerQuorumDataRecoveryThreads(pindexNew);
    StartCleanupOldQuorumDataThread(pindexNew);
}

Uint256HashSet QuorumObserver::GetQuorumsToDelete(const Consensus::LLMQParams& llmqParams,
                                                  gsl::not_null<const CBlockIndex*> pindexNew) const
{
    auto connmanQuorumsToDelete = m_connman.GetMasternodeQuorums(llmqParams.type);

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
        LogPrint(BCLog::LLMQ, "QuorumObserver::%s -- llmqType[%d] h[%d] keeping mn quorum connections for rotated quorums: [%s]\n", __func__, ToUnderlying(llmqParams.type), pindexNew->nHeight, ss.str());
    } else {
        int curDkgHeight = pindexNew->nHeight - (pindexNew->nHeight % llmqParams.dkgInterval);
        auto curDkgBlock = pindexNew->GetAncestor(curDkgHeight)->GetBlockHash();
        connmanQuorumsToDelete.erase(curDkgBlock);
        LogPrint(BCLog::LLMQ, "QuorumObserver::%s -- llmqType[%d] h[%d] keeping mn quorum connections for quorum: [%d:%s]\n", __func__, ToUnderlying(llmqParams.type), pindexNew->nHeight, curDkgHeight, curDkgBlock.ToString());
    }

    return connmanQuorumsToDelete;
}

void QuorumObserver::CheckQuorumConnections(const Consensus::LLMQParams& llmqParams,
                                            gsl::not_null<const CBlockIndex*> pindexNew) const
{
    auto lastQuorums = m_qman.ScanQuorums(llmqParams.type, pindexNew, (size_t)llmqParams.keepOldConnections);
    auto deletableQuorums = GetQuorumsToDelete(llmqParams, pindexNew);

    for (const auto& quorum : lastQuorums) {
        if (utils::EnsureQuorumConnections(llmqParams, m_connman, m_sporkman, {m_dmnman, m_qsnapman, m_chainman, quorum->m_quorum_base_block_index},
                                           m_dmnman.GetListAtChainTip(), /*myProTxHash=*/uint256{}, /*is_masternode=*/false, /*quorums_watch=*/true)) {
            if (deletableQuorums.erase(quorum->qc->quorumHash) > 0) {
                LogPrint(BCLog::LLMQ, "QuorumObserver::%s -- llmqType[%d] h[%d] keeping mn quorum connections for quorum: [%d:%s]\n", __func__, ToUnderlying(llmqParams.type), pindexNew->nHeight, quorum->m_quorum_base_block_index->nHeight, quorum->m_quorum_base_block_index->GetBlockHash().ToString());
            }
        }
    }

    for (const auto& quorumHash : deletableQuorums) {
        LogPrint(BCLog::LLMQ, "QuorumObserver::%s -- removing masternodes quorum connections for quorum %s:\n", __func__, quorumHash.ToString());
        m_connman.RemoveMasternodeQuorumNodes(llmqParams.type, quorumHash);
    }
}

bool QuorumObserver::SetQuorumSecretKeyShare(CQuorum& quorum, Span<CBLSSecretKey> skContributions) const
{
    // Watch-only nodes cannot work with secret keys
    return false;
}

MessageProcessingResult QuorumObserver::ProcessContribQGETDATA(bool request_limit_exceeded, CDataStream& vStream,
                                                               const CQuorum& quorum, CQuorumDataRequest& request,
                                                               gsl::not_null<const CBlockIndex*> block_index)
{
    // Watch-only nodes cannot provide encrypted contributions
    if (request.GetDataMask() & CQuorumDataRequest::ENCRYPTED_CONTRIBUTIONS) {
        request.SetError(CQuorumDataRequest::Errors::ENCRYPTED_CONTRIBUTIONS_MISSING);
        return request_limit_exceeded ? MisbehavingError{25, "request limit exceeded"} : MessageProcessingResult{};
    }
    return {};
}

MessageProcessingResult QuorumObserver::ProcessContribQDATA(CNode& pfrom, CDataStream& vStream,
                                                            CQuorum& quorum, CQuorumDataRequest& request)
{
    // Watch-only nodes ignore encrypted contributions
    return {};
}

bool QuorumObserver::IsMasternode() const
{
    // Watch-only nodes are not masternodes
    return false;
}

bool QuorumObserver::IsWatching() const
{
    // We are only initialized if watch-only mode is enabled
    return true;
}

void QuorumObserver::DataRecoveryThread(gsl::not_null<const CBlockIndex*> block_index, CQuorumCPtr pQuorum,
                                        uint16_t data_mask, const uint256& protx_hash, size_t start_offset) const
{
        size_t nTries{0};
        uint16_t nDataMask{data_mask};
        int64_t nTimeLastSuccess{0};
        uint256* pCurrentMemberHash{nullptr};
        std::vector<uint256> vecMemberHashes;
        const int64_t nRequestTimeout{10};

        auto printLog = [&](const std::string& strMessage) {
            const std::string strMember{pCurrentMemberHash == nullptr ? "nullptr" : pCurrentMemberHash->ToString()};
            LogPrint(BCLog::LLMQ, "QuorumObserver::DataRecoveryThread -- %s - for llmqType %d, quorumHash %s, nDataMask (%d/%d), pCurrentMemberHash %s, nTries %d\n",
                strMessage, ToUnderlying(pQuorum->qc->llmqType), pQuorum->qc->quorumHash.ToString(), nDataMask, data_mask, strMember, nTries);
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
            if (pQuorum->IsValidMember(member->proTxHash) && member->proTxHash != protx_hash) {
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
                pCurrentMemberHash = &vecMemberHashes[(start_offset + nTries++) % vecMemberHashes.size()];
                if (m_qman.IsDataRequestPending(*pCurrentMemberHash, /*we_requested=*/true, pQuorum->qc->quorumHash,
                                                pQuorum->qc->llmqType)) {
                    printLog("Already asked");
                    continue;
                }
                // Sleep a bit depending on the start offset to balance out multiple requests to same masternode
                quorumThreadInterrupt.sleep_for(std::chrono::milliseconds(start_offset * 100));
                nTimeLastSuccess = GetTime<std::chrono::seconds>().count();
                m_connman.AddPendingMasternode(*pCurrentMemberHash);
                printLog("Connect");
            }

            m_connman.ForEachNode([&](CNode* pNode) {
                auto verifiedProRegTxHash = pNode->GetVerifiedProRegTxHash();
                if (pCurrentMemberHash == nullptr || verifiedProRegTxHash != *pCurrentMemberHash) {
                    return;
                }

                if (m_qman.RequestQuorumData(pNode, m_connman, *pQuorum, nDataMask, protx_hash)) {
                    nTimeLastSuccess = GetTime<std::chrono::seconds>().count();
                    printLog("Requested");
                } else {
                    const auto status = m_qman.GetDataRequestStatus(*pCurrentMemberHash, /*we_requested=*/true,
                                                                    pQuorum->qc->quorumHash, pQuorum->qc->llmqType);
                    switch (status) {
                    case DataRequestStatus::NotFound:
                        printLog("Failed");
                        pNode->fDisconnect = true;
                        pCurrentMemberHash = nullptr;
                        return;
                    case DataRequestStatus::Processed:
                        printLog("Processed");
                        pNode->fDisconnect = true;
                        pCurrentMemberHash = nullptr;
                        return;
                    case DataRequestStatus::Pending:
                        printLog("Waiting");
                        return;
                    }
                }
            });
            quorumThreadInterrupt.sleep_for(std::chrono::seconds(1));
        }
        pQuorum->fQuorumDataRecoveryThreadRunning = false;
        printLog("Done");
}

void QuorumObserver::StartVvecSyncThread(gsl::not_null<const CBlockIndex*> block_index, CQuorumCPtr pQuorum) const
{
    bool expected = false;
    if (!pQuorum->fQuorumDataRecoveryThreadRunning.compare_exchange_strong(expected, true)) {
        LogPrint(BCLog::LLMQ, "QuorumObserver::%s -- Already running\n", __func__);
        return;
    }

    workerPool.push([pQuorum = std::move(pQuorum), block_index, this](int threadId) mutable {
        DataRecoveryThread(block_index, std::move(pQuorum), CQuorumDataRequest::QUORUM_VERIFICATION_VECTOR,
                           /*protx_hash=*/uint256(), /*start_offset=*/0);
    });
}

void QuorumObserver::TriggerQuorumDataRecoveryThreads(gsl::not_null<const CBlockIndex*> block_index) const
{
    if (!m_quorums_recovery) {
        return;
    }

    LogPrint(BCLog::LLMQ, "QuorumObserver::%s -- Process block %s\n", __func__, block_index->GetBlockHash().ToString());
    for (const auto& params : Params().GetConsensus().llmqs) {
        auto vecQuorums = m_qman.ScanQuorums(params.type, block_index, params.keepOldConnections);
        for (auto& pQuorum : vecQuorums) {
            TryStartVvecSyncThread(block_index, std::move(pQuorum), /*fWeAreQuorumTypeMember=*/false);
        }
    }
}

void QuorumObserver::TryStartVvecSyncThread(gsl::not_null<const CBlockIndex*> block_index, CQuorumCPtr pQuorum,
                                            bool fWeAreQuorumTypeMember) const
{
    if (pQuorum->fQuorumDataRecoveryThreadRunning) return;

    const bool fSyncForTypeEnabled = m_sync_map.count(pQuorum->qc->llmqType) > 0;
    const QvvecSyncMode syncMode = fSyncForTypeEnabled ? m_sync_map.at(pQuorum->qc->llmqType) : QvvecSyncMode::Invalid;
    const bool fSyncCurrent = syncMode == QvvecSyncMode::Always ||
                              (syncMode == QvvecSyncMode::OnlyIfTypeMember && fWeAreQuorumTypeMember);

    if ((fSyncForTypeEnabled && fSyncCurrent) && !pQuorum->HasVerificationVector()) {
        StartVvecSyncThread(block_index, std::move(pQuorum));
    } else {
        LogPrint(BCLog::LLMQ, "QuorumObserver::%s -- No data needed from (%d, %s) at height %d\n", __func__,
                 ToUnderlying(pQuorum->qc->llmqType), pQuorum->qc->quorumHash.ToString(), block_index->nHeight);
    }
}

void QuorumObserver::StartCleanupOldQuorumDataThread(gsl::not_null<const CBlockIndex*> pIndex) const
{
    // Note: this function is CPU heavy and we don't want it to be running during DKGs.
    // The largest dkgMiningWindowStart for a related quorum type is 42 (LLMQ_60_75).
    // At the same time most quorums use dkgInterval = 24 so the next DKG for them
    // (after block 576 + 42) will start at block 576 + 24 * 2. That's only a 6 blocks
    // window and it's better to have more room so we pick next cycle.
    // dkgMiningWindowStart for small quorums is 10 i.e. a safe block to start
    // these calculations is at height 576 + 24 * 2 + 10 = 576 + 58.
    if (pIndex->nHeight % 576 != 58) {
        return;
    }

    cxxtimer::Timer t(/*start=*/ true);
    LogPrint(BCLog::LLMQ, "QuorumObserver::%s -- start\n", __func__);

    // do not block the caller thread
    workerPool.push([pIndex, t, this](int threadId) {
        std::set<uint256> dbKeysToSkip;

        if (LOCK(cs_cleanup); cleanupQuorumsCache.empty()) {
            utils::InitQuorumsCache(cleanupQuorumsCache, m_chainman.GetConsensus(), /*limit_by_connections=*/false);
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
            for (const auto& pQuorum : m_qman.ScanQuorums(params.type, pIndex, params.keepOldKeys - quorum_keys.size())) {
                const uint256 quorum_key = MakeQuorumKey(*pQuorum);
                quorum_keys.insert(quorum_key);
                cache.insert(pQuorum->m_quorum_base_block_index->GetBlockHash(), quorum_key);
            }
            dbKeysToSkip.merge(quorum_keys);
        }

        if (!quorumThreadInterrupt) {
            m_qman.CleanupOldQuorumData(dbKeysToSkip);
        }

        LogPrint(BCLog::LLMQ, "QuorumObserver::StartCleanupOldQuorumDataThread -- done. time=%d\n", t.count());
    });
}
} // namespace llmq
