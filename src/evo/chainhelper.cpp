// Copyright (c) 2024-2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <evo/chainhelper.h>

#include <chainparams.h>
#include <evo/specialtxman.h>
#include <instantsend/instantsend.h>
#include <instantsend/lock.h>
#include <llmq/chainlocks.h>
#include <masternode/payments.h>

CChainstateHelper::CChainstateHelper(CCreditPoolManager& cpoolman, CDeterministicMNManager& dmnman,
                                     CMNHFManager& mnhfman, CGovernanceManager& govman, llmq::CInstantSendManager& isman,
                                     llmq::CQuorumBlockProcessor& qblockman, llmq::CQuorumSnapshotManager& qsnapman,
                                     const ChainstateManager& chainman, const Consensus::Params& consensus_params,
                                     const CMasternodeSync& mn_sync, const CSporkManager& sporkman,
                                     const llmq::CChainLocksHandler& clhandler, const llmq::CQuorumManager& qman) :
    isman{isman},
    clhandler{clhandler},
    mn_payments{std::make_unique<CMNPaymentsProcessor>(dmnman, govman, chainman, consensus_params, mn_sync, sporkman)},
    special_tx{std::make_unique<CSpecialTxProcessor>(cpoolman, dmnman, mnhfman, qblockman, qsnapman, chainman,
                                                     consensus_params, clhandler, qman)}
{}

CChainstateHelper::~CChainstateHelper() = default;

/** Passthrough functions to CChainLocksHandler */
bool CChainstateHelper::HasConflictingChainLock(int nHeight, const uint256& blockHash) const
{
    return clhandler.HasConflictingChainLock(nHeight, blockHash);
}

bool CChainstateHelper::HasChainLock(int nHeight, const uint256& blockHash) const
{
    return clhandler.HasChainLock(nHeight, blockHash);
}

int32_t CChainstateHelper::GetBestChainLockHeight() const { return clhandler.GetBestChainLock().getHeight(); }

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

