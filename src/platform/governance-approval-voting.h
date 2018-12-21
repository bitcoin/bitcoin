// Copyright (c) 2014-2018 Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_PLATFORM_GOVERNANCE_APPROVAL_VOTING_H
#define CROWN_PLATFORM_GOVERNANCE_APPROVAL_VOTING_H

#include "governance.h"
#include <boost/range/numeric.hpp>
#include <functional>

namespace Platform
{
    class ApprovalVoting : public VotingRound
    {
    public:
        ApprovalVoting(int threshhold)
            : m_threshold(threshhold)
        {}

        void SetThreshold(int threshhold);

        void RegisterCandidate(uint256 id) override;
        void AcceptVote(const Vote &vote) override;
        std::vector<uint256> CalculateResult() const override;
        void NotifyResultChange(std::function<void()> onChanged) override;

    private:
        struct CompareTxIn // TODO: replace with a more efficient solution
        {
            bool operator () (const CTxIn& a, const CTxIn& b) const
            {
                return a.ToString() < b.ToString();
            }
        };
        int m_threshold;
        std::map<uint256, std::map<CTxIn, Vote, CompareTxIn>> m_votes; //TODO: refactor to boost::multi_index
        std::vector<std::function<void()>> m_observers;
    };
}

#endif //CROWN_PLATFORM_GOVERNANCE_APPROVAL_VOTING_H
