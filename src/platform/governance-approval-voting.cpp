// Copyright (c) 2014-2018 Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "governance-approval-voting.h"

namespace Platform
{

    namespace
    {
        std::vector<uint256>& AppendVote(std::vector<uint256> &result, std::pair<uint256, int> vote)
        {
            if (vote.second > 0)
                result.push_back(vote.first);
            return result;
        }
    }

    void ApprovalVoting::RegisterCandidate(uint256 id)
    {
    }

    void ApprovalVoting::AcceptVote(const Vote &vote)
    {
        if (vote.Value() == VoteValue::yes)
            ++m_votes[vote.Candidate()];
        else
            --m_votes[vote.Candidate()];
    }

    std::vector<uint256> ApprovalVoting::CalculateResult() const
    {
        std::vector<uint256> result;
        return boost::accumulate(m_votes, result, AppendVote);
    }

    void ApprovalVoting::NotifyResultChange(std::function<void()> onStateChanged) {}
}