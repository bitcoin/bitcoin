// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_FEES_H
#define BITCOIN_UTIL_FEES_H

#include <attributes.h>
#include <util/expected.h>
#include <util/feefrac.h>

#include <string>
#include <string_view>
#include <utility>

/* Used to determine type of fee estimation requested */
enum class FeeEstimateMode {
    UNSET,        //!< Use default settings based on other criteria
    ECONOMICAL,   //!< Force Fee rate estimator to return non-conservative estimates
    CONSERVATIVE, //!< Force Fee rate estimator to return conservative estimates
};

/* Used to determine the reason a wallet selected a transaction fee rate */
enum class FeeReason {
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
    NONE,
    BLOCK_POLICY,
    MEMPOOL_POLICY,
};

/**
 * @struct FeeRateEstimation
 * A successful fee rate estimate returned by a fee rate estimator.
 */
struct FeeRateEstimation {
    //! This identifies which fee rate estimator is providing this feerate estimate
    FeeRateEstimatorType feerate_estimator;
    //! Fee rate sufficient to confirm a package within target
    FeePerVSize feerate;
    //! The returned target at which the package is likely to confirm within
    int returned_target;
    /**
     * Compare two FeeRateEstimation objects based on fee rate
     * @param other The other FeeRateEstimation object to compare with
     * @return strong ordering of either less, greater or equal; based on feerate ratio comparison.
     */
    auto operator<=>(const FeeRateEstimation& other) const
    {
        return ByRatio{feerate} <=> ByRatio{other.feerate};
    }
};

/**
 * @struct FeeRateEstimationError
 * A failed fee rate estimation, carrying the zero-value estimation that
 * identifies the estimator and target alongside the error message.
 */
struct FeeRateEstimationError {
    FeeRateEstimation estimation;
    std::string reason; // why estimation failed
};

/**
 * Build a fee rate estimation error result: a zero-value estimation
 * identifying the estimator and target, alongside the error message.
 */
inline util::Unexpected<FeeRateEstimationError> EstimationError(FeeRateEstimatorType estimator, int returned_target, std::string error)
{
    return util::Unexpected{FeeRateEstimationError{{estimator, FeePerVSize{0, 0}, returned_target}, std::move(error)}};
}

/**
 * Return the estimation carried by a fee rate estimate result: the
 * successful estimation, or the error's zero-value estimation.
 */
inline const FeeRateEstimation& FeeRateEstimationRef(const util::Expected<FeeRateEstimation, FeeRateEstimationError>& result LIFETIMEBOUND)
{
    return result ? *result : result.error().estimation;
}

std::string FeeRateEstimatorTypeToString(FeeRateEstimatorType feerate_estimator_type);
FeeRateEstimatorType FeeRateEstimatorTypeFromString(std::string_view feerate_estimator_type);

#endif // BITCOIN_UTIL_FEES_H
