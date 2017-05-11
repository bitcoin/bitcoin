// Copyright (c) 2013-2015 The Bitcoin Core developers
// Copyright (c) 2015-2017 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "thinblock.h"
#include "test/test_bitcoin.h"
#include <boost/test/unit_test.hpp>
#include <limits>


BOOST_FIXTURE_TEST_SUITE(thinblock_util_tests, BasicTestingSetup)
BOOST_AUTO_TEST_CASE(TestCBufferedFile)
{
    const double pinf = std::numeric_limits<double>::infinity();
    const double ninf = -pinf;
    const double qnan = std::numeric_limits<double>::quiet_NaN();
    const double snan = std::numeric_limits<double>::signaling_NaN();

    // exercise special values as well to make sure they don't due
    // any overruns etc.
    formatInfoUnit(pinf);
    formatInfoUnit(ninf);
    formatInfoUnit(qnan);
    formatInfoUnit(snan);

    assert(formatInfoUnit(-1001000.0) == "-1.00MB");
    assert(formatInfoUnit(-1001.0) == "-1.00KB");
    assert(formatInfoUnit(-1.0) == "-1.00B");
    assert(formatInfoUnit(0.1) == "0.10B");
    assert(formatInfoUnit(1.0) == "1.00B");
    assert(formatInfoUnit(10.) == "10.00B");
    assert(formatInfoUnit(1e3 + 1e0) == "1.00KB");
    assert(formatInfoUnit(1e6 + 1e2) == "1.00MB");
    assert(formatInfoUnit(1e9 + 1e5) == "1.00GB");
    assert(formatInfoUnit(1e12 + 1e8) == "1.00TB");
    assert(formatInfoUnit(1e15 + 1e11) == "1.00PB");
    assert(formatInfoUnit(1e18 + 1e14) == "1.00EB");
    assert(formatInfoUnit(1e21 + 1e17) == "1000.10EB");
}

BOOST_AUTO_TEST_SUITE_END()
