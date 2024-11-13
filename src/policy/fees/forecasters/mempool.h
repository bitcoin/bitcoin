// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license. See the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_FEES_FORECASTERS_MEMPOOL_H
#define BITCOIN_POLICY_FEES_FORECASTERS_MEMPOOL_H


#include <policy/fees/forecaster.h>

class Chainstate;
class CTxMemPool;

struct ConfirmationTarget;

// Fee rate estimates above this confirmation target are not reliable,
// mempool condition might likely change.
constexpr int MEMPOOL_FORECAST_MAX_TARGET{2};

/** \class MemPoolForecaster
 * This fee estimate forecaster estimates the fee rate that a transaction will pay
 * to be included in a block as soon as possible.
 * It uses the unconfirmed transactions in the mempool to generate the next block template
 * that will likely be mined.
 * The percentile fee rate's are computed, and the bottom 25th percentile and 50th percentile fee rate's are returned.
 */
class MemPoolForecaster : public Forecaster
{
public:
    MemPoolForecaster(const CTxMemPool* mempool, Chainstate* chainstate)
        : Forecaster(ForecastType::MEMPOOL_FORECAST), m_mempool(mempool), m_chainstate(chainstate) {};
    ~MemPoolForecaster() = default;

    /**
     * Estimate the fee rate from mempool transactions given a confirmation target.
     * @param[in] target The confirmation target to provide estimate for.
     * @return The forecasted fee rates.
     */
    ForecastResult EstimateFee(ConfirmationTarget& target) override;

private:
    const CTxMemPool* m_mempool;
    Chainstate* m_chainstate;
};
#endif // BITCOIN_POLICY_FEES_FORECASTERS_MEMPOOL_H
