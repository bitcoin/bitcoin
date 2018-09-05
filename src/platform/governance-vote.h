// Copyright (c) 2014-2018 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_GOVERNANCE_VOTE_H
#define CROWN_GOVERNANCE_VOTE_H

#include "primitives/transaction.h"

class CBlockIndex;
class CValidationState;
class CKey;
class CPubKey;

class VoteTx
{
public:
    static const int CURRENT_VERSION = 1;

    enum Value { abstain = 0, yes, no };

public:
    CTxIn voterId;
    int64_t electionCode;
    int64_t vote;
    uint256 candidate;
    std::vector<unsigned char> signature;

    bool Sign(CKey& keyMasternode, CPubKey& pubKeyMasternode) { return true; }

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

#endif //PROJECT_GOVERNANCE_VOTE_H
