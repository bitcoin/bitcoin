// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license. See the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <policy/fees/mempool_estimator.h>

#include <logging.h>
#include <node/miner.h>
#include <policy/feerate.h>
#include <policy/policy.h>
#include <sync.h>
#include <tinyformat.h>
#include <txmempool.h>
#include <util/check.h>
#include <util/feefrac.h>
#include <util/fees.h>
#include <validation.h>

#include <algorithm>

MemPoolFeeRateEstimator::Percentiles MemPoolFeeRateEstimator::CalculateMaxWeightPercentiles(std::span<const FeePerVSize> chunk_feerates)
{
    Assume(std::is_sorted(chunk_feerates.begin(), chunk_feerates.end(), [](const auto& a, const auto& b) { return ByRatio{a} > ByRatio{b}; }));
    constexpr int64_t total_weight{DEFAULT_BLOCK_MAX_WEIGHT};
    const int64_t p50_weight{total_weight / 2};
    const int64_t p75_weight{total_weight * 3 / 4};
    Percentiles percentiles{};
    int64_t accumulated_weight{0};
    for (const auto& curr_feerate : chunk_feerates) {
        accumulated_weight += int64_t{curr_feerate.size} * WITNESS_SCALE_FACTOR;
        if (accumulated_weight >= p50_weight && percentiles.p50.IsEmpty()) {
            percentiles.p50 = curr_feerate;
        }
        if (accumulated_weight >= p75_weight && percentiles.p75.IsEmpty()) {
            percentiles.p75 = curr_feerate;
            break;
        }
    }
    return percentiles;
}

//! Build the error result for a failed mempool fee rate estimation.
static util::Unexpected<FeeRateEstimationError> EstimationError(std::string error)
{
    return EstimationError(FeeRateEstimatorType::MEMPOOL_POLICY, MEMPOOL_FEE_ESTIMATOR_MAX_TARGET, std::move(error));
}

util::Expected<FeeRateEstimation, FeeRateEstimationError> MemPoolFeeRateEstimator::EstimateFeeRate(bool conservative) const
{
    constexpr auto estimator_type{FeeRateEstimatorType::MEMPOOL_POLICY};
    if (!m_mempool.GetLoadTried()) {
        return EstimationError(strprintf("%s: Mempool not loaded yet, no fee rate estimate available", FeeRateEstimatorTypeToString(estimator_type)));
    }
    node::BlockCreateOptions options;
    options.test_block_validity = false;
    const auto blocktemplate = WITH_LOCK(::cs_main, return (node::BlockAssembler{m_chainman.CurrentChainstate(), &m_mempool, options}).CreateNewBlock());
    if (!blocktemplate) return EstimationError(strprintf("%s: Failed to create block template for fee rate estimation", FeeRateEstimatorTypeToString(estimator_type)));
    // Sort again because the rounding up when converting from weight to vsize may cause slight misorder.
    std::sort(blocktemplate->m_package_feerates.begin(), blocktemplate->m_package_feerates.end(), [](const auto& a, const auto& b) { return ByRatio{a} > ByRatio{b}; });
    const auto percentiles = CalculateMaxWeightPercentiles(blocktemplate->m_package_feerates);
    // Fall back to a relayable floor (the higher of the min relay fee and the current
    // mempool min fee) for any percentile the mempool was too sparse to fill.
    const FeePerVSize floor{std::max(m_mempool.m_opts.min_relay_feerate, m_mempool.GetMinFee()).GetFeePerVSize()};
    const FeePerVSize p50{percentiles.p50.IsEmpty() ? floor : percentiles.p50};
    const FeePerVSize p75{percentiles.p75.IsEmpty() ? floor : percentiles.p75};
    LogDebug(BCLog::ESTIMATEFEE, "%s: conservative/economical fee rate: %s/%s %s/kvB",
             FeeRateEstimatorTypeToString(estimator_type), CFeeRate(p50).GetFeePerK(),
             CFeeRate(p75).GetFeePerK(), CURRENCY_ATOM);
    return FeeRateEstimation{estimator_type, conservative ? p50 : p75, MEMPOOL_FEE_ESTIMATOR_MAX_TARGET};
}
