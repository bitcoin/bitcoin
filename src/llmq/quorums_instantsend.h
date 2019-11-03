// Copyright (c) 2019 The Dash Core developers
// Copyright (c) 2019 The BitGreen Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITGREEN_QUORUMS_INSTANTSEND_H
#define BITGREEN_QUORUMS_INSTANTSEND_H

#include <llmq/quorums_signing.h>

#include <coins.h>
#include <unordered_lru_cache.h>
#include <primitives/transaction.h>

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
    ADD_SERIALIZE_METHODS

    template<typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        READWRITE(inputs);
        READWRITE(txid);
        READWRITE(sig);
    }

    uint256 GetRequestId() const;
};

typedef std::shared_ptr<CInstantSendLock> CInstantSendLockPtr;

class CInstantSendDb
{
private:
    CDBWrapper& db;

    unordered_lru_cache<uint256, CInstantSendLockPtr, StaticSaltedHasher, 10000> islockCache;
    unordered_lru_cache<uint256, uint256, StaticSaltedHasher, 10000> txidCache;
    unordered_lru_cache<COutPoint, uint256, SaltedOutpointHasher, 10000> outpointCache;

public:
    CInstantSendDb(CDBWrapper& _db) : db(_db) {}

    void WriteNewInstantSendLock(const uint256& hash, const CInstantSendLock& islock);
    void RemoveInstantSendLock(CDBBatch& batch, const uint256& hash, CInstantSendLockPtr islock);

    void WriteInstantSendLockMined(const uint256& hash, int nHeight);
    void RemoveInstantSendLockMined(const uint256& hash, int nHeight);
    void WriteInstantSendLockArchived(CDBBatch& batch, const uint256& hash, int nHeight);
    std::unordered_map<uint256, CInstantSendLockPtr> RemoveConfirmedInstantSendLocks(int nUntilHeight);
    void RemoveArchivedInstantSendLocks(int nUntilHeight);
    bool HasArchivedInstantSendLock(const uint256& islockHash);
    size_t GetInstantSendLockCount();

    CInstantSendLockPtr GetInstantSendLockByHash(const uint256& hash);
    uint256 GetInstantSendLockHashByTxid(const uint256& txid);
    CInstantSendLockPtr GetInstantSendLockByTxid(const uint256& txid);
    CInstantSendLockPtr GetInstantSendLockByInput(const COutPoint& outpoint);

    std::vector<uint256> GetInstantSendLocksByParent(const uint256& parent);
    std::vector<uint256> RemoveChainedInstantSendLocks(const uint256& islockHash, const uint256& txid, int nHeight);
};

class CInstantSendManager : public CRecoveredSigsListener
{
private:
    CCriticalSection cs;
    CInstantSendDb db;

    std::thread workThread;
    CThreadInterrupt workInterrupt;

    /**
     * Request ids of inputs that we signed. Used to determine if a recovered signature belongs to an
     * in-progress input lock.
     */
    std::unordered_set<uint256, StaticSaltedHasher> inputRequestIds;

    /**
     * These are the islocks that are currently in the middle of being created. Entries are created when we observed
     * recovered signatures for all inputs of a TX. At the same time, we initiate signing of our sigshare for the islock.
     * When the recovered sig for the islock later arrives, we can finish the islock and propagate it.
     */
    std::unordered_map<uint256, CInstantSendLock, StaticSaltedHasher> creatingInstantSendLocks;
    // maps from txid to the in-progress islock
    std::unordered_map<uint256, CInstantSendLock*, StaticSaltedHasher> txToCreatingInstantSendLocks;

    // Incoming and not verified yet
    std::unordered_map<uint256, std::pair<NodeId, CInstantSendLock>> pendingInstantSendLocks;

    // TXs which are neither IS locked nor ChainLocked. We use this to determine for which TXs we need to retry IS locking
    // of child TXs
    struct NonLockedTxInfo {
        const CBlockIndex* pindexMined{nullptr};
        CTransactionRef tx;
        std::unordered_set<uint256, StaticSaltedHasher> children;
    };
    std::unordered_map<uint256, NonLockedTxInfo, StaticSaltedHasher> nonLockedTxs;
    std::unordered_multimap<uint256, std::pair<uint32_t, uint256>> nonLockedTxsByInputs;

    std::unordered_set<uint256, StaticSaltedHasher> pendingRetryTxs;

public:
    CInstantSendManager(CDBWrapper& _llmqDb);
    ~CInstantSendManager();

    void Start();
    void Stop();
    void InterruptWorkerThread();

public:
    bool ProcessTx(const CTransaction& tx, const Consensus::Params& params);
    bool CheckCanLock(const CTransaction& tx, bool printDebug, const Consensus::Params& params);
    bool CheckCanLock(const COutPoint& outpoint, bool printDebug, const uint256& txHash, CAmount* retValue, const Consensus::Params& params);
    bool IsLocked(const uint256& txHash);
    bool IsConflicted(const CTransaction& tx);
    CInstantSendLockPtr GetConflictingLock(const CTransaction& tx);

    virtual void HandleNewRecoveredSig(const CRecoveredSig& recoveredSig);
    void HandleNewInputLockRecoveredSig(const CRecoveredSig& recoveredSig, const uint256& txid);
    void HandleNewInstantSendLockRecoveredSig(const CRecoveredSig& recoveredSig);

    void TrySignInstantSendLock(const CTransaction& tx);

    void ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, CConnman* connman);
    void ProcessMessageInstantSendLock(CNode* pfrom, const CInstantSendLock& islock, CConnman* connman);
    bool PreVerifyInstantSendLock(NodeId nodeId, const CInstantSendLock& islock, bool& retBan);
    bool ProcessPendingInstantSendLocks();
    std::unordered_set<uint256> ProcessPendingInstantSendLocks(int signHeight, const std::unordered_map<uint256, std::pair<NodeId, CInstantSendLock>>& pend, bool ban);
    void ProcessInstantSendLock(NodeId from, const uint256& hash, const CInstantSendLock& islock);
    void UpdateWalletTransaction(const uint256& txid, const CTransactionRef& tx);

    void SyncTransaction(const CTransaction &tx, const CBlockIndex *pindex, int posInBlock);
    void AddNonLockedTx(const CTransactionRef& tx);
    void RemoveNonLockedTx(const uint256& txid, bool retryChildren);
    void RemoveConflictedTx(const CTransaction& tx);

    void NotifyChainLock(const CBlockIndex* pindexChainLock);
    void UpdatedBlockTip(const CBlockIndex* pindexNew);

    void HandleFullyConfirmedBlock(const CBlockIndex* pindex);

    void RemoveMempoolConflictsForLock(const uint256& hash, const CInstantSendLock& islock);
    void ResolveBlockConflicts(const uint256& islockHash, const CInstantSendLock& islock);
    void RemoveChainLockConflictingLock(const uint256& islockHash, const CInstantSendLock& islock);
    void AskNodesForLockedTx(const uint256& txid);
    bool ProcessPendingRetryLockTxs();

    bool AlreadyHave(const CInv& inv);
    bool GetInstantSendLockByHash(const uint256& hash, CInstantSendLock& ret);

    size_t GetInstantSendLockCount();

    void WorkThreadMain();
};

extern CInstantSendManager* quorumInstantSendManager;

// SPORK_5_INSTANTSEND_ENABLED generally enables/disables InstantSend
bool IsInstantSendEnabled();

}

#endif // BITGREEN_QUORUMS_INSTANTSEND_H
