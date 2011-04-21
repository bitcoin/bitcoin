#include "uint256.h"

BOOST_AUTO_TEST_SUITE(uint256_tests)

BOOST_AUTO_TEST_CASE(equality)
{
    uint256 num1 = 10;
    uint256 num2 = 10;
    BOOST_CHECK(num1 == num2);
}

BOOST_AUTO_TEST_SUITE_END()
