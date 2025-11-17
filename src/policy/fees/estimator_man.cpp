// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license. See the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <kernel/mempool_entry.h>
#include <kernel/mempool_removal_reason.h>
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
    Assume(estimators.contains(FeeRateEstimatorType::BLOCK_POLICY));
    CBlockPolicyEstimator* estimator =
        GetFeeRateEstimator<CBlockPolicyEstimator>(FeeRateEstimatorType::BLOCK_POLICY);
    return estimator->EstimateFeeRate(target, conservative);
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
