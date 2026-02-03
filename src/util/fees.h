// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_FEES_H
#define BITCOIN_UTIL_FEES_H

#include <uint256.h>
#include <util/feefrac.h>

#include <compare>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

/* Used to determine type of fee estimation requested */
enum class FeeEstimateMode {
    UNSET,        //!< Use default settings based on other criteria
    ECONOMICAL,   //!< Force Fee rate estimator to return non-conservative estimates
    CONSERVATIVE, //!< Force Fee rate estimator to return conservative estimates
};

/* Used to determine the source behind a transaction fee */
enum class FeeSource {
    FEE_RATE_ESTIMATOR,
    MEMPOOL_MIN,
    USER_SPECIFIED,
    FALLBACK,
    REQUIRED,
};

/**
 * @enum FeeRateEstimatorType
 * Identifier for fee rate estimator.
 */
enum class FeeRateEstimatorType {
    BLOCK_POLICY,
    MEMPOOL_POLICY,
};

// Block percentiles fee rate (in sat/vB).
struct Percentiles {
    FeePerVSize p25;
    FeePerVSize p50;
    FeePerVSize p75;
    FeePerVSize p95;
    Percentiles() = default;
    bool IsEmpty() const
    {
        return p25.IsEmpty() && p50.IsEmpty() && p75.IsEmpty() && p95.IsEmpty();
    }
};

/**
 * Calculate the 25th, 50th, 75th, and 95th percentile fee rates from block template chunks,
 * sorted in descending mining-score order. Returns empty Percentiles if the 95th percentile
 * cannot be reached.
 *
 * @param[in] chunk_feerates A vector of block template chunk fee rates sorted by descending mining score.
 * @param[in] total_weight The total weight to use for percentile calculations.
 */
Percentiles CalculatePercentiles(const std::vector<FeePerVSize>& chunk_feerates, int32_t total_weight);

/**
 * @struct FeeRateEstimatorResult
 * Represents the response returned by a fee rate estimator.
 */
struct FeeRateEstimatorResult {
    //! This identifies which fee rate estimator is providing this feerate estimate
    FeeRateEstimatorType feerate_estimator;
    //! Fee rate sufficient to confirm a package within target
    FeePerVSize feerate{0, 0};
    //! The returned target at which the package is likely to confirm within
    int returned_target{0};
    std::vector<std::string> errors{}; // error messages encountered
    /**
     * Compare two FeeRateEstimatorResult objects based on fee rate
     * @param other The other FeeRateEstimatorResult object to compare with
     * @return strong ordering of either less, greater or equal; based on strict feerate comparison.
     */
    auto operator<=>(const FeeRateEstimatorResult& other) const
    {
        if (feerate << other.feerate) return std::strong_ordering::less;
        if (feerate >> other.feerate) return std::strong_ordering::greater;
        return std::strong_ordering::equal;
    }
};

std::string FeeRateEstimatorTypeToString(FeeRateEstimatorType feerate_estimator_type);

#endif // BITCOIN_UTIL_FEES_H
