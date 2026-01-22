// Copyright (c) 2019-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CHAINLOCK_SIGNING_H
#define BITCOIN_CHAINLOCK_SIGNING_H

#include <chainlock/chainlock.h>
#include <chainlock/clsig.h>
#include <llmq/signing.h>
#include <util/time.h>
#include <validationinterface.h>

class CScheduler;
class CMasternodeSync;
struct MessageProcessingResult;
namespace llmq {
class CInstantSendManager;
class CRecoveredSig;
class CSigningManager;
class CSigSharesManager;
} // namespace llmq

namespace chainlock {
class ChainlockHandler;

class ChainLockSigner final : public llmq::CRecoveredSigsListener, public CValidationInterface
{
private:
    CChainState& m_chainstate;
    const chainlock::Chainlocks& m_chainlocks;
    ChainlockHandler& m_clhandler;
    const llmq::CInstantSendManager& m_isman;
    const llmq::CQuorumManager& m_qman;
    llmq::CSigningManager& m_sigman;
    llmq::CSigSharesManager& m_shareman;
    const CMasternodeSync& m_mn_sync;

private:
    // We keep track of txids from recently received blocks so that we can check if all TXs got islocked
    struct BlockHasher {
        size_t operator()(const uint256& hash) const { return ReadLE64(hash.begin()); }
    };
    using BlockTxs = std::unordered_map<uint256, std::shared_ptr<Uint256HashSet>, BlockHasher>;

private:
    mutable Mutex cs_signer;

    std::unique_ptr<CScheduler> m_scheduler;
    std::unique_ptr<std::thread> m_scheduler_thread;
    CleanupThrottler<NodeClock> m_cleanup_throttler;

    BlockTxs blockTxs GUARDED_BY(cs_signer);
    int32_t lastSignedHeight GUARDED_BY(cs_signer){-1};
    uint256 lastSignedRequestId GUARDED_BY(cs_signer);
    uint256 lastSignedMsgHash GUARDED_BY(cs_signer);

public:
    ChainLockSigner() = delete;
    ChainLockSigner(const ChainLockSigner&) = delete;
    ChainLockSigner& operator=(const ChainLockSigner&) = delete;
    explicit ChainLockSigner(CChainState& chainstate, const chainlock::Chainlocks& chainlocks,
                             ChainlockHandler& clhandler, const llmq::CInstantSendManager& isman,
                             const llmq::CQuorumManager& qman, llmq::CSigningManager& sigman,
                             llmq::CSigSharesManager& shareman, const CMasternodeSync& mn_sync);
    ~ChainLockSigner();

    void Start();
    void Stop();

    void RegisterRecoveryInterface();
    void UnregisterRecoveryInterface();

    // implements validation interface:
    void BlockConnected(const std::shared_ptr<const CBlock>& block, const CBlockIndex* pindex) override
        EXCLUSIVE_LOCKS_REQUIRED(!cs_signer);
    void BlockDisconnected(const std::shared_ptr<const CBlock>& block, const CBlockIndex* pindex) override
        EXCLUSIVE_LOCKS_REQUIRED(!cs_signer);
    void UpdatedBlockTip(const CBlockIndex* pindexNew, const CBlockIndex* pindexFork, bool fInitialDownload) override
        EXCLUSIVE_LOCKS_REQUIRED(!cs_signer);

    [[nodiscard]] MessageProcessingResult HandleNewRecoveredSig(const llmq::CRecoveredSig& recoveredSig) override
        EXCLUSIVE_LOCKS_REQUIRED(!cs_signer);

    void Cleanup() EXCLUSIVE_LOCKS_REQUIRED(!cs_signer);

private:
    void TrySignChainTip() EXCLUSIVE_LOCKS_REQUIRED(!cs_signer);

    [[nodiscard]] BlockTxs::mapped_type GetBlockTxs(const uint256& blockHash)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_signer);
};
} // namespace chainlock

#endif // BITCOIN_CHAINLOCK_SIGNING_H
