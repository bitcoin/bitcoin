// Copyright (c) 2014-2017 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MASTERNODE_H
#define MASTERNODE_H

#include "key.h"
#include "validation.h"
#include "spork.h"
#include "bls/bls.h"

#include "evo/deterministicmns.h"

class CMasternode;
class CConnman;

static const int MASTERNODE_MAX_MIXING_TXES             = 5;

struct masternode_info_t
{
    // Note: all these constructors can be removed once C++14 is enabled.
    // (in C++11 the member initializers wrongly disqualify this as an aggregate)
    masternode_info_t() = default;
    masternode_info_t(masternode_info_t const&) = default;

    // only called when the network is in legacy MN list mode
    masternode_info_t(COutPoint const& outpnt) :
        outpoint{outpnt}
        {}

    COutPoint outpoint{};

    int64_t nLastDsq = 0; //the dsq count from the last dsq broadcast of this node
    bool fInfoValid = false; //* not in CMN
};

//
// The Masternode Class. For managing the Darksend process. It contains the input of the 1000DRK, signature to prove
// it's the one who own that ip address and code for calculating the payment election.
//
class CMasternode : public masternode_info_t
{
private:
    // critical section to protect the inner data structures
    mutable CCriticalSection cs;

public:
    int nMixingTxCount{};

    // KEEP TRACK OF GOVERNANCE ITEMS EACH MASTERNODE HAS VOTE UPON FOR RECALCULATION
    std::map<uint256, int> mapGovernanceObjectsVotedOn;

    CMasternode();
    CMasternode(const CMasternode& other);
    CMasternode(const uint256 &proTxHash, const CDeterministicMNCPtr& dmn);

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        LOCK(cs);
        READWRITE(outpoint);
        READWRITE(nLastDsq);
        READWRITE(nMixingTxCount);
        READWRITE(mapGovernanceObjectsVotedOn);
    }

    bool IsValidForMixingTxes() const
    {
        return nMixingTxCount <= MASTERNODE_MAX_MIXING_TXES;
    }

    masternode_info_t GetInfo() const;

    // KEEP TRACK OF EACH GOVERNANCE ITEM INCASE THIS NODE GOES OFFLINE, SO WE CAN RECALC THEIR STATUS
    void AddGovernanceVote(uint256 nGovernanceObjectHash);
    // RECALCULATE CACHED STATUS FLAGS FOR ALL AFFECTED OBJECTS
    void FlagGovernanceItemsAsDirty();

    void RemoveGovernanceObject(uint256 nGovernanceObjectHash);

    CMasternode& operator=(CMasternode const& from)
    {
        static_cast<masternode_info_t&>(*this)=from;
        nMixingTxCount = from.nMixingTxCount;
        mapGovernanceObjectsVotedOn = from.mapGovernanceObjectsVotedOn;
        return *this;
    }
};

inline bool operator==(const CMasternode& a, const CMasternode& b)
{
    return a.outpoint == b.outpoint;
}
inline bool operator!=(const CMasternode& a, const CMasternode& b)
{
    return !(a.outpoint == b.outpoint);
}

#endif
