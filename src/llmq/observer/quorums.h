// Copyright (c) 2018-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LLMQ_OBSERVER_QUORUMS_H
#define BITCOIN_LLMQ_OBSERVER_QUORUMS_H

#include <llmq/quorumsman.h>
#include <llmq/types.h>

#include <consensus/params.h>
#include <saltedhasher.h>
#include <span.h>
#include <sync.h>
#include <threadsafety.h>
#include <uint256.h>

#include <gsl/pointers.h>

#include <map>

class CConnman;
struct MessageProcessingResult;

namespace llmq {
class CQuorumManager;
enum class QvvecSyncMode : int8_t;

class QuorumObserver
{
protected:
    CConnman& m_connman;
    CDeterministicMNManager& m_dmnman;
    CQuorumManager& m_qman;
    CQuorumSnapshotManager& m_qsnapman;
    const ChainstateManager& m_chainman;
    const CMasternodeSync& m_mn_sync;
    const CSporkManager& m_sporkman;
    const bool m_quorums_recovery{false};
    const llmq::QvvecSyncModeMap m_sync_map;

private:
    mutable Mutex cs_cleanup;
    mutable std::map<Consensus::LLMQType, Uint256LruHashMap<uint256>> cleanupQuorumsCache GUARDED_BY(cs_cleanup);

public:
    QuorumObserver() = delete;
    QuorumObserver(const QuorumObserver&) = delete;
    QuorumObserver& operator=(const QuorumObserver&) = delete;
    explicit QuorumObserver(CConnman& connman, CDeterministicMNManager& dmnman, CQuorumManager& qman,
                            CQuorumSnapshotManager& qsnapman, const ChainstateManager& chainman,
                            const CMasternodeSync& mn_sync, const CSporkManager& sporkman,
                            const llmq::QvvecSyncModeMap& sync_map, bool quorums_recovery);
    virtual ~QuorumObserver();

    void UpdatedBlockTip(const CBlockIndex* pindexNew, bool fInitialDownload) const;

public:
    virtual bool SetQuorumSecretKeyShare(CQuorum& quorum, Span<CBLSSecretKey> skContributions) const;
    [[nodiscard]] virtual MessageProcessingResult ProcessContribQGETDATA(
        bool request_limit_exceeded, CDataStream& vStream, const CQuorum& quorum, CQuorumDataRequest& request,
        gsl::not_null<const CBlockIndex*> block_index);
    [[nodiscard]] virtual MessageProcessingResult ProcessContribQDATA(
        CNode& pfrom, CDataStream& vStream, CQuorum& quorum, CQuorumDataRequest& request);
    virtual bool IsMasternode() const;
    virtual bool IsWatching() const;

protected:
    virtual void CheckQuorumConnections(const Consensus::LLMQParams& llmqParams,
                                        gsl::not_null<const CBlockIndex*> pindexNew) const;
    virtual void TriggerQuorumDataRecoveryThreads(gsl::not_null<const CBlockIndex*> pIndex) const;

protected:
    Uint256HashSet GetQuorumsToDelete(const Consensus::LLMQParams& llmqParams,
                                      gsl::not_null<const CBlockIndex*> pindexNew) const;

    void DataRecoveryThread(gsl::not_null<const CBlockIndex*> block_index, CQuorumCPtr quorum, uint16_t data_mask,
                            const uint256& protx_hash, size_t start_offset) const;
    void StartVvecSyncThread(gsl::not_null<const CBlockIndex*> block_index, CQuorumCPtr pQuorum) const;
    void TryStartVvecSyncThread(gsl::not_null<const CBlockIndex*> block_index, CQuorumCPtr pQuorum,
                                bool fWeAreQuorumTypeMember) const;

    void StartCleanupOldQuorumDataThread(gsl::not_null<const CBlockIndex*> pIndex) const;
};
} // namespace llmq

#endif // BITCOIN_LLMQ_OBSERVER_QUORUMS_H
