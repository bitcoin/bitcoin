// Copyright (c) 2018-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <active/quorums.h>

#include <bls/bls_ies.h>
#include <evo/deterministicmns.h>
#include <llmq/commitment.h>
#include <llmq/dkgsessionmgr.h>
#include <llmq/options.h>
#include <llmq/quorums.h>
#include <llmq/utils.h>
#include <masternode/node.h>
#include <masternode/sync.h>

#include <chain.h>
#include <chainparams.h>
#include <logging.h>
#include <net.h>
#include <netmessagemaker.h>
#include <validation.h>

#include <cxxtimer.hpp>

namespace llmq {
QuorumParticipant::QuorumParticipant(CBLSWorker& bls_worker, CDeterministicMNManager& dmnman, CQuorumManager& qman,
                                     CQuorumSnapshotManager& qsnapman, const CActiveMasternodeManager* const mn_activeman,
                                     const ChainstateManager& chainman, const CMasternodeSync& mn_sync,
                                     const CSporkManager& sporkman, const llmq::QvvecSyncModeMap& sync_map,
                                     bool quorums_recovery, bool quorums_watch) :
    m_bls_worker{bls_worker},
    m_dmnman{dmnman},
    m_qman{qman},
    m_qsnapman{qsnapman},
    m_mn_activeman{mn_activeman},
    m_chainman{chainman},
    m_mn_sync{mn_sync},
    m_sporkman{sporkman},
    m_quorums_recovery{quorums_recovery},
    m_quorums_watch{quorums_watch},
    m_sync_map{sync_map}
{
}

QuorumParticipant::~QuorumParticipant() = default;

void QuorumParticipant::TriggerQuorumDataRecoveryThreads(CConnman& connman, gsl::not_null<const CBlockIndex*> pIndex) const
{
    if ((m_mn_activeman == nullptr && !m_quorums_watch) || !m_quorums_recovery) {
        return;
    }

    LogPrint(BCLog::LLMQ, "QuorumParticipant::%s -- Process block %s\n", __func__, pIndex->GetBlockHash().ToString());

    for (const auto& params : Params().GetConsensus().llmqs) {
        auto vecQuorums = m_qman.ScanQuorums(params.type, pIndex, params.keepOldConnections);

        // First check if we are member of any quorum of this type
        const uint256 proTxHash = m_mn_activeman != nullptr ? m_mn_activeman->GetProTxHash() : uint256();

        bool fWeAreQuorumTypeMember = ranges::any_of(vecQuorums, [&proTxHash](const auto& pQuorum) {
            return pQuorum->IsValidMember(proTxHash);
        });

        for (auto& pQuorum : vecQuorums) {
            // If there is already a thread running for this specific quorum skip it
            if (pQuorum->fQuorumDataRecoveryThreadRunning) {
                continue;
            }

            uint16_t nDataMask{0};
            const bool fWeAreQuorumMember = pQuorum->IsValidMember(proTxHash);
            const bool fSyncForTypeEnabled = m_sync_map.count(pQuorum->qc->llmqType) > 0;
            const QvvecSyncMode syncMode = fSyncForTypeEnabled ? m_sync_map.at(pQuorum->qc->llmqType)
                                                               : QvvecSyncMode::Invalid;
            const bool fSyncCurrent = syncMode == QvvecSyncMode::Always || (syncMode == QvvecSyncMode::OnlyIfTypeMember && fWeAreQuorumTypeMember);

            if ((fWeAreQuorumMember || (fSyncForTypeEnabled && fSyncCurrent)) && !pQuorum->HasVerificationVector()) {
                nDataMask |= llmq::CQuorumDataRequest::QUORUM_VERIFICATION_VECTOR;
            }

            if (fWeAreQuorumMember && !pQuorum->GetSkShare().IsValid()) {
                nDataMask |= llmq::CQuorumDataRequest::ENCRYPTED_CONTRIBUTIONS;
            }

            if (nDataMask == 0) {
                LogPrint(BCLog::LLMQ, "QuorumParticipant::%s -- No data needed from (%d, %s) at height %d\n",
                    __func__, ToUnderlying(pQuorum->qc->llmqType), pQuorum->qc->quorumHash.ToString(), pIndex->nHeight);
                continue;
            }

            // Finally start the thread which triggers the requests for this quorum
            StartQuorumDataRecoveryThread(connman, std::move(pQuorum), pIndex, nDataMask);
        }
    }
}

void QuorumParticipant::UpdatedBlockTip(const CBlockIndex* pindexNew, CConnman& connman, bool fInitialDownload) const
{
    if (!pindexNew) return;
    if (!m_mn_sync.IsBlockchainSynced()) return;

    for (const auto& params : Params().GetConsensus().llmqs) {
        CheckQuorumConnections(connman, params, pindexNew);
    }

    if (m_mn_activeman != nullptr || m_quorums_watch) {
        // Cleanup expired data requests
        LOCK(m_qman.cs_data_requests);
        auto it = m_qman.mapQuorumDataRequests.begin();
        while (it != m_qman.mapQuorumDataRequests.end()) {
            if (it->second.IsExpired(/*add_bias=*/true)) {
                it = m_qman.mapQuorumDataRequests.erase(it);
            } else {
                ++it;
            }
        }
    }

    TriggerQuorumDataRecoveryThreads(connman, pindexNew);
    StartCleanupOldQuorumDataThread(pindexNew);
}

void QuorumParticipant::CheckQuorumConnections(CConnman& connman, const Consensus::LLMQParams& llmqParams,
                                               gsl::not_null<const CBlockIndex*> pindexNew) const
{
    if (m_mn_activeman == nullptr && !m_quorums_watch) return;

    auto lastQuorums = m_qman.ScanQuorums(llmqParams.type, pindexNew, (size_t)llmqParams.keepOldConnections);

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
        LogPrint(BCLog::LLMQ, "QuorumParticipant::%s -- llmqType[%d] h[%d] keeping mn quorum connections for rotated quorums: [%s]\n", __func__, ToUnderlying(llmqParams.type), pindexNew->nHeight, ss.str());
    } else {
        int curDkgHeight = pindexNew->nHeight - (pindexNew->nHeight % llmqParams.dkgInterval);
        auto curDkgBlock = pindexNew->GetAncestor(curDkgHeight)->GetBlockHash();
        connmanQuorumsToDelete.erase(curDkgBlock);
        LogPrint(BCLog::LLMQ, "QuorumParticipant::%s -- llmqType[%d] h[%d] keeping mn quorum connections for quorum: [%d:%s]\n", __func__, ToUnderlying(llmqParams.type), pindexNew->nHeight, curDkgHeight, curDkgBlock.ToString());
    }

    const uint256 myProTxHash = m_mn_activeman != nullptr ? m_mn_activeman->GetProTxHash() : uint256();

    bool isISType = llmqParams.type == Params().GetConsensus().llmqTypeDIP0024InstantSend;

    bool watchOtherISQuorums = isISType && !myProTxHash.IsNull() &&
                    ranges::any_of(lastQuorums, [&myProTxHash](const auto& old_quorum){
                        return old_quorum->IsMember(myProTxHash);
                    });

    for (const auto& quorum : lastQuorums) {
        if (utils::EnsureQuorumConnections(llmqParams, connman, m_sporkman, {m_dmnman, m_qsnapman, m_chainman, quorum->m_quorum_base_block_index},
                                           m_dmnman.GetListAtChainTip(), myProTxHash, /*is_masternode=*/m_mn_activeman != nullptr, m_quorums_watch)) {
            if (connmanQuorumsToDelete.erase(quorum->qc->quorumHash) > 0) {
                LogPrint(BCLog::LLMQ, "QuorumParticipant::%s -- llmqType[%d] h[%d] keeping mn quorum connections for quorum: [%d:%s]\n", __func__, ToUnderlying(llmqParams.type), pindexNew->nHeight, quorum->m_quorum_base_block_index->nHeight, quorum->m_quorum_base_block_index->GetBlockHash().ToString());
            }
        } else if (watchOtherISQuorums && !quorum->IsMember(myProTxHash)) {
            Uint256HashSet connections;
            const auto& cindexes = utils::CalcDeterministicWatchConnections(llmqParams.type, quorum->m_quorum_base_block_index, quorum->members.size(), 1);
            for (auto idx : cindexes) {
                connections.emplace(quorum->members[idx]->proTxHash);
            }
            if (!connections.empty()) {
                if (!connman.HasMasternodeQuorumNodes(llmqParams.type, quorum->m_quorum_base_block_index->GetBlockHash())) {
                    LogPrint(BCLog::LLMQ, "QuorumParticipant::%s -- llmqType[%d] h[%d] adding mn inter-quorum connections for quorum: [%d:%s]\n", __func__, ToUnderlying(llmqParams.type), pindexNew->nHeight, quorum->m_quorum_base_block_index->nHeight, quorum->m_quorum_base_block_index->GetBlockHash().ToString());
                    connman.SetMasternodeQuorumNodes(llmqParams.type, quorum->m_quorum_base_block_index->GetBlockHash(), connections);
                    connman.SetMasternodeQuorumRelayMembers(llmqParams.type, quorum->m_quorum_base_block_index->GetBlockHash(), connections);
                }
                if (connmanQuorumsToDelete.erase(quorum->qc->quorumHash) > 0) {
                    LogPrint(BCLog::LLMQ, "QuorumParticipant::%s -- llmqType[%d] h[%d] keeping mn inter-quorum connections for quorum: [%d:%s]\n", __func__, ToUnderlying(llmqParams.type), pindexNew->nHeight, quorum->m_quorum_base_block_index->nHeight, quorum->m_quorum_base_block_index->GetBlockHash().ToString());
                }
            }
        }
    }
    for (const auto& quorumHash : connmanQuorumsToDelete) {
        LogPrint(BCLog::LLMQ, "QuorumParticipant::%s -- removing masternodes quorum connections for quorum %s:\n", __func__, quorumHash.ToString());
        connman.RemoveMasternodeQuorumNodes(llmqParams.type, quorumHash);
    }
}

bool QuorumParticipant::SetQuorumSecretKeyShare(CQuorum& quorum, Span<CBLSSecretKey> skContributions) const
{
    return m_mn_activeman && quorum.SetSecretKeyShare(m_bls_worker.AggregateSecretKeys(skContributions), m_mn_activeman->GetProTxHash());
}

size_t QuorumParticipant::GetQuorumRecoveryStartOffset(const CQuorum& quorum, gsl::not_null<const CBlockIndex*> pIndex) const
{
    assert(m_mn_activeman);

    auto mns = m_dmnman.GetListForBlock(pIndex);
    std::vector<uint256> vecProTxHashes;
    vecProTxHashes.reserve(mns.GetValidMNsCount());
    mns.ForEachMN(/*onlyValid=*/true,
                  [&](const auto& pMasternode) { vecProTxHashes.emplace_back(pMasternode.proTxHash); });
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
    return nIndex % quorum.qc->validMembers.size();
}

MessageProcessingResult QuorumParticipant::ProcessContribQGETDATA(bool request_limit_exceeded, CDataStream& vStream,
                                                                  const CQuorum& quorum, CQuorumDataRequest& request,
                                                                  gsl::not_null<const CBlockIndex*> block_index)
{
    if (request.GetDataMask() & CQuorumDataRequest::ENCRYPTED_CONTRIBUTIONS) {
        assert(block_index);

        int memberIdx = quorum.GetMemberIndex(request.GetProTxHash());
        if (memberIdx == -1) {
            request.SetError(CQuorumDataRequest::Errors::MASTERNODE_IS_NO_MEMBER);
            return request_limit_exceeded ? MisbehavingError{25, "request limit exceeded"} : MessageProcessingResult{};
        }

        std::vector<CBLSIESEncryptedObject<CBLSSecretKey>> vecEncrypted;
        if (!m_qman.m_qdkgsman ||
            !m_qman.m_qdkgsman->GetEncryptedContributions(request.GetLLMQType(), block_index,
                                                          quorum.qc->validMembers, request.GetProTxHash(), vecEncrypted)) {
            request.SetError(CQuorumDataRequest::Errors::ENCRYPTED_CONTRIBUTIONS_MISSING);
            return request_limit_exceeded ? MisbehavingError{25, "request limit exceeded"} : MessageProcessingResult{};
        }

        vStream << vecEncrypted;
    }

    return {};
}

MessageProcessingResult QuorumParticipant::ProcessContribQDATA(CNode& pfrom, CDataStream& vStream,
                                                               CQuorum& quorum, CQuorumDataRequest& request)
{
    if (request.GetDataMask() & CQuorumDataRequest::ENCRYPTED_CONTRIBUTIONS) {
        assert(m_mn_activeman);

        if (WITH_LOCK(quorum.cs_vvec_shShare, return quorum.quorumVvec->size() != size_t(quorum.params.threshold))) {
            // Don't bump score because we asked for it
            LogPrint(BCLog::LLMQ, "QuorumParticipant::%s -- %s: No valid quorum verification vector available, from peer=%d\n", __func__, NetMsgType::QDATA, pfrom.GetId());
            return {};
        }

        int memberIdx = quorum.GetMemberIndex(request.GetProTxHash());
        if (memberIdx == -1) {
            // Don't bump score because we asked for it
            LogPrint(BCLog::LLMQ, "QuorumParticipant::%s -- %s: Not a member of the quorum, from peer=%d\n", __func__, NetMsgType::QDATA, pfrom.GetId());
            return {};
        }

        std::vector<CBLSIESEncryptedObject<CBLSSecretKey>> vecEncrypted;
        vStream >> vecEncrypted;

        std::vector<CBLSSecretKey> vecSecretKeys;
        vecSecretKeys.resize(vecEncrypted.size());
        for (const auto i : irange::range(vecEncrypted.size())) {
            if (!m_mn_activeman->Decrypt(vecEncrypted[i], memberIdx, vecSecretKeys[i], PROTOCOL_VERSION)) {
                return MisbehavingError{10, "failed to decrypt"};
            }
        }

        if (!quorum.SetSecretKeyShare(m_bls_worker.AggregateSecretKeys(vecSecretKeys), m_mn_activeman->GetProTxHash())) {
            return MisbehavingError{10, "invalid secret key share received"};
        }
    }

    return {};
}

void QuorumParticipant::StartQuorumDataRecoveryThread(CConnman& connman, CQuorumCPtr pQuorum,
                                                      gsl::not_null<const CBlockIndex*> pIndex, uint16_t nDataMaskIn) const
{
    assert(m_mn_activeman);

    bool expected = false;
    if (!pQuorum->fQuorumDataRecoveryThreadRunning.compare_exchange_strong(expected, true)) {
        LogPrint(BCLog::LLMQ, "QuorumParticipant::%s -- Already running\n", __func__);
        return;
    }

    m_qman.workerPool.push([&connman, pQuorum = std::move(pQuorum), pIndex, nDataMaskIn, this](int threadId) {
        size_t nTries{0};
        uint16_t nDataMask{nDataMaskIn};
        int64_t nTimeLastSuccess{0};
        uint256* pCurrentMemberHash{nullptr};
        std::vector<uint256> vecMemberHashes;
        const size_t nMyStartOffset{GetQuorumRecoveryStartOffset(*pQuorum, pIndex)};
        const int64_t nRequestTimeout{10};

        auto printLog = [&](const std::string& strMessage) {
            const std::string strMember{pCurrentMemberHash == nullptr ? "nullptr" : pCurrentMemberHash->ToString()};
            LogPrint(BCLog::LLMQ, "QuorumParticipant::StartQuorumDataRecoveryThread -- %s - for llmqType %d, quorumHash %s, nDataMask (%d/%d), pCurrentMemberHash %s, nTries %d\n",
                strMessage, ToUnderlying(pQuorum->qc->llmqType), pQuorum->qc->quorumHash.ToString(), nDataMask, nDataMaskIn, strMember, nTries);
        };
        printLog("Start");

        while (!m_mn_sync.IsBlockchainSynced() && !m_qman.quorumThreadInterrupt) {
            m_qman.quorumThreadInterrupt.sleep_for(std::chrono::seconds(nRequestTimeout));
        }

        if (m_qman.quorumThreadInterrupt) {
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

        while (nDataMask > 0 && !m_qman.quorumThreadInterrupt) {
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
                    LOCK(m_qman.cs_data_requests);
                    const CQuorumDataRequestKey key(*pCurrentMemberHash, true, pQuorum->qc->quorumHash, pQuorum->qc->llmqType);
                    auto it = m_qman.mapQuorumDataRequests.find(key);
                    if (it != m_qman.mapQuorumDataRequests.end() && !it->second.IsExpired(/*add_bias=*/true)) {
                        printLog("Already asked");
                        continue;
                    }
                }
                // Sleep a bit depending on the start offset to balance out multiple requests to same masternode
                m_qman.quorumThreadInterrupt.sleep_for(std::chrono::milliseconds(nMyStartOffset * 100));
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

                if (m_qman.RequestQuorumData(pNode, connman, *pQuorum, nDataMask, proTxHash)) {
                    nTimeLastSuccess = GetTime<std::chrono::seconds>().count();
                    printLog("Requested");
                } else {
                    LOCK(m_qman.cs_data_requests);
                    const CQuorumDataRequestKey key(*pCurrentMemberHash, true, pQuorum->qc->quorumHash, pQuorum->qc->llmqType);
                    auto it = m_qman.mapQuorumDataRequests.find(key);
                    if (it == m_qman.mapQuorumDataRequests.end()) {
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
            m_qman.quorumThreadInterrupt.sleep_for(std::chrono::seconds(1));
        }
        pQuorum->fQuorumDataRecoveryThreadRunning = false;
        printLog("Done");
    });
}

void QuorumParticipant::StartCleanupOldQuorumDataThread(gsl::not_null<const CBlockIndex*> pIndex) const
{
    // Note: this function is CPU heavy and we don't want it to be running during DKGs.
    // The largest dkgMiningWindowStart for a related quorum type is 42 (LLMQ_60_75).
    // At the same time most quorums use dkgInterval = 24 so the next DKG for them
    // (after block 576 + 42) will start at block 576 + 24 * 2. That's only a 6 blocks
    // window and it's better to have more room so we pick next cycle.
    // dkgMiningWindowStart for small quorums is 10 i.e. a safe block to start
    // these calculations is at height 576 + 24 * 2 + 10 = 576 + 58.
    if ((m_mn_activeman == nullptr && !m_quorums_watch) || (pIndex->nHeight % 576 != 58)) {
        return;
    }

    cxxtimer::Timer t(/*start=*/ true);
    LogPrint(BCLog::LLMQ, "QuorumParticipant::%s -- start\n", __func__);

    // do not block the caller thread
    m_qman.workerPool.push([pIndex, t, this](int threadId) {
        std::set<uint256> dbKeysToSkip;

        if (LOCK(cs_cleanup); cleanupQuorumsCache.empty()) {
            utils::InitQuorumsCache(cleanupQuorumsCache, m_chainman.GetConsensus(), /*limit_by_connections=*/false);
        }
        for (const auto& params : Params().GetConsensus().llmqs) {
            if (m_qman.quorumThreadInterrupt) {
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

        if (!m_qman.quorumThreadInterrupt) {
            WITH_LOCK(m_qman.cs_db, DataCleanupHelper(*m_qman.db, dbKeysToSkip));
        }

        LogPrint(BCLog::LLMQ, "QuorumParticipant::StartCleanupOldQuorumDataThread -- done. time=%d\n", t.count());
    });
}
} // namespace llmq
