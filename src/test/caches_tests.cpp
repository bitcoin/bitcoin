#include <node/caches.h>
#include <util/byte_units.h>

#include <boost/test/unit_test.hpp>

using namespace node;

BOOST_AUTO_TEST_SUITE(caches_tests)

BOOST_AUTO_TEST_CASE(oversized_dbcache_warning)
{
    // memory restricted setup - cap is DEFAULT_DB_CACHE (450 MiB)
    BOOST_CHECK(!ShouldWarnOversizedDbCache(/*dbcache=*/4_MiB, /*total_ram=*/1024_MiB));    // Under cap
    BOOST_CHECK( ShouldWarnOversizedDbCache(/*dbcache=*/512_MiB, /*total_ram=*/1024_MiB));  // At cap
    BOOST_CHECK( ShouldWarnOversizedDbCache(/*dbcache=*/1500_MiB, /*total_ram=*/1024_MiB)); // Over cap

    // 2 GiB RAM - cap is 75%
    BOOST_CHECK(!ShouldWarnOversizedDbCache(/*dbcache=*/1500_MiB, /*total_ram=*/2048_MiB)); // Under cap
    BOOST_CHECK( ShouldWarnOversizedDbCache(/*dbcache=*/1600_MiB, /*total_ram=*/2048_MiB)); // Over cap

    if constexpr (SIZE_MAX == UINT64_MAX) {
        // 4 GiB RAM - cap is 75%
        BOOST_CHECK(!ShouldWarnOversizedDbCache(/*dbcache=*/2500_MiB, /*total_ram=*/4096_MiB)); // Under cap
        BOOST_CHECK( ShouldWarnOversizedDbCache(/*dbcache=*/3500_MiB, /*total_ram=*/4096_MiB)); // Over cap

        // 8 GiB RAM - cap is 75%
        BOOST_CHECK(!ShouldWarnOversizedDbCache(/*dbcache=*/6000_MiB, /*total_ram=*/8192_MiB)); // Under cap
        BOOST_CHECK( ShouldWarnOversizedDbCache(/*dbcache=*/7000_MiB, /*total_ram=*/8192_MiB)); // Over cap

        // 16 GiB RAM - cap is 75%
        BOOST_CHECK(!ShouldWarnOversizedDbCache(/*dbcache=*/10'000_MiB, /*total_ram=*/16384_MiB)); // Under cap
        BOOST_CHECK( ShouldWarnOversizedDbCache(/*dbcache=*/15'000_MiB, /*total_ram=*/16384_MiB)); // Over cap

        // 32 GiB RAM - cap is 75%
        BOOST_CHECK(!ShouldWarnOversizedDbCache(/*dbcache=*/20'000_MiB, /*total_ram=*/32768_MiB)); // Under cap
        BOOST_CHECK( ShouldWarnOversizedDbCache(/*dbcache=*/30'000_MiB, /*total_ram=*/32768_MiB)); // Over cap
    }
}

BOOST_AUTO_TEST_SUITE_END()
