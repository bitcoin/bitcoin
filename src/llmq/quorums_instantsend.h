// Copyright (c) 2019 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DASH_QUORUMS_INSTANTX_H
#define DASH_QUORUMS_INSTANTX_H

#include "quorums_signing.h"

#include "coins.h"
#include "primitives/transaction.h"

#include <unordered_map>
#include <unordered_set>

class CScheduler;

namespace llmq
{

class CInstantXLock
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

class CInstantXLockInfo
{
public:
    // might be nullptr when ixlock is received before the TX itself
    CTransactionRef tx;
    CInstantXLock ixlock;
    // only valid when recovered sig was received
    uint256 ixlockHash;
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
     * These are the votes/signatures we performed locally. It's indexed by the LLMQ requestId, which is
     * hash(TXLOCK_REQUESTID_PREFIX, prevout). The map values are the txids we voted for. This map is used to
     * avoid voting for the same input twice.
     */
    std::unordered_map<uint256, uint256, StaticSaltedHasher> inputVotes;

    /**
     * These are the ixlocks that are currently in the middle of being created. Entries are created when we observed
     * recovered signatures for all inputs of a TX. At the same time, we initiate signing of our sigshare for the ixlock.
     * When the recovered sig for the ixlock later arrives, we can finish the ixlock and propagate it.
     */
    std::unordered_map<uint256, CInstantXLockInfo, StaticSaltedHasher> creatingInstantXLocks;
    // maps from txid to the in-progress ixlock
    std::unordered_map<uint256, CInstantXLockInfo*, StaticSaltedHasher> txToCreatingInstantXLocks;

    /**
     * These are the final ixlocks, indexed by their own hash. The other maps are used to get from TXs, inputs and blocks
     * to ixlocks.
     */
    std::unordered_map<uint256, CInstantXLockInfo, StaticSaltedHasher> finalInstantXLocks;
    std::unordered_map<uint256, CInstantXLockInfo*, StaticSaltedHasher> txToInstantXLock;
    std::unordered_map<COutPoint, CInstantXLockInfo*, SaltedOutpointHasher> inputToInstantXLock;
    std::unordered_multimap<uint256, CInstantXLockInfo*, StaticSaltedHasher> blockToInstantXLocks;

    const CBlockIndex* pindexLastChainLock{nullptr};

    // Incoming and not verified yet
    std::unordered_map<uint256, std::pair<NodeId, CInstantXLock>> pendingInstantXLocks;
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
    void HandleNewInstantXLockRecoveredSig(const CRecoveredSig& recoveredSig);

    void TrySignInstantXLock(const CTransaction& tx);

    void ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, CConnman& connman);
    void ProcessMessageInstantXLock(CNode* pfrom, const CInstantXLock& ixlock, CConnman& connman);
    bool PreVerifyInstantXLock(NodeId nodeId, const CInstantXLock& ixlock, bool& retBan);
    void ProcessPendingInstantXLocks();
    void ProcessInstantXLock(NodeId from, const uint256& hash, const CInstantXLock& ixlock);
    void UpdateWalletTransaction(const uint256& txid);

    void SyncTransaction(const CTransaction &tx, const CBlockIndex *pindex, int posInBlock);
    void NotifyChainLock(const CBlockIndex* pindex);
    void UpdateIxLockMinedBlock(CInstantXLockInfo* ixlockInfo, const CBlockIndex* pindex);
    void RemoveFinalIxLock(const uint256& hash);

    void RemoveMempoolConflictsForLock(const uint256& hash, const CInstantXLock& ixlock);
    void RetryLockMempoolTxs(const uint256& lockedParentTx);

    bool AlreadyHave(const CInv& inv);
    bool GetInstantXLockByHash(const uint256& hash, CInstantXLock& ret);
};

extern CInstantSendManager* quorumInstantSendManager;

// The meaning of spork 2 has changed in v0.14. Before that, spork 2 was simply time based and either enabled or not
// After 0.14, spork 2 can have 3 states.
// 0 = old system is active (0 is compatible with the value set on mainnet at time of deployment)
// 1 = new system is active (old nodes will interpret this as the old system being enabled, but then won't get enough IX lock votes)
// everything else = disabled
// TODO When the new system is fully deployed and enabled, we can remove this special handling of the spork in a future version
// and revert to the old behaviour.
bool IsOldInstantSendEnabled();
bool IsNewInstantSendEnabled();
bool IsInstantSendEnabled();

}

#endif//DASH_QUORUMS_INSTANTX_H
