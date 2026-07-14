// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit.

#include <node/caches.h>
#include <util/byte_units.h>

#include <boost/test/unit_test.hpp>

#include <cstdint>

using namespace node;

namespace {
void CheckDbCacheWarnThreshold(uint64_t threshold, uint64_t total_ram)
{
    BOOST_CHECK(!ShouldWarnOversizedDbCache(threshold, total_ram));
    BOOST_CHECK( ShouldWarnOversizedDbCache(threshold + 1, total_ram));
}
} // namespace

BOOST_AUTO_TEST_SUITE(caches_tests)

BOOST_AUTO_TEST_CASE(oversized_dbcache_warning)
{
    BOOST_CHECK(!ShouldWarnOversizedDbCache(MIN_DB_CACHE, 1_GiB));

    // Below DBCACHE_WARNING_RESERVED_RAM the existing fixed default dominates.
    CheckDbCacheWarnThreshold(DEFAULT_DB_CACHE, 1_GiB);
    CheckDbCacheWarnThreshold(DEFAULT_DB_CACHE, DBCACHE_WARNING_RESERVED_RAM);

    // Above DBCACHE_WARNING_RESERVED_RAM the warning fires at 75% of the headroom.
    CheckDbCacheWarnThreshold(((3_GiB - DBCACHE_WARNING_RESERVED_RAM) / 4) * 3, 3_GiB);

    for (const auto total_ram : {8_GiB, 16_GiB, 32_GiB}) {
        CheckDbCacheWarnThreshold(((total_ram - DBCACHE_WARNING_RESERVED_RAM) / 4) * 3, total_ram);
    }
}

BOOST_AUTO_TEST_SUITE_END()
