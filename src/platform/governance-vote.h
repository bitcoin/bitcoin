// Copyright (c) 2014-2018 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_GOVERNANCE_VOTE_H
#define CROWN_GOVERNANCE_VOTE_H

#include "primitives/transaction.h"
#include "platform/governance.h"

class CBlockIndex;
class CValidationState;
class CKey;
class CPubKey;

namespace Platform
{
    class VoteTx
    {
    public:
        static const int CURRENT_VERSION = 1;

    public:
        VoteTx(Vote vote)
            : voterId(vote.VoterId())
            , electionCode(vote.ElectionCode())
            , vote(ConvertVote(vote.Value()))
            , time(vote.Time())
            , candidate(vote.Candidate())
            , signature(vote.Signature())
        {}

        VoteTx() = default;

        CTxIn voterId;
        int64_t electionCode;
        int64_t vote;
        int64_t time;
        uint256 candidate;
        std::vector<unsigned char> signature;

        bool Sign(const CKey& keyMasternode, const CPubKey& pubKeyMasternode) { return true; }

        static int64_t ConvertVote(VoteValue vote)
        {
            // Replace with more explicit serialization
            return static_cast<int64_t>(vote);
        }

        static VoteValue ConvertVote(int64_t vote)
        {
            // Replace with more explicit serialization
            return static_cast<VoteValue>(vote);
        }

        Vote GetVote() const
        {
            return { candidate, ConvertVote(vote), time, electionCode, voterId, signature };
        }

    public:
        ADD_SERIALIZE_METHODS;
        template <typename Stream, typename Operation>
        inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
        {
            READWRITE(voterId);
            READWRITE(electionCode);
            READWRITE(vote);
            READWRITE(candidate);
            READWRITE(signature);
        }

        std::string ToString() const {return ""; }
    };



    bool CheckVoteTx(const CTransaction& tx, const CBlockIndex* pindex, CValidationState& state);
}

#endif //PROJECT_GOVERNANCE_VOTE_H
