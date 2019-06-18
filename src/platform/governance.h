// Copyright (c) 2014-2018 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_PLATFORM_GOVERNANCE_H
#define CROWN_PLATFORM_GOVERNANCE_H

#include "primitives/transaction.h"
#include "uint256.h"

#include <boost/function.hpp>

#include <utility>
#include <vector>

class CKey;
class CPubKey;

namespace Platform
{
    enum class VoteValue { abstain = 0, yes, no };

    class Vote
    {
    public:
        Vote(
            const uint256& candidate,
            VoteValue value,
            int64_t time,
            CTxIn voterId
        )
            : m_voterId(std::move(voterId)), m_candidate(candidate), m_value(value), m_time(time)
        {}


        Vote() = default;
        Vote(const Vote&) = default;
        Vote& operator=(const Vote&) = default;
        Vote(Vote&&) = default;
        Vote& operator=(Vote&&) = default;

        CTxIn VoterId() const { return m_voterId; }
        uint256 Candidate() const { return m_candidate; }
        VoteValue Value() const { return m_value; }
        int64_t Time() const { return m_time; }

    private:
        CTxIn m_voterId;
        uint256 m_candidate;
        VoteValue m_value;
        int64_t m_time;
    };

    class VotingRound
    {
    public:
        virtual ~VotingRound() = default;

        virtual void RegisterCandidate(uint256 id) = 0;
        virtual void AcceptVote(const Vote& vote) = 0;  // Votes are assumed to be signed correclty
        virtual std::vector<uint256> CalculateResult() const = 0;
        virtual void NotifyResultChange(std::function<void()> onStateChanged) = 0;
    };
}

#endif //CROWN_PLATFORM_GOVERNANCE_H
