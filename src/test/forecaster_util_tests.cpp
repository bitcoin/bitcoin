// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include <policy/fees/forecaster_util.h>
#include <policy/policy.h>

#include <boost/test/unit_test.hpp>
#include <vector>

BOOST_AUTO_TEST_SUITE(forecaster_util_tests)

BOOST_AUTO_TEST_CASE(calculate_percentile_test)
{
    // Test that CalculatePercentiles returns empty when vector is empty
    BOOST_CHECK(CalculatePercentiles({}, DEFAULT_BLOCK_MAX_WEIGHT).empty());

    const int32_t package_size{10};
    const int32_t individual_tx_vsize = static_cast<int32_t>(DEFAULT_BLOCK_MAX_WEIGHT / WITNESS_SCALE_FACTOR) / package_size;

    const FeeFrac super_high_fee_rate{500 * individual_tx_vsize, individual_tx_vsize}; // Super High fee: 500 sat/kvB
    const FeeFrac high_fee_rate{100 * individual_tx_vsize, individual_tx_vsize};       // High fee: 100 sat/kvB
    const FeeFrac medium_fee_rate{50 * individual_tx_vsize, individual_tx_vsize};      // Medium fee: 50 sat/kvB
    const FeeFrac low_fee_rate{10 * individual_tx_vsize, individual_tx_vsize};         // Low fee: 10 sat/kvB

    std::vector<FeeFrac> package_feerates;
    package_feerates.reserve(package_size);

    // Populate the feerate histogram based on specified index ranges.
    for (int i = 0; i < package_size; ++i) {
        if (i < 3) {
            package_feerates.emplace_back(super_high_fee_rate); // Super High fee rate for top 3
        } else if (i < 5) {
            package_feerates.emplace_back(high_fee_rate); // High fee rate for next 2
        } else if (i < 8) {
            package_feerates.emplace_back(medium_fee_rate);                                          // Medium fee rate for next 3
            BOOST_CHECK(CalculatePercentiles(package_feerates, DEFAULT_BLOCK_MAX_WEIGHT).empty());   // CalculatePercentiles should return empty until reaching the 95th percentile
        } else {
            package_feerates.emplace_back(low_fee_rate); // Low fee rate for remaining 2
        }
    }

    // Test percentile calculation on a complete histogram
    {
        const auto percentiles = CalculatePercentiles(package_feerates, DEFAULT_BLOCK_MAX_WEIGHT);
        BOOST_CHECK(percentiles.p25 == super_high_fee_rate);
        BOOST_CHECK(percentiles.p50 == high_fee_rate);
        BOOST_CHECK(percentiles.p75 == medium_fee_rate);
        BOOST_CHECK(percentiles.p95 == low_fee_rate);
    }

    // Test that CalculatePercentiles maintains monotonicity across all percentiles
    {
        package_feerates[7] = super_high_fee_rate; // Increase 8th index to a high fee rate
        const auto percentiles = CalculatePercentiles(package_feerates, DEFAULT_BLOCK_MAX_WEIGHT);
        BOOST_CHECK(percentiles.p25 == super_high_fee_rate);
        BOOST_CHECK(percentiles.p50 == high_fee_rate);
        BOOST_CHECK(percentiles.p75 == medium_fee_rate); // Should still reflect the previous medium rate
        BOOST_CHECK(percentiles.p95 == low_fee_rate);
    }
}

BOOST_AUTO_TEST_SUITE_END()
