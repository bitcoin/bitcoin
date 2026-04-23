// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <common/system_ram.h>
#include <util/byte_units.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(system_ram_tests)

BOOST_AUTO_TEST_CASE(total_ram)
{
    const auto total{TryGetTotalRam()};
    BOOST_REQUIRE(total);
    BOOST_CHECK_GE(*total, 1_GiB);
    BOOST_CHECK_LT(*total, 10'000_GiB); // ~10 TiB memory is unlikely
}

BOOST_AUTO_TEST_SUITE_END()
