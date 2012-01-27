#include "../uint256.h"

BOOST_AUTO_TEST_SUITE(uint160_tests)

BOOST_AUTO_TEST_CASE(equality)
{
    uint160 num1 = 10;
    uint160 num2 = 11;
    BOOST_CHECK(num1+1 == num2);

    uint64 num3 = 10;
    BOOST_CHECK(num1 == num3);
    BOOST_CHECK(num1+num2 == num3+num2);
}

BOOST_AUTO_TEST_SUITE_END()
