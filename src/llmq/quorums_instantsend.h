// Copyright (c) 2019-2021 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LLMQ_QUORUMS_INSTANTSEND_H
#define BITCOIN_LLMQ_QUORUMS_INSTANTSEND_H

#include <llmq/quorums_signing.h>

#include <coins.h>
#include <unordered_lru_cache.h>
#include <primitives/transaction.h>
#include <threadinterrupt.h>
#include <chain.h>

#include <unordered_map>
#include <unordered_set>

namespace llmq
{

class CInstantSendLock
{
public:
    std::vector<COutPoint> inputs;
    uint256 txid;
    CBLSLazySignature sig;

public:
    SERIALIZE_METHODS(CInstantSendLock, obj)
    {
        READWRITE(obj.inputs, obj.txid, obj.sig);
    }

    uint256 GetRequestId() const;
};

typedef std::shared_ptr<CInstantSendLock> CInstantSendLockPtr;

class CInstantSendDb
{
private:
    static const int CURRENT_VERSION = 1;

    int best_confirmed_height{0};

    CDBWrapper& db;

    mutable unordered_lru_cache<uint256, CInstantSendLockPtr, StaticSaltedHasher, 10000> islockCache;
    mutable unordered_lru_cache<uint256, uint256, StaticSaltedHasher, 10000> txidCache;
    mutable unordered_lru_cache<COutPoint, uint256, SaltedOutpointHasher, 10000> outpointCache;

    void WriteInstantSendLockMined(CDBBatch& batch, const uint256& hash, int nHeight);
    void RemoveInstantSendLockMined(CDBBatch& batch, const uint256& hash, int nHeight);

public:
    explicit CInstantSendDb(CDBWrapper& _db);

    void Upgrade();

    void WriteNewInstantSendLock(const uint256& hash, const CInstantSendLock& islock);
    void RemoveInstantSendLock(CDBBatch& batch, const uint256& hash, CInstantSendLockPtr islock, bool keep_cache = true);

    void WriteInstantSendLockMined(const uint256& hash, int nHeight);
    static void WriteInstantSendLockArchived(CDBBatch& batch, const uint256& hash, int nHeight);
    std::unordered_map<uint256, CInstantSendLockPtr> RemoveConfirmedInstantSendLocks(int nUntilHeight);
    void RemoveArchivedInstantSendLocks(int nUntilHeight);
    void WriteBlockInstantSendLocks(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindexConnected);
    void RemoveBlockInstantSendLocks(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindexDisconnected);
    bool KnownInstantSendLock(const uint256& islockHash) const;
    size_t GetInstantSendLockCount() const;

    CInstantSendLockPtr GetInstantSendLockByHash(const uint256& hash, bool use_cache = true) const;
    uint256 GetInstantSendLockHashByTxid(const uint256& txid) const;
    CInstantSendLockPtr GetInstantSendLockByTxid(const uint256& txid) const;
    CInstantSendLockPtr GetInstantSendLockByInput(const COutPoint& outpoint) const;

    std::vector<uint256> GetInstantSendLocksByParent(const uint256& parent) const;
    std::vector<uint256> RemoveChainedInstantSendLocks(const uint256& islockHash, const uint256& txid, int nHeight);
};

class CInstantSendManager : public CRecoveredSigsListener
{
private:
    mutable CCriticalSection cs;
    CInstantSendDb db;

    std::atomic<bool> fUpgradedDB{false};

    std::thread workThread;
    CThreadInterrupt workInterrupt;

    /**
     * Request ids of inputs that we signed. Used to determine if a recovered signature belongs to an
     * in-progress input lock.
     */
    std::unordered_set<uint256, StaticSaltedHasher> inputRequestIds GUARDED_BY(cs);

    /**
     * These are the islocks that are currently in the middle of being created. Entries are created when we observed
     * recovered signatures for all inputs of a TX. At the same time, we initiate signing of our sigshare for the islock.
     * When the recovered sig for the islock later arrives, we can finish the islock and propagate it.
     */
    std::unordered_map<uint256, CInstantSendLock, StaticSaltedHasher> creatingInstantSendLocks GUARDED_BY(cs);
    // maps from txid to the in-progress islock
    std::unordered_map<uint256, CInstantSendLock*, StaticSaltedHasher> txToCreatingInstantSendLocks GUARDED_BY(cs);

    // Incoming and not verified yet
    std::unordered_map<uint256, std::pair<NodeId, CInstantSendLockPtr>, StaticSaltedHasher> pendingInstantSendLocks GUARDED_BY(cs);

    // TXs which are neither IS locked nor ChainLocked. We use this to determine for which TXs we need to retry IS locking
    // of child TXs
    struct NonLockedTxInfo {
        const CBlockIndex* pindexMined{nullptr};
        CTransactionRef tx;
        std::unordered_set<uint256, StaticSaltedHasher> children;
    };
    std::unordered_map<uint256, NonLockedTxInfo, StaticSaltedHasher> nonLockedTxs GUARDED_BY(cs);
    std::unordered_map<COutPoint, uint256, SaltedOutpointHasher> nonLockedTxsByOutpoints GUARDED_BY(cs);

    std::unordered_set<uint256, StaticSaltedHasher> pendingRetryTxs GUARDED_BY(cs);

public:
    explicit CInstantSendManager(CDBWrapper& _llmqDb);
    ~CInstantSendManager();

    void Start();
    void Stop();
    void InterruptWorkerThread();

private:
    void ProcessTx(const CTransaction& tx, bool fRetroactive, const Consensus::Params& params);
    bool CheckCanLock(const CTransaction& tx, bool printDebug, const Consensus::Params& params) const;
    bool CheckCanLock(const COutPoint& outpoint, bool printDebug, const uint256& txHash, const Consensus::Params& params) const;
    bool IsConflicted(const CTransaction& tx) const;

    void HandleNewInputLockRecoveredSig(const CRecoveredSig& recoveredSig, const uint256& txid);
    void HandleNewInstantSendLockRecoveredSig(const CRecoveredSig& recoveredSig);

    bool TrySignInputLocks(const CTransaction& tx, bool allowResigning, Consensus::LLMQType llmqType);
    void TrySignInstantSendLock(const CTransaction& tx);

    void ProcessMessageInstantSendLock(const CNode* pfrom, const CInstantSendLockPtr& islock);
    static bool PreVerifyInstantSendLock(const CInstantSendLock& islock);
    bool ProcessPendingInstantSendLocks();
    std::unordered_set<uint256> ProcessPendingInstantSendLocks(int signOffset, const std::unordered_map<uint256, std::pair<NodeId, CInstantSendLockPtr>, StaticSaltedHasher>& pend, bool ban);
    void ProcessInstantSendLock(NodeId from, const uint256& hash, const CInstantSendLockPtr& islock);

    void AddNonLockedTx(const CTransactionRef& tx, const CBlockIndex* pindexMined);
    void RemoveNonLockedTx(const uint256& txid, bool retryChildren);
    void RemoveConflictedTx(const CTransaction& tx);
    void TruncateRecoveredSigsForInputs(const CInstantSendLock& islock);

    void RemoveMempoolConflictsForLock(const uint256& hash, const CInstantSendLock& islock);
    void ResolveBlockConflicts(const uint256& islockHash, const CInstantSendLock& islock);
    static void AskNodesForLockedTx(const uint256& txid);
    void ProcessPendingRetryLockTxs();

    void WorkThreadMain();

    void HandleFullyConfirmedBlock(const CBlockIndex* pindex);

public:
    bool IsLocked(const uint256& txHash) const;
    CInstantSendLockPtr GetConflictingLock(const CTransaction& tx) const;

    virtual void HandleNewRecoveredSig(const CRecoveredSig& recoveredSig);

    void ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv);

    void TransactionAddedToMempool(const CTransactionRef& tx);
    void TransactionRemovedFromMempool(const CTransactionRef& tx);
    void BlockConnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindex, const std::vector<CTransactionRef>& vtxConflicted);
    void BlockDisconnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindexDisconnected);

    bool AlreadyHave(const CInv& inv) const;
    bool GetInstantSendLockByHash(const uint256& hash, CInstantSendLock& ret) const;
    CInstantSendLockPtr GetInstantSendLockByTxid(const uint256& txid) const;
    bool GetInstantSendLockHashByTxid(const uint256& txid, uint256& ret) const;

    void NotifyChainLock(const CBlockIndex* pindexChainLock);
    void UpdatedBlockTip(const CBlockIndex* pindexNew);

    void RemoveConflictingLock(const uint256& islockHash, const CInstantSendLock& islock);

    size_t GetInstantSendLockCount() const;
};

extern CInstantSendManager* quorumInstantSendManager;

bool IsInstantSendEnabled();
/**
 * If true, MN should sign all transactions, if false, MN should not sign
 * transactions in mempool, but should sign txes included in a block. This
 * allows ChainLocks to continue even while this spork is disabled.
 */
bool IsInstantSendMempoolSigningEnabled();
bool RejectConflictingBlocks();

} // namespace llmq

#endif // BITCOIN_LLMQ_QUORUMS_INSTANTSEND_H
