// Copyright (c) 2019-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INSTANTSEND_INSTANTSEND_H
#define BITCOIN_INSTANTSEND_INSTANTSEND_H

#include <instantsend/db.h>
#include <instantsend/lock.h>
#include <instantsend/signing.h>

#include <net_types.h>
#include <primitives/transaction.h>
#include <protocol.h>
#include <sync.h>
#include <threadsafety.h>
#include <unordered_lru_cache.h>

#include <atomic>
#include <optional>
#include <unordered_set>
#include <variant>
#include <vector>

#include <saltedhasher.h>

class CBlockIndex;
class CChainState;
class CDataStream;
class CMasternodeSync;
class CSporkManager;
class CTxMemPool;
namespace Consensus {
struct LLMQParams;
} // namespace Consensus
namespace util {
struct DbWrapperParams;
} // namespace util

namespace chainlock {
class Chainlocks;
}

namespace instantsend {
class InstantSendSigner;

struct PendingISLockFromPeer {
    NodeId node_id;
    InstantSendLockPtr islock;
};

struct PendingISLockEntry : PendingISLockFromPeer {
    uint256 islock_hash;
};

struct PendingState {
    bool m_pending_work{false};
    std::vector<PendingISLockEntry> m_pending_is;
};
} // namespace instantsend

namespace llmq {
class CSigningManager;

class CInstantSendManager final : public instantsend::InstantSendSignerParent
{
private:
    instantsend::CInstantSendDb db;

    const chainlock::Chainlocks& m_chainlocks;
    CChainState& m_chainstate;
    CSigningManager& sigman;
    CSporkManager& spork_manager;
    CTxMemPool& mempool;
    const CMasternodeSync& m_mn_sync;

    std::atomic<instantsend::InstantSendSigner*> m_signer{nullptr};

    mutable Mutex cs_pendingLocks;
    // Incoming and not verified yet
    Uint256HashMap<instantsend::PendingISLockFromPeer> pendingInstantSendLocks GUARDED_BY(cs_pendingLocks);
    // Tried to verify but there is no tx yet
    Uint256HashMap<instantsend::PendingISLockFromPeer> pendingNoTxInstantSendLocks GUARDED_BY(cs_pendingLocks);

    // TXs which are neither IS locked nor ChainLocked. We use this to determine for which TXs we need to retry IS
    // locking of child TXs
    struct NonLockedTxInfo {
        const CBlockIndex* pindexMined;
        CTransactionRef tx;
        Uint256HashSet children;
    };

    mutable Mutex cs_nonLocked;
    Uint256HashMap<NonLockedTxInfo> nonLockedTxs GUARDED_BY(cs_nonLocked);
    std::unordered_map<COutPoint, uint256, SaltedOutpointHasher> nonLockedTxsByOutpoints GUARDED_BY(cs_nonLocked);

    mutable Mutex cs_pendingRetry;
    Uint256HashSet pendingRetryTxs GUARDED_BY(cs_pendingRetry);

    mutable Mutex cs_timingsTxSeen;
    Uint256HashMap<int64_t> timingsTxSeen GUARDED_BY(cs_timingsTxSeen);

    mutable Mutex cs_height_cache;
    static constexpr size_t MAX_BLOCK_HEIGHT_CACHE{16384};
    mutable unordered_lru_cache<uint256, int, StaticSaltedHasher, MAX_BLOCK_HEIGHT_CACHE> m_cached_block_heights
        GUARDED_BY(cs_height_cache);
    mutable int m_cached_tip_height GUARDED_BY(cs_height_cache){-1};

    void CacheBlockHeightInternal(const CBlockIndex* const block_index) const EXCLUSIVE_LOCKS_REQUIRED(cs_height_cache);

public:
    CInstantSendManager() = delete;
    CInstantSendManager(const CInstantSendManager&) = delete;
    CInstantSendManager& operator=(const CInstantSendManager&) = delete;
    explicit CInstantSendManager(const chainlock::Chainlocks& chainlocks, CChainState& chainstate,
                                 CSigningManager& _sigman, CSporkManager& sporkman, CTxMemPool& _mempool,
                                 const CMasternodeSync& mn_sync, const util::DbWrapperParams& db_params);
    ~CInstantSendManager();

    void ConnectSigner(gsl::not_null<instantsend::InstantSendSigner*> signer)
    {
        // Prohibit double initialization
        assert(m_signer.load(std::memory_order_acquire) == nullptr);
        m_signer.store(signer, std::memory_order_release);
    }
    void DisconnectSigner() { m_signer.store(nullptr, std::memory_order_release); }

    instantsend::InstantSendSigner* Signer() const { return m_signer.load(std::memory_order_acquire); }

private:
    void AddNonLockedTx(const CTransactionRef& tx, const CBlockIndex* pindexMined)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_nonLocked, !cs_pendingLocks, !cs_timingsTxSeen);
    void RemoveNonLockedTx(const uint256& txid, bool retryChildren)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_nonLocked, !cs_pendingRetry);
    void RemoveConflictedTx(const CTransaction& tx)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_nonLocked, !cs_pendingRetry);
    void TruncateRecoveredSigsForInputs(const instantsend::InstantSendLock& islock);

    void RemoveMempoolConflictsForLock(const uint256& hash, const instantsend::InstantSendLock& islock)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_nonLocked, !cs_pendingRetry);
    void ResolveBlockConflicts(const uint256& islockHash, const instantsend::InstantSendLock& islock)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_nonLocked, !cs_pendingLocks, !cs_pendingRetry, !cs_height_cache);

    void HandleFullyConfirmedBlock(const CBlockIndex* pindex)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_nonLocked, !cs_pendingRetry);

public:
    bool IsLocked(const uint256& txHash) const override;
    bool IsWaitingForTx(const uint256& txHash) const EXCLUSIVE_LOCKS_REQUIRED(!cs_pendingLocks);
    instantsend::InstantSendLockPtr GetConflictingLock(const CTransaction& tx) const override;

    /* Helpers for communications between CInstantSendManager & NetInstantSend */
    // This helper returns up to 32 pending locks and remove them from queue of pending
    [[nodiscard]] instantsend::PendingState FetchPendingLocks() EXCLUSIVE_LOCKS_REQUIRED(!cs_pendingLocks);
    void EnqueueInstantSendLock(NodeId from, const uint256& hash, std::shared_ptr<instantsend::InstantSendLock> islock)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_pendingLocks);
    [[nodiscard]] std::vector<CTransactionRef> PrepareTxToRetry()
        EXCLUSIVE_LOCKS_REQUIRED(!cs_nonLocked, !cs_pendingRetry);
    CSigningManager& Sigman() { return sigman; }
    [[nodiscard]] std::variant<uint256, CTransactionRef, std::monostate> ProcessInstantSendLock(
        NodeId from, const uint256& hash, const instantsend::InstantSendLockPtr& islock)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_nonLocked, !cs_pendingLocks, !cs_pendingRetry, !cs_height_cache);

    void TransactionAddedToMempool(const CTransactionRef& tx)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_nonLocked, !cs_pendingLocks, !cs_pendingRetry, !cs_timingsTxSeen);
    void TransactionRemovedFromMempool(const CTransactionRef& tx) EXCLUSIVE_LOCKS_REQUIRED(!cs_height_cache);
    void BlockConnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindex)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_nonLocked, !cs_pendingLocks, !cs_pendingRetry, !cs_timingsTxSeen, !cs_height_cache);
    void BlockDisconnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindexDisconnected)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_height_cache);

    bool AlreadyHave(const CInv& inv) const EXCLUSIVE_LOCKS_REQUIRED(!cs_pendingLocks);
    bool GetInstantSendLockByHash(const uint256& hash, instantsend::InstantSendLock& ret) const
        EXCLUSIVE_LOCKS_REQUIRED(!cs_pendingLocks);
    instantsend::InstantSendLockPtr GetInstantSendLockByTxid(const uint256& txid) const;

    void NotifyChainLock(const CBlockIndex* pindexChainLock)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_nonLocked, !cs_pendingRetry);
    void UpdatedBlockTip(const CBlockIndex* pindexNew)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_nonLocked, !cs_pendingRetry, !cs_height_cache);

    void RemoveConflictingLock(const uint256& islockHash, const instantsend::InstantSendLock& islock)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_height_cache);
    void TryEmplacePendingLock(const uint256& hash, const NodeId id, const instantsend::InstantSendLockPtr& islock) override
        EXCLUSIVE_LOCKS_REQUIRED(!cs_pendingLocks);

    size_t GetInstantSendLockCount() const;

    void CacheBlockHeight(const CBlockIndex* const block_index) const EXCLUSIVE_LOCKS_REQUIRED(!cs_height_cache);
    std::optional<int> GetBlockHeight(const uint256& hash) const override EXCLUSIVE_LOCKS_REQUIRED(!cs_height_cache);
    void CacheTipHeight(const CBlockIndex* const tip) const EXCLUSIVE_LOCKS_REQUIRED(!cs_height_cache);
    int GetTipHeight() const override EXCLUSIVE_LOCKS_REQUIRED(!cs_height_cache);

    bool IsInstantSendEnabled() const override;
    /**
     * If true, MN should sign all transactions, if false, MN should not sign
     * transactions in mempool, but should sign txes included in a block. This
     * allows ChainLocks to continue even while this spork is disabled.
     */
    bool RejectConflictingBlocks() const;
};
} // namespace llmq

#endif // BITCOIN_INSTANTSEND_INSTANTSEND_H
