// Copyright (c) 2018-2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LLMQ_OBSERVER_QUORUMS_H
#define BITCOIN_LLMQ_OBSERVER_QUORUMS_H

#include <bls/bls_ies.h>
#include <ctpl_stl.h>
#include <llmq/options.h>
#include <llmq/types.h>

#include <consensus/params.h>
#include <saltedhasher.h>
#include <span.h>
#include <sync.h>
#include <threadsafety.h>
#include <uint256.h>
#include <util/threadinterrupt.h>

#include <gsl/pointers.h>

#include <map>
#include <set>

class CConnman;
class CDataStream;
class CDeterministicMNManager;
class CMasternodeSync;
class CNode;
class CSporkManager;
struct MessageProcessingResult;
namespace llmq {
class CQuorumDataRequest;
class CQuorumSnapshotManager;
} // namespace llmq

namespace llmq {
enum class DataRequestStatus : uint8_t {
    NotFound,
    Pending,
    Processed,
};

class QuorumObserverParent
{
public:
    virtual ~QuorumObserverParent() = default;
    virtual bool GetEncryptedContributions(Consensus::LLMQType llmq_type, const CBlockIndex* block_index,
                                           const std::vector<bool>& valid_members, const uint256& protx_hash,
                                           std::vector<CBLSIESEncryptedObject<CBLSSecretKey>>& vec_enc) const = 0;
    virtual bool IsDataRequestPending(const uint256& proRegTx, bool we_requested, const uint256& quorumHash,
                                      Consensus::LLMQType llmqType) const = 0;
    virtual bool RequestQuorumData(CNode* pfrom, CConnman& connman, const CQuorum& quorum, uint16_t nDataMask,
                                   const uint256& proTxHash) const = 0;
    virtual DataRequestStatus GetDataRequestStatus(const uint256& proRegTx, bool we_requested,
                                                   const uint256& quorumHash, Consensus::LLMQType llmqType) const = 0;
    virtual std::vector<CQuorumCPtr> ScanQuorums(Consensus::LLMQType llmqType,
                                                 gsl::not_null<const CBlockIndex*> pindexStart,
                                                 size_t nCountRequested) const = 0;
    virtual void CleanupExpiredDataRequests() const = 0;
    virtual void CleanupOldQuorumData(const std::set<uint256>& dbKeysToSkip) const = 0;
};

class QuorumObserver
{
protected:
    CConnman& m_connman;
    CDeterministicMNManager& m_dmnman;
    QuorumObserverParent& m_qman;
    CQuorumSnapshotManager& m_qsnapman;
    const ChainstateManager& m_chainman;
    const CMasternodeSync& m_mn_sync;
    const CSporkManager& m_sporkman;
    const bool m_quorums_recovery{false};
    const llmq::QvvecSyncModeMap m_sync_map;

protected:
    mutable Mutex cs_cleanup;
    mutable std::map<Consensus::LLMQType, Uint256LruHashMap<uint256>> cleanupQuorumsCache GUARDED_BY(cs_cleanup);

    mutable ctpl::thread_pool workerPool;
    mutable CThreadInterrupt quorumThreadInterrupt;

public:
    QuorumObserver() = delete;
    QuorumObserver(const QuorumObserver&) = delete;
    QuorumObserver& operator=(const QuorumObserver&) = delete;
    explicit QuorumObserver(CConnman& connman, CDeterministicMNManager& dmnman, QuorumObserverParent& qman,
                            CQuorumSnapshotManager& qsnapman, const ChainstateManager& chainman,
                            const CMasternodeSync& mn_sync, const CSporkManager& sporkman,
                            const llmq::QvvecSyncModeMap& sync_map, bool quorums_recovery);
    virtual ~QuorumObserver();

    void Start();
    void Stop();

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
