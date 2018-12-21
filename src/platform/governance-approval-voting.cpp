// Copyright (c) 2014-2018 Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "governance-approval-voting.h"

#include <boost/range/algorithm.hpp>

namespace Platform
{
    void ApprovalVoting::RegisterCandidate(uint256 id)
    {
        m_votes.insert(std::make_pair(id, std::map<CTxIn, Vote, CompareTxIn>{}));
    }

    void ApprovalVoting::AcceptVote(const Vote &vote)
    {
        auto votes = m_votes.find(vote.Candidate());
        if (votes == m_votes.end())
            return;

        boost::for_each(m_observers, [](std::function<void()> notify) { notify(); });
        votes->second.insert(std::make_pair(vote.VoterId(), vote));
    }

    std::vector<uint256> ApprovalVoting::CalculateResult() const
    {
        auto AppendVote = [this](std::vector<uint256> &result, const std::pair<uint256, std::map<CTxIn, Vote, CompareTxIn>>& vote) {
            auto count = 0;
            for (const auto& item: vote.second)
            {
                if (item.second.Value() == VoteValue::yes)
                    ++count;
                else
                    --count;
            }

            if (count > m_threshold)
                result.push_back(vote.first);
            return result;
        };
        std::vector<uint256> result;
        return boost::accumulate(m_votes, result, AppendVote);
    }

    void ApprovalVoting::NotifyResultChange(std::function<void()> onStateChanged)
    {
        m_observers.push_back(onStateChanged);
    }

    void ApprovalVoting::SetThreshold(int threshhold)
    {
        m_threshold = threshhold;
        boost::for_each(m_observers, [](std::function<void()> notify) { notify(); });
    }
}