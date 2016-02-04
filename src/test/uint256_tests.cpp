#include <boost/test/unit_test.hpp>

#include "uint256.h"

BOOST_AUTO_TEST_SUITE(uint256_tests)

BOOST_AUTO_TEST_CASE(uint256_equality)
{
    uint256 num1 = 10;
    uint256 num2 = 11;
    BOOST_CHECK(num1+1 == num2);

    uint64_t num3 = 10;
    BOOST_CHECK(num1 == num3);
    BOOST_CHECK(num1+num2 == num3+num2);
}

BOOST_AUTO_TEST_SUITE_END()
