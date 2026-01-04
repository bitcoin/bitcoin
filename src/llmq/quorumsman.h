// Copyright (c) 2018-2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LLMQ_QUORUMSMAN_H
#define BITCOIN_LLMQ_QUORUMSMAN_H

#include <evo/types.h>
#include <llmq/observer/quorums.h>
#include <llmq/options.h>
#include <llmq/params.h>
#include <llmq/quorums.h>
#include <llmq/types.h>
#include <msg_result.h>
#include <unordered_lru_cache.h>

#include <sync.h>
#include <util/threadinterrupt.h>
#include <util/time.h>

#include <gsl/pointers.h>

#include <atomic>
#include <deque>
#include <map>
#include <memory>
#include <thread>
#include <utility>

class CBLSSignature;
class CBLSWorker;
class CBlockIndex;
class CChain;
class CConnman;
class CDataStream;
class CDeterministicMNManager;
class CDBWrapper;
class CEvoDB;
class ChainstateManager;
class CNode;
namespace util {
struct DbWrapperParams;
} // namespace util

namespace llmq {
enum class VerifyRecSigStatus : uint8_t {
    NoQuorum,
    Invalid,
    Valid,
};

class CDKGSessionManager;
class CQuorumBlockProcessor;
class CQuorumSnapshotManager;
class QuorumObserver;
class QuorumParticipant;

/**
 * The quorum manager maintains quorums which were mined on chain. When a quorum is requested from the manager,
 * it will lookup the commitment (through CQuorumBlockProcessor) and build a CQuorum object from it.
 *
 * It is also responsible for initialization of the intra-quorum connections for new quorums.
 */
class CQuorumManager final : public QuorumObserverParent
{
    friend class llmq::QuorumObserver;
    friend class llmq::QuorumParticipant;

private:
    CBLSWorker& blsWorker;
    CDeterministicMNManager& m_dmnman;
    CQuorumBlockProcessor& quorumBlockProcessor;
    CQuorumSnapshotManager& m_qsnapman;
    const ChainstateManager& m_chainman;
    llmq::CDKGSessionManager* m_qdkgsman{nullptr};
    llmq::QuorumObserver* m_handler{nullptr};

private:
    mutable Mutex cs_db;
    std::unique_ptr<CDBWrapper> db GUARDED_BY(cs_db){nullptr};

    mutable Mutex cs_data_requests;
    mutable std::unordered_map<CQuorumDataRequestKey, CQuorumDataRequest, StaticSaltedHasher> mapQuorumDataRequests
        GUARDED_BY(cs_data_requests);

    mutable Mutex cs_map_quorums;
    mutable std::map<Consensus::LLMQType, Uint256LruHashMap<CQuorumPtr>> mapQuorumsCache GUARDED_BY(cs_map_quorums);

    mutable Mutex cs_scan_quorums; // TODO: merge cs_map_quorums, cs_scan_quorums mutexes
    mutable std::map<Consensus::LLMQType, Uint256LruHashMap<std::vector<CQuorumCPtr>>> scanQuorumsCache
        GUARDED_BY(cs_scan_quorums);

    // On mainnet, we have around 62 quorums active at any point; let's cache a little more than double that to be safe.
    // it maps `quorum_hash` to `pindex`
    mutable Mutex cs_quorumBaseBlockIndexCache;
    mutable Uint256LruHashMap<const CBlockIndex*, /*max_size=*/128> quorumBaseBlockIndexCache
        GUARDED_BY(cs_quorumBaseBlockIndexCache);

    mutable Mutex m_cache_cs;
    mutable std::deque<CQuorumCPtr> m_cache_queue GUARDED_BY(m_cache_cs);
    mutable CThreadInterrupt m_cache_interrupt;
    mutable std::thread m_cache_thread;

public:
    CQuorumManager() = delete;
    CQuorumManager(const CQuorumManager&) = delete;
    CQuorumManager& operator=(const CQuorumManager&) = delete;
    explicit CQuorumManager(CBLSWorker& _blsWorker, CDeterministicMNManager& dmnman, CEvoDB& _evoDb,
                            CQuorumBlockProcessor& _quorumBlockProcessor, CQuorumSnapshotManager& qsnapman,
                            const ChainstateManager& chainman, const util::DbWrapperParams& db_params);
    ~CQuorumManager();

    void ConnectManagers(gsl::not_null<llmq::QuorumObserver*> handler, gsl::not_null<llmq::CDKGSessionManager*> qdkgsman)
    {
        // Prohibit double initialization
        assert(m_handler == nullptr);
        m_handler = handler;
        assert(m_qdkgsman == nullptr);
        m_qdkgsman = qdkgsman;
    }
    void DisconnectManagers()
    {
        m_handler = nullptr;
        m_qdkgsman = nullptr;
    }

    bool GetEncryptedContributions(Consensus::LLMQType llmq_type, const CBlockIndex* block_index,
                                   const std::vector<bool>& valid_members, const uint256& protx_hash,
                                   std::vector<CBLSIESEncryptedObject<CBLSSecretKey>>& vec_enc) const override;

    [[nodiscard]] MessageProcessingResult ProcessMessage(CNode& pfrom, CConnman& connman, std::string_view msg_type,
                                                         CDataStream& vRecv)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_db, !cs_data_requests, !cs_map_quorums, !m_cache_cs);

    static bool HasQuorum(Consensus::LLMQType llmqType, const CQuorumBlockProcessor& quorum_block_processor, const uint256& quorumHash);

    bool RequestQuorumData(CNode* pfrom, CConnman& connman, const CQuorum& quorum, uint16_t nDataMask,
                           const uint256& proTxHash = uint256()) const override
        EXCLUSIVE_LOCKS_REQUIRED(!cs_data_requests);

    // all these methods will lock cs_main for a short period of time
    CQuorumCPtr GetQuorum(Consensus::LLMQType llmqType, const uint256& quorumHash) const
        EXCLUSIVE_LOCKS_REQUIRED(!cs_db, !cs_map_quorums, !m_cache_cs);
    std::vector<CQuorumCPtr> ScanQuorums(Consensus::LLMQType llmqType, size_t nCountRequested) const
        EXCLUSIVE_LOCKS_REQUIRED(!cs_db, !cs_map_quorums, !cs_scan_quorums, !m_cache_cs);

    // this one is cs_main-free
    std::vector<CQuorumCPtr> ScanQuorums(Consensus::LLMQType llmqType, gsl::not_null<const CBlockIndex*> pindexStart,
                                         size_t nCountRequested) const override
        EXCLUSIVE_LOCKS_REQUIRED(!cs_db, !cs_map_quorums, !cs_scan_quorums, !m_cache_cs);

    bool IsMasternode() const;
    bool IsWatching() const;

    bool IsDataRequestPending(const uint256& proRegTx, bool we_requested, const uint256& quorumHash,
                              Consensus::LLMQType llmqType) const override EXCLUSIVE_LOCKS_REQUIRED(!cs_data_requests);
    DataRequestStatus GetDataRequestStatus(const uint256& proRegTx, bool we_requested, const uint256& quorumHash,
                                           Consensus::LLMQType llmqType) const override
        EXCLUSIVE_LOCKS_REQUIRED(!cs_data_requests);
    void CleanupExpiredDataRequests() const override EXCLUSIVE_LOCKS_REQUIRED(!cs_data_requests);
    void CleanupOldQuorumData(const std::set<uint256>& dbKeysToSkip) const override EXCLUSIVE_LOCKS_REQUIRED(!cs_db);

private:
    // all private methods here are cs_main-free
    bool BuildQuorumContributions(const CFinalCommitmentPtr& fqc, const std::shared_ptr<CQuorum>& quorum) const;

    CQuorumPtr BuildQuorumFromCommitment(Consensus::LLMQType llmqType,
                                         gsl::not_null<const CBlockIndex*> pQuorumBaseBlockIndex,
                                         bool populate_cache) const
        EXCLUSIVE_LOCKS_REQUIRED(!cs_db, !cs_map_quorums, !m_cache_cs);

    CQuorumCPtr GetQuorum(Consensus::LLMQType llmqType, gsl::not_null<const CBlockIndex*> pindex,
                          bool populate_cache = true) const
        EXCLUSIVE_LOCKS_REQUIRED(!cs_db, !cs_map_quorums, !m_cache_cs);

    void QueueQuorumForWarming(CQuorumCPtr pQuorum) const EXCLUSIVE_LOCKS_REQUIRED(!m_cache_cs);
    void CacheWarmingThreadMain() const EXCLUSIVE_LOCKS_REQUIRED(!m_cache_cs);
    void MigrateOldQuorumDB(CEvoDB& evoDb) const EXCLUSIVE_LOCKS_REQUIRED(!cs_db);
};

// when selecting a quorum for signing and verification, we use CQuorumManager::SelectQuorum with this offset as
// starting height for scanning. This is because otherwise the resulting signatures would not be verifiable by nodes
// which are not 100% at the chain tip.
static constexpr int SIGN_HEIGHT_OFFSET{8};

CQuorumCPtr SelectQuorumForSigning(const Consensus::LLMQParams& llmq_params, const CChain& active_chain, const CQuorumManager& qman,
                                   const uint256& selectionHash, int signHeight = -1 /*chain tip*/, int signOffset = SIGN_HEIGHT_OFFSET);

// Verifies a recovered sig that was signed while the chain tip was at signedAtTip
VerifyRecSigStatus VerifyRecoveredSig(Consensus::LLMQType llmqType, const CChain& active_chain, const CQuorumManager& qman,
                                      int signedAtHeight, const uint256& id, const uint256& msgHash, const CBLSSignature& sig,
                                      int signOffset = SIGN_HEIGHT_OFFSET);
} // namespace llmq

#endif // BITCOIN_LLMQ_QUORUMSMAN_H
