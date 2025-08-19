// Copyright (c) 2019-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CHAINLOCK_SIGNING_H
#define BITCOIN_CHAINLOCK_SIGNING_H

#include <chainlock/clsig.h>
#include <llmq/signing.h>

class CMasternodeSync;
class CSporkManager;
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

    virtual int32_t GetBestChainLockHeight() const = 0;
    virtual bool HasChainLock(int nHeight, const uint256& blockHash) const = 0;
    virtual bool HasConflictingChainLock(int nHeight, const uint256& blockHash) const = 0;
    virtual bool IsEnabled() const = 0;
    virtual bool IsTxSafeForMining(const uint256& txid) const = 0;
    [[nodiscard]] virtual MessageProcessingResult ProcessNewChainLock(NodeId from, const ChainLockSig& clsig, const uint256& hash) = 0;
    virtual void UpdateTxFirstSeenMap(const std::unordered_set<uint256, StaticSaltedHasher>& tx, const int64_t& time) = 0;
};

class ChainLockSigner final : public llmq::CRecoveredSigsListener
{
private:
    CChainState& m_chainstate;
    ChainLockSignerParent& m_clhandler;
    llmq::CSigningManager& m_sigman;
    llmq::CSigSharesManager& m_shareman;
    CSporkManager& m_sporkman;
    const CMasternodeSync& m_mn_sync;

private:
    // We keep track of txids from recently received blocks so that we can check if all TXs got islocked
    struct BlockHasher {
        size_t operator()(const uint256& hash) const { return ReadLE64(hash.begin()); }
    };
    using BlockTxs = std::unordered_map<uint256, std::shared_ptr<std::unordered_set<uint256, StaticSaltedHasher>>, BlockHasher>;

private:
    mutable Mutex cs_signer;

    BlockTxs blockTxs GUARDED_BY(cs_signer);
    int32_t lastSignedHeight GUARDED_BY(cs_signer){-1};
    uint256 lastSignedRequestId GUARDED_BY(cs_signer);
    uint256 lastSignedMsgHash GUARDED_BY(cs_signer);

public:
    explicit ChainLockSigner(CChainState& chainstate, ChainLockSignerParent& clhandler, llmq::CSigningManager& sigman,
                             llmq::CSigSharesManager& shareman, CSporkManager& sporkman, const CMasternodeSync& mn_sync);
    ~ChainLockSigner();

    void Start();
    void Stop();

    void EraseFromBlockHashTxidMap(const uint256& hash)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_signer);
    void UpdateBlockHashTxidMap(const uint256& hash, const std::vector<CTransactionRef>& vtx)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_signer);

    void TrySignChainTip(const llmq::CInstantSendManager& isman)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_signer);
    [[nodiscard]] MessageProcessingResult HandleNewRecoveredSig(const llmq::CRecoveredSig& recoveredSig) override
        EXCLUSIVE_LOCKS_REQUIRED(!cs_signer);

    [[nodiscard]] std::vector<std::shared_ptr<std::unordered_set<uint256, StaticSaltedHasher>>> Cleanup()
        EXCLUSIVE_LOCKS_REQUIRED(!cs_signer);

private:
    [[nodiscard]] BlockTxs::mapped_type GetBlockTxs(const uint256& blockHash)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_signer);
};
} // namespace chainlock

#endif // BITCOIN_CHAINLOCK_SIGNING_H
