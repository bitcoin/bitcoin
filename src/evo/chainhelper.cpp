// Copyright (c) 2024-2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <evo/chainhelper.h>

#include <chainparams.h>

#include <chainlock/chainlock.h>
#include <evo/creditpool.h>
#include <evo/mnhftx.h>
#include <evo/specialtxman.h>
#include <instantsend/instantsend.h>
#include <instantsend/lock.h>
#include <masternode/payments.h>

CChainstateHelper::CChainstateHelper(CEvoDB& evodb, CDeterministicMNManager& dmnman, CGovernanceManager& govman,
                                     llmq::CInstantSendManager& isman, llmq::CQuorumBlockProcessor& qblockman,
                                     llmq::CQuorumSnapshotManager& qsnapman, const ChainstateManager& chainman,
                                     const Consensus::Params& consensus_params, const CMasternodeSync& mn_sync,
                                     const CSporkManager& sporkman, const chainlock::Chainlocks& chainlocks,
                                     const llmq::CQuorumManager& qman) :
    isman{isman},
    m_chainlocks{chainlocks},
    ehf_manager{std::make_unique<CMNHFManager>(evodb, chainman, qman)},
    credit_pool_manager{std::make_unique<CCreditPoolManager>(evodb, chainman)},
    mn_payments{std::make_unique<CMNPaymentsProcessor>(dmnman, govman, chainman, consensus_params, mn_sync, sporkman)},
    special_tx{std::make_unique<CSpecialTxProcessor>(*credit_pool_manager, dmnman, *ehf_manager, qblockman, qsnapman,
                                                     chainman, consensus_params, chainlocks, qman)}
{}

CChainstateHelper::~CChainstateHelper() = default;

/** Passthrough functions to chainlock::Chainlocks */
bool CChainstateHelper::HasConflictingChainLock(int nHeight, const uint256& blockHash) const
{
    return m_chainlocks.HasConflictingChainLock(nHeight, blockHash);
}

bool CChainstateHelper::HasChainLock(int nHeight, const uint256& blockHash) const
{
    return m_chainlocks.HasChainLock(nHeight, blockHash);
}

int32_t CChainstateHelper::GetBestChainLockHeight() const { return m_chainlocks.GetBestChainLockHeight(); }

/** Passthrough functions to CInstantSendManager */
std::optional<std::pair</*islock_hash=*/uint256, /*txid=*/uint256>> CChainstateHelper::ConflictingISLockIfAny(
    const CTransaction& tx) const
{
    const auto islock = isman.GetConflictingLock(tx);
    if (!islock) return std::nullopt;
    return std::make_pair(::SerializeHash(*islock), islock->txid);
}

bool CChainstateHelper::IsInstantSendWaitingForTx(const uint256& hash) const { return isman.IsWaitingForTx(hash); }

bool CChainstateHelper::RemoveConflictingISLockByTx(const CTransaction& tx)
{
    const auto islock = isman.GetConflictingLock(tx);
    if (!islock) return false;
    isman.RemoveConflictingLock(::SerializeHash(*islock), *islock);
    return true;
}

bool CChainstateHelper::ShouldInstantSendRejectConflicts() const { return isman.RejectConflictingBlocks(); }

std::unordered_map<uint8_t, int> CChainstateHelper::GetSignalsStage(const CBlockIndex* const pindexPrev)
{
    return ehf_manager->GetSignalsStage(pindexPrev);
}
