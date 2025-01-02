// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license. See the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_FEES_FEE_ESTIMATOR_H
#define BITCOIN_POLICY_FEES_FEE_ESTIMATOR_H

#include <util/fs.h>

#include <memory>
#include <optional>
#include <unordered_map>

class CBlockPolicyEstimator;
class Forecaster;
class ForecastResult;

enum class ForecastType;

struct ConfirmationTarget;

/** \class FeeEstimator
 * Module for managing and utilising multiple fee rate forecasters to provide fee estimates.
 *
 * The FeeEstimator class allows for the registration of multiple fee rate
 * forecasters.
 */
class FeeEstimator
{
private:
    //! Map of all registered forecasters to their shared pointers.
    std::unordered_map<ForecastType, std::unique_ptr<Forecaster>> forecasters;

    //! Given a confirmation target get a fee estimate from Block Policy Estimator
    ForecastResult GetPolicyEstimatorEstimate(ConfirmationTarget& target) const;

public:
    //! Optional unique pointer to block Block policy estimator.
    std::optional<std::unique_ptr<CBlockPolicyEstimator>> block_policy_estimator;

    /**
     * Constructor that initialises FeeEstimator with a Block policy estimator estimator
     *
     * @param[in] block_policy_estimator_file_path Path to the Block policy estimator estimator dump file.
     * @param[in] read_stale_block_policy_estimates Boolean flag indicating whether to read stale Block policy estimator estimates.
     */
    FeeEstimator(const fs::path& block_policy_estimator_file_path, const bool read_stale_block_policy_estimates);

    /**
     * Default constructor that initialises without a Block policy estimator estimator.
     */
    FeeEstimator();

    ~FeeEstimator();

    /**
     * Register a forecaster to provide fee rate estimates.
     *
     * @param[in] forecaster unique pointer to a Forecaster instance.
     */
    void RegisterForecaster(std::unique_ptr<Forecaster>&& forecaster);
};

#endif // BITCOIN_POLICY_FEES_FEE_ESTIMATOR_H
