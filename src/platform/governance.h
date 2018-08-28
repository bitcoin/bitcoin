#ifndef CROWN_PLATFORM_GOVERNANCE_H
#define CROWN_PLATFORM_GOVERNANCE_H

#include "primitives/transaction.h"
#include "uint256.h"

#include <boost/function.hpp>

#include <vector>

class Vote
{
public:
    enum Value { abstain = 0, yes, no };

    CTxIn voterId;
    int64_t electionCode;
    uint256 candidate;
    Value value;
    std::vector<unsigned char> signature;
};

class VotingRound
{
public:
    virtual void RegisterCandidate(uint256 id) = 0;
    virtual void AcceptVote(const Vote& vote) = 0;
    virtual std::vector<uint256> CalculateResult() const = 0;
    virtual void NotifyResultChange(
        boost::function<void(uint256)> onElected,
        boost::function<void(uint256)> onDismissed
    ) = 0;
};

VotingRound& AgentsVoting();

#endif //CROWN_PLATFORM_GOVERNANCE_H
