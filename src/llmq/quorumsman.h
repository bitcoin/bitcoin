// Copyright (c) 2018-2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LLMQ_QUORUMSMAN_H
#define BITCOIN_LLMQ_QUORUMSMAN_H

#include <bls/bls_ies.h>
#include <evo/types.h>
#include <kernel/cs_main.h> // IWYU pragma: export
#include <llmq/cache.h>
#include <llmq/params.h>
#include <llmq/quorums.h>
#include <llmq/types.h>
#include <saltedhasher.h>
#include <sync.h>
#include <util/threadinterrupt.h>

#include <gsl/pointers.h>

#include <deque>
#include <map>
#include <memory>
#include <thread>

class CBLSSignature;
class CBLSWorker;
class CBlockIndex;
class CChain;
class CDeterministicMNManager;
class CDBWrapper;
class CEvoDB;
class ChainstateManager;
namespace util {
struct DbWrapperParams;
} // namespace util

namespace llmq {
enum class VerifyRecSigStatus : uint8_t {
    NoQuorum,
    Invalid,
    Valid,
};

class QuorumRole;
class CDKGSessionManager;
class CQuorumBlockProcessor;
class CQuorumSnapshotManager;

//! A peer normally needs data for only a handful of quorums at once. These limits keep
//! attacker-controlled inbound tracking bounded independently of request expiry or block production.
static constexpr size_t MAX_INBOUND_DATA_REQUESTS_PER_REQUESTER{64};
static constexpr size_t MAX_INBOUND_DATA_REQUESTS{4096};

enum class DataRequestRegistration : uint8_t {
    Accepted,
    RateLimited,
    RequesterLimitExceeded,
    CapacityExhausted,
};

/**
 * The quorum manager maintains quorums which were mined on chain. When a quorum is requested from the manager,
 * it will lookup the commitment (through CQuorumBlockProcessor) and build a CQuorum object from it.
 */
class CQuorumManager final
{
private:
    CBLSWorker& blsWorker;
    CDeterministicMNManager& m_dmnman;
    CQuorumBlockProcessor& quorumBlockProcessor;
    CQuorumSnapshotManager& m_qsnapman;
    const ChainstateManager& m_chainman;
    llmq::CDKGSessionManager* m_qdkgsman{nullptr};
    llmq::QuorumRole* m_handler{nullptr};

private:
    mutable Mutex cs_db;
    std::unique_ptr<CDBWrapper> db GUARDED_BY(cs_db){nullptr};

    mutable Mutex cs_data_requests;
    mutable std::unordered_map<CQuorumDataRequestKey, CQuorumDataRequest, StaticSaltedHasher> mapQuorumDataRequests
        GUARDED_BY(cs_data_requests);
    mutable std::unordered_map<uint256, size_t, StaticSaltedHasher> m_inbound_request_counts GUARDED_BY(cs_data_requests);
    mutable size_t m_inbound_request_count GUARDED_BY(cs_data_requests){0};

    mutable Mutex m_cs_maps;
    mutable std::map<Consensus::LLMQType, Uint256LruHashMap<std::vector<CFinalCommitment>>> scanCommitmentsCache
        GUARDED_BY(m_cs_maps);
    mutable PerLlmqTypeCache<CQuorumPtr> mapQuorumsCache GUARDED_BY(m_cs_maps);
    mutable PerLlmqTypeCache<std::vector<CQuorumCPtr>> scanQuorumsCache GUARDED_BY(m_cs_maps);

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
                            const ChainstateManager& chainman, const util::DbWrapperParams& db_params)
        EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    ~CQuorumManager();

    void ConnectManagers(gsl::not_null<llmq::QuorumRole*> handler, gsl::not_null<llmq::CDKGSessionManager*> qdkgsman)
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
                                   std::vector<CBLSIESEncryptedObject<CBLSSecretKey>>& vec_enc) const;

    static bool HasQuorum(Consensus::LLMQType llmqType, const CQuorumBlockProcessor& quorum_block_processor, const uint256& quorumHash);
    static bool HasQuorum(Consensus::LLMQType llmqType, const CQuorumBlockProcessor& quorum_block_processor,
                          const uint256& quorumHash, const CChain& chain)
        EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    // all these methods will lock cs_main for a short period of time
    CQuorumCPtr GetQuorum(Consensus::LLMQType llmqType, const uint256& quorumHash) const
        EXCLUSIVE_LOCKS_REQUIRED(!cs_db, !m_cs_maps, !m_cache_cs);
    CQuorumCPtr GetQuorum(Consensus::LLMQType llmqType, const uint256& quorumHash, const CChain& chain) const
        EXCLUSIVE_LOCKS_REQUIRED(::cs_main, !cs_db, !m_cs_maps, !m_cache_cs);
    std::vector<CQuorumCPtr> ScanQuorums(Consensus::LLMQType llmqType, size_t nCountRequested) const
        EXCLUSIVE_LOCKS_REQUIRED(!cs_db, !m_cs_maps, !m_cache_cs);

    // this one needs no cs_main held; it locks cs_main briefly unless the full result is cached
    std::vector<CQuorumCPtr> ScanQuorums(Consensus::LLMQType llmqType, gsl::not_null<const CBlockIndex*> pindexStart,
                                         size_t nCountRequested) const
        EXCLUSIVE_LOCKS_REQUIRED(!cs_db, !m_cs_maps, !m_cache_cs);
    std::vector<CQuorumCPtr> ScanQuorums(Consensus::LLMQType llmqType,
                                         gsl::not_null<const CBlockIndex*> pindexStart,
                                         size_t nCountRequested, const CChain& chain) const
        EXCLUSIVE_LOCKS_REQUIRED(::cs_main, !cs_db, !m_cs_maps, !m_cache_cs);

    std::vector<CFinalCommitment> ScanCommitments(Consensus::LLMQType llmqType, size_t nCountRequested) const
        EXCLUSIVE_LOCKS_REQUIRED(!cs_db, !m_cs_maps);
    std::vector<CFinalCommitment> ScanCommitments(Consensus::LLMQType llmqType, gsl::not_null<const CBlockIndex*> pindexStart,
                                                  size_t nCountRequested) const
        EXCLUSIVE_LOCKS_REQUIRED(!cs_db, !m_cs_maps);

    bool IsMasternode() const;
    bool IsWatching() const;

    //! Request tracking for QGETDATA/QDATA — used by NetQuorum and RPC. Inbound entries are
    //! bounded per requester and globally; outbound entries initiated by us do not consume either budget.
    DataRequestRegistration RegisterDataRequest(const CQuorumDataRequestKey& key, const CQuorumDataRequest& request,
                                                bool add_expiry_bias = true) const
        EXCLUSIVE_LOCKS_REQUIRED(!cs_data_requests);
    enum class DataResponseValidation : uint8_t { OK, NotRequested, AlreadyReceived, Mismatch };
    DataResponseValidation ValidateDataResponse(const CQuorumDataRequestKey& key,
                                                const CQuorumDataRequest& response) const
        EXCLUSIVE_LOCKS_REQUIRED(!cs_data_requests);
    bool IsDataRequestPending(const uint256& proRegTx, bool we_requested, const uint256& quorumHash,
                              Consensus::LLMQType llmqType) const EXCLUSIVE_LOCKS_REQUIRED(!cs_data_requests);
    DataRequestStatus GetDataRequestStatus(const uint256& proRegTx, bool we_requested, const uint256& quorumHash,
                                           Consensus::LLMQType llmqType) const
        EXCLUSIVE_LOCKS_REQUIRED(!cs_data_requests);
    void CleanupExpiredDataRequests() const EXCLUSIVE_LOCKS_REQUIRED(!cs_data_requests);

    void CleanupOldQuorumData(const Uint256HashSet& dbKeysToSkip) const EXCLUSIVE_LOCKS_REQUIRED(!cs_db);

    //! Used by NetQuorum for QDATA processing
    CQuorumPtr GetCachedMutableQuorum(Consensus::LLMQType llmqType, const uint256& quorumHash) const
        EXCLUSIVE_LOCKS_REQUIRED(!m_cs_maps);
    void WriteContributions(const CQuorumPtr& quorum) const EXCLUSIVE_LOCKS_REQUIRED(!cs_db);
    void QueueQuorumForWarming(CQuorumCPtr pQuorum) const EXCLUSIVE_LOCKS_REQUIRED(!m_cache_cs);

private:
    //! Unless the full result is cached it locks cs_main, so that only results computed against the
    //! active chain are cached.
    std::vector<CQuorumCPtr> ScanQuorums(Consensus::LLMQType llmqType,
                                         gsl::not_null<const CBlockIndex*> pindexStart,
                                         size_t nCountRequested, const CChain* chain) const
        EXCLUSIVE_LOCKS_REQUIRED(!cs_db, !m_cs_maps, !m_cache_cs);
    bool BuildQuorumContributions(const CFinalCommitmentPtr& fqc, const std::shared_ptr<CQuorum>& quorum) const;

    CQuorumPtr BuildQuorumFromCommitment(Consensus::LLMQType llmqType,
                                         gsl::not_null<const CBlockIndex*> pQuorumBaseBlockIndex,
                                         bool populate_cache) const
        EXCLUSIVE_LOCKS_REQUIRED(!cs_db, !m_cs_maps, !m_cache_cs);

    CQuorumCPtr GetQuorum(Consensus::LLMQType llmqType, gsl::not_null<const CBlockIndex*> pindex,
                          bool populate_cache = true) const
        EXCLUSIVE_LOCKS_REQUIRED(!cs_db, !m_cs_maps, !m_cache_cs);
    CQuorumCPtr GetQuorum(Consensus::LLMQType llmqType, gsl::not_null<const CBlockIndex*> pindex,
                          const CChain& chain, bool populate_cache = true) const
        EXCLUSIVE_LOCKS_REQUIRED(::cs_main, !cs_db, !m_cs_maps, !m_cache_cs);

    void CacheWarmingThreadMain() const EXCLUSIVE_LOCKS_REQUIRED(!m_cache_cs);
    void MigrateOldQuorumDB(CEvoDB& evoDb) const EXCLUSIVE_LOCKS_REQUIRED(::cs_main, !cs_db);
};

// when selecting a quorum for signing and verification, we use CQuorumManager::SelectQuorum with this offset as
// starting height for scanning. This is because otherwise the resulting signatures would not be verifiable by nodes
// which are not 100% at the chain tip.
static constexpr int SIGN_HEIGHT_OFFSET{8};

CBlockIndex* SelectQuorumForSigningStartBlock(const CChain& active_chain, int signHeight = -1 /*chain tip*/,
                                              int signOffset = SIGN_HEIGHT_OFFSET) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

CQuorumCPtr SelectQuorumForSigning(const Consensus::LLMQParams& llmq_params, const CQuorumManager& qman,
                                   const uint256& selectionHash, const CBlockIndex* pindexStart);

CQuorumCPtr SelectQuorumForSigning(const Consensus::LLMQParams& llmq_params, const CChain& active_chain, const CQuorumManager& qman,
                                   const uint256& selectionHash, int signHeight = -1 /*chain tip*/, int signOffset = SIGN_HEIGHT_OFFSET);

std::optional<CFinalCommitment> SelectCommitmentForSigning(const Consensus::LLMQParams& llmq_params, const CQuorumManager& qman,
                                                           const uint256& selectionHash, const CBlockIndex* pindexStart);

VerifyRecSigStatus VerifyRecoveredSig(Consensus::LLMQType llmqType, const CQuorumManager& qman,
                                      const CBlockIndex* pindexStart, const uint256& id, const uint256& msgHash,
                                      const CBLSSignature& sig);

// Verifies a recovered sig that was signed while the chain tip was at signedAtTip
VerifyRecSigStatus VerifyRecoveredSig(Consensus::LLMQType llmqType, const CChain& active_chain, const CQuorumManager& qman,
                                      int signedAtHeight, const uint256& id, const uint256& msgHash, const CBLSSignature& sig,
                                      int signOffset = SIGN_HEIGHT_OFFSET);

/**
 * Base class providing the interface used by CQuorumManager and NetQuorum.
 * Both ObserverContext and ActiveContext inherit from this class.
 */
class QuorumRole
{
protected:
    CQuorumManager& m_qman;

public:
    QuorumRole() = delete;
    QuorumRole(const QuorumRole&) = delete;
    QuorumRole& operator=(const QuorumRole&) = delete;
    explicit QuorumRole(CQuorumManager& qman) : m_qman{qman} {}
    virtual ~QuorumRole() = default;

    virtual bool IsMasternode() const = 0;
    virtual bool IsWatching() const = 0;
    virtual uint256 GetProTxHash() const { return {}; }
    virtual bool SetQuorumSecretKeyShare(CQuorum& quorum, Span<CBLSSecretKey> skContributions) const = 0;
};

} // namespace llmq

#endif // BITCOIN_LLMQ_QUORUMSMAN_H
