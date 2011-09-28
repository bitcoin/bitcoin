#include <vector>
#include <boost/test/unit_test.hpp>
#include <boost/foreach.hpp>

#include "../util.h"

using namespace std;

BOOST_AUTO_TEST_SUITE(util_tests)

BOOST_AUTO_TEST_CASE(util_MedianFilter)
{    
    CMedianFilter<int> filter(5, 15);

    BOOST_CHECK(filter.median() == 15);

    filter.input(20); // [15 20]
    BOOST_CHECK(filter.median() == 17);

    filter.input(30); // [15 20 30]
    BOOST_CHECK(filter.median() == 20);

    filter.input(3); // [3 15 20 30]
    BOOST_CHECK(filter.median() == 17);

    filter.input(7); // [3 7 15 20 30]
    BOOST_CHECK(filter.median() == 15);

    filter.input(18); // [3 7 18 20 30]
    BOOST_CHECK(filter.median() == 18);

    filter.input(0); // [0 3 7 18 30]
    BOOST_CHECK(filter.median() == 7);
}

BOOST_AUTO_TEST_SUITE_END()
