#include "uint160.h"

BOOST_AUTO_TEST_SUITE(uint160_tests)

BOOST_AUTO_TEST_CASE(equality)
{
    uint160 num1 = 10;
    uint160 num2 = 10;
    BOOST_CHECK(num1 == num2);
}

BOOST_AUTO_TEST_SUITE_END()
