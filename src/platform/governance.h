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
            int64_t electionCode,
            CTxIn voterId,
            std::vector<unsigned char> signature
        )
            : m_voterId(std::move(voterId)), m_electionCode(electionCode), m_candidate(candidate), m_value(value), m_time(time), m_signature(std::move(signature))
        {}

        Vote(
            const uint256& candidate,
            VoteValue value,
            int64_t time,
            int64_t electionCode,
            const CTxIn& voterId
        )
            : Vote(candidate, value, time, electionCode, voterId, {})
        {}

        CTxIn VoterId() const { return m_voterId; }
        int64_t ElectionCode() const { return m_electionCode; }
        uint256 Candidate() const { return m_candidate; }
        VoteValue Value() const { return m_value; }
        int64_t Time() const { return m_time; }
        const std::vector<unsigned char>& Signature() const { return m_signature; }

        bool Sign(const CKey& keyMasternode) { return true; }
        bool Verify(const CPubKey& keyMasternode) { return true; }


    private:
        CTxIn m_voterId;
        int64_t m_electionCode;
        uint256 m_candidate;
        VoteValue m_value;
        int64_t m_time;
        std::vector<unsigned char> m_signature;
    };

    class VotingRound
    {
    public:
        virtual ~VotingRound() = default;

        virtual void RegisterCandidate(uint256 id) = 0;
        virtual void AcceptVote(const Vote& vote) = 0;
        virtual std::vector<uint256> CalculateResult() const = 0;
        virtual void NotifyResultChange(
            boost::function<void(uint256)> onElected,
            boost::function<void(uint256)> onDismissed
        ) = 0;
    };

    VotingRound& AgentsVoting();
}

#endif //CROWN_PLATFORM_GOVERNANCE_H
