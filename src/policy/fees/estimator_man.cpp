// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license. See the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <policy/fees/estimator_man.h>

#include <kernel/mempool_entry.h>
#include <kernel/mempool_removal_reason.h>
#include <policy/feerate.h>
#include <policy/fees/block_policy_estimator.h>
#include <util/fees.h>

FeeRateEstimatorManager::FeeRateEstimatorManager(const fs::path& block_policy_path, bool read_stale_estimates)
    : m_block_policy_estimator(std::make_unique<CBlockPolicyEstimator>(block_policy_path, read_stale_estimates))
{
}

FeeRateEstimatorResult FeeRateEstimatorManager::GetFeeRateEstimate(int target, bool conservative) const
{
    return m_block_policy_estimator->EstimateFeeRate(target, conservative);
}

void FeeRateEstimatorManager::IntervalFlush()
{
    m_block_policy_estimator->FlushFeeEstimates();
}

void FeeRateEstimatorManager::ShutdownFlush()
{
    m_block_policy_estimator->Flush();
}

void FeeRateEstimatorManager::TransactionAddedToMempool(const NewMempoolTransactionInfo& tx, uint64_t /*unused*/)
{
    m_block_policy_estimator->processTransaction(tx);
}

void FeeRateEstimatorManager::TransactionRemovedFromMempool(const CTransactionRef& tx, MemPoolRemovalReason /*unused*/, uint64_t /*unused*/)
{
    m_block_policy_estimator->removeTx(tx->GetHash());
}

void FeeRateEstimatorManager::MempoolTransactionsRemovedForBlock(const std::vector<RemovedMempoolTransactionInfo>& txs_removed_for_block, unsigned int nBlockHeight)
{
    m_block_policy_estimator->processBlock(txs_removed_for_block, nBlockHeight);
}

CFeeRate FeeRateEstimatorManager::BlockPolicyEstimateRawFee(unsigned int target, double threshold, FeeEstimateHorizon horizon, EstimationResult* buckets)
{
    return m_block_policy_estimator->estimateRawFee(target, threshold, horizon, buckets);
}

unsigned int FeeRateEstimatorManager::BlockPolicyHighestTargetTracked(FeeEstimateHorizon horizon)
{
    return m_block_policy_estimator->HighestTargetTracked(horizon);
}

unsigned int FeeRateEstimatorManager::MaximumTarget() const
{
    return m_block_policy_estimator->MaximumTarget();
}
