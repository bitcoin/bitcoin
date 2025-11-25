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
#include <utility>

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

bool MemPoolFeeRateEstimatorCache::IsStale() const
{
    return !m_fee_rate_estimation || (m_last_updated + CACHE_LIFE) < NodeClock::now();
}

std::optional<MemPoolFeeRateEstimatorCache::FeeRateEstimate>
MemPoolFeeRateEstimatorCache::GetCachedEstimate(const uint256& tip_hash) const
{
    if (IsStale() || tip_hash != m_tip_hash) return std::nullopt;
    return m_fee_rate_estimation;
}

void MemPoolFeeRateEstimatorCache::Update(FeePerVSize conservative, FeePerVSize economical, const uint256& tip_hash)
{
    m_fee_rate_estimation = {conservative, economical};
    m_tip_hash = tip_hash;
    m_last_updated = NodeClock::now();
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
    // The estimator lock is not held while building a block template, so
    // in a rare edge case concurrent callers may duplicate work.
    //
    // Cached estimates are tagged with the chain tip they were computed on
    // and only served from the cache while that tip is current.
    //
    // The estimate returned directly below may still reflect a tip that went
    // stale during the call; that is an accepted tradeoff of not holding
    // locks across block assembly.
    Chainstate& chainstate{WITH_LOCK(::cs_main, return m_chainman->CurrentChainstate())};
    const uint256 tip_hash{WITH_LOCK(::cs_main, return Assume(chainstate.m_chain.Tip())->GetBlockHash())};
    {
        LOCK(cs);
        const auto cached_estimate = m_cache.GetCachedEstimate(tip_hash);
        if (cached_estimate) {
            const auto cached_feerate{
                conservative ? cached_estimate->m_conservative : cached_estimate->m_economical};
            return FeeRateEstimation{estimator_type, cached_feerate, MEMPOOL_FEE_ESTIMATOR_MAX_TARGET};
        }
    }
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
    WITH_LOCK(cs, m_cache.Update(p50, p75, blocktemplate->block.hashPrevBlock));
    LogDebug(BCLog::ESTIMATEFEE, "%s: conservative/economical fee rate: %s/%s %s/kvB",
             FeeRateEstimatorTypeToString(estimator_type), CFeeRate(p50).GetFeePerK(),
             CFeeRate(p75).GetFeePerK(), CURRENCY_ATOM);
    return FeeRateEstimation{
        estimator_type, conservative ? p50 : p75, MEMPOOL_FEE_ESTIMATOR_MAX_TARGET};
}
