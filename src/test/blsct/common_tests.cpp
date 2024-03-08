// Copyright (c) 2023 The Navcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#define BOOST_UNIT_TEST

#include <boost/test/unit_test.hpp>
#include <test/util/setup_common.h>
#include <blsct/common.h>
#include <streams.h>

BOOST_FIXTURE_TEST_SUITE(common_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(test_cdatastream_to_vector)
{
    DataStream st{};
    uint64_t one = 1;
    uint64_t two = 2;
    uint64_t three = 3;

    st << one << two << three;
    auto vec = blsct::Common::DataStreamToVector(st);
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

BOOST_AUTO_TEST_CASE(test_trim_preceeding_zeros)
{
    {
        std::vector<bool> vec {};
        auto act = blsct::Common::TrimPreceedingZeros<bool>(vec);
        std::vector<bool> exp {};
        BOOST_CHECK(exp == act);
    }
    {
        std::vector<uint8_t> vec {};
        auto act = blsct::Common::TrimPreceedingZeros<uint8_t>(vec);
        std::vector<uint8_t> exp {};
        BOOST_CHECK(exp == act);
    }
    {
        std::vector<bool> vec { false };
        auto act = blsct::Common::TrimPreceedingZeros<bool>(vec);
        std::vector<bool> exp {};
        BOOST_CHECK(exp == act);
    }
    {
        std::vector<uint8_t> vec { 0 };
        auto act = blsct::Common::TrimPreceedingZeros<uint8_t>(vec);
        std::vector<uint8_t> exp {};
        BOOST_CHECK(exp == act);
    }
    // one should remain one
    {
        std::vector<bool> vec { true };
        auto act = blsct::Common::TrimPreceedingZeros<bool>(vec);
        std::vector<bool> exp { true };
        BOOST_CHECK(exp == act);
    }
    {
        std::vector<uint8_t> vec { 1 };
        auto act = blsct::Common::TrimPreceedingZeros<uint8_t>(vec);
        std::vector<uint8_t> exp { 1 };
        BOOST_CHECK(exp == act);
    }
    // preceding zero should be removed
    {
        std::vector<bool> vec { false, true };
        auto act = blsct::Common::TrimPreceedingZeros<bool>(vec);
        std::vector<bool> exp { true };
        BOOST_CHECK(exp == act);
    }
    {
        std::vector<uint8_t> vec { 0, 1 };
        auto act = blsct::Common::TrimPreceedingZeros<uint8_t>(vec);
        std::vector<uint8_t> exp { 1 };
        BOOST_CHECK(exp == act);
    }
    // multiple preceding zeroes should be removed
    {
        std::vector<bool> vec { false, false, true };
        auto act = blsct::Common::TrimPreceedingZeros<bool>(vec);
        std::vector<bool> exp { true };
        BOOST_CHECK(exp == act);
    }
    {
        std::vector<uint8_t> vec { 0, 0, 1 };
        auto act = blsct::Common::TrimPreceedingZeros<uint8_t>(vec);
        std::vector<uint8_t> exp { 1 };
        BOOST_CHECK(exp == act);
    }
    // zero after first one should be preserved
    {
        std::vector<bool> vec { false, true, false };
        auto act = blsct::Common::TrimPreceedingZeros<bool>(vec);
        std::vector<bool> exp { true, false };
        BOOST_CHECK(exp == act);
    }
    {
        std::vector<uint8_t> vec { 0, 1, 0 };
        auto act = blsct::Common::TrimPreceedingZeros<uint8_t>(vec);
        std::vector<uint8_t> exp { 1, 0 };
        BOOST_CHECK(exp == act);
    }
}

BOOST_AUTO_TEST_CASE(test_add_zero_if_empty)
{
    {
        std::vector<uint8_t> act { 0 };
        blsct::Common::AddZeroIfEmpty<uint8_t>(act);
        std::vector<uint8_t> exp { 0 };
        BOOST_CHECK(exp == act);
    }
    {
        std::vector<uint8_t> act { 1 };
        blsct::Common::AddZeroIfEmpty<uint8_t>(act);
        std::vector<uint8_t> exp { 1 };
        BOOST_CHECK(exp == act);
    }
    {
        std::vector<uint8_t> act {};
        blsct::Common::AddZeroIfEmpty<uint8_t>(act);
        std::vector<uint8_t> exp { 0 };
        BOOST_CHECK(exp == act);
    }
    {
        std::vector<bool> act { false };
        blsct::Common::AddZeroIfEmpty<bool>(act);
        std::vector<bool> exp { false };
        BOOST_CHECK(exp == act);
    }
    {
        std::vector<bool> act { true };
        blsct::Common::AddZeroIfEmpty<bool>(act);
        std::vector<bool> exp { true };
        BOOST_CHECK(exp == act);
    }
    {
        std::vector<bool> act {};
        blsct::Common::AddZeroIfEmpty<bool>(act);
        std::vector<bool> exp { false };
        BOOST_CHECK(exp == act);
    }
}

BOOST_AUTO_TEST_SUITE_END()

