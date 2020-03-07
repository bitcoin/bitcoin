// Copyright (c) 2014-2018 Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>

#include "platform/governance-approval-voting.h"
#include "arith_uint256.h"
#include "utiltime.h"
#include "primitives/transaction.h"

static std::ostream& operator<<(std::ostream& os, uint256 value)
{
    return os << value.ToString();
}

namespace
{
    struct TestApprovalVotingFixture
    {
        uint256 agent006{ArithToUint256(0xA006)};
        uint256 agent007{ArithToUint256(0xA007)};
        uint256 agent008{ArithToUint256(0xA008)};

        CTxIn voter1{CTxIn(COutPoint(ArithToUint256(1), 1 * COIN))};
        CTxIn voter2{CTxIn(COutPoint(ArithToUint256(2), 1 * COIN))};

        TestApprovalVotingFixture()
        {
            SetMockTime(525960000);
        }

        ~TestApprovalVotingFixture()
        {
            SetMockTime(0);
        }

        Platform::Vote VoteFor(uint256 candidate, const CTxIn& voter, int64_t time = GetTime())
        {
            return {
                candidate,
                Platform::VoteValue::yes,
                time,
                voter
            };

        }

        Platform::Vote VoteAgainst(uint256 candidate, const CTxIn& voter, int64_t time = GetTime())
        {
            return {
                candidate,
                Platform::VoteValue::no,
                time,
                voter
            };

        }
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

    BOOST_AUTO_TEST_CASE(VoteForOneAgent)
    {
        auto threshold = 0;
        auto av = Platform::ApprovalVoting(threshold);

        av.RegisterCandidate(agent007);
        av.AcceptVote(VoteFor(agent007, voter1));

        auto agents = av.CalculateResult();
        auto expected = std::vector<uint256>{agent007};

        BOOST_CHECK_EQUAL_COLLECTIONS(agents.begin(), agents.end(), expected.begin(), expected.end());
    }

    BOOST_AUTO_TEST_CASE(VoteForUnregisteredCandidate)
    {
        auto threshold = 0;
        auto av = Platform::ApprovalVoting(threshold);

        av.AcceptVote(VoteFor(agent007, voter1));

        auto agents = av.CalculateResult();

        BOOST_CHECK(agents.empty());
    }

    BOOST_AUTO_TEST_CASE(VoteForTwoAgents)
    {
        auto threshold = 0;
        auto av = Platform::ApprovalVoting(threshold);

        av.RegisterCandidate(agent006);
        av.RegisterCandidate(agent007);
        av.RegisterCandidate(agent008);

        av.AcceptVote(VoteFor(agent007, voter1));
        av.AcceptVote(VoteFor(agent008, voter1));

        auto agents = av.CalculateResult();
        auto expected = std::vector<uint256>{agent007, agent008};

        BOOST_CHECK_EQUAL_COLLECTIONS(agents.begin(), agents.end(), expected.begin(), expected.end());
    }

    BOOST_AUTO_TEST_CASE(VoteForTwoAgentsByTwoVoters)
    {
        auto threshold = 0;
        auto av = Platform::ApprovalVoting(threshold);

        av.RegisterCandidate(agent006);
        av.RegisterCandidate(agent007);
        av.RegisterCandidate(agent008);

        av.AcceptVote(VoteFor(agent007, voter1));
        av.AcceptVote(VoteFor(agent008, voter1));

        av.AcceptVote(VoteFor(agent006, voter2));
        av.AcceptVote(VoteFor(agent007, voter2));

        auto agents = av.CalculateResult();
        auto expected = std::vector<uint256>{agent006, agent007, agent008};

        BOOST_CHECK_EQUAL_COLLECTIONS(agents.begin(), agents.end(), expected.begin(), expected.end());
    }

    BOOST_AUTO_TEST_CASE(VoteForAndAgainst)
    {
        auto threshold = 0;
        auto av = Platform::ApprovalVoting(threshold);

        av.RegisterCandidate(agent006);
        av.RegisterCandidate(agent007);
        av.RegisterCandidate(agent008);

        av.AcceptVote(VoteFor(agent007, voter1));
        av.AcceptVote(VoteFor(agent008, voter1));

        av.AcceptVote(VoteAgainst(agent006, voter2));
        av.AcceptVote(VoteAgainst(agent007, voter2));

        auto agents = av.CalculateResult();
        auto expected = std::vector<uint256>{agent008};

        BOOST_CHECK_EQUAL_COLLECTIONS(agents.begin(), agents.end(), expected.begin(), expected.end());
    }

    BOOST_AUTO_TEST_CASE(NonZeroThreshold)
    {
        auto threshold = 1;
        auto av = Platform::ApprovalVoting(threshold);

        av.RegisterCandidate(agent006);
        av.RegisterCandidate(agent007);
        av.RegisterCandidate(agent008);

        av.AcceptVote(VoteFor(agent007, voter1));
        av.AcceptVote(VoteFor(agent008, voter1));

        av.AcceptVote(VoteFor(agent006, voter2));
        av.AcceptVote(VoteFor(agent007, voter2));

        auto agents = av.CalculateResult(); // 006 && 008 have 1 vote, 007 has two votes
        auto expected = std::vector<uint256>{agent007};

        BOOST_CHECK_EQUAL_COLLECTIONS(agents.begin(), agents.end(), expected.begin(), expected.end());
    }

    BOOST_AUTO_TEST_CASE(DoubleVoting)
    {
        auto threshold = 1;
        auto av = Platform::ApprovalVoting(threshold);

        av.RegisterCandidate(agent007);
        av.AcceptVote(VoteFor(agent007, voter1));
        av.AcceptVote(VoteFor(agent007, voter1));

        auto agents = av.CalculateResult();

        BOOST_CHECK(agents.empty());
    }

    BOOST_AUTO_TEST_CASE(ChangeThreshold)
    {
        auto threshold = 0;
        auto av = Platform::ApprovalVoting(threshold);

        av.RegisterCandidate(agent007);
        av.AcceptVote(VoteFor(agent007, voter1));

        BOOST_REQUIRE(!av.CalculateResult().empty());
        av.SetThreshold(1);

        BOOST_CHECK(av.CalculateResult().empty());
    }

    BOOST_AUTO_TEST_CASE(ChangeThresholdFiresNotification)
    {
        auto threshold = 0;
        auto av = Platform::ApprovalVoting(threshold);

        auto fired = false;
        auto OnChanged = [&fired](){ fired = true; };
        av.NotifyResultChange(OnChanged);

        av.SetThreshold(1);

        BOOST_CHECK(fired);
    }

    BOOST_AUTO_TEST_CASE(AcceptVoteFiresNotification)
    {
        auto threshold = 0;
        auto av = Platform::ApprovalVoting(threshold);

        auto fired = false;
        auto OnChanged = [&fired](){ fired = true; };
        av.NotifyResultChange(OnChanged);

        av.RegisterCandidate(agent007);
        av.AcceptVote(VoteFor(agent007, voter1));

        BOOST_CHECK(fired);
    }

    BOOST_AUTO_TEST_CASE(RejectVoteDoesntFireNotification)
    {
        auto threshold = 0;
        auto av = Platform::ApprovalVoting(threshold);

        auto fired = false;
        auto OnChanged = [&fired](){ fired = true; };
        av.NotifyResultChange(OnChanged);

        av.AcceptVote(VoteFor(agent007, voter1));

        BOOST_CHECK(!fired);
    }

BOOST_AUTO_TEST_SUITE_END()
