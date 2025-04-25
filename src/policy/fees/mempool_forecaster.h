// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license. See the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_FEES_MEMPOOL_FORECASTER_H
#define BITCOIN_POLICY_FEES_MEMPOOL_FORECASTER_H


#include <logging.h>
#include <policy/fees/forecaster.h>
#include <policy/fees/forecaster_util.h>
#include <sync.h>
#include <uint256.h>
#include <util/time.h>


#include <chrono>

class Chainstate;
class CTxMemPool;

// Fee rate forecasts above this confirmation target are not reliable,
// as mempool conditions are likely to change.
constexpr int MEMPOOL_FORECAST_MAX_TARGET{2};
constexpr std::chrono::seconds CACHE_LIFE{7};

/**
 * CachedMempoolForecast holds a cache of recent fee rate forecasts.
 * We only provide fresh fee rate if the last saved cache ages more than CACHE_LIFE.
 */
struct CachedMempoolForecast {
private:
    mutable Mutex cache_mutex;
    uint256 chain_tip_hash GUARDED_BY(cache_mutex);
    Percentiles fee_rate GUARDED_BY(cache_mutex);
    NodeClock::time_point last_updated GUARDED_BY(cache_mutex){NodeClock::now() - CACHE_LIFE - std::chrono::seconds(1)};

public:
    CachedMempoolForecast() = default;
    CachedMempoolForecast(const CachedMempoolForecast&) = delete;
    CachedMempoolForecast& operator=(const CachedMempoolForecast&) = delete;

    bool isStale() const EXCLUSIVE_LOCKS_REQUIRED(cache_mutex)
    {
        AssertLockHeld(cache_mutex);
        return (last_updated + CACHE_LIFE) < NodeClock::now();
    }

    std::optional<Percentiles> get_cached_forecast() const EXCLUSIVE_LOCKS_REQUIRED(!cache_mutex)
    {
        LOCK(cache_mutex);
        if (isStale()) return std::nullopt;
        LogDebug(BCLog::ESTIMATEFEE, "%s: cache is not stale, using cached value\n", forecastTypeToString(ForecastType::MEMPOOL_FORECAST));
        return fee_rate;
    }

    uint256 get_chain_tip_hash() const EXCLUSIVE_LOCKS_REQUIRED(!cache_mutex)
    {
        LOCK(cache_mutex);
        return chain_tip_hash;
    }

    void update(const Percentiles& new_fee_rate, uint256 current_tip_hash) EXCLUSIVE_LOCKS_REQUIRED(!cache_mutex)
    {
        LOCK(cache_mutex);
        fee_rate = new_fee_rate;
        last_updated = NodeClock::now();
        chain_tip_hash = current_tip_hash;
        LogDebug(BCLog::ESTIMATEFEE, "%s: updated cache\n", forecastTypeToString(ForecastType::MEMPOOL_FORECAST));
    }
};

/** \class MemPoolForecaster
 * @brief Forecasts the fee rate required for a transaction to be included in the next block.
 *
 * This forecaster uses Bitcoin Core's block-building algorithm to generate the block
 * template that will likely be mined from unconfirmed transactions in the mempool. It calculates percentile
 * fee rates from the selected packages of the template: the 75th percentile fee rate is used as the economical
 * forecast, and the 50th fee rate percentile as the conservative forecast.
 */
class MemPoolForecaster : public Forecaster
{
public:
    MemPoolForecaster(const CTxMemPool* mempool, Chainstate* chainstate)
        : Forecaster(ForecastType::MEMPOOL_FORECAST), m_mempool(mempool), m_chainstate(chainstate) {};
    ~MemPoolForecaster() = default;

    /** Overridden from Forecaster. */
    ForecastResult ForecastFeeRate(int target, bool conservative) const override;
    unsigned int MaximumTarget() const override
    {
        return MEMPOOL_FORECAST_MAX_TARGET;
    };

private:
    const CTxMemPool* m_mempool;
    Chainstate* m_chainstate;
    mutable CachedMempoolForecast cache;
};
#endif // BITCOIN_POLICY_FEES_MEMPOOL_FORECASTER_H
