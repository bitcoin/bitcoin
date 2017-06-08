#include "omnicore/parse_string.h"

#include "test/test_bitcoin.h"

#include <boost/test/unit_test.hpp>

#include <stdint.h>
#include <string>

using namespace mastercore;

BOOST_FIXTURE_TEST_SUITE(omnicore_strtoint64_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(strtoint64_invidisible)
{
    // zero amount
    BOOST_CHECK(StrToInt64("0", false) == 0);
    // big num
    BOOST_CHECK(StrToInt64("4000000000000000", false) == static_cast<int64_t>(4000000000000000LL));
    // max int64
    BOOST_CHECK(StrToInt64("9223372036854775807", false) == static_cast<int64_t>(9223372036854775807LL));
}

BOOST_AUTO_TEST_CASE(strtoint64_invidisible_truncate)
{
    // ignore any char after decimal mark
    BOOST_CHECK(StrToInt64("8.76543210123456878901", false) == 8);
    BOOST_CHECK(StrToInt64("8.765432101.2345687890", false) == 8);
    BOOST_CHECK(StrToInt64("2345.AbCdEhf71z1.23", false) == 2345);
}

BOOST_AUTO_TEST_CASE(strtoint64_invidisible_invalid)
{
    // invalid, number is negative
    BOOST_CHECK(StrToInt64("-4", false) == 0);
    // invalid, number is over max int64
    BOOST_CHECK(StrToInt64("9223372036854775808", false) == 0);
    // invalid, minus sign in string
    BOOST_CHECK(StrToInt64("2345.AbCdEFG71z88-1.23", false) == 0);
}

BOOST_AUTO_TEST_CASE(strtoint64_divisible)
{
    // range 0 to max int64
    BOOST_CHECK(StrToInt64("0.000", true) == 0);    
    BOOST_CHECK(StrToInt64("92233720368.54775807", true) == static_cast<int64_t>(9223372036854775807LL));
    // check padding
    BOOST_CHECK(StrToInt64("0.00000004", true) == 4);
    BOOST_CHECK(StrToInt64("0.0000004", true) == 40);
    BOOST_CHECK(StrToInt64("0.0004", true) == 40000);
    BOOST_CHECK(StrToInt64("0.4", true) == 40000000);
    BOOST_CHECK(StrToInt64("4.0", true) == 400000000);    
    // truncate after 8 digits
    BOOST_CHECK(StrToInt64("40.00000000000099", true) == static_cast<int64_t>(4000000000LL));
    BOOST_CHECK(StrToInt64("92233720368.54775807000", true) == static_cast<int64_t>(9223372036854775807LL));
}

BOOST_AUTO_TEST_CASE(strtoint64_divisible_truncate)
{
    // truncate after 8 digits
    BOOST_CHECK(StrToInt64("40.00000000000099", true) == static_cast<int64_t>(4000000000LL));
    BOOST_CHECK(StrToInt64("92233720368.54775807000", true) == static_cast<int64_t>(9223372036854775807LL));
    BOOST_CHECK(StrToInt64("92233720368.54775807000", true) == static_cast<int64_t>(9223372036854775807LL));
}

BOOST_AUTO_TEST_CASE(strtoint64_divisible_invalid)
{
    // invalid, number is over max int64
    BOOST_CHECK(StrToInt64("92233720368.54775808", true) == 0);
    // invalid, more than one decimal mark in string
    BOOST_CHECK(StrToInt64("1234..12345678", true) == 0);
    // invalid, alpha chars in string
    BOOST_CHECK(StrToInt64("1234.12345A", true) == 0);
    // invalid, number is negative
    BOOST_CHECK(StrToInt64("-4.0", true) == 0);
    // invalid, minus sign in string
    BOOST_CHECK(StrToInt64("4.1234-5678", true) == 0);
}


BOOST_AUTO_TEST_SUITE_END()
