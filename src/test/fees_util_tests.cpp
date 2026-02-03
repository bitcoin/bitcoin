// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/fees.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(fees_util_tests)

BOOST_AUTO_TEST_CASE(fee_rate_estimator_type_to_string)
{
    BOOST_CHECK_EQUAL(FeeRateEstimatorTypeToString(FeeRateEstimatorType::NONE), "None");
    BOOST_CHECK_EQUAL(FeeRateEstimatorTypeToString(FeeRateEstimatorType::BLOCK_POLICY), "Block Policy Fee Rate Estimator");
    BOOST_CHECK_EQUAL(FeeRateEstimatorTypeToString(FeeRateEstimatorType::MEMPOOL_POLICY), "Mempool Fee Rate Estimator");
}

BOOST_AUTO_TEST_CASE(fee_rate_estimator_type_from_string)
{
    BOOST_CHECK(FeeRateEstimatorTypeFromString("none") == FeeRateEstimatorType::NONE);
    BOOST_CHECK(FeeRateEstimatorTypeFromString("block_policy") == FeeRateEstimatorType::BLOCK_POLICY);
    BOOST_CHECK(FeeRateEstimatorTypeFromString("mempool_policy") == FeeRateEstimatorType::NONE);
}

BOOST_AUTO_TEST_SUITE_END()
