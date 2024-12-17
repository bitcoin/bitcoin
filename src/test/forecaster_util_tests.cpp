// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include <policy/forecaster_util.h>
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

    // Define fee rate categories with corresponding fee rates in satoshis per kvB
    const FeeFrac superHighFeerate{500 * individual_tx_vsize, individual_tx_vsize}; // Super High fee: 500 sat/kvB
    const FeeFrac highFeerate{100 * individual_tx_vsize, individual_tx_vsize};      // High fee: 100 sat/kvB
    const FeeFrac mediumFeerate{50 * individual_tx_vsize, individual_tx_vsize};     // Medium fee: 50 sat/kvB
    const FeeFrac lowFeerate{10 * individual_tx_vsize, individual_tx_vsize};        // Low fee: 10 sat/kvB

    std::vector<FeeFrac> feerateHistogram;
    feerateHistogram.reserve(package_size);

    // Populate the feerate histogram based on specified index ranges.
    for (int i = 0; i < package_size; ++i) {
        if (i < 3) {
            feerateHistogram.push_back(superHighFeerate); // Super High fee rate for top 3
        } else if (i < 5) {
            feerateHistogram.push_back(highFeerate); // High fee rate for next 2
        } else if (i < 8) {
            feerateHistogram.push_back(mediumFeerate);                                             // Medium fee rate for next 3
            BOOST_CHECK(CalculatePercentiles(feerateHistogram, DEFAULT_BLOCK_MAX_WEIGHT).empty()); // CalculatePercentiles should return empty until reaching the 95th percentile
        } else {
            feerateHistogram.push_back(lowFeerate); // Low fee rate for remaining 2
        }
    }

    // Test percentile calculation on a complete histogram
    {
        const auto percentiles = CalculatePercentiles(feerateHistogram, DEFAULT_BLOCK_MAX_WEIGHT);
        BOOST_CHECK(percentiles.p25 == superHighFeerate);
        BOOST_CHECK(percentiles.p50 == highFeerate);
        BOOST_CHECK(percentiles.p75 == mediumFeerate);
        BOOST_CHECK(percentiles.p95 == lowFeerate);
    }

    // Test that CalculatePercentiles maintains monotonicity across all percentiles
    {
        feerateHistogram[7] = superHighFeerate; // Increase 8th index to a high fee rate
        const auto percentiles = CalculatePercentiles(feerateHistogram, DEFAULT_BLOCK_MAX_WEIGHT);
        BOOST_CHECK(percentiles.p25 == superHighFeerate);
        BOOST_CHECK(percentiles.p50 == highFeerate);
        BOOST_CHECK(percentiles.p75 == mediumFeerate); // Should still reflect the previous medium rate
        BOOST_CHECK(percentiles.p95 == lowFeerate);
    }
}

BOOST_AUTO_TEST_SUITE_END()
