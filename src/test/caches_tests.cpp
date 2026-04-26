// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit.

#include <node/dbcache.h>
#include <util/byte_units.h>

#include <boost/test/unit_test.hpp>

#include <array>
#include <cstdint>
#include <limits>
#include <utility>

using namespace node;

namespace {
void CheckDbCacheWarnThreshold(uint64_t threshold, uint64_t total_ram)
{
    BOOST_CHECK(!ShouldWarnOversizedDbCache(threshold, total_ram));
    BOOST_CHECK( ShouldWarnOversizedDbCache(threshold + 1, total_ram));
}
} // namespace

BOOST_AUTO_TEST_SUITE(caches_tests)

BOOST_AUTO_TEST_CASE(default_dbcache_formula_by_total_ram)
{
    BOOST_CHECK(FALLBACK_RAM_BYTES >= 1_GiB);
    for (const auto& [total_ram, expected] : std::array<std::pair<uint64_t, uint64_t>, 4>{{
        {512_MiB, MIN_DEFAULT_DBCACHE},
        {1_GiB, MIN_DEFAULT_DBCACHE},
        {RESERVED_RAM - 1, MIN_DEFAULT_DBCACHE},
        {RESERVED_RAM, MIN_DEFAULT_DBCACHE}
    }}) {
        BOOST_CHECK_EQUAL(GetDefaultDBCache(total_ram), expected);
    }

    BOOST_CHECK_EQUAL(GetDefaultDBCache(3_GiB), 256_MiB);

    if constexpr (SIZE_MAX == UINT64_MAX) {
        for (const auto& [total_ram_64, expected] : std::array<std::pair<uint64_t, uint64_t>, 3>{{
            {8_GiB, 1536_MiB},
            {16_GiB, MAX_DEFAULT_DBCACHE},
            {32_GiB, MAX_DEFAULT_DBCACHE}
        }}) {
            BOOST_CHECK_EQUAL(GetDefaultDBCache(total_ram_64), expected);
        }
    }
}

BOOST_AUTO_TEST_CASE(default_dbcache_uses_current_total_ram)
{
    BOOST_CHECK_EQUAL(GetDefaultDBCache(), GetDefaultDBCache(GetTotalRam()));
}

BOOST_AUTO_TEST_CASE(oversized_dbcache_warning)
{
    BOOST_CHECK(!ShouldWarnOversizedDbCache(MIN_DBCACHE_BYTES, 1_GiB));

    // Below RESERVED_RAM the auto default dominates (headroom is zero).
    CheckDbCacheWarnThreshold(GetDefaultDBCache(1_GiB), 1_GiB);
    CheckDbCacheWarnThreshold(GetDefaultDBCache(RESERVED_RAM), RESERVED_RAM);

    // Above RESERVED_RAM the warning fires at 75% of the headroom.
    CheckDbCacheWarnThreshold(((3_GiB - RESERVED_RAM) / 4) * 3, 3_GiB);

    for (const auto total_ram : {8_GiB, 16_GiB, 32_GiB}) {
        CheckDbCacheWarnThreshold(((total_ram - RESERVED_RAM) / 4) * 3, total_ram);
    }
}

BOOST_AUTO_TEST_CASE(default_dbcache_never_warns)
{
    for (const auto total_ram : {1_GiB, 2_GiB, 3_GiB}) {
        BOOST_CHECK(!ShouldWarnOversizedDbCache(GetDefaultDBCache(total_ram), total_ram));
    }

    for (const auto total_ram : {4_GiB, 8_GiB, 16_GiB, 32_GiB}) {
        BOOST_CHECK(!ShouldWarnOversizedDbCache(GetDefaultDBCache(total_ram), total_ram));
    }
}

BOOST_AUTO_TEST_SUITE_END()
