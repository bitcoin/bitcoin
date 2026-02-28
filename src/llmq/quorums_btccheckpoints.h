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
#include <vector>

class CBlockIndex;
class ChainstateManager;
class CConnman;
class PeerManager;

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
    int32_t nHeight{-1};         //!< Syscoin height being attested (expected: h-10 of the carrier block)
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

class CBTCCheckpointsHandler : public CRecoveredSigsListener
{
    static constexpr int32_t RECENT_BTCCHECKPOINTS_MAX{256};

private:
    mutable Mutex cs;
    ChainstateManager& chainman;
    CConnman& connman;
    PeerManager& peerman;

    // requestId -> (height, sysHash)
    std::map<uint256, std::pair<int32_t, uint256>> mapSignedRequestIds GUARDED_BY(cs);
    int32_t lastSignedHeight GUARDED_BY(cs){-1};
    uint256 lastSignedSysHash GUARDED_BY(cs);

    // Best shares per height (signed by single quorum), later aggregated.
    std::map<int32_t, std::map<CQuorumCPtr, CBTCCheckpointSigCPtr>> bestShares GUARDED_BY(cs);
    std::map<int32_t, CBTCCheckpointSigCPtr> bestCandidates GUARDED_BY(cs);
    std::map<int32_t, CBTCCheckpointSig> recentBTCCheckpoints GUARDED_BY(cs);

public:
    CBTCCheckpointsHandler(CConnman& connman, PeerManager& peerman, ChainstateManager& chainman);

    void Start();
    void Stop();

    // Periodic signer: signs a deterministic, lagged checkpoint (activeHeight-10, aligned to 10).
    void TrySignBTCCheckpointTip();

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
    bool VerifyBTCCheckpointShare(const CBTCCheckpointSig& btcsig, const CBlockIndex* pindexScan, const uint256& idIn, std::pair<int, CQuorumCPtr>& ret) const EXCLUSIVE_LOCKS_REQUIRED(!cs);
};

extern CBTCCheckpointsHandler* btcCheckpointsHandler;

} // namespace llmq

#endif // SYSCOIN_LLMQ_QUORUMS_BTCCHECKPOINTS_H

