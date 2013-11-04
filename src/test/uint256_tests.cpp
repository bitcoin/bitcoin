#include <boost/test/unit_test.hpp>

#include "uint256.h"
#include <string>

BOOST_AUTO_TEST_SUITE(uint256_tests)

BOOST_AUTO_TEST_CASE(uint256_equality)
{
    uint256 num1 = 10;
    uint256 num2 = 11;
    BOOST_CHECK(num1+1 == num2);

    uint64 num3 = 10;
    BOOST_CHECK(num1 == num3);
    BOOST_CHECK(num1+num2 == num3+num2);
}

BOOST_AUTO_TEST_CASE(uint256_hex)
{
    std::string hexStr = "d35583ed493a5eee756931353144f558e6a9ab3ad6024a63ced7f10daf7faad9";
    uint256 num1;
    num1.SetHex(hexStr);
    BOOST_CHECK(num1.GetHex() == hexStr);
}

BOOST_AUTO_TEST_SUITE_END()
