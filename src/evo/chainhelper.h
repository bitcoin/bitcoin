// Copyright (c) 2024-2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_EVO_CHAINHELPER_H
#define BITCOIN_EVO_CHAINHELPER_H

#include <memory>
#include <optional>
#include <unordered_map>

class CBlockIndex;
class CCreditPoolManager;
class CDeterministicMNManager;
class CEvoDB;
class ChainstateManager;
class CMNHFManager;
class CMNPaymentsProcessor;
class CMasternodeSync;
class CGovernanceManager;
class CSpecialTxProcessor;
class CSporkManager;
class CTransaction;
class uint256;

namespace chainlock {
class Chainlocks;
}
namespace Consensus { struct Params; }
namespace llmq {
class CInstantSendManager;
class CQuorumBlockProcessor;
class CQuorumManager;
class CQuorumSnapshotManager;
}

class CChainstateHelper
{
private:
    llmq::CInstantSendManager& isman;

public:
    const chainlock::Chainlocks& m_chainlocks;

public:
    CChainstateHelper() = delete;
    CChainstateHelper(const CChainstateHelper&) = delete;
    CChainstateHelper& operator=(const CChainstateHelper&) = delete;
    explicit CChainstateHelper(CEvoDB& evodb, CDeterministicMNManager& dmnman, CGovernanceManager& govman,
                               llmq::CInstantSendManager& isman, llmq::CQuorumBlockProcessor& qblockman,
                               llmq::CQuorumSnapshotManager& qsnapman, const ChainstateManager& chainman,
                               const Consensus::Params& consensus_params, const CMasternodeSync& mn_sync,
                               const CSporkManager& sporkman, const chainlock::Chainlocks& chainlocks,
                               const llmq::CQuorumManager& qman);
    ~CChainstateHelper();

    /** Passthrough functions to chainlock::Chainlocks*/
    bool HasConflictingChainLock(int nHeight, const uint256& blockHash) const;
    bool HasChainLock(int nHeight, const uint256& blockHash) const;
    int32_t GetBestChainLockHeight() const;

    /** Passthrough functions to CInstantSendManager */
    std::optional<std::pair</*islock_hash=*/uint256, /*txid=*/uint256>> ConflictingISLockIfAny(const CTransaction& tx) const;
    bool IsInstantSendWaitingForTx(const uint256& hash) const;
    bool RemoveConflictingISLockByTx(const CTransaction& tx);
    bool ShouldInstantSendRejectConflicts() const;

    std::unordered_map<uint8_t, int> GetSignalsStage(const CBlockIndex* const pindexPrev);

public:
    const std::unique_ptr<CMNHFManager> ehf_manager;
    const std::unique_ptr<CCreditPoolManager> credit_pool_manager;
    const std::unique_ptr<CMNPaymentsProcessor> mn_payments;
    const std::unique_ptr<CSpecialTxProcessor> special_tx;
};

#endif // BITCOIN_EVO_CHAINHELPER_H
