// Copyright (c) 2014-2018 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_GOVERNANCE_VOTE_H
#define CROWN_GOVERNANCE_VOTE_H

#include <json/json_spirit_value.h>
#include <pubkey.h>
#include "primitives/transaction.h"
#include "platform/governance.h"

class CBlockIndex;
class CValidationState;

namespace Platform
{
    class VoteTx
    {
    public:
        static const int CURRENT_VERSION = 1;

    public:
        VoteTx(Vote vote, int64_t electionCode)
            : voterId(vote.VoterId())
            , electionCode(electionCode)
            , vote(ConvertVote(vote.Value()))
            , time(vote.Time())
            , candidate(vote.Candidate())
        {}

        VoteTx() = default;

        CTxIn voterId;
        int64_t electionCode;
        int64_t vote;
        int64_t time;
        uint256 candidate;
        CKeyID keyId;
        std::vector<unsigned char> signature;

        static int64_t ConvertVote(VoteValue vote);
        static VoteValue ConvertVote(int64_t vote);
        Vote GetVote() const;

    public:
        ADD_SERIALIZE_METHODS;
        template <typename Stream, typename Operation>
        inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
        {
            READWRITE(voterId);
            READWRITE(electionCode);
            READWRITE(vote);
            READWRITE(candidate);
            READWRITE(keyId);
            if (!(s.nType & SER_GETHASH))
            {
                READWRITE(signature);
            }
        }

        std::string ToString() const;
        void ToJson(json_spirit::Object & result) const;
    };



    bool CheckVoteTx(const CTransaction& tx, const CBlockIndex* pindex, CValidationState& state);
    bool ProcessVoteTx(const CTransaction& tx, const CBlockIndex* pindex, CValidationState& state);
}

#endif //PROJECT_GOVERNANCE_VOTE_H
