// Copyright (c) 2019-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CHAINLOCK_SIGNING_H
#define BITCOIN_CHAINLOCK_SIGNING_H

#include <chainlock/chainlock.h>
#include <chainlock/clsig.h>
#include <llmq/signing.h>

class CMasternodeSync;
struct MessageProcessingResult;
namespace llmq {
class CChainLocksHandler;
class CInstantSendManager;
class CRecoveredSig;
class CSigningManager;
class CSigSharesManager;
} // namespace llmq

namespace chainlock {
//! Depth of block including transactions before it's considered safe
static constexpr int32_t TX_CONFIRM_THRESHOLD{5};

class ChainLockSignerParent
{
public:
    virtual ~ChainLockSignerParent() = default;

    virtual bool IsTxSafeForMining(const uint256& txid) const = 0;
    [[nodiscard]] virtual MessageProcessingResult ProcessNewChainLock(NodeId from, const ChainLockSig& clsig, const uint256& hash) = 0;
    virtual void UpdateTxFirstSeenMap(const Uint256HashSet& tx, const int64_t& time) = 0;
};

class ChainLockSigner final : public llmq::CRecoveredSigsListener
{
private:
    CChainState& m_chainstate;
    const chainlock::Chainlocks& m_chainlocks;
    ChainLockSignerParent& m_clhandler; // TODO: fully replace to Chainlocks
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

    BlockTxs blockTxs GUARDED_BY(cs_signer);
    int32_t lastSignedHeight GUARDED_BY(cs_signer){-1};
    uint256 lastSignedRequestId GUARDED_BY(cs_signer);
    uint256 lastSignedMsgHash GUARDED_BY(cs_signer);

public:
    ChainLockSigner() = delete;
    ChainLockSigner(const ChainLockSigner&) = delete;
    ChainLockSigner& operator=(const ChainLockSigner&) = delete;
    explicit ChainLockSigner(CChainState& chainstate, const chainlock::Chainlocks& chainlocks, ChainLockSignerParent& clhandler, llmq::CSigningManager& sigman,
                             llmq::CSigSharesManager& shareman, const CMasternodeSync& mn_sync);
    ~ChainLockSigner();

    void RegisterRecoveryInterface();
    void UnregisterRecoveryInterface();

    void EraseFromBlockHashTxidMap(const uint256& hash)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_signer);
    void UpdateBlockHashTxidMap(const uint256& hash, const std::vector<CTransactionRef>& vtx)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_signer);

    void TrySignChainTip(const llmq::CInstantSendManager& isman)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_signer);
    [[nodiscard]] MessageProcessingResult HandleNewRecoveredSig(const llmq::CRecoveredSig& recoveredSig) override
        EXCLUSIVE_LOCKS_REQUIRED(!cs_signer);

    [[nodiscard]] std::vector<std::shared_ptr<Uint256HashSet>> Cleanup() EXCLUSIVE_LOCKS_REQUIRED(!cs_signer);

private:
    [[nodiscard]] BlockTxs::mapped_type GetBlockTxs(const uint256& blockHash)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_signer);
};
} // namespace chainlock

#endif // BITCOIN_CHAINLOCK_SIGNING_H
