// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//

#include <node/timeoffsets.h>
#include <node/warnings.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

#include <chrono>
#include <vector>

using namespace std::chrono_literals;

static void AddMulti(TimeOffsets& offsets, const std::vector<std::chrono::seconds>& to_add)
{
    for (auto offset : to_add) {
        offsets.Add(offset);
    }
}

BOOST_FIXTURE_TEST_SUITE(timeoffsets_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(timeoffsets)
{
    node::Warnings warnings{};
    TimeOffsets offsets{warnings};
    BOOST_CHECK(offsets.Median() == 0s);

    AddMulti(offsets, {{0s, -1s, -2s, -3s}});
    // median should be zero for < 5 offsets
    BOOST_CHECK(offsets.Median() == 0s);

    offsets.Add(-4s);
    // we now have 5 offsets: [-4, -3, -2, -1, 0]
    BOOST_CHECK(offsets.Median() == -2s);

    AddMulti(offsets, {4, 5s});
    // we now have 9 offsets: [-4, -3, -2, -1, 0, 5, 5, 5, 5]
    BOOST_CHECK(offsets.Median() == 0s);

    AddMulti(offsets, {41, 10s});
    // the TimeOffsets is now at capacity with 50 offsets, oldest offsets is discarded for any additional offset
    BOOST_CHECK(offsets.Median() == 10s);

    AddMulti(offsets, {25, 15s});
    // we now have 25 offsets of 10s followed by 25 offsets of 15s
    BOOST_CHECK(offsets.Median() == 15s);
}

static bool IsWarningRaised(const std::vector<std::chrono::seconds>& check_offsets)
{
    node::Warnings warnings{};
    TimeOffsets offsets{warnings};
    AddMulti(offsets, check_offsets);
    return offsets.WarnIfOutOfSync();
}


BOOST_AUTO_TEST_CASE(timeoffsets_warning)
{
    BOOST_CHECK(IsWarningRaised({{-60min, -40min, -30min, 0min, 10min}}));
    BOOST_CHECK(IsWarningRaised({5, 11min}));

    BOOST_CHECK(!IsWarningRaised({4, 60min}));
    BOOST_CHECK(!IsWarningRaised({100, 3min}));
}


BOOST_AUTO_TEST_SUITE_END()
