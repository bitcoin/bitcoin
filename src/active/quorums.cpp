// Copyright (c) 2018-2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <active/quorums.h>

#include <active/masternode.h>
#include <bls/bls_ies.h>
#include <bls/bls_worker.h>
#include <evo/deterministicmns.h>
#include <llmq/commitment.h>
#include <llmq/dkgsessionmgr.h>
#include <llmq/options.h>
#include <llmq/quorums.h>
#include <llmq/utils.h>
#include <masternode/sync.h>

#include <chain.h>
#include <chainparams.h>
#include <logging.h>
#include <net.h>
#include <netmessagemaker.h>
#include <validation.h>

#include <cxxtimer.hpp>

namespace llmq {
QuorumParticipant::QuorumParticipant(CBLSWorker& bls_worker, CConnman& connman, CDeterministicMNManager& dmnman,
                                     QuorumObserverParent& qman, CQuorumSnapshotManager& qsnapman,
                                     const CActiveMasternodeManager& mn_activeman, const ChainstateManager& chainman,
                                     const CMasternodeSync& mn_sync, const CSporkManager& sporkman,
                                     const llmq::QvvecSyncModeMap& sync_map, bool quorums_recovery, bool quorums_watch) :
    QuorumObserver(connman, dmnman, qman, qsnapman, chainman, mn_sync, sporkman, sync_map, quorums_recovery),
    m_bls_worker{bls_worker},
    m_mn_activeman{mn_activeman},
    m_quorums_watch{quorums_watch}
{
}

QuorumParticipant::~QuorumParticipant() = default;

void QuorumParticipant::CheckQuorumConnections(const Consensus::LLMQParams& llmqParams,
                                               gsl::not_null<const CBlockIndex*> pindexNew) const
{
    auto lastQuorums = m_qman.ScanQuorums(llmqParams.type, pindexNew, (size_t)llmqParams.keepOldConnections);
    auto deletableQuorums = GetQuorumsToDelete(llmqParams, pindexNew);

    const uint256 proTxHash = m_mn_activeman.GetProTxHash();
    const bool watchOtherISQuorums = llmqParams.type == Params().GetConsensus().llmqTypeDIP0024InstantSend &&
                                     ranges::any_of(lastQuorums, [&proTxHash](const auto& old_quorum){ return old_quorum->IsMember(proTxHash); });

    for (const auto& quorum : lastQuorums) {
        if (utils::EnsureQuorumConnections(llmqParams, m_connman, m_sporkman, {m_dmnman, m_qsnapman, m_chainman, quorum->m_quorum_base_block_index},
                                           m_dmnman.GetListAtChainTip(), proTxHash, /*is_masternode=*/true, m_quorums_watch)) {
            if (deletableQuorums.erase(quorum->qc->quorumHash) > 0) {
                LogPrint(BCLog::LLMQ, "QuorumParticipant::%s -- llmqType[%d] h[%d] keeping mn quorum connections for quorum: [%d:%s]\n", __func__, ToUnderlying(llmqParams.type), pindexNew->nHeight, quorum->m_quorum_base_block_index->nHeight, quorum->m_quorum_base_block_index->GetBlockHash().ToString());
            }
        } else if (watchOtherISQuorums && !quorum->IsMember(proTxHash)) {
            Uint256HashSet connections;
            const auto& cindexes = utils::CalcDeterministicWatchConnections(llmqParams.type, quorum->m_quorum_base_block_index, quorum->members.size(), 1);
            for (auto idx : cindexes) {
                connections.emplace(quorum->members[idx]->proTxHash);
            }
            if (!connections.empty()) {
                if (!m_connman.HasMasternodeQuorumNodes(llmqParams.type, quorum->m_quorum_base_block_index->GetBlockHash())) {
                    LogPrint(BCLog::LLMQ, "QuorumParticipant::%s -- llmqType[%d] h[%d] adding mn inter-quorum connections for quorum: [%d:%s]\n", __func__, ToUnderlying(llmqParams.type), pindexNew->nHeight, quorum->m_quorum_base_block_index->nHeight, quorum->m_quorum_base_block_index->GetBlockHash().ToString());
                    m_connman.SetMasternodeQuorumNodes(llmqParams.type, quorum->m_quorum_base_block_index->GetBlockHash(), connections);
                    m_connman.SetMasternodeQuorumRelayMembers(llmqParams.type, quorum->m_quorum_base_block_index->GetBlockHash(), connections);
                }
                if (deletableQuorums.erase(quorum->qc->quorumHash) > 0) {
                    LogPrint(BCLog::LLMQ, "QuorumParticipant::%s -- llmqType[%d] h[%d] keeping mn inter-quorum connections for quorum: [%d:%s]\n", __func__, ToUnderlying(llmqParams.type), pindexNew->nHeight, quorum->m_quorum_base_block_index->nHeight, quorum->m_quorum_base_block_index->GetBlockHash().ToString());
                }
            }
        }
    }

    for (const auto& quorumHash : deletableQuorums) {
        LogPrint(BCLog::LLMQ, "QuorumParticipant::%s -- removing masternodes quorum connections for quorum %s:\n", __func__, quorumHash.ToString());
        m_connman.RemoveMasternodeQuorumNodes(llmqParams.type, quorumHash);
    }
}

bool QuorumParticipant::SetQuorumSecretKeyShare(CQuorum& quorum, Span<CBLSSecretKey> skContributions) const
{
    return quorum.SetSecretKeyShare(m_bls_worker.AggregateSecretKeys(skContributions), m_mn_activeman.GetProTxHash());
}

size_t QuorumParticipant::GetQuorumRecoveryStartOffset(const CQuorum& quorum, gsl::not_null<const CBlockIndex*> pIndex) const
{
    auto mns = m_dmnman.GetListForBlock(pIndex);
    std::vector<uint256> vecProTxHashes;
    vecProTxHashes.reserve(mns.GetValidMNsCount());
    mns.ForEachMN(/*onlyValid=*/true,
                  [&](const auto& pMasternode) { vecProTxHashes.emplace_back(pMasternode.proTxHash); });
    std::sort(vecProTxHashes.begin(), vecProTxHashes.end());
    size_t nIndex{0};
    {
        auto my_protx_hash = m_mn_activeman.GetProTxHash();
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
        if (!m_qman.GetEncryptedContributions(request.GetLLMQType(), block_index,
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
        if (WITH_LOCK(quorum.cs_vvec_shShare, return !quorum.HasVerificationVectorInternal()
                                                     || quorum.quorumVvec->size() != size_t(quorum.params.threshold))) {
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
            if (!m_mn_activeman.Decrypt(vecEncrypted[i], memberIdx, vecSecretKeys[i], PROTOCOL_VERSION)) {
                return MisbehavingError{10, "failed to decrypt"};
            }
        }

        if (!quorum.SetSecretKeyShare(m_bls_worker.AggregateSecretKeys(vecSecretKeys), m_mn_activeman.GetProTxHash())) {
            return MisbehavingError{10, "invalid secret key share received"};
        }
    }

    return {};
}

bool QuorumParticipant::IsMasternode() const
{
    // We are only initialized if masternode mode is enabled
    return true;
}

bool QuorumParticipant::IsWatching() const
{
    // Watch-only mode can co-exist with masternode mode
    return m_quorums_watch;
}

void QuorumParticipant::StartDataRecoveryThread(gsl::not_null<const CBlockIndex*> pIndex, CQuorumCPtr pQuorum,
                                                uint16_t nDataMaskIn) const
{
    bool expected = false;
    if (!pQuorum->fQuorumDataRecoveryThreadRunning.compare_exchange_strong(expected, true)) {
        LogPrint(BCLog::LLMQ, "QuorumParticipant::%s -- Already running\n", __func__);
        return;
    }

    workerPool.push([pQuorum = std::move(pQuorum), pIndex, nDataMaskIn, this](int threadId) mutable {
        const size_t size_offset = GetQuorumRecoveryStartOffset(*pQuorum, pIndex);
        DataRecoveryThread(pIndex, std::move(pQuorum), nDataMaskIn, m_mn_activeman.GetProTxHash(), size_offset);
    });
}

void QuorumParticipant::TriggerQuorumDataRecoveryThreads(gsl::not_null<const CBlockIndex*> block_index) const
{
    if (!m_quorums_recovery) {
        return;
    }

    LogPrint(BCLog::LLMQ, "QuorumParticipant::%s -- Process block %s\n", __func__, block_index->GetBlockHash().ToString());

    const uint256 proTxHash = m_mn_activeman.GetProTxHash();

    for (const auto& params : Params().GetConsensus().llmqs) {
        auto vecQuorums = m_qman.ScanQuorums(params.type, block_index, params.keepOldConnections);
        const bool fWeAreQuorumTypeMember = ranges::any_of(vecQuorums, [&proTxHash](const auto& pQuorum) { return pQuorum->IsValidMember(proTxHash); });

        for (auto& pQuorum : vecQuorums) {
            if (pQuorum->IsValidMember(proTxHash)) {
                uint16_t nDataMask{0};
                if (!pQuorum->HasVerificationVector()) {
                    nDataMask |= CQuorumDataRequest::QUORUM_VERIFICATION_VECTOR;
                }
                if (!pQuorum->GetSkShare().IsValid()) {
                    nDataMask |= CQuorumDataRequest::ENCRYPTED_CONTRIBUTIONS;
                }
                if (nDataMask != 0) {
                    StartDataRecoveryThread(block_index, std::move(pQuorum), nDataMask);
                } else {
                    LogPrint(BCLog::LLMQ, "QuorumParticipant::%s -- No data needed from (%d, %s) at height %d\n", __func__,
                             ToUnderlying(pQuorum->qc->llmqType), pQuorum->qc->quorumHash.ToString(), block_index->nHeight);
                }
            } else {
                TryStartVvecSyncThread(block_index, std::move(pQuorum), fWeAreQuorumTypeMember);
            }
        }
    }
}
} // namespace llmq
