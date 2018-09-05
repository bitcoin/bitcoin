#include "governance.h"

#include <boost/range/numeric.hpp>

class ApprovalVoting: public VotingRound
{
public:
    virtual void RegisterCandidate(uint256 id)
    {
    }

    virtual void AcceptVote(const Vote& vote)
    {
        if (vote.value == Vote::yes)
            ++m_votes[vote.candidate];
        else
            --m_votes[vote.candidate];
    }

    virtual std::vector<uint256> CalculateResult() const
    {
        std::vector<uint256> result;

        return boost::accumulate(m_votes, result, AppendVote);
    }

    static std::vector<uint256>& AppendVote(std::vector<uint256>& result, std::pair<uint256, int> vote)
    {
        if (vote.second > 0)
            result.push_back(vote.first);
        return result;
    }

    virtual void NotifyResultChange(
        boost::function<void(uint256)> onElected,
        boost::function<void(uint256)> onDismissed
    )
    {}

private:
    std::map<uint256, int> m_votes;
};


VotingRound& AgentsVoting()
{
    static ApprovalVoting agentsVoting;

    return agentsVoting;
}