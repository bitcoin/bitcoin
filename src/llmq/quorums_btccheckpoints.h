// Copyright (c) 2026 The Syscoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_LLMQ_QUORUMS_BTCCHECKPOINTS_H
#define SYSCOIN_LLMQ_QUORUMS_BTCCHECKPOINTS_H

#include <bls/bls.h>
#include <llmq/quorums_signing.h>
#include <uint256.h>

#include <atomic>
#include <map>
#include <memory>
#include <string>
#include <vector>

class CBlockIndex;
class ChainstateManager;
class CConnman;
class PeerManager;
class CNode;
class CDataStream;
class UniValue;

namespace llmq
{

/**
 * In-block BTC checkpoint attestation (null allowed).
 *
 * This is a multi-quorum aggregated signature (same style as ChainLocks),
 * but signing a message hash that binds {height, sysHash}.
 *
 * The BTC prev hash is implied by the referenced Syscoin block's AuxPoW and
 * does not need to be carried (or disk-read validated) by all nodes.
 */
class CBTCCheckpointSig
{
public:
    int32_t nHeight{-1};         //!< Syscoin height being attested (expected: h-5 of the carrier block)
    uint256 sysHash;             //!< Syscoin block hash at nHeight
    CBLSSignature sig;           //!< Aggregated signature
    std::vector<bool> signers;   //!< Which of the active quorums signed

public:
    CBTCCheckpointSig() = default;
    CBTCCheckpointSig(int32_t nHeight, const uint256& sysHash, const CBLSSignature& sig, const std::vector<bool>& signers) :
        nHeight(nHeight),
        sysHash(sysHash),
        sig(sig),
        signers(signers)
    {}

    SERIALIZE_METHODS(CBTCCheckpointSig, obj) {
        READWRITE(obj.nHeight, obj.sysHash, obj.sig);
        READWRITE(DYNBITSET(obj.signers));
    }

    bool IsNull() const;
    std::string ToString() const;
};

using CBTCCheckpointSigCPtr = std::shared_ptr<const CBTCCheckpointSig>;

class CBTCHeaderPolicyWatchdog
{
private:
    mutable Mutex cs;
    ChainstateManager& chainman;

    int64_t lastProbeTime GUARDED_BY(cs){0};
    bool lastProbeHealthy GUARDED_BY(cs){true};
    std::string lastProbeReason GUARDED_BY(cs);
    int64_t lastRestartTime GUARDED_BY(cs){0};
    int32_t consecutiveRestartFailures GUARDED_BY(cs){0};
    bool reindexAttemptedInFailureStreak GUARDED_BY(cs){false};
    int64_t lastProgressTime GUARDED_BY(cs){0};
    int64_t lastTipHeight GUARDED_BY(cs){-1};

public:
    explicit CBTCHeaderPolicyWatchdog(ChainstateManager& chainman);
    bool CheckAndRecover(std::string& denyReason) EXCLUSIVE_LOCKS_REQUIRED(!cs);

private:
    bool ProbeChainInfo(UniValue& out, std::string& err) const;
    bool ParseChainInfo(const UniValue& chainInfo, bool& ibd, int64_t& tipHeight, std::string& err) const;
    bool AttemptManagedRestart(const std::string& restartReason, std::string& denyReason) EXCLUSIVE_LOCKS_REQUIRED(!cs);
};

class CBTCCheckpointsHandler : public CRecoveredSigsListener
{
    static constexpr int32_t RECENT_BTCCHECKPOINTS_MAX{256};
    static constexpr size_t PENDING_VERIFIED_BTCCSIG_MAX{256};
    static const int64_t PENDING_VERIFIED_BTCCSIG_TIMEOUT = 1000 * 60 * 10; // 10 minutes
    static const int64_t CLEANUP_INTERVAL = 1000 * 30;
    static const int64_t CLEANUP_SEEN_TIMEOUT = 24 * 60 * 60 * 1000;

private:
    mutable Mutex cs;
    ChainstateManager& chainman;
    CConnman& connman;
    PeerManager& peerman;
    CBTCHeaderPolicyWatchdog btcheaderWatchdog;

    // hashes of btccsig objects we've already processed/relayed
    std::map<uint256, int64_t> seenBTCCheckpointSigs GUARDED_BY(cs);
    mutable std::map<uint256, int64_t> sigChecked GUARDED_BY(cs);
    int64_t lastCleanupTime GUARDED_BY(cs) {0};

    // requestId -> (height, sysHash)
    std::map<uint256, std::pair<int32_t, uint256>> mapSignedRequestIds GUARDED_BY(cs);
    int32_t lastSignedHeight GUARDED_BY(cs){-1};
    uint256 lastSignedSysHash GUARDED_BY(cs);
    // Last BTC header hash/height we queued for signing (policy continuity guard).
    uint256 lastSignedBTCHash GUARDED_BY(cs);
    int32_t lastSignedBTCHeight GUARDED_BY(cs){-1};
    // Tip progress tracking used by height-based freshness checks (clock-skew tolerant).
    int64_t lastPolicyObservedTipHeight GUARDED_BY(cs){-1};
    int64_t lastPolicyTipProgressTime GUARDED_BY(cs){0};
    // Deduplicate policy-denial logs for the same sign target.
    int32_t lastPolicyRejectHeight GUARDED_BY(cs){-1};
    std::string lastPolicyRejectReason GUARDED_BY(cs);

    // Best shares per height (signed by single quorum), later aggregated.
    std::map<int32_t, std::map<CQuorumCPtr, CBTCCheckpointSigCPtr>> bestShares GUARDED_BY(cs);
    std::map<int32_t, CBTCCheckpointSigCPtr> bestCandidates GUARDED_BY(cs);
    std::map<int32_t, CBTCCheckpointSig> recentBTCCheckpoints GUARDED_BY(cs);
    // Verified BTCCSIG objects received early (expectedHeight mismatch), kept briefly and re-applied
    // once the local expected height reaches the object's height. Keyed by object hash.
    std::map<uint256, std::pair<CBTCCheckpointSig, int64_t>> pendingVerifiedBTCCheckpointSigs GUARDED_BY(cs);

public:
    CBTCCheckpointsHandler(CConnman& connman, PeerManager& peerman, ChainstateManager& chainman);

    void Start();
    void Stop();

    // Periodic signer: signs a deterministic checkpoint (activeHeight aligned to 10).
    // The carrier block at height (nHeight+5) embeds this checkpoint for a 5-block propagation buffer.
    void TrySignBTCCheckpointTip();
    void ProcessPendingVerifiedBTCCheckpointSigs();

    void ProcessMessage(CNode* pfrom, const std::string& msg_type, CDataStream& vRecv) EXCLUSIVE_LOCKS_REQUIRED(!cs);
    bool AlreadyHave(const uint256& hash) const EXCLUSIVE_LOCKS_REQUIRED(!cs);
    bool GetBTCCheckpointByHash(const uint256& hash, CBTCCheckpointSig& ret) const EXCLUSIVE_LOCKS_REQUIRED(!cs);

    void HandleNewRecoveredSig(const CRecoveredSig& recoveredSig) override;

    CBTCCheckpointSig GetMostRecentBTCCheckpoint() const EXCLUSIVE_LOCKS_REQUIRED(!cs);
    CBTCCheckpointSig GetBestBTCCheckpoint() const EXCLUSIVE_LOCKS_REQUIRED(!cs);
    std::map<CQuorumCPtr, CBTCCheckpointSigCPtr> GetBestBTCCheckpointShares() const EXCLUSIVE_LOCKS_REQUIRED(!cs);

    bool GetRecentBTCCheckpointByHeight(int32_t nHeight, CBTCCheckpointSig& ret) const EXCLUSIVE_LOCKS_REQUIRED(!cs);

    // Consensus-facing verifier (BLS verify), used by specialtx and miner.
    bool VerifyAggregatedBTCCheckpoint(const CBTCCheckpointSig& btcsig, const CBlockIndex* pindexScan) const EXCLUSIVE_LOCKS_REQUIRED(!cs);

private:
    void AddRecentBTCCheckpoint(const CBTCCheckpointSig& btcsig) EXCLUSIVE_LOCKS_REQUIRED(cs);
    bool TryUpdateBestBTCCheckpoint(const CBlockIndex* pindexScan) EXCLUSIVE_LOCKS_REQUIRED(cs);
    bool VerifyBTCCheckpointShare(const CBTCCheckpointSig& btcsig, const CBlockIndex* pindexScan, const uint256& idIn, std::pair<int, CQuorumCPtr>& ret, const uint256& hash) const EXCLUSIVE_LOCKS_REQUIRED(!cs);
    bool VerifyAggregatedBTCCheckpointNoCache(const CBTCCheckpointSig& btcsig, const CBlockIndex* pindexScan) const EXCLUSIVE_LOCKS_REQUIRED(!cs);
    void Cleanup() EXCLUSIVE_LOCKS_REQUIRED(!cs);
    void AddPendingVerifiedBTCCheckpointSig(const uint256& hash, const CBTCCheckpointSig& btcsig) EXCLUSIVE_LOCKS_REQUIRED(!cs);
    void AcceptVerifiedBTCCSig(const CBTCCheckpointSig& btccsig, const uint256& hash, const CBlockIndex* pindexScan) EXCLUSIVE_LOCKS_REQUIRED(!cs);
    bool RunBTCHeaderCommand(const std::string& method_and_args, UniValue& out, std::string& err) const EXCLUSIVE_LOCKS_REQUIRED(!cs);
    bool CheckBTCHeaderSigningPolicy(const uint256& btcHash, int32_t sysHeight, int32_t& btcHeightOut, std::string& denyReason) EXCLUSIVE_LOCKS_REQUIRED(!cs);
};

extern CBTCCheckpointsHandler* btcCheckpointsHandler;

} // namespace llmq

#endif // SYSCOIN_LLMQ_QUORUMS_BTCCHECKPOINTS_H

