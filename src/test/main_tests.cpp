#include "core.h"
#include "main.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(main_tests)

BOOST_AUTO_TEST_CASE(subsidy_limit_test)
{
    uint64_t nSum = 0;
    for (int nHeight = 0; nHeight < 7000000; nHeight += 1000) {
         nSum += GetBlockValue(nHeight, 0) * 1000;
         BOOST_CHECK(MoneyRange(nSum));
    }
}

BOOST_AUTO_TEST_SUITE_END()
