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

MemPoolFeeRateEstimator::Percentiles MemPoolFeeRateEstimator::CalculateMaxWeightPercentiles(const std::vector<FeePerVSize>& chunk_feerates)
{
    Assume(std::is_sorted(chunk_feerates.begin(), chunk_feerates.end(), [](const auto& a, const auto& b) { return ByRatio{a} > ByRatio{b}; }));
    constexpr int32_t total_weight{DEFAULT_BLOCK_MAX_WEIGHT};
    const int32_t p50_weight{total_weight / 2};
    const int32_t p75_weight{total_weight * 3 / 4};
    Percentiles percentiles{};
    int32_t accumulated_weight{0};
    for (const auto& curr_feerate : chunk_feerates) {
        accumulated_weight += curr_feerate.size * WITNESS_SCALE_FACTOR;
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
    if (!m_mempool->GetLoadTried()) {
        return EstimationError(strprintf("%s: Mempool not loaded yet, no fee rate estimate available", FeeRateEstimatorTypeToString(estimator_type)));
    }
    Chainstate& chainstate = WITH_LOCK(::cs_main, return m_chainman->CurrentChainstate());
    node::BlockCreateOptions options;
    options.test_block_validity = false;
    node::BlockAssembler assembler{chainstate, m_mempool, options};
    const auto blocktemplate = assembler.CreateNewBlock();
    auto package_feerates = blocktemplate->m_package_feerates;
    // Sort again because the rounding up when converting from weight to vsize may cause slight misorder.
    std::sort(package_feerates.begin(), package_feerates.end(), [](auto& a, auto& b) { return ByRatio{a} > ByRatio{b}; });
    const auto percentiles = CalculateMaxWeightPercentiles(package_feerates);
    // Fall back to a relayable floor (the higher of the min relay fee and the current
    // mempool min fee) for any percentile the mempool was too sparse to fill.
    const FeePerVSize floor{std::max(m_mempool->m_opts.min_relay_feerate, m_mempool->GetMinFee()).GetFeePerVSize()};
    const FeePerVSize p50{percentiles.p50.IsEmpty() ? floor : percentiles.p50};
    const FeePerVSize p75{percentiles.p75.IsEmpty() ? floor : percentiles.p75};
    LogDebug(BCLog::ESTIMATEFEE, "%s: conservative/economical fee rate: %s/%s %s/kvB",
             FeeRateEstimatorTypeToString(estimator_type), CFeeRate(p50).GetFeePerK(),
             CFeeRate(p75).GetFeePerK(), CURRENCY_ATOM);
    return FeeRateEstimation{estimator_type, conservative ? p50 : p75, MEMPOOL_FEE_ESTIMATOR_MAX_TARGET};
}
