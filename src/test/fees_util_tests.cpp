// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/fees.h>

#include <test/util/framework.h>

BOOST_AUTO_TEST_SUITE(fees_util_tests)

BOOST_AUTO_TEST_CASE(fee_rate_estimator_type_to_string)
{
    BOOST_CHECK_EQUAL(FeeRateEstimatorTypeToString(FeeRateEstimatorType::NONE), "none");
    BOOST_CHECK_EQUAL(FeeRateEstimatorTypeToString(FeeRateEstimatorType::BLOCK_POLICY), "block_policy");
    BOOST_CHECK_EQUAL(FeeRateEstimatorTypeToString(FeeRateEstimatorType::MEMPOOL_POLICY), "mempool_policy");
}

BOOST_AUTO_TEST_CASE(fee_rate_estimator_type_from_string)
{
    BOOST_CHECK(FeeRateEstimatorTypeFromString("none") == FeeRateEstimatorType::NONE);
    BOOST_CHECK(FeeRateEstimatorTypeFromString("block_policy") == FeeRateEstimatorType::BLOCK_POLICY);
    BOOST_CHECK(FeeRateEstimatorTypeFromString("mempool_policy") == FeeRateEstimatorType::MEMPOOL_POLICY);
    BOOST_CHECK(FeeRateEstimatorTypeFromString("unknown") == FeeRateEstimatorType::NONE);
}

BOOST_AUTO_TEST_SUITE_END()
