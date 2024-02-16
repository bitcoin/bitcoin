// Copyright (c) 2019-2023 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LLMQ_CHAINLOCKS_H
#define BITCOIN_LLMQ_CHAINLOCKS_H

#include <llmq/clsig.h>

#include <crypto/common.h>
#include <llmq/signing.h>
#include <net.h>
#include <net_types.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <saltedhasher.h>
#include <sync.h>

#include <gsl/pointers.h>

#include <atomic>
#include <map>
#include <unordered_map>
#include <unordered_set>

class CChainState;
class CConnman;
class CBlockIndex;
class CMasternodeSync;
class CScheduler;
class CSporkManager;
class CTxMemPool;

namespace llmq
{
class CSigningManager;
class CSigSharesManager;

class CChainLocksHandler : public CRecoveredSigsListener
{
    static constexpr int64_t CLEANUP_INTERVAL = 1000 * 30;
    static constexpr int64_t CLEANUP_SEEN_TIMEOUT = 24 * 60 * 60 * 1000;

    // how long to wait for islocks until we consider a block with non-islocked TXs to be safe to sign
    static constexpr int64_t WAIT_FOR_ISLOCK_TIMEOUT = 10 * 60;

private:
    CChainState& m_chainstate;
    CConnman& connman;
    CMasternodeSync& m_mn_sync;
    CQuorumManager& qman;
    CSigningManager& sigman;
    CSigSharesManager& shareman;
    CSporkManager& spork_manager;
    CTxMemPool& mempool;

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
    explicit CChainLocksHandler(CChainState& chainstate, CConnman& _connman, CMasternodeSync& mn_sync, CQuorumManager& _qman,
                                CSigningManager& _sigman, CSigSharesManager& _shareman, CSporkManager& sporkManager,
                                CTxMemPool& _mempool);
    ~CChainLocksHandler();

    void Start();
    void Stop();

    bool AlreadyHave(const CInv& inv) const LOCKS_EXCLUDED(cs);
    bool GetChainLockByHash(const uint256& hash, CChainLockSig& ret) const LOCKS_EXCLUDED(cs);
    CChainLockSig GetBestChainLock() const LOCKS_EXCLUDED(cs);

    PeerMsgRet ProcessMessage(const CNode& pfrom, const std::string& msg_type, CDataStream& vRecv);
    PeerMsgRet ProcessNewChainLock(NodeId from, const CChainLockSig& clsig, const uint256& hash) LOCKS_EXCLUDED(cs);

    void AcceptedBlockHeader(gsl::not_null<const CBlockIndex*> pindexNew) LOCKS_EXCLUDED(cs);
    void UpdatedBlockTip();
    void TransactionAddedToMempool(const CTransactionRef& tx, int64_t nAcceptTime) LOCKS_EXCLUDED(cs);
    void BlockConnected(const std::shared_ptr<const CBlock>& pblock, gsl::not_null<const CBlockIndex*> pindex) LOCKS_EXCLUDED(cs);
    void BlockDisconnected(const std::shared_ptr<const CBlock>& pblock, gsl::not_null<const CBlockIndex*> pindexDisconnected) LOCKS_EXCLUDED(cs);
    void CheckActiveState() LOCKS_EXCLUDED(cs);
    void TrySignChainTip() LOCKS_EXCLUDED(cs);
    void EnforceBestChainLock() LOCKS_EXCLUDED(cs);
    void HandleNewRecoveredSig(const CRecoveredSig& recoveredSig) override LOCKS_EXCLUDED(cs);

    bool HasChainLock(int nHeight, const uint256& blockHash) const LOCKS_EXCLUDED(cs);
    bool HasConflictingChainLock(int nHeight, const uint256& blockHash) const LOCKS_EXCLUDED(cs);
    bool VerifyChainLock(const CChainLockSig& clsig) const;

    bool IsTxSafeForMining(const uint256& txid) const LOCKS_EXCLUDED(cs);

private:
    // these require locks to be held already
    bool InternalHasChainLock(int nHeight, const uint256& blockHash) const EXCLUSIVE_LOCKS_REQUIRED(cs);
    bool InternalHasConflictingChainLock(int nHeight, const uint256& blockHash) const EXCLUSIVE_LOCKS_REQUIRED(cs);

    BlockTxs::mapped_type GetBlockTxs(const uint256& blockHash) LOCKS_EXCLUDED(cs);

    void Cleanup() LOCKS_EXCLUDED(cs);
};

extern std::unique_ptr<CChainLocksHandler> chainLocksHandler;

bool AreChainLocksEnabled(const CSporkManager& sporkManager);
bool ChainLocksSigningEnabled(const CSporkManager& sporkManager);

} // namespace llmq

#endif // BITCOIN_LLMQ_CHAINLOCKS_H
