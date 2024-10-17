// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license. See the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_FEE_ESTIMATOR_H
#define BITCOIN_POLICY_FEE_ESTIMATOR_H

#include <policy/fees.h>
#include <policy/forecaster.h>
#include <policy/forecaster_util.h>
#include <util/fs.h>

#include <memory>
#include <optional>
#include <unordered_map>
#include <utility>

class CTxMemPool;
struct ForecastResult;

/** \class FeeEstimator
 * Module for managing and utilising multiple fee rate forecasters to provide fee estimates.
 *
 * The FeeEstimator class allows for the registration of multiple fee rate
 * forecasters.
 */
class FeeEstimator
{
private:
    const CTxMemPool* m_mempool;
    //! Map of all registered forecasters to their shared pointers.
    std::unordered_map<ForecastType, std::unique_ptr<Forecaster>> forecasters;

    //! Given a confirmation target get a fee estimate from Block Policy Estimator
    ForecastResult GetPolicyEstimatorEstimate(int targetBlocks);

public:
    //! Optional unique pointer block Block policy estimator.
    std::optional<std::unique_ptr<CBlockPolicyEstimator>> block_policy_estimator;

    /**
     * Constructor that initialises with a Block policy estimator estimator
     *
     * @param[in] block_policy_estimator_filepath Path to the Block policy estimator estimator dump file.
     * @param[in] read_stale_block_policy_estimates Boolean flag indicating whether to read stale Block policy estimator estimates.
     */
    FeeEstimator(const fs::path& block_policy_estimator_filepath, const bool read_stale_block_policy_estimates, const CTxMemPool* mempool)
        : m_mempool(mempool), block_policy_estimator(std::make_unique<CBlockPolicyEstimator>(block_policy_estimator_filepath, read_stale_block_policy_estimates))
    {
    }

    /**
     * Default constructor that initialises without a Block policy estimator estimator.
     */
    FeeEstimator(const CTxMemPool* mempool) : m_mempool(mempool), block_policy_estimator(std::nullopt) {}

    ~FeeEstimator() = default;

    /**
     * Register a forecaster to provide fee rate estimates.
     *
     * @param[in] forecaster Shared pointer to a Forecaster instance to be registered.
     */
    void RegisterForecaster(std::unique_ptr<Forecaster> forecaster);

    /**
     * Get a fee rate estimate from all registered forecasters for a given target block count.
     *
     * Polls all registered forecasters and selects the lowest fee rate
     * estimate with acceptable confidence.
     *
     * @param[in] targetBlocks The number of blocks within which the transaction should be confirmed.
     * @return A pair consisting of the forecast result and a vector of forecaster names.
     */
    std::pair<ForecastResult, std::vector<std::string>> GetFeeEstimateFromForecasters(int targetBlocks);

    /**
     * Retrieve the maximum target block range across all registered forecasters.
     *
     * @return The maximum number of blocks for which any forecaster can provide a fee rate estimate.
     */
    int MaxForecastingTarget();
};

#endif // BITCOIN_POLICY_FEE_ESTIMATOR_H
