// Copyright (c) 2019-2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CHAINLOCK_CHAINLOCK_H
#define BITCOIN_CHAINLOCK_CHAINLOCK_H

#include <chainlock/clsig.h>

#include <primitives/transaction.h>
#include <protocol.h>
#include <saltedhasher.h>
#include <sync.h>

#include <chainlock/signing.h>

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

using NodeId = int64_t;

namespace llmq {
class CInstantSendManager;
class CQuorumManager;
class CSigningManager;
class CSigSharesManager;
enum class VerifyRecSigStatus;

class CChainLocksHandler final : public chainlock::ChainLockSignerParent
{
private:
    CChainState& m_chainstate;
    CQuorumManager& qman;
    CSporkManager& spork_manager;
    CTxMemPool& mempool;
    const CMasternodeSync& m_mn_sync;
    std::unique_ptr<CScheduler> scheduler;
    std::unique_ptr<std::thread> scheduler_thread;

    std::unique_ptr<chainlock::ChainLockSigner> m_signer{nullptr};

    mutable Mutex cs;
    std::atomic<bool> tryLockChainTipScheduled{false};
    std::atomic<bool> isEnabled{false};

    uint256 bestChainLockHash GUARDED_BY(cs);
    chainlock::ChainLockSig bestChainLock GUARDED_BY(cs);

    chainlock::ChainLockSig bestChainLockWithKnownBlock GUARDED_BY(cs);
    const CBlockIndex* bestChainLockBlockIndex GUARDED_BY(cs) {nullptr};
    const CBlockIndex* lastNotifyChainLockBlockIndex GUARDED_BY(cs) {nullptr};

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
    bool GetChainLockByHash(const uint256& hash, chainlock::ChainLockSig& ret) const EXCLUSIVE_LOCKS_REQUIRED(!cs);
    chainlock::ChainLockSig GetBestChainLock() const EXCLUSIVE_LOCKS_REQUIRED(!cs);
    void UpdateTxFirstSeenMap(const std::unordered_set<uint256, StaticSaltedHasher>& tx, const int64_t& time) override
        EXCLUSIVE_LOCKS_REQUIRED(!cs);

    [[nodiscard]] MessageProcessingResult ProcessNewChainLock(NodeId from, const chainlock::ChainLockSig& clsig,
                                                              const uint256& hash) override
        EXCLUSIVE_LOCKS_REQUIRED(!cs);

    void AcceptedBlockHeader(gsl::not_null<const CBlockIndex*> pindexNew) EXCLUSIVE_LOCKS_REQUIRED(!cs);
    void UpdatedBlockTip(const llmq::CInstantSendManager& isman);
    void TransactionAddedToMempool(const CTransactionRef& tx, int64_t nAcceptTime) EXCLUSIVE_LOCKS_REQUIRED(!cs);
    void BlockConnected(const std::shared_ptr<const CBlock>& pblock, gsl::not_null<const CBlockIndex*> pindex) EXCLUSIVE_LOCKS_REQUIRED(!cs);
    void BlockDisconnected(const std::shared_ptr<const CBlock>& pblock, gsl::not_null<const CBlockIndex*> pindexDisconnected) EXCLUSIVE_LOCKS_REQUIRED(!cs);
    void CheckActiveState() EXCLUSIVE_LOCKS_REQUIRED(!cs);
    void EnforceBestChainLock() EXCLUSIVE_LOCKS_REQUIRED(!cs);

    bool HasChainLock(int nHeight, const uint256& blockHash) const override
        EXCLUSIVE_LOCKS_REQUIRED(!cs);
    bool HasConflictingChainLock(int nHeight, const uint256& blockHash) const override
        EXCLUSIVE_LOCKS_REQUIRED(!cs);
    VerifyRecSigStatus VerifyChainLock(const chainlock::ChainLockSig& clsig) const;

    [[nodiscard]] int32_t GetBestChainLockHeight() const override
        EXCLUSIVE_LOCKS_REQUIRED(!cs);
    bool IsTxSafeForMining(const uint256& txid) const override
        EXCLUSIVE_LOCKS_REQUIRED(!cs);
    [[nodiscard]] bool IsEnabled() const override { return isEnabled; }

private:
    void Cleanup() EXCLUSIVE_LOCKS_REQUIRED(!cs);
};

bool AreChainLocksEnabled(const CSporkManager& sporkman);
} // namespace llmq

#endif // BITCOIN_CHAINLOCK_CHAINLOCK_H
