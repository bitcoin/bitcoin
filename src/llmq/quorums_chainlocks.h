// Copyright (c) 2019 The Dash Core developers
// Copyright (c) 2019 The BitGreen Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITGREEN_LLMQ_QUORUMS_CHAINLOCKS_H
#define BITGREEN_LLMQ_QUORUMS_CHAINLOCKS_H

#include <llmq/quorums.h>
#include <llmq/quorums_signing.h>

#include <net.h>
#include <chainparams.h>

#include <atomic>
#include <unordered_set>

class CBlockIndex;
class CScheduler;
class uint256;

namespace llmq
{

class CChainLockSig
{
public:
    int32_t nHeight{-1};
    uint256 blockHash;
    CBLSSignature sig;

public:
    ADD_SERIALIZE_METHODS

    template<typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        READWRITE(nHeight);
        READWRITE(blockHash);
        READWRITE(sig);
    }

    std::string ToString() const;
};

class CChainLocksHandler : public CRecoveredSigsListener
{
    static const int64_t CLEANUP_INTERVAL = 1000 * 30;
    static const int64_t CLEANUP_SEEN_TIMEOUT = 24 * 60 * 60 * 1000;

    // how long to wait for ixlocks until we consider a block with non-ixlocked TXs to be safe to sign
    static const int64_t WAIT_FOR_ISLOCK_TIMEOUT = 10 * 60;

private:
    CScheduler* scheduler;
    CCriticalSection cs;
    bool tryLockChainTipScheduled{false};
    bool isSporkActive{false};
    bool isEnforced{false};

    uint256 bestChainLockHash;
    CChainLockSig bestChainLock;

    CChainLockSig bestChainLockWithKnownBlock;
    const CBlockIndex* bestChainLockBlockIndex{nullptr};
    const CBlockIndex* lastNotifyChainLockBlockIndex{nullptr};

    int32_t lastSignedHeight{-1};
    uint256 lastSignedRequestId;
    uint256 lastSignedMsgHash;

    // We keep track of txids from recently received blocks so that we can check if all TXs got ixlocked
    typedef std::unordered_map<uint256, std::shared_ptr<std::unordered_set<uint256, StaticSaltedHasher>>> BlockTxs;
    BlockTxs blockTxs;
    std::unordered_map<uint256, int64_t> txFirstSeenTime;

    std::map<uint256, int64_t> seenChainLocks;

    int64_t lastCleanupTime{0};

public:
    CChainLocksHandler(CScheduler* _scheduler);
    ~CChainLocksHandler();

    void Start();
    void Stop();

    bool AlreadyHave(const CInv& inv);
    bool GetChainLockByHash(const uint256& hash, CChainLockSig& ret);

    void ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, CConnman* connman);
    void ProcessNewChainLock(NodeId from, const CChainLockSig& clsig, const uint256& hash);
    void AcceptedBlockHeader(const CBlockIndex* pindexNew);
    void UpdatedBlockTip(const CBlockIndex* pindexNew);
    void SyncTransaction(const CTransaction &tx, const CBlockIndex *pindex, int posInBlock);
    void CheckActiveState();
    void TrySignChainTip();
    void EnforceBestChainLock();
    virtual void HandleNewRecoveredSig(const CRecoveredSig& recoveredSig);

    bool HasChainLock(int nHeight, const uint256& blockHash);
    bool HasConflictingChainLock(int nHeight, const uint256& blockHash);

    bool IsTxSafeForMining(const uint256& txid);

private:
    // these require locks to be held already
    bool InternalHasChainLock(int nHeight, const uint256& blockHash);
    bool InternalHasConflictingChainLock(int nHeight, const uint256& blockHash);

    void DoInvalidateBlock(const CBlockIndex* pindex, bool activateBestChain);

    BlockTxs::mapped_type GetBlockTxs(const uint256& blockHash);

    void Cleanup();
};

extern CChainLocksHandler* chainLocksHandler;


}

#endif //BITGREEN_LLMQ_QUORUMS_CHAINLOCKS_H
