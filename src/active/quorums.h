// Copyright (c) 2018-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_ACTIVE_QUORUMS_H
#define BITCOIN_ACTIVE_QUORUMS_H

#include <llmq/quorums.h>
#include <llmq/types.h>

#include <consensus/params.h>
#include <saltedhasher.h>
#include <span.h>
#include <sync.h>
#include <threadsafety.h>
#include <uint256.h>

#include <map>

class CActiveMasternodeManager;
class CBlockIndex;
class CBLSWorker;
class CConnman;
class CDeterministicMNManager;
class CDKGSessionManager;
class CNode;
class CSporkManager;
struct MessageProcessingResult;

namespace llmq {
class CQuorum;
class CQuorumDataRequest;
class CQuorumManager;
class CQuorumSnapshotManager;

class QuorumParticipant
{
private:
    CBLSWorker& m_bls_worker;
    CDeterministicMNManager& m_dmnman;
    CQuorumManager& m_qman;
    CQuorumSnapshotManager& m_qsnapman;
    const CActiveMasternodeManager* const m_mn_activeman;
    const ChainstateManager& m_chainman;
    const CMasternodeSync& m_mn_sync;
    const CSporkManager& m_sporkman;
    const bool m_quorums_recovery{false};
    const bool m_quorums_watch{false};
    const llmq::QvvecSyncModeMap m_sync_map;

private:
    mutable Mutex cs_cleanup;
    mutable std::map<Consensus::LLMQType, Uint256LruHashMap<uint256>> cleanupQuorumsCache GUARDED_BY(cs_cleanup);

public:
    QuorumParticipant() = delete;
    QuorumParticipant(const QuorumParticipant&) = delete;
    QuorumParticipant& operator=(const QuorumParticipant&) = delete;
    explicit QuorumParticipant(CBLSWorker& bls_worker, CDeterministicMNManager& dmnman, CQuorumManager& qman,
                               CQuorumSnapshotManager& qsnapman, const CActiveMasternodeManager* const mn_activeman,
                               const ChainstateManager& chainman, const CMasternodeSync& mn_sync,
                               const CSporkManager& sporkman, const llmq::QvvecSyncModeMap& sync_map,
                               bool quorums_recovery, bool quorums_watch);
    ~QuorumParticipant();

    bool SetQuorumSecretKeyShare(CQuorum& quorum, Span<CBLSSecretKey> skContributions) const;

    [[nodiscard]] MessageProcessingResult ProcessContribQGETDATA(bool request_limit_exceeded, CDataStream& vStream,
                                                                 const CQuorum& quorum, CQuorumDataRequest& request,
                                                                 gsl::not_null<const CBlockIndex*> block_index);
    [[nodiscard]] MessageProcessingResult ProcessContribQDATA(CNode& pfrom, CDataStream& vStream, CQuorum& quorum,
                                                              CQuorumDataRequest& request);

    void UpdatedBlockTip(const CBlockIndex* pindexNew, CConnman& connman, bool fInitialDownload) const;

private:
    /// Returns the start offset for the masternode with the given proTxHash. This offset is applied when picking data
    /// recovery members of a quorum's memberlist and is calculated based on a list of all member of all active quorums
    /// for the given llmqType in a way that each member should receive the same number of request if all active
    /// llmqType members requests data from one llmqType quorum.
    size_t GetQuorumRecoveryStartOffset(const CQuorum& quorum, gsl::not_null<const CBlockIndex*> pIndex) const;

    void CheckQuorumConnections(CConnman& connman, const Consensus::LLMQParams& llmqParams,
                                gsl::not_null<const CBlockIndex*> pindexNew) const;
    void DataRecoveryThread(CConnman& connman, gsl::not_null<const CBlockIndex*> block_index, CQuorumCPtr quorum,
                            uint16_t data_mask, const uint256& protx_hash, size_t start_offset) const;
    void StartCleanupOldQuorumDataThread(gsl::not_null<const CBlockIndex*> pIndex) const;
    void StartDataRecoveryThread(CConnman& connman, CQuorumCPtr pQuorum, gsl::not_null<const CBlockIndex*> pIndex,
                                 uint16_t nDataMask) const;
    void TriggerQuorumDataRecoveryThreads(CConnman& connman, gsl::not_null<const CBlockIndex*> pIndex) const;
};
} // namespace llmq

#endif // BITCOIN_ACTIVE_QUORUMS_H
