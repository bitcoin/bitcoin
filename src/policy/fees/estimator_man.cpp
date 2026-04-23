// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license. See the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <policy/fees/estimator_man.h>

#include <kernel/mempool_entry.h>
#include <kernel/mempool_removal_reason.h>
#include <logging.h>
#include <policy/feerate.h>
#include <policy/fees/block_policy_estimator.h>
#include <policy/fees/mempool_estimator.h>
#include <util/fees.h>

FeeRateEstimatorManager::~FeeRateEstimatorManager() = default;

FeeRateEstimatorManager::FeeRateEstimatorManager(const fs::path& block_policy_path, bool read_stale_estimates, const CTxMemPool* mempool, ChainstateManager* chainman)
    : m_block_policy_estimator(std::make_unique<CBlockPolicyEstimator>(block_policy_path, read_stale_estimates)),
      m_mempool_estimator(std::make_unique<MemPoolFeeRateEstimator>(mempool, chainman))
{
}

util::Expected<FeeRateEstimation, FeeRateEstimationError> FeeRateEstimatorManager::GetFeeRateEstimate(int target, bool conservative) const
{
    auto block_policy_estimate = m_block_policy_estimator->EstimateFeeRate(target, conservative);
    if (!block_policy_estimate) {
        LogDebug(BCLog::ESTIMATEFEE, "%s", block_policy_estimate.error().reason);
        return block_policy_estimate;
    }
    auto mempool_estimate = m_mempool_estimator->EstimateFeeRate(conservative);
    if (!mempool_estimate) {
        // A failed mempool estimate is surfaced as a warning rather than silently returning the
        // block policy estimate, which callers can still request explicitly.
        LogDebug(BCLog::ESTIMATEFEE, "%s", mempool_estimate.error().reason);
        return mempool_estimate;
    }
    auto selected_estimate = std::min(*block_policy_estimate, *mempool_estimate);
    LogDebug(BCLog::ESTIMATEFEE, "Fee rate estimated using %s: target=%s feerate=%s %s/kvB.",
             FeeRateEstimatorTypeToString(selected_estimate.feerate_estimator),
             selected_estimate.returned_target, CFeeRate(selected_estimate.feerate).GetFeePerK(), CURRENCY_ATOM);
    return selected_estimate;
}

util::Expected<FeeRateEstimation, FeeRateEstimationError> FeeRateEstimatorManager::GetFeeRateEstimate(FeeRateEstimatorType type, int target, bool conservative) const
{
    switch (type) {
    case FeeRateEstimatorType::NONE:
        return GetFeeRateEstimate(target, conservative);
    case FeeRateEstimatorType::BLOCK_POLICY:
        return m_block_policy_estimator->EstimateFeeRate(target, conservative);
    case FeeRateEstimatorType::MEMPOOL_POLICY:
        return m_mempool_estimator->EstimateFeeRate(conservative);
    } // no default case, so the compiler can warn about missing cases
    assert(false);
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
    return std::max(m_block_policy_estimator->MaximumTarget(), m_mempool_estimator->MaximumTarget());
}
