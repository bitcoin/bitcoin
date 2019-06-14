#include "omnicore/convert.h"

#include "test/test_bitcoin.h"

#include <stdint.h>

#include <boost/test/unit_test.hpp>

using namespace mastercore;

BOOST_FIXTURE_TEST_SUITE(omnicore_rounduint64_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(rounduint64_simple)
{
    const int64_t COIN = 100000000;
    double unit_price = 23.45678923999;
    BOOST_CHECK_EQUAL(2345678924U, rounduint64(COIN * unit_price));
}

BOOST_AUTO_TEST_CASE(rounduint64_whole_units)
{
    BOOST_CHECK_EQUAL(0U, rounduint64(0.0));
    BOOST_CHECK_EQUAL(1U, rounduint64(1.0));
    BOOST_CHECK_EQUAL(2U, rounduint64(2.0));
    BOOST_CHECK_EQUAL(3U, rounduint64(3.0));
}

BOOST_AUTO_TEST_CASE(rounduint64_round_below_point_5)
{    
    BOOST_CHECK_EQUAL(0U, rounduint64(0.49999999));
    BOOST_CHECK_EQUAL(1U, rounduint64(1.49999999));
    BOOST_CHECK_EQUAL(2U, rounduint64(2.49999999));
    BOOST_CHECK_EQUAL(3U, rounduint64(3.49999999));
}

BOOST_AUTO_TEST_CASE(rounduint64_round_point_5)
{    
    BOOST_CHECK_EQUAL(1U, rounduint64(0.5));
    BOOST_CHECK_EQUAL(2U, rounduint64(1.5));
    BOOST_CHECK_EQUAL(3U, rounduint64(2.5));
    BOOST_CHECK_EQUAL(4U, rounduint64(3.5));
}

BOOST_AUTO_TEST_CASE(rounduint64_round_over_point_5)
{    
    BOOST_CHECK_EQUAL(1U, rounduint64(0.50000001));
    BOOST_CHECK_EQUAL(2U, rounduint64(1.50000001));
    BOOST_CHECK_EQUAL(3U, rounduint64(2.50000001));
    BOOST_CHECK_EQUAL(4U, rounduint64(3.50000001));
}

BOOST_AUTO_TEST_CASE(rounduint64_limits)
{    
    BOOST_CHECK_EQUAL( // 1 byte signed
            127U,
            rounduint64(127.0));
    BOOST_CHECK_EQUAL( // 1 byte unsigned
            255U,
            rounduint64(255.0));
    BOOST_CHECK_EQUAL( // 2 byte signed
            32767U,
            rounduint64(32767.0));
    BOOST_CHECK_EQUAL( // 2 byte unsigned
            65535U,
            rounduint64(65535.0));
    BOOST_CHECK_EQUAL( // 4 byte signed
            2147483647U,
            rounduint64(2147483647.0));
    BOOST_CHECK_EQUAL( // 4 byte unsigned
            4294967295U,
            rounduint64(4294967295.0L));
    BOOST_CHECK_EQUAL( // 8 byte signed
            9223372036854775807U,
            rounduint64(9223372036854775807.0L));
    BOOST_CHECK_EQUAL( // 8 byte unsigned
            18446744073709551615U,
            rounduint64(18446744073709551615.0L));
}

BOOST_AUTO_TEST_CASE(rounduint64_types)
{
    BOOST_CHECK_EQUAL(
            rounduint64(static_cast<float>(1.23456789)),
            rounduint64(static_cast<float>(1.23456789)));
    BOOST_CHECK_EQUAL(
            rounduint64(static_cast<float>(1.23456789)),
            rounduint64(static_cast<double>(1.23456789)));
    BOOST_CHECK_EQUAL(
            rounduint64(static_cast<float>(1.23456789)),
            rounduint64(static_cast<long double>(1.23456789)));
    BOOST_CHECK_EQUAL(
            rounduint64(static_cast<double>(1.23456789)),
            rounduint64(static_cast<double>(1.23456789)));
    BOOST_CHECK_EQUAL(
            rounduint64(static_cast<double>(1.23456789)),
            rounduint64(static_cast<long double>(1.23456789)));
    BOOST_CHECK_EQUAL(
            rounduint64(static_cast<long double>(1.23456789)),
            rounduint64(static_cast<long double>(1.23456789)));
}

BOOST_AUTO_TEST_CASE(rounduint64_promotion)
{
    BOOST_CHECK_EQUAL(42U, rounduint64(static_cast<int8_t>(42)));
    BOOST_CHECK_EQUAL(42U, rounduint64(static_cast<uint8_t>(42)));
    BOOST_CHECK_EQUAL(42U, rounduint64(static_cast<int16_t>(42)));    
    BOOST_CHECK_EQUAL(42U, rounduint64(static_cast<uint16_t>(42)));
    BOOST_CHECK_EQUAL(42U, rounduint64(static_cast<int32_t>(42)));
    BOOST_CHECK_EQUAL(42U, rounduint64(static_cast<uint32_t>(42)));
    BOOST_CHECK_EQUAL(42U, rounduint64(static_cast<int64_t>(42)));    
    BOOST_CHECK_EQUAL(42U, rounduint64(static_cast<uint64_t>(42)));
    BOOST_CHECK_EQUAL(42U, rounduint64(static_cast<float>(42)));
    BOOST_CHECK_EQUAL(42U, rounduint64(static_cast<double>(42)));
    BOOST_CHECK_EQUAL(42U, rounduint64(static_cast<long double>(42)));
}

BOOST_AUTO_TEST_CASE(rounduint64_absolute)
{
    BOOST_CHECK_EQUAL(
            rounduint64(-128.0f),
            rounduint64(128.0f));
    BOOST_CHECK_EQUAL(
            rounduint64(-32768.0),
            rounduint64(32768.0));
    BOOST_CHECK_EQUAL(
            rounduint64(-65536.0),
            rounduint64(65536.0));
    BOOST_CHECK_EQUAL(
            rounduint64(-2147483648.0L),
            rounduint64(2147483648.0L));
    BOOST_CHECK_EQUAL(            
            rounduint64(-4294967296.0L),
            rounduint64(4294967296.0L));
    BOOST_CHECK_EQUAL(
            rounduint64(-9223372036854775807.0L),
            rounduint64(9223372036854775807.0L));
}

BOOST_AUTO_TEST_CASE(rounduint64_special_cases)
{    
    BOOST_CHECK_EQUAL(0U, rounduint64(static_cast<double>(0.49999999999999994)));
    BOOST_CHECK_EQUAL(2147483648U, rounduint64(static_cast<int32_t>(-2147483647-1)));
}


BOOST_AUTO_TEST_SUITE_END()
