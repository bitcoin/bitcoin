// Copyright (c) 2023 The Navcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#define BOOST_UNIT_TEST
#define BLS_ETH 1

#include <boost/test/unit_test.hpp>
#include <test/util/setup_common.h>
#include <blsct/common.h>
#include <streams.h>

BOOST_FIXTURE_TEST_SUITE(common_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(test_cdatasteam_to_vector)
{
    CDataStream st(SER_DISK, PROTOCOL_VERSION);
    uint64_t one = 1;
    uint64_t two = 2;
    uint64_t three = 3;

    st << one << two << three;
    auto vec = blsct::Common::CDataSteamToVector(st);
    size_t unit = sizeof(uint64_t);

    BOOST_CHECK_EQUAL(vec.size(), unit * 3);
    BOOST_CHECK_EQUAL(vec[unit * 0], one);
    BOOST_CHECK_EQUAL(vec[unit * 1], two);
    BOOST_CHECK_EQUAL(vec[unit * 2], three);
}

BOOST_AUTO_TEST_CASE(test_get_first_power_of_2_greater_or_eq_to)
{
    // 2^0 = 1
    BOOST_CHECK(blsct::Common::GetFirstPowerOf2GreaterOrEqTo(0) == 1);
    BOOST_CHECK(blsct::Common::GetFirstPowerOf2GreaterOrEqTo(1) == 1);

    // 2^1 = 2
    BOOST_CHECK(blsct::Common::GetFirstPowerOf2GreaterOrEqTo(2) == 2);

    // 2^2 = 4
    BOOST_CHECK(blsct::Common::GetFirstPowerOf2GreaterOrEqTo(3) == 4);
    BOOST_CHECK(blsct::Common::GetFirstPowerOf2GreaterOrEqTo(4) == 4);

    // 2^3 = 8
    BOOST_CHECK(blsct::Common::GetFirstPowerOf2GreaterOrEqTo(5) == 8);
    BOOST_CHECK(blsct::Common::GetFirstPowerOf2GreaterOrEqTo(6) == 8);
    BOOST_CHECK(blsct::Common::GetFirstPowerOf2GreaterOrEqTo(7) == 8);
    BOOST_CHECK(blsct::Common::GetFirstPowerOf2GreaterOrEqTo(8) == 8);

    // check boundary of 8
    BOOST_CHECK(blsct::Common::GetFirstPowerOf2GreaterOrEqTo(9) == 16);
}

BOOST_AUTO_TEST_SUITE_END()

