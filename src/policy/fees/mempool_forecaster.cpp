// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license. See the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <logging.h>
#include <node/miner.h>
#include <policy/fees/forecaster.h>
#include <policy/fees/forecaster_util.h>
#include <policy/fees/mempool_forecaster.h>
#include <policy/policy.h>
#include <validation.h>

ForecastResult MemPoolForecaster::ForecastFeeRate(int target, bool conservative) const
{
    ForecastResult result;
    result.forecaster = m_forecastType;
    LOCK2(cs_main, m_mempool->cs);
    auto activeTip = m_chainstate->m_chainman.ActiveTip();
    if (!activeTip) {
        result.m_error = "No active chainstate available";
        return result;
    }
    result.current_block_height = static_cast<unsigned int>(activeTip->nHeight);

    if (target > MEMPOOL_FORECAST_MAX_TARGET) {
        result.m_error = strprintf("Confirmation target %s exceeds the maximum limit of %s. mempool conditions might change",
                                   target, MEMPOOL_FORECAST_MAX_TARGET);
        return result;
    }

    const auto cached_estimate = cache.get();
    if (cached_estimate) {
        result.feerate = conservative ? cached_estimate->p50 : cached_estimate->p75;
        return result;
    }

    node::BlockAssembler::Options options;
    options.test_block_validity = false;
    node::BlockAssembler assembler(*m_chainstate, m_mempool, options);

    const auto pblocktemplate = assembler.CreateNewBlock();
    const auto& m_package_feerates = pblocktemplate->m_package_feerates;
    const auto percentiles = CalculatePercentiles(m_package_feerates, DEFAULT_BLOCK_MAX_WEIGHT);
    if (percentiles.empty()) {
        result.m_error = "Forecaster unable a fee rate due to insufficient data";
        return result;
    }

    LogDebug(BCLog::MEMPOOL,
             "%s: Block height %s, Block template 25th percentile fee rate: %s %s/kvB, "
             "50th percentile fee rate: %s %s/kvB, 75th percentile fee rate: %s %s/kvB, "
             "95th percentile fee rate: %s %s/kvB\n",
             forecastTypeToString(m_forecastType), result.current_block_height,
             CFeeRate(percentiles.p25.fee, percentiles.p25.size).GetFeePerK(), CURRENCY_ATOM,
             CFeeRate(percentiles.p50.fee, percentiles.p50.size).GetFeePerK(), CURRENCY_ATOM,
             CFeeRate(percentiles.p75.fee, percentiles.p75.size).GetFeePerK(), CURRENCY_ATOM,
             CFeeRate(percentiles.p95.fee, percentiles.p95.size).GetFeePerK(), CURRENCY_ATOM);

    cache.update(percentiles);
    result.feerate = conservative ? percentiles.p50 : percentiles.p75;
    return result;
}
