// Copyright (c) 2019-2020 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_LLMQ_QUORUMS_CHAINLOCKS_H
#define SYSCOIN_LLMQ_QUORUMS_CHAINLOCKS_H

#include <llmq/quorums.h>
#include <llmq/quorums_signing.h>

#include <chainparams.h>

#include <atomic>
#include <unordered_set>


class CBlockIndex;
class CConnman;
class PeerManager;
namespace llmq
{

class CChainLockSig
{
public:
    int32_t nHeight{-1};
    uint256 blockHash;
    CBLSSignature sig;

public:
    SERIALIZE_METHODS(CChainLockSig, obj) {
        READWRITE(obj.nHeight, obj.blockHash, obj.sig);
    }

    bool IsNull() const;
    std::string ToString() const;
};

typedef std::shared_ptr<const CChainLockSig> CChainLockSigCPtr;

struct ReverseHeightComparator
{
    bool operator()(const int h1, const int h2) const {
        return h1 > h2;
    }
};

class CChainLocksHandler : public CRecoveredSigsListener
{
    static const int64_t CLEANUP_INTERVAL = 1000 * 30;
    static const int64_t CLEANUP_SEEN_TIMEOUT = 24 * 60 * 60 * 1000;


private:
    mutable RecursiveMutex cs;
    bool isEnabled{false};
    bool isEnforced{false};

    CChainLockSig mostRecentChainLockCandidate;
    // Keep best chainlock candidates, sorted by height (highest heght first).
    std::map<int, std::map<CQuorumCPtr, CChainLockSigCPtr>, ReverseHeightComparator> bestChainLockCandidates;

    const CBlockIndex* bestChainLockBlockIndex{nullptr};

    int32_t lastTrySignHeight{-1};
    std::set<uint256> lastSignedRequestIds;
    uint256 lastSignedMsgHash;

    std::map<uint256, int64_t> seenChainLocks;

    int64_t lastCleanupTime{0};

public:
    CConnman& connman;
    PeerManager& peerman;
    explicit CChainLocksHandler(CConnman &connman, PeerManager& peerman);
    ~CChainLocksHandler();

    void Start();
    void Stop();

    bool AlreadyHave(const uint256& hash);
    bool GetChainLockByHash(const uint256& hash, CChainLockSig& ret);
    CChainLockSig GetMostRecentChainLock();
    const std::map<CQuorumCPtr, CChainLockSigCPtr> GetBestChainLocks();

    void ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv);
    void ProcessNewChainLock(NodeId from, const CChainLockSig& clsig, const uint256& hash, const uint256& id=uint256());
    void AcceptedBlockHeader(const CBlockIndex* pindexNew);
    void UpdatedBlockTip(const CBlockIndex* pindexNew, bool fInitialDownload);
    void CheckActiveState();
    void TrySignChainTip(const CBlockIndex* pindexNew);
    virtual void HandleNewRecoveredSig(const CRecoveredSig& recoveredSig) override;

    bool HasChainLock(int nHeight, const uint256& blockHash);
    bool HasConflictingChainLock(int nHeight, const uint256& blockHash);


private:
    // these require locks to be held already
    bool InternalHasChainLock(int nHeight, const uint256& blockHash) EXCLUSIVE_LOCKS_REQUIRED(cs);
    bool InternalHasConflictingChainLock(int nHeight, const uint256& blockHash) EXCLUSIVE_LOCKS_REQUIRED(cs);

    void TryUpdateBestChainLockIndex(const CBlockIndex* pindex, size_t threshold) EXCLUSIVE_LOCKS_REQUIRED(cs);
    void Cleanup();
};

extern CChainLocksHandler* chainLocksHandler;

bool AreChainLocksEnabled();

} // namespace llmq

#endif // SYSCOIN_LLMQ_QUORUMS_CHAINLOCKS_H
