// Copyright (c) 2019-2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CHAINLOCK_CHAINLOCK_H
#define BITCOIN_CHAINLOCK_CHAINLOCK_H

#include <chainlock/clsig.h>

#include <crypto/common.h> // For ReadLE64
#include <llmq/signing.h>
#include <net.h> // For NodeId
#include <net_types.h>
#include <primitives/transaction.h>
#include <saltedhasher.h>
#include <sync.h>

#include <gsl/pointers.h>

#include <atomic>
#include <map>
#include <unordered_map>

class CBlock;
class CBlockIndex;
class CChainState;
class CMasternodeSync;
class CScheduler;
class CSporkManager;
class CTxMemPool;

namespace llmq
{
class CInstantSendManager;
class CSigningManager;
class CSigSharesManager;
enum class VerifyRecSigStatus;

class CChainLocksHandler : public CRecoveredSigsListener
{
    static constexpr int64_t CLEANUP_INTERVAL = 1000 * 30;
    static constexpr int64_t CLEANUP_SEEN_TIMEOUT = 24 * 60 * 60 * 1000;

    // how long to wait for islocks until we consider a block with non-islocked TXs to be safe to sign
    static constexpr int64_t WAIT_FOR_ISLOCK_TIMEOUT = 10 * 60;

private:
    CChainState& m_chainstate;
    CQuorumManager& qman;
    CSigningManager& sigman;
    CSigSharesManager& shareman;
    CSporkManager& spork_manager;
    CTxMemPool& mempool;
    const CMasternodeSync& m_mn_sync;

    const bool m_is_masternode;
    std::unique_ptr<CScheduler> scheduler;
    std::unique_ptr<std::thread> scheduler_thread;
    mutable Mutex cs;
    std::atomic<bool> tryLockChainTipScheduled{false};
    std::atomic<bool> isEnabled{false};

    uint256 bestChainLockHash GUARDED_BY(cs);
    CChainLockSig bestChainLock GUARDED_BY(cs);

    CChainLockSig bestChainLockWithKnownBlock GUARDED_BY(cs);
    const CBlockIndex* bestChainLockBlockIndex GUARDED_BY(cs) {nullptr};
    const CBlockIndex* lastNotifyChainLockBlockIndex GUARDED_BY(cs) {nullptr};

    int32_t lastSignedHeight GUARDED_BY(cs) {-1};
    uint256 lastSignedRequestId GUARDED_BY(cs);
    uint256 lastSignedMsgHash GUARDED_BY(cs);

    // We keep track of txids from recently received blocks so that we can check if all TXs got islocked
    struct BlockHasher
    {
        size_t operator()(const uint256& hash) const { return ReadLE64(hash.begin()); }
    };
    using BlockTxs = std::unordered_map<uint256, std::shared_ptr<std::unordered_set<uint256, StaticSaltedHasher>>, BlockHasher>;
    BlockTxs blockTxs GUARDED_BY(cs);
    std::unordered_map<uint256, int64_t, StaticSaltedHasher> txFirstSeenTime GUARDED_BY(cs);

    std::map<uint256, int64_t> seenChainLocks GUARDED_BY(cs);

    std::atomic<int64_t> lastCleanupTime{0};

public:
    explicit CChainLocksHandler(CChainState& chainstate, CQuorumManager& _qman, CSigningManager& _sigman,
                                CSigSharesManager& _shareman, CSporkManager& sporkman, CTxMemPool& _mempool,
                                const CMasternodeSync& mn_sync, bool is_masternode);
    ~CChainLocksHandler();

    void Start(const llmq::CInstantSendManager& isman);
    void Stop();

    bool AlreadyHave(const CInv& inv) const EXCLUSIVE_LOCKS_REQUIRED(!cs);
    bool GetChainLockByHash(const uint256& hash, CChainLockSig& ret) const EXCLUSIVE_LOCKS_REQUIRED(!cs);
    CChainLockSig GetBestChainLock() const EXCLUSIVE_LOCKS_REQUIRED(!cs);

    [[nodiscard]] MessageProcessingResult ProcessNewChainLock(NodeId from, const CChainLockSig& clsig,
                                                              const uint256& hash) EXCLUSIVE_LOCKS_REQUIRED(!cs);

    void AcceptedBlockHeader(gsl::not_null<const CBlockIndex*> pindexNew) EXCLUSIVE_LOCKS_REQUIRED(!cs);
    void UpdatedBlockTip(const llmq::CInstantSendManager& isman);
    void TransactionAddedToMempool(const CTransactionRef& tx, int64_t nAcceptTime) EXCLUSIVE_LOCKS_REQUIRED(!cs);
    void BlockConnected(const std::shared_ptr<const CBlock>& pblock, gsl::not_null<const CBlockIndex*> pindex) EXCLUSIVE_LOCKS_REQUIRED(!cs);
    void BlockDisconnected(const std::shared_ptr<const CBlock>& pblock, gsl::not_null<const CBlockIndex*> pindexDisconnected) EXCLUSIVE_LOCKS_REQUIRED(!cs);
    void CheckActiveState() EXCLUSIVE_LOCKS_REQUIRED(!cs);
    void TrySignChainTip(const llmq::CInstantSendManager& isman) EXCLUSIVE_LOCKS_REQUIRED(!cs);
    void EnforceBestChainLock() EXCLUSIVE_LOCKS_REQUIRED(!cs);
    [[nodiscard]] MessageProcessingResult HandleNewRecoveredSig(const CRecoveredSig& recoveredSig) override
        EXCLUSIVE_LOCKS_REQUIRED(!cs);

    bool HasChainLock(int nHeight, const uint256& blockHash) const EXCLUSIVE_LOCKS_REQUIRED(!cs);
    bool HasConflictingChainLock(int nHeight, const uint256& blockHash) const EXCLUSIVE_LOCKS_REQUIRED(!cs);
    VerifyRecSigStatus VerifyChainLock(const CChainLockSig& clsig) const;

    bool IsTxSafeForMining(const uint256& txid) const EXCLUSIVE_LOCKS_REQUIRED(!cs);

private:
    // these require locks to be held already
    bool InternalHasChainLock(int nHeight, const uint256& blockHash) const EXCLUSIVE_LOCKS_REQUIRED(cs);
    bool InternalHasConflictingChainLock(int nHeight, const uint256& blockHash) const EXCLUSIVE_LOCKS_REQUIRED(cs);

    BlockTxs::mapped_type GetBlockTxs(const uint256& blockHash) EXCLUSIVE_LOCKS_REQUIRED(!cs);

    void Cleanup() EXCLUSIVE_LOCKS_REQUIRED(!cs);
};

bool AreChainLocksEnabled(const CSporkManager& sporkman);
} // namespace llmq

#endif // BITCOIN_CHAINLOCK_CHAINLOCK_H
