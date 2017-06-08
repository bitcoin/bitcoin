#include "omnicore/uint256_extensions.h"

#include "arith_uint256.h"
#include "test/test_bitcoin.h"

#include <boost/test/unit_test.hpp>

#include <stdint.h>
#include <limits>

using namespace mastercore;

BOOST_FIXTURE_TEST_SUITE(omnicore_uint256_extensions_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(uint256_from_uint64_t)
{
    uint64_t number = 103242;

    arith_uint256 a(number);

    BOOST_CHECK_EQUAL(number, a.GetLow64());
}

BOOST_AUTO_TEST_CASE(uint256_add)
{
    uint64_t number_a = 103242;
    uint64_t number_b = 234324;
    uint64_t number_c = number_a + number_b;

    arith_uint256 a(number_a);
    arith_uint256 b(number_b);
    arith_uint256 c = a + b;

    BOOST_CHECK_EQUAL(number_c, c.GetLow64());
}

BOOST_AUTO_TEST_CASE(uint256_sub)
{
    uint64_t number_a = 503242;
    uint64_t number_b = 234324;
    uint64_t number_c = number_a - number_b;

    arith_uint256 a(number_a);
    arith_uint256 b(number_b);
    arith_uint256 c = a - b;

    BOOST_CHECK_EQUAL(number_c, c.GetLow64());
}

BOOST_AUTO_TEST_CASE(uint256_mul)
{
    uint64_t number_a = 503242;
    uint64_t number_b = 234324;
    uint64_t number_c = number_a * number_b;

    arith_uint256 a(number_a);
    arith_uint256 b(number_b);
    arith_uint256 c = a * b;

    BOOST_CHECK_EQUAL(number_c, c.GetLow64());
}

BOOST_AUTO_TEST_CASE(uint256_div)
{
    uint64_t number_a = 9;
    uint64_t number_b = 4;
    uint64_t number_c = number_a / number_b;

    arith_uint256 a(number_a);
    arith_uint256 b(number_b);
    arith_uint256 c = a / b;

    BOOST_CHECK_EQUAL(number_c, c.GetLow64());
}

BOOST_AUTO_TEST_CASE(uint256_conversion)
{
    BOOST_CHECK_EQUAL(0, ConvertTo64(ConvertTo256(0)));
    BOOST_CHECK_EQUAL(1, ConvertTo64(ConvertTo256(1)));
    BOOST_CHECK_EQUAL(1000000000000000000LL, ConvertTo64(ConvertTo256(1000000000000000000LL)));
    BOOST_CHECK_EQUAL(9223372036854775807LL, ConvertTo64(ConvertTo256(9223372036854775807LL)));
}

BOOST_AUTO_TEST_CASE(uint256_modulo)
{
    BOOST_CHECK_EQUAL(1, ConvertTo64(Modulo256(ConvertTo256(9), ConvertTo256(4))));
}

BOOST_AUTO_TEST_CASE(uint256_modulo_auto_conversion)
{
    BOOST_CHECK_EQUAL(2, ConvertTo64(Modulo256(17, 3)));
}

BOOST_AUTO_TEST_CASE(uint256_divide_and_round_up)
{
    BOOST_CHECK_EQUAL(3, ConvertTo64(DivideAndRoundUp(ConvertTo256(5), ConvertTo256(2))));
}

BOOST_AUTO_TEST_CASE(uint256_const)
{
    BOOST_CHECK_EQUAL(1, ConvertTo64(mastercore::uint256_const::one));
    BOOST_CHECK_EQUAL(std::numeric_limits<int64_t>::max(), ConvertTo64(mastercore::uint256_const::max_int64));
}


BOOST_AUTO_TEST_SUITE_END()
