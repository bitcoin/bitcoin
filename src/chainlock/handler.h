// Copyright (c) 2019-2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CHAINLOCK_HANDLER_H
#define BITCOIN_CHAINLOCK_HANDLER_H

#include <net_types.h>
#include <primitives/transaction.h>
#include <protocol.h>
#include <saltedhasher.h>
#include <sync.h>
#include <util/time.h>

#include <gsl/pointers.h>

#include <atomic>
#include <cassert>
#include <chrono>
#include <map>
#include <memory>
#include <thread>

class CBlock;
class CBlockIndex;
class CChain;
class CMasternodeSync;
class ChainstateManager;
class CScheduler;
class CTxMemPool;
struct MessageProcessingResult;

namespace chainlock {
class Chainlocks;
struct ChainLockSig;
} // namespace chainlock
namespace Consensus {
struct Params;
} // namespace Consensus

namespace llmq
{
class CQuorumManager;
enum class VerifyRecSigStatus : uint8_t;

class CChainLocksHandler
{
private:
    chainlock::Chainlocks& m_chainlocks;

    ChainstateManager& m_chainman;
    CTxMemPool& mempool;
    const CMasternodeSync& m_mn_sync;
    std::unique_ptr<CScheduler> scheduler;
    std::unique_ptr<std::thread> scheduler_thread;

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
    explicit CChainLocksHandler(chainlock::Chainlocks& chainlocks, ChainstateManager& chainman,
                                CTxMemPool& _mempool, const CMasternodeSync& mn_sync);
    ~CChainLocksHandler();

    void Start();
    void Stop();

    bool AlreadyHave(const CInv& inv) const
        EXCLUSIVE_LOCKS_REQUIRED(!cs);
    void UpdateTxFirstSeenMap(const Uint256HashSet& tx, const int64_t& time) EXCLUSIVE_LOCKS_REQUIRED(!cs);

    [[nodiscard]] MessageProcessingResult ProcessNewChainLock(NodeId from, const chainlock::ChainLockSig& clsig, const llmq::CQuorumManager& qman,

                                                              const uint256& hash)
        EXCLUSIVE_LOCKS_REQUIRED(!cs);

    void AcceptedBlockHeader(gsl::not_null<const CBlockIndex*> pindexNew);
    void UpdatedBlockTip();
    void TransactionAddedToMempool(const CTransactionRef& tx, int64_t nAcceptTime)
        EXCLUSIVE_LOCKS_REQUIRED(!cs);
    void BlockConnected(const std::shared_ptr<const CBlock>& pblock, gsl::not_null<const CBlockIndex*> pindex)
        EXCLUSIVE_LOCKS_REQUIRED(!cs);
    void CheckActiveState()
        EXCLUSIVE_LOCKS_REQUIRED(!cs);
    void EnforceBestChainLock()
        EXCLUSIVE_LOCKS_REQUIRED(!cs);

    bool IsTxSafeForMining(const uint256& txid) const
        EXCLUSIVE_LOCKS_REQUIRED(!cs);

    void CleanupFromSigner(const std::vector<std::shared_ptr<Uint256HashSet>>& cleanup_txes)
        EXCLUSIVE_LOCKS_REQUIRED(!cs);
private:
    void Cleanup()
        EXCLUSIVE_LOCKS_REQUIRED(!cs);
};
} // namespace llmq

namespace chainlock
{
llmq::VerifyRecSigStatus VerifyChainLock(const Consensus::Params& params, const CChain& chain, const llmq::CQuorumManager& qman, const chainlock::ChainLockSig& clsig);
} // namespace chainlock

#endif // BITCOIN_CHAINLOCK_HANDLER_H
