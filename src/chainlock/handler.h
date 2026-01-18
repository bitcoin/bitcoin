// Copyright (c) 2019-2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CHAINLOCK_HANDLER_H
#define BITCOIN_CHAINLOCK_HANDLER_H

#include <chainlock/chainlock.h>
#include <chainlock/clsig.h>

#include <net_types.h>
#include <primitives/transaction.h>
#include <protocol.h>
#include <saltedhasher.h>
#include <sync.h>
#include <util/time.h>

#include <chainlock/signing.h>

#include <gsl/pointers.h>

#include <atomic>
#include <cassert>
#include <chrono>
#include <map>
#include <memory>
#include <thread>
#include <unordered_map>

class CBlock;
class CBlockIndex;
class CChainState;
class CMasternodeSync;
class CScheduler;
class CTxMemPool;

namespace llmq {
class CInstantSendManager;
class CQuorumManager;
class CSigningManager;
enum class VerifyRecSigStatus : uint8_t;

class CChainLocksHandler final : public chainlock::ChainLockSignerParent
{
private:
    chainlock::Chainlocks& m_chainlocks;

    CChainState& m_chainstate;
    CQuorumManager& qman;
    CTxMemPool& mempool;
    const CMasternodeSync& m_mn_sync;
    std::unique_ptr<CScheduler> scheduler;
    std::unique_ptr<std::thread> scheduler_thread;

    std::atomic<chainlock::ChainLockSigner*> m_signer{nullptr};

    mutable Mutex cs;
    std::atomic<bool> tryLockChainTipScheduled{false};
    std::atomic<bool> isEnabled{false};

    const CBlockIndex* lastNotifyChainLockBlockIndex GUARDED_BY(cs){nullptr};
    Uint256HashMap<std::chrono::seconds> txFirstSeenTime GUARDED_BY(cs);

    std::map<uint256, std::chrono::seconds> seenChainLocks GUARDED_BY(cs);

    CleanupThrottler<NodeClock> cleanupThrottler;

public:
    CChainLocksHandler() = delete;
    CChainLocksHandler(const CChainLocksHandler&) = delete;
    CChainLocksHandler& operator=(const CChainLocksHandler&) = delete;
    explicit CChainLocksHandler(chainlock::Chainlocks& chainlocks, CChainState& chainstate, CQuorumManager& _qman,
                                CTxMemPool& _mempool, const CMasternodeSync& mn_sync);
    ~CChainLocksHandler();

    void ConnectSigner(gsl::not_null<chainlock::ChainLockSigner*> signer)
    {
        // Prohibit double initialization
        assert(m_signer.load(std::memory_order_acquire) == nullptr);
        m_signer.store(signer, std::memory_order_release);
    }
    void DisconnectSigner() { m_signer.store(nullptr, std::memory_order_release); }

    void Start(const llmq::CInstantSendManager& isman);
    void Stop();

    bool AlreadyHave(const CInv& inv) const
        EXCLUSIVE_LOCKS_REQUIRED(!cs);
    void UpdateTxFirstSeenMap(const Uint256HashSet& tx, const int64_t& time) override EXCLUSIVE_LOCKS_REQUIRED(!cs);

    [[nodiscard]] MessageProcessingResult ProcessNewChainLock(NodeId from, const chainlock::ChainLockSig& clsig,
                                                              const uint256& hash) override
        EXCLUSIVE_LOCKS_REQUIRED(!cs);

    void UpdatedBlockTip(const llmq::CInstantSendManager& isman);
    void TransactionAddedToMempool(const CTransactionRef& tx, int64_t nAcceptTime)
        EXCLUSIVE_LOCKS_REQUIRED(!cs);
    void BlockConnected(const std::shared_ptr<const CBlock>& pblock, gsl::not_null<const CBlockIndex*> pindex)
        EXCLUSIVE_LOCKS_REQUIRED(!cs);
    void BlockDisconnected(const std::shared_ptr<const CBlock>& pblock, gsl::not_null<const CBlockIndex*> pindexDisconnected)
        EXCLUSIVE_LOCKS_REQUIRED(!cs);
    void CheckActiveState()
        EXCLUSIVE_LOCKS_REQUIRED(!cs);
    void EnforceBestChainLock()
        EXCLUSIVE_LOCKS_REQUIRED(!cs);

    VerifyRecSigStatus VerifyChainLock(const chainlock::ChainLockSig& clsig) const;

    bool IsTxSafeForMining(const uint256& txid) const override
        EXCLUSIVE_LOCKS_REQUIRED(!cs);

private:
    void Cleanup()
        EXCLUSIVE_LOCKS_REQUIRED(!cs);
};
} // namespace llmq

#endif // BITCOIN_CHAINLOCK_HANDLER_H
