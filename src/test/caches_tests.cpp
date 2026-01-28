// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit.

#include <node/caches.h>
#include <util/byte_units.h>

#include <boost/test/unit_test.hpp>

using namespace node;

BOOST_AUTO_TEST_SUITE(caches_tests)

BOOST_AUTO_TEST_CASE(oversized_dbcache_warning)
{
    // memory restricted setup - cap is DEFAULT_DB_CACHE (450 MiB)
    BOOST_CHECK(!ShouldWarnOversizedDbCache(/*dbcache=*/4_MiB, /*total_ram=*/1_GiB));    // Under cap
    BOOST_CHECK( ShouldWarnOversizedDbCache(/*dbcache=*/512_MiB, /*total_ram=*/1_GiB));  // At cap
    BOOST_CHECK( ShouldWarnOversizedDbCache(/*dbcache=*/1500_MiB, /*total_ram=*/1_GiB)); // Over cap

    // 2 GiB RAM - cap is 75%
    BOOST_CHECK(!ShouldWarnOversizedDbCache(/*dbcache=*/1500_MiB, /*total_ram=*/2_GiB)); // Under cap
    BOOST_CHECK( ShouldWarnOversizedDbCache(/*dbcache=*/1600_MiB, /*total_ram=*/2_GiB)); // Over cap

    if constexpr (SIZE_MAX == UINT64_MAX) {
        // 4 GiB RAM - cap is 75%
        BOOST_CHECK(!ShouldWarnOversizedDbCache(/*dbcache=*/2500_MiB, /*total_ram=*/4_GiB)); // Under cap
        BOOST_CHECK( ShouldWarnOversizedDbCache(/*dbcache=*/3500_MiB, /*total_ram=*/4_GiB)); // Over cap

        // 8 GiB RAM - cap is 75%
        BOOST_CHECK(!ShouldWarnOversizedDbCache(/*dbcache=*/6000_MiB, /*total_ram=*/8_GiB)); // Under cap
        BOOST_CHECK( ShouldWarnOversizedDbCache(/*dbcache=*/7000_MiB, /*total_ram=*/8_GiB)); // Over cap

        // 16 GiB RAM - cap is 75%
        BOOST_CHECK(!ShouldWarnOversizedDbCache(/*dbcache=*/10'000_MiB, /*total_ram=*/16_GiB)); // Under cap
        BOOST_CHECK( ShouldWarnOversizedDbCache(/*dbcache=*/15'000_MiB, /*total_ram=*/16_GiB)); // Over cap

        // 32 GiB RAM - cap is 75%
        BOOST_CHECK(!ShouldWarnOversizedDbCache(/*dbcache=*/20'000_MiB, /*total_ram=*/32_GiB)); // Under cap
        BOOST_CHECK( ShouldWarnOversizedDbCache(/*dbcache=*/30'000_MiB, /*total_ram=*/32_GiB)); // Over cap
    }
}

BOOST_AUTO_TEST_SUITE_END()
