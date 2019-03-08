// Copyright (c) 2019 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DASH_QUORUMS_INSTANTSEND_H
#define DASH_QUORUMS_INSTANTSEND_H

#include "quorums_signing.h"

#include "coins.h"
#include "primitives/transaction.h"

#include <unordered_map>
#include <unordered_set>

class CScheduler;

namespace llmq
{

class CInstantSendLock
{
public:
    std::vector<COutPoint> inputs;
    uint256 txid;
    CBLSSignature sig;

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

class CInstantSendLockInfo
{
public:
    // might be nullptr when islock is received before the TX itself
    CTransactionRef tx;
    CInstantSendLock islock;
    // only valid when recovered sig was received
    uint256 islockHash;
    // time when it was created/received
    int64_t time;

    // might be null initially (when TX was not mined yet) and will later be filled by SyncTransaction
    const CBlockIndex* pindexMined{nullptr};
};

class CInstantSendManager : public CRecoveredSigsListener
{
private:
    CCriticalSection cs;
    CScheduler* scheduler;

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
    std::unordered_map<uint256, CInstantSendLockInfo, StaticSaltedHasher> creatingInstantSendLocks;
    // maps from txid to the in-progress islock
    std::unordered_map<uint256, CInstantSendLockInfo*, StaticSaltedHasher> txToCreatingInstantSendLocks;

    /**
     * These are the final islocks, indexed by their own hash. The other maps are used to get from TXs, inputs and blocks
     * to islocks.
     */
    std::unordered_map<uint256, CInstantSendLockInfo, StaticSaltedHasher> finalInstantSendLocks;
    std::unordered_map<uint256, CInstantSendLockInfo*, StaticSaltedHasher> txToInstantSendLock;
    std::unordered_map<COutPoint, CInstantSendLockInfo*, SaltedOutpointHasher> inputToInstantSendLock;
    std::unordered_multimap<uint256, CInstantSendLockInfo*, StaticSaltedHasher> blockToInstantSendLocks;

    const CBlockIndex* pindexLastChainLock{nullptr};

    // Incoming and not verified yet
    std::unordered_map<uint256, std::pair<NodeId, CInstantSendLock>> pendingInstantSendLocks;
    bool hasScheduledProcessPending{false};

public:
    CInstantSendManager(CScheduler* _scheduler);
    ~CInstantSendManager();

    void RegisterAsRecoveredSigsListener();
    void UnregisterAsRecoveredSigsListener();

public:
    bool ProcessTx(CNode* pfrom, const CTransaction& tx, CConnman& connman, const Consensus::Params& params);
    bool CheckCanLock(const CTransaction& tx, bool printDebug, const Consensus::Params& params);
    bool CheckCanLock(const COutPoint& outpoint, bool printDebug, const uint256* _txHash, CAmount* retValue, const Consensus::Params& params);
    bool IsLocked(const uint256& txHash);
    bool IsConflicted(const CTransaction& tx);
    bool GetConflictingTx(const CTransaction& tx, uint256& retConflictTxHash);

    virtual void HandleNewRecoveredSig(const CRecoveredSig& recoveredSig);
    void HandleNewInputLockRecoveredSig(const CRecoveredSig& recoveredSig, const uint256& txid);
    void HandleNewInstantSendLockRecoveredSig(const CRecoveredSig& recoveredSig);

    void TrySignInstantSendLock(const CTransaction& tx);

    void ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, CConnman& connman);
    void ProcessMessageInstantSendLock(CNode* pfrom, const CInstantSendLock& islock, CConnman& connman);
    bool PreVerifyInstantSendLock(NodeId nodeId, const CInstantSendLock& islock, bool& retBan);
    void ProcessPendingInstantSendLocks();
    void ProcessInstantSendLock(NodeId from, const uint256& hash, const CInstantSendLock& islock);
    void UpdateWalletTransaction(const uint256& txid);

    void SyncTransaction(const CTransaction &tx, const CBlockIndex *pindex, int posInBlock);
    void NotifyChainLock(const CBlockIndex* pindex);
    void UpdateISLockMinedBlock(CInstantSendLockInfo* islockInfo, const CBlockIndex* pindex);
    void RemoveFinalISLock(const uint256& hash);

    void RemoveMempoolConflictsForLock(const uint256& hash, const CInstantSendLock& islock);
    void RetryLockMempoolTxs(const uint256& lockedParentTx);

    bool AlreadyHave(const CInv& inv);
    bool GetInstantSendLockByHash(const uint256& hash, CInstantSendLock& ret);
};

extern CInstantSendManager* quorumInstantSendManager;

// This involves 2 sporks: SPORK_2_INSTANTSEND_ENABLED and SPORK_20_INSTANTSEND_LLMQ_BASED
// SPORK_2_INSTANTSEND_ENABLED generally enables/disables InstantSend and SPORK_20_INSTANTSEND_LLMQ_BASED switches
// between the old and the new (LLMQ based) system
// TODO When the new system is fully deployed and enabled, we can remove this special handling in a future version
// and revert to only using SPORK_2_INSTANTSEND_ENABLED.
bool IsOldInstantSendEnabled();
bool IsNewInstantSendEnabled();
bool IsInstantSendEnabled();

}

#endif//DASH_QUORUMS_INSTANTSEND_H
