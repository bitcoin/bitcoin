// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license. See the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <logging.h>
#include <node/miner.h>
#include <policy/forecaster.h>
#include <policy/forecaster_util.h>
#include <policy/forecasters/mempool.h>
#include <policy/policy.h>
#include <validation.h>


ForecastResult MemPoolForecaster::EstimateFee(ConfirmationTarget& target)
{
    ForecastResponse response;
    response.forecaster = m_forecastType;
    LOCK2(cs_main, m_mempool->cs);
    auto activeTip = m_chainstate->m_chainman.ActiveTip();
    if (!activeTip) {
        return ForecastResult(response, "No active chainstate available");
    }
    response.current_block_height = static_cast<unsigned int>(activeTip->nHeight);

    if (target.type != ConfirmationTargetType::BLOCKS) {
        return ForecastResult(response, "Forecaster can only provide an estimate for block targets");
    }
    if (target.value > MEMPOOL_FORECAST_MAX_TARGET) {
        return ForecastResult(response, strprintf(
                                            "Confirmation target %s exceeds the maximum limit of %s. mempool conditions might change, "
                                            "making forecasts above %s block may be unreliable",
                                            target.value, MEMPOOL_FORECAST_MAX_TARGET, MEMPOOL_FORECAST_MAX_TARGET));
    }

    const auto cached_estimate = cache.get();
    if (cached_estimate) {
        response.low_priority = cached_estimate->p75;
        response.high_priority = cached_estimate->p50;
        return ForecastResult(response);
    }

    node::BlockAssembler::Options options;
    options.test_block_validity = false;
    node::BlockAssembler assembler(*m_chainstate, m_mempool, options);

    const auto pblocktemplate = assembler.CreateNewBlock(CScript{});
    const auto& feerateHistogram = pblocktemplate->vFeerateHistogram;
    if (feerateHistogram.empty()) {
        return ForecastResult(response, "No enough transactions in the mempool to provide a fee rate forecast");
    }

    const auto percentiles = CalculatePercentiles(feerateHistogram, DEFAULT_BLOCK_MAX_WEIGHT);
    if (percentiles.empty()) {
        return ForecastResult(response, "Forecaster unable to provide an estimate due to insufficient data");
    }

    LogDebug(BCLog::ESTIMATEFEE,
             "FeeEstimation: %s: Block height %s, 25th percentile fee rate: %s %s/kvB, "
             "50th percentile fee rate: %s %s/kvB, 75th percentile fee rate: %s %s/kvB, "
             "95th percentile fee rate: %s %s/kvB\n",
             forecastTypeToString(m_forecastType), response.current_block_height,
             CFeeRate(percentiles.p25.fee, percentiles.p25.size).GetFeePerK(), CURRENCY_ATOM,
             CFeeRate(percentiles.p50.fee, percentiles.p50.size).GetFeePerK(), CURRENCY_ATOM,
             CFeeRate(percentiles.p75.fee, percentiles.p75.size).GetFeePerK(), CURRENCY_ATOM,
             CFeeRate(percentiles.p95.fee, percentiles.p95.size).GetFeePerK(), CURRENCY_ATOM);

    cache.update(percentiles);
    response.low_priority = percentiles.p75;
    response.high_priority = percentiles.p50;

    return ForecastResult(response);
}
