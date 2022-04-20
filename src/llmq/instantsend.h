// Copyright (c) 2019-2021 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LLMQ_INSTANTSEND_H
#define BITCOIN_LLMQ_INSTANTSEND_H

#include <llmq/signing.h>
#include <unordered_lru_cache.h>

#include <chain.h>
#include <coins.h>
#include <dbwrapper.h>
#include <primitives/transaction.h>
#include <threadinterrupt.h>

#include <unordered_map>
#include <unordered_set>

namespace llmq
{

struct CInstantSendLock
{
    // This is the old format of instant send lock, it must be 0
    static constexpr uint8_t islock_version{0};
    // This is the new format of instant send deterministic lock, this should be incremented for new isdlock versions
    static constexpr uint8_t isdlock_version{1};

    uint8_t nVersion;
    std::vector<COutPoint> inputs;
    uint256 txid;
    uint256 cycleHash;
    CBLSLazySignature sig;

    CInstantSendLock() : CInstantSendLock(islock_version) {}
    explicit CInstantSendLock(const uint8_t desiredVersion) : nVersion(desiredVersion) {}

    SERIALIZE_METHODS(CInstantSendLock, obj)
    {
        if (s.GetVersion() >= ISDLOCK_PROTO_VERSION && obj.IsDeterministic()) {
            READWRITE(obj.nVersion);
        }
        READWRITE(obj.inputs);
        READWRITE(obj.txid);
        if (s.GetVersion() >= ISDLOCK_PROTO_VERSION && obj.IsDeterministic()) {
            READWRITE(obj.cycleHash);
        }
        READWRITE(obj.sig);
    }

    uint256 GetRequestId() const;
    bool IsDeterministic() const { return nVersion != islock_version; }
};

using CInstantSendLockPtr = std::shared_ptr<CInstantSendLock>;

class CInstantSendDb
{
private:
    mutable CCriticalSection cs_db;

    static constexpr int CURRENT_VERSION{1};

    int best_confirmed_height GUARDED_BY(cs_db) {0};

    std::unique_ptr<CDBWrapper> db GUARDED_BY(cs_db) {nullptr};
    mutable unordered_lru_cache<uint256, CInstantSendLockPtr, StaticSaltedHasher, 10000> islockCache GUARDED_BY(cs_db);
    mutable unordered_lru_cache<uint256, uint256, StaticSaltedHasher, 10000> txidCache GUARDED_BY(cs_db);

    mutable unordered_lru_cache<COutPoint, uint256, SaltedOutpointHasher, 10000> outpointCache GUARDED_BY(cs_db);
    void WriteInstantSendLockMined(CDBBatch& batch, const uint256& hash, int nHeight) EXCLUSIVE_LOCKS_REQUIRED(cs_db);

    void RemoveInstantSendLockMined(CDBBatch& batch, const uint256& hash, int nHeight) EXCLUSIVE_LOCKS_REQUIRED(cs_db);

    /**
     * This method removes a InstantSend Lock from the database and is called when a tx with an IS lock is confirmed and Chainlocked
     * @param batch Object used to batch many calls together
     * @param hash The hash of the InstantSend Lock
     * @param islock The InstantSend Lock object itself
     * @param keep_cache Should we still keep corresponding entries in the cache or not
     */
    void RemoveInstantSendLock(CDBBatch& batch, const uint256& hash, CInstantSendLockPtr islock, bool keep_cache = true) EXCLUSIVE_LOCKS_REQUIRED(cs_db);
    /**
     * Marks an InstantSend Lock as archived.
     * @param batch Object used to batch many calls together
     * @param hash The hash of the InstantSend Lock
     * @param nHeight The height that the transaction was included at
     */
    void WriteInstantSendLockArchived(CDBBatch& batch, const uint256& hash, int nHeight) EXCLUSIVE_LOCKS_REQUIRED(cs_db);
    /**
     * Gets a vector of IS Lock hashes of the IS Locks which rely on or are children of the parent IS Lock
     * @param parent The hash of the parent IS Lock
     * @return Returns a vector of IS Lock hashes
     */
    std::vector<uint256> GetInstantSendLocksByParent(const uint256& parent) const EXCLUSIVE_LOCKS_REQUIRED(cs_db);

public:
    explicit CInstantSendDb(bool unitTests, bool fWipe) :
            db(std::make_unique<CDBWrapper>(unitTests ? "" : (GetDataDir() / "llmq/isdb"), 32 << 20, unitTests, fWipe))
    {}

    void Upgrade();

    /**
     * This method is called when an InstantSend Lock is processed and adds the lock to the database
     * @param hash The hash of the InstantSend Lock
     * @param islock The InstantSend Lock object itself
     */
    void WriteNewInstantSendLock(const uint256& hash, const CInstantSendLock& islock);
    /**
     * This method updates a DB entry for an InstantSend Lock from being not included in a block to being included in a block
     * @param hash The hash of the InstantSend Lock
     * @param nHeight The height that the transaction was included at
     */
    void WriteInstantSendLockMined(const uint256& hash, int nHeight);
    /**
     * Archives and deletes all IS Locks which were mined into a block before nUntilHeight
     * @param nUntilHeight Removes all IS Locks confirmed up until nUntilHeight
     * @return returns an unordered_map of the hash of the IS Locks and a pointer object to the IS Locks for all IS Locks which were removed
     */
    std::unordered_map<uint256, CInstantSendLockPtr, StaticSaltedHasher> RemoveConfirmedInstantSendLocks(int nUntilHeight);
    /**
     * Removes IS Locks from the archive if the tx was confirmed 100 blocks before nUntilHeight
     * @param nUntilHeight the height from which to base the remove of archive IS Locks
     */
    void RemoveArchivedInstantSendLocks(int nUntilHeight);
    void WriteBlockInstantSendLocks(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindexConnected);
    void RemoveBlockInstantSendLocks(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindexDisconnected);
    bool KnownInstantSendLock(const uint256& islockHash) const;
    /**
     * Gets the number of IS Locks which have not been confirmed by a block
     * @return size_t value of the number of IS Locks not confirmed by a block
     */
    size_t GetInstantSendLockCount() const;
    /**
     * Gets a pointer to the IS Lock based on the hash
     * @param hash The hash of the IS Lock
     * @param use_cache Should we try using the cache first or not
     * @return A Pointer object to the IS Lock, returns nullptr if it doesn't exist
     */
    CInstantSendLockPtr GetInstantSendLockByHash(const uint256& hash, bool use_cache = true) const;
    /**
     * Gets an IS Lock hash based on the txid the IS Lock is for
     * @param txid The txid which is being searched for
     * @return Returns the hash the IS Lock of the specified txid, returns uint256() if it doesn't exist
     */
    uint256 GetInstantSendLockHashByTxid(const uint256& txid) const;
    /**
     * Gets an IS Lock pointer from the txid given
     * @param txid The txid for which the IS Lock Pointer is being returned
     * @return Returns the IS Lock Pointer associated with the txid, returns nullptr if it doesn't exist
     */
    CInstantSendLockPtr GetInstantSendLockByTxid(const uint256& txid) const;
    /**
     * Gets an IS Lock pointer from an input given
     * @param outpoint Since all inputs are really just outpoints that are being spent
     * @return IS Lock Pointer associated with that input.
     */
    CInstantSendLockPtr GetInstantSendLockByInput(const COutPoint& outpoint) const;
    /**
     * Called when a ChainLock invalidated a IS Lock, removes any chained/children IS Locks and the invalidated IS Lock
     * @param islockHash IS Lock hash which has been invalidated
     * @param txid Transaction id associated with the islockHash
     * @param nHeight height of the block which received a chainlock and invalidated the IS Lock
     * @return A vector of IS Lock hashes of all IS Locks removed
     */
    std::vector<uint256> RemoveChainedInstantSendLocks(const uint256& islockHash, const uint256& txid, int nHeight);

    void RemoveAndArchiveInstantSendLock(const CInstantSendLockPtr& islock, int nHeight);
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
    // Tried to verify but there is no tx yet
    std::unordered_map<uint256, std::pair<NodeId, CInstantSendLockPtr>, StaticSaltedHasher> pendingNoTxInstantSendLocks GUARDED_BY(cs);

    // TXs which are neither IS locked nor ChainLocked. We use this to determine for which TXs we need to retry IS locking
    // of child TXs
    struct NonLockedTxInfo {
        const CBlockIndex* pindexMined;
        CTransactionRef tx;
        std::unordered_set<uint256, StaticSaltedHasher> children;
    };
    std::unordered_map<uint256, NonLockedTxInfo, StaticSaltedHasher> nonLockedTxs GUARDED_BY(cs);
    std::unordered_map<COutPoint, uint256, SaltedOutpointHasher> nonLockedTxsByOutpoints GUARDED_BY(cs);

    std::unordered_set<uint256, StaticSaltedHasher> pendingRetryTxs GUARDED_BY(cs);

public:
    explicit CInstantSendManager(bool unitTests, bool fWipe) : db(unitTests, fWipe) { workInterrupt.reset(); }
    ~CInstantSendManager() = default;

    void Start();
    void Stop();
    void InterruptWorkerThread() { workInterrupt(); };

private:
    void ProcessTx(const CTransaction& tx, bool fRetroactive, const Consensus::Params& params) LOCKS_EXCLUDED(cs);
    bool CheckCanLock(const CTransaction& tx, bool printDebug, const Consensus::Params& params) const;
    bool CheckCanLock(const COutPoint& outpoint, bool printDebug, const uint256& txHash, const Consensus::Params& params) const LOCKS_EXCLUDED(cs);
    bool IsConflicted(const CTransaction& tx) const { return GetConflictingLock(tx) != nullptr; };

    void HandleNewInputLockRecoveredSig(const CRecoveredSig& recoveredSig, const uint256& txid);
    void HandleNewInstantSendLockRecoveredSig(const CRecoveredSig& recoveredSig);

    bool TrySignInputLocks(const CTransaction& tx, bool allowResigning, Consensus::LLMQType llmqType, const Consensus::Params& params);
    void TrySignInstantSendLock(const CTransaction& tx);

    void ProcessMessageInstantSendLock(const CNode* pfrom, const CInstantSendLockPtr& islock) LOCKS_EXCLUDED(cs);
    static bool PreVerifyInstantSendLock(const CInstantSendLock& islock);
    bool ProcessPendingInstantSendLocks();
    bool ProcessPendingInstantSendLocks(bool deterministic);

    std::unordered_set<uint256, StaticSaltedHasher> ProcessPendingInstantSendLocks(const Consensus::LLMQType llmqType, int signOffset, const std::unordered_map<uint256, std::pair<NodeId, CInstantSendLockPtr>, StaticSaltedHasher>& pend, bool ban);
    void ProcessInstantSendLock(NodeId from, const uint256& hash, const CInstantSendLockPtr& islock);

    void AddNonLockedTx(const CTransactionRef& tx, const CBlockIndex* pindexMined);
    void RemoveNonLockedTx(const uint256& txid, bool retryChildren) EXCLUSIVE_LOCKS_REQUIRED(cs);
    void RemoveConflictedTx(const CTransaction& tx) EXCLUSIVE_LOCKS_REQUIRED(cs);
    void TruncateRecoveredSigsForInputs(const CInstantSendLock& islock) EXCLUSIVE_LOCKS_REQUIRED(cs);

    void RemoveMempoolConflictsForLock(const uint256& hash, const CInstantSendLock& islock);
    void ResolveBlockConflicts(const uint256& islockHash, const CInstantSendLock& islock);
    static void AskNodesForLockedTx(const uint256& txid);
    void ProcessPendingRetryLockTxs();

    void WorkThreadMain();

    void HandleFullyConfirmedBlock(const CBlockIndex* pindex);

public:
    bool IsLocked(const uint256& txHash) const;
    bool IsWaitingForTx(const uint256& txHash) const;
    CInstantSendLockPtr GetConflictingLock(const CTransaction& tx) const;

    void HandleNewRecoveredSig(const CRecoveredSig& recoveredSig) override;

    void ProcessMessage(CNode* pfrom, const std::string& msg_type, CDataStream& vRecv);

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

    void RemoveConflictingLock(const uint256& islockHash, const CInstantSendLock& islock) LOCKS_EXCLUDED(cs);

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

#endif // BITCOIN_LLMQ_INSTANTSEND_H
