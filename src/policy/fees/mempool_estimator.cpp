// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license. See the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <logging.h>
#include <node/miner.h>
#include <policy/fees/estimator.h>
#include <policy/fees/mempool_estimator.h>
#include <policy/policy.h>
#include <util/fees.h>
#include <validation.h>

bool MemPoolFeeRateEstimatorCache::IsStale() const
{
    return (last_updated + CACHE_LIFE) < NodeClock::now();
}

std::optional<Percentiles> MemPoolFeeRateEstimatorCache::GetCachedEstimate() const
{
    if (IsStale()) return std::nullopt;
    return estimates;
}

uint256 MemPoolFeeRateEstimatorCache::GetChainTipHash() const
{
    return chain_tip_hash;
}

void MemPoolFeeRateEstimatorCache::Update(const Percentiles& new_estimates, const uint256& current_tip_hash)
{
    estimates = new_estimates;
    last_updated = NodeClock::now();
    chain_tip_hash = current_tip_hash;
}

FeeRateEstimatorResult MemPoolFeeRateEstimator::EstimateFeeRate(int target, bool conservative) const
{
    LOCK(cs);
    Assume(m_mempool);
    Assume(m_chainman);
    FeeRateEstimatorResult result;
    result.feerate_estimator = GetFeeRateEstimatorType();
    Chainstate& chainstate = WITH_LOCK(::cs_main, return m_chainman->CurrentChainstate());
    int current_height = WITH_LOCK(::cs_main, return chainstate.m_chain.Height());
    Assume(current_height > -1);
    uint256 tip_hash = WITH_LOCK(::cs_main, return *Assume(chainstate.m_chain.Tip())->phashBlock);
    result.current_block_height = static_cast<unsigned int>(current_height);
    const auto cached_estimate = cache.GetCachedEstimate();
    const auto known_chain_tip_hash = cache.GetChainTipHash();
    if (cached_estimate && tip_hash == known_chain_tip_hash) {
        result.returned_target = MEMPOOL_FEE_ESTIMATOR_MAX_TARGET;
        result.feerate = conservative ? cached_estimate->p50 : cached_estimate->p75;
        return result;
    }

    node::BlockAssembler::Options options;
    options.test_block_validity = false;
    node::BlockAssembler assembler{chainstate, m_mempool, options};
    const auto blocktemplate = assembler.CreateNewBlock();
    const auto& m_package_feerates = blocktemplate->m_package_feerates;
    const auto percentiles = CalculatePercentiles(m_package_feerates, DEFAULT_BLOCK_MAX_WEIGHT);
    if (percentiles.empty()) {
        result.errors.emplace_back(strprintf("%s: Unable to provide a fee rate due to insufficient data", FeeRateEstimatorTypeToString(result.feerate_estimator)));
        return result;
    }

    LogDebug(BCLog::ESTIMATEFEE,
             "%s: Block height %s, Block template 25th percentile fee rate: %s %s/kvB, "
             "50th percentile fee rate: %s %s/kvB, 75th percentile fee rate: %s %s/kvB, "
             "95th percentile fee rate: %s %s/kvB\n",
             FeeRateEstimatorTypeToString(result.feerate_estimator), result.current_block_height,
             CFeeRate(percentiles.p25.fee, percentiles.p25.size).GetFeePerK(), CURRENCY_ATOM,
             CFeeRate(percentiles.p50.fee, percentiles.p50.size).GetFeePerK(), CURRENCY_ATOM,
             CFeeRate(percentiles.p75.fee, percentiles.p75.size).GetFeePerK(), CURRENCY_ATOM,
             CFeeRate(percentiles.p95.fee, percentiles.p95.size).GetFeePerK(), CURRENCY_ATOM);

    cache.Update(percentiles, tip_hash);
    result.feerate = conservative ? percentiles.p50 : percentiles.p75;
    result.returned_target = MEMPOOL_FEE_ESTIMATOR_MAX_TARGET;
    return result;
}
