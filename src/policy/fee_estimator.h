// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license. See the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_FEE_ESTIMATOR_H
#define BITCOIN_POLICY_FEE_ESTIMATOR_H

#include <policy/fees.h>
#include <policy/forecaster_util.h>
#include <util/fs.h>

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>

class CTxMemPool;
class Forecaster;
struct ForecastResult;

/** \class FeeEstimator
 * Module for managing and utilising multiple fee rate forecasters to provide fee estimates.
 *
 * The FeeEstimator class allows for the registration of multiple fee rate
 * forecasters. When an estimate is requested, all registered forecasters are
 * polled, and the best estimate is selected.
 */
class FeeEstimator
{
private:
    CTxMemPool* m_mempool;
    //! Map of all registered forecasters to their shared pointers.
    std::map<ForecastType, std::shared_ptr<Forecaster>> forecasters;

public:
    //! Optional unique ptr block block_policy estimator.
    std::optional<std::unique_ptr<CBlockPolicyEstimator>> block_policy_estimator;

    /**
     * Constructor that initialises with a block_policy estimator
     *
     * @param[in] block_policy_estimator_filepath Path to the block_policy estimator dump file.
     * @param[in] read_stale_block_policy_estimates Boolean flag indicating whether to read stale block_policy estimates.
     */
    FeeEstimator(const fs::path& block_policy_estimator_filepath, const bool read_stale_block_policy_estimates, CTxMemPool* mempool)
        : m_mempool(mempool), block_policy_estimator(std::make_unique<CBlockPolicyEstimator>(block_policy_estimator_filepath, read_stale_block_policy_estimates))
    {
    }

    /**
     * Default constructor that initialises without a block_policy estimator.
     */
    FeeEstimator() : block_policy_estimator(std::nullopt) {}

    ~FeeEstimator() = default;

    /**
     * Register a forecaster to provide fee rate estimates.
     *
     * @param[in] forecaster Shared pointer to a Forecaster instance to be registered.
     */
    void RegisterForecaster(std::shared_ptr<Forecaster> forecaster);

    /**
     * Get a fee rate estimate from all registered forecasters for a given target block count.
     *
     * Polls all registered forecasters and selects the lowest fee rate
     * estimate with acceptable confidence.
     *
     * @param[in] targetBlocks The number of blocks within which the transaction should be confirmed.
     * @return A pair consisting of the forecast result and a vector of forecaster names.
     */
    std::pair<ForecastResult, std::vector<std::string>> GetFeeEstimateFromForecasters(unsigned int targetBlocks);

    /**
     * Retrieve the maximum target block range across all registered forecasters.
     *
     * @return The maximum number of blocks for which any forecaster can provide a fee rate estimate.
     */
    unsigned int MaxForecastingTarget();

private:
    ForecastResult GetPolicyEstimatorEstimate(unsigned int targetBlocks);
};

#endif // BITCOIN_POLICY_FEE_ESTIMATOR_H
