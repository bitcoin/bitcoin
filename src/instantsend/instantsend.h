// Copyright (c) 2019-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INSTANTSEND_INSTANTSEND_H
#define BITCOIN_INSTANTSEND_INSTANTSEND_H

#include <net_types.h>
#include <primitives/transaction.h>
#include <protocol.h>
#include <sync.h>
#include <threadsafety.h>
#include <util/threadinterrupt.h>

#include <instantsend/db.h>
#include <instantsend/lock.h>
#include <instantsend/signing.h>
#include <unordered_lru_cache.h>

#include <atomic>
#include <unordered_map>
#include <unordered_set>

class CBlockIndex;
class CChainState;
class CDataStream;
class CMasternodeSync;
class CSporkManager;
class CTxMemPool;
class PeerManager;
namespace Consensus {
struct LLMQParams;
} // namespace Consensus

namespace instantsend {
class InstantSendSigner;

struct PendingState {
    bool m_pending_work{false};
    std::vector<std::pair<NodeId, MessageProcessingResult>> m_peer_activity{};
};
} // namespace instantsend

namespace llmq {
class CChainLocksHandler;
class CQuorumManager;
class CSigningManager;

class CInstantSendManager final : public instantsend::InstantSendSignerParent
{
private:
    instantsend::CInstantSendDb db;

    CChainLocksHandler& clhandler;
    CChainState& m_chainstate;
    CQuorumManager& qman;
    CSigningManager& sigman;
    CSporkManager& spork_manager;
    CTxMemPool& mempool;
    const CMasternodeSync& m_mn_sync;

    std::atomic<instantsend::InstantSendSigner*> m_signer{nullptr};

    std::thread workThread;
    CThreadInterrupt workInterrupt;

    mutable Mutex cs_pendingLocks;
    // Incoming and not verified yet
    std::unordered_map<uint256, std::pair<NodeId, instantsend::InstantSendLockPtr>, StaticSaltedHasher> pendingInstantSendLocks GUARDED_BY(cs_pendingLocks);
    // Tried to verify but there is no tx yet
    std::unordered_map<uint256, std::pair<NodeId, instantsend::InstantSendLockPtr>, StaticSaltedHasher> pendingNoTxInstantSendLocks GUARDED_BY(cs_pendingLocks);

    // TXs which are neither IS locked nor ChainLocked. We use this to determine for which TXs we need to retry IS
    // locking of child TXs
    struct NonLockedTxInfo {
        const CBlockIndex* pindexMined;
        CTransactionRef tx;
        std::unordered_set<uint256, StaticSaltedHasher> children;
    };

    mutable Mutex cs_nonLocked;
    std::unordered_map<uint256, NonLockedTxInfo, StaticSaltedHasher> nonLockedTxs GUARDED_BY(cs_nonLocked);
    std::unordered_map<COutPoint, uint256, SaltedOutpointHasher> nonLockedTxsByOutpoints GUARDED_BY(cs_nonLocked);

    mutable Mutex cs_pendingRetry;
    std::unordered_set<uint256, StaticSaltedHasher> pendingRetryTxs GUARDED_BY(cs_pendingRetry);

    mutable Mutex cs_timingsTxSeen;
    std::unordered_map<uint256, int64_t, StaticSaltedHasher> timingsTxSeen GUARDED_BY(cs_timingsTxSeen);

public:
    explicit CInstantSendManager(CChainLocksHandler& _clhandler, CChainState& chainstate, CQuorumManager& _qman,
                                 CSigningManager& _sigman, CSporkManager& sporkman, CTxMemPool& _mempool,
                                 const CMasternodeSync& mn_sync, bool unitTests, bool fWipe);
    ~CInstantSendManager();

    void ConnectSigner(gsl::not_null<instantsend::InstantSendSigner*> signer)
    {
        // Prohibit double initialization
        assert(m_signer.load(std::memory_order_acquire) == nullptr);
        m_signer.store(signer, std::memory_order_release);
    }
    void DisconnectSigner() { m_signer.store(nullptr, std::memory_order_release); }

    void Start(PeerManager& peerman);
    void Stop();
    void InterruptWorkerThread() { workInterrupt(); };

private:
    instantsend::PendingState ProcessPendingInstantSendLocks()
        EXCLUSIVE_LOCKS_REQUIRED(!cs_nonLocked, !cs_pendingLocks, !cs_pendingRetry);

    std::unordered_set<uint256, StaticSaltedHasher> ProcessPendingInstantSendLocks(
        const Consensus::LLMQParams& llmq_params, int signOffset, bool ban,
        const std::unordered_map<uint256, std::pair<NodeId, instantsend::InstantSendLockPtr>, StaticSaltedHasher>& pend,
        std::vector<std::pair<NodeId, MessageProcessingResult>>& peer_activity)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_nonLocked, !cs_pendingLocks, !cs_pendingRetry);
    MessageProcessingResult ProcessInstantSendLock(NodeId from, const uint256& hash,
                                                   const instantsend::InstantSendLockPtr& islock)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_nonLocked, !cs_pendingLocks, !cs_pendingRetry);

    void AddNonLockedTx(const CTransactionRef& tx, const CBlockIndex* pindexMined)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_nonLocked, !cs_pendingLocks);
    void RemoveNonLockedTx(const uint256& txid, bool retryChildren)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_nonLocked, !cs_pendingRetry);
    void RemoveConflictedTx(const CTransaction& tx)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_nonLocked, !cs_pendingRetry);
    void TruncateRecoveredSigsForInputs(const instantsend::InstantSendLock& islock);

    void RemoveMempoolConflictsForLock(const uint256& hash, const instantsend::InstantSendLock& islock)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_nonLocked, !cs_pendingRetry);
    void ResolveBlockConflicts(const uint256& islockHash, const instantsend::InstantSendLock& islock)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_nonLocked, !cs_pendingLocks, !cs_pendingRetry);

    void WorkThreadMain(PeerManager& peerman)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_nonLocked, !cs_pendingLocks, !cs_pendingRetry);

    void HandleFullyConfirmedBlock(const CBlockIndex* pindex)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_nonLocked, !cs_pendingRetry);

public:
    bool IsLocked(const uint256& txHash) const override;
    bool IsWaitingForTx(const uint256& txHash) const EXCLUSIVE_LOCKS_REQUIRED(!cs_pendingLocks);
    instantsend::InstantSendLockPtr GetConflictingLock(const CTransaction& tx) const override;

    [[nodiscard]] MessageProcessingResult ProcessMessage(NodeId from, std::string_view msg_type, CDataStream& vRecv);

    void TransactionAddedToMempool(const CTransactionRef& tx)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_nonLocked, !cs_pendingLocks, !cs_pendingRetry);
    void TransactionRemovedFromMempool(const CTransactionRef& tx);
    void BlockConnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindex)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_nonLocked, !cs_pendingLocks, !cs_pendingRetry);
    void BlockDisconnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindexDisconnected);

    bool AlreadyHave(const CInv& inv) const EXCLUSIVE_LOCKS_REQUIRED(!cs_pendingLocks);
    bool GetInstantSendLockByHash(const uint256& hash, instantsend::InstantSendLock& ret) const
        EXCLUSIVE_LOCKS_REQUIRED(!cs_pendingLocks);
    instantsend::InstantSendLockPtr GetInstantSendLockByTxid(const uint256& txid) const;

    void NotifyChainLock(const CBlockIndex* pindexChainLock)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_nonLocked, !cs_pendingRetry);
    void UpdatedBlockTip(const CBlockIndex* pindexNew)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_nonLocked, !cs_pendingRetry);

    void RemoveConflictingLock(const uint256& islockHash, const instantsend::InstantSendLock& islock);
    void TryEmplacePendingLock(const uint256& hash, const NodeId id, const instantsend::InstantSendLockPtr& islock) override
        EXCLUSIVE_LOCKS_REQUIRED(!cs_pendingLocks);

    size_t GetInstantSendLockCount() const;

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
