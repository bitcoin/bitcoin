// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license. See the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <kernel/mempool_entry.h>
#include <kernel/mempool_removal_reason.h>
#include <logging.h>
#include <policy/feerate.h>
#include <policy/fees/block_policy_estimator.h>
#include <policy/fees/estimator.h>
#include <policy/fees/estimator_man.h>
#include <util/fees.h>


void FeeRateEstimatorManager::RegisterFeeRateEstimator(std::unique_ptr<FeeRateEstimator> estimator)
{
    auto type = estimator->GetFeeRateEstimatorType();
    estimators.emplace(type, std::move(estimator));
}

FeeRateEstimatorResult FeeRateEstimatorManager::GetFeeRateEstimate(int target, bool conservative)
{
    FeeRateEstimatorResult selected_estimate;
    std::vector<std::string> errors;

    for (const auto& [type, estimate_ptr] : estimators) {
        auto current_estimate = estimate_ptr->EstimateFeeRate(target, conservative);
        errors.insert(errors.end(), current_estimate.errors.begin(), current_estimate.errors.end());
        if (current_estimate.feerate.IsEmpty()) {
            // Handle case where block policy estimator does not have enough data yet.
            if (type == FeeRateEstimatorType::BLOCK_POLICY) {
                return current_estimate;
            } else {
                continue;
            }
        }

        if (selected_estimate.feerate.IsEmpty()) {
            // If there's no selected fee rate estimate, choose current_estimate as the fee rate estimate.
            selected_estimate = current_estimate;
        } else {
            // Otherwise, choose the smaller as estimate.
            selected_estimate = std::min(selected_estimate, current_estimate);
        }
    }

    if (!selected_estimate.feerate.IsEmpty()) {
        LogDebug(BCLog::ESTIMATEFEE, "Fee rate estimated using %s: for target %s, current height %s, fee rate %s %s/kvB.\n",
                 FeeRateEstimatorTypeToString(selected_estimate.feerate_estimator),
                 selected_estimate.returned_target,
                 selected_estimate.current_block_height,
                 CFeeRate(selected_estimate.feerate.fee, selected_estimate.feerate.size).GetFeePerK(),
                 CURRENCY_ATOM);
    }
    selected_estimate.errors = errors;
    return selected_estimate;
}

void FeeRateEstimatorManager::IntervalFlush()
{
    if (estimators.contains(FeeRateEstimatorType::BLOCK_POLICY)) {
        GetFeeRateEstimator<CBlockPolicyEstimator>(FeeRateEstimatorType::BLOCK_POLICY)->FlushFeeEstimates();
    }
}

void FeeRateEstimatorManager::ShutdownFlush()
{
    if (estimators.contains(FeeRateEstimatorType::BLOCK_POLICY)) {
        GetFeeRateEstimator<CBlockPolicyEstimator>(FeeRateEstimatorType::BLOCK_POLICY)->Flush();
    }
}

void FeeRateEstimatorManager::TransactionAddedToMempool(const NewMempoolTransactionInfo& tx, uint64_t /*unused*/)
{
    if (estimators.contains(FeeRateEstimatorType::BLOCK_POLICY)) {
        GetFeeRateEstimator<CBlockPolicyEstimator>(FeeRateEstimatorType::BLOCK_POLICY)->processTransaction(tx);
    }
}

void FeeRateEstimatorManager::TransactionRemovedFromMempool(const CTransactionRef& tx, MemPoolRemovalReason /*unused*/, uint64_t /*unused*/)
{
    if (estimators.contains(FeeRateEstimatorType::BLOCK_POLICY)) {
        GetFeeRateEstimator<CBlockPolicyEstimator>(FeeRateEstimatorType::BLOCK_POLICY)->removeTx(tx->GetHash());
    }
}

void FeeRateEstimatorManager::MempoolTransactionsRemovedForBlock(const std::vector<RemovedMempoolTransactionInfo>& txs_removed_for_block, unsigned int nBlockHeight)
{
    if (estimators.contains(FeeRateEstimatorType::BLOCK_POLICY)) {
        GetFeeRateEstimator<CBlockPolicyEstimator>(FeeRateEstimatorType::BLOCK_POLICY)->processBlock(txs_removed_for_block, nBlockHeight);
    }
}

CFeeRate FeeRateEstimatorManager::BlockPolicyEstimateRawFee(unsigned int target, double threshold, FeeEstimateHorizon horizon, EstimationResult* buckets)
{
    Assume(estimators.contains(FeeRateEstimatorType::BLOCK_POLICY));
    return GetFeeRateEstimator<CBlockPolicyEstimator>(FeeRateEstimatorType::BLOCK_POLICY)->estimateRawFee(target, threshold, horizon, buckets);
}

unsigned int FeeRateEstimatorManager::BlockPolicyHighestTargetTracked(FeeEstimateHorizon horizon)
{
    Assume(estimators.contains(FeeRateEstimatorType::BLOCK_POLICY));
    return GetFeeRateEstimator<CBlockPolicyEstimator>(FeeRateEstimatorType::BLOCK_POLICY)->HighestTargetTracked(horizon);
}

unsigned int FeeRateEstimatorManager::MaximumTarget() const
{
    unsigned int maximum_target{0};
    for (const auto& [_, feerate_estimator] : estimators) {
        maximum_target = std::max(maximum_target, feerate_estimator->MaximumTarget());
    }
    return maximum_target;
}

FeeRateEstimatorManager::~FeeRateEstimatorManager() = default;
