// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_FEES_H
#define BITCOIN_UTIL_FEES_H

#include <util/feefrac.h>

#include <string>
#include <vector>

/* Used to determine type of fee estimation requested */
enum class FeeEstimateMode {
    UNSET,        //!< Use default settings based on other criteria
    ECONOMICAL,   //!< Force FeeRateEstimator to use non-conservative estimates
    CONSERVATIVE, //!< Force FeeRateEstimator to use conservative estimates
};

enum class FeeSource {
    FEE_RATE_ESTIMATOR,
    MEMPOOL_MIN,
    PAYTXFEE,
    FALLBACK,
    REQUIRED,
};

/**
 * @enum FeeRateEstimatorType
 * Identifier for fee rate estimator.
 */
enum class FeeRateEstimatorType {
    BLOCK_POLICY,
};

/**
 * @struct FeeRateEstimatorResult
 * Represents the response returned by a fee rate estimator.
 */
struct FeeRateEstimatorResult {
    //! This identifies which fee rate estimator is providing this feerate estimate
    FeeRateEstimatorType feerate_estimator;

    //! Fee rate sufficient to confirm a package within target
    FeePerVSize feerate{0, 0};

    //! The block height at which the fee rate estimator was made
    unsigned int current_block_height{0};

    //! The returned target at which the package is likely to confirm within
    int returned_target{0};

    std::vector<std::string> errors{}; // error messages encountered

    /**
     * Compare two FeeRateEstimatorResult objects based on fee rate
     * @param other The other FeeRateEstimatorResult object to compare with
     * @return true if the current object's fee rate is less than the other, false otherwise.
     */
    bool operator<(const FeeRateEstimatorResult& other) const
    {
        return feerate << other.feerate;
    }
};

#endif // BITCOIN_UTIL_FEES_H
