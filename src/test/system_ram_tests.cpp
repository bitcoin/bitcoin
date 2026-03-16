// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <common/system.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

#include <cstdint>
#include <optional>

BOOST_AUTO_TEST_SUITE(system_ram_tests)

BOOST_AUTO_TEST_CASE(total_ram)
{
    const auto total{GetTotalRAM()};
    if (!total) {
        BOOST_WARN_MESSAGE(false, "skipping total_ram: total RAM unknown");
        return;
    }

    BOOST_CHECK_GE(*total, 1000_MiB);

    if constexpr (SIZE_MAX == UINT64_MAX) {
        // Upper bound check only on 64-bit: 32-bit systems can reasonably have max memory,
        // but extremely large values on 64-bit likely indicate detection errors
        BOOST_CHECK_LT(*total, 10'000'000_MiB); // >10 TiB memory is unlikely
    }
}

BOOST_AUTO_TEST_SUITE_END()
