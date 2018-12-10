// Copyright (c) 2014-2018 Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>

#include "platform/governance-approval-voting.h"
#include "arith_uint256.h"

namespace
{
    struct TestApprovalVotingFixture
    {
        uint256 agent006{ArithToUint256(0xA006)};
        uint256 agent007{ArithToUint256(0xA007)};
        uint256 agent008{ArithToUint256(0xA008)};
    };
}

BOOST_FIXTURE_TEST_SUITE(TestApprovalVoting, TestApprovalVotingFixture)

    BOOST_AUTO_TEST_CASE(EmptyRoundIsEmpty)
    {
        auto threshold = 0;
        auto av = Platform::ApprovalVoting(threshold);

        auto agents = av.CalculateResult();

        BOOST_CHECK(agents.empty());
    }

BOOST_AUTO_TEST_SUITE_END()
