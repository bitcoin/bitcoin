// Copyright (c) 2014-2017 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef GOVERNANCE_VOTEDB_H
#define GOVERNANCE_VOTEDB_H

#include <list>
#include <map>

#include "governance-vote.h"
#include "serialize.h"
#include "uint256.h"

/**
 * Represents the collection of votes associated with a given CGovernanceObject
 * Recently received votes are held in memory until a maximum size is reached after
 * which older votes a flushed to a disk file.
 *
 * Note: This is a stub implementation that doesn't limit the number of votes held
 * in memory and doesn't flush to disk.
 */
class CGovernanceObjectVoteFile
{
public: // Types
    typedef std::list<CGovernanceVote> vote_l_t;

    typedef vote_l_t::iterator vote_l_it;

    typedef vote_l_t::const_iterator vote_l_cit;

    typedef std::map<uint256,vote_l_it> vote_m_t;

    typedef vote_m_t::iterator vote_m_it;

    typedef vote_m_t::const_iterator vote_m_cit;

private:
    static const int MAX_MEMORY_VOTES = -1;

    int nMemoryVotes;

    vote_l_t listVotes;

    vote_m_t mapVoteIndex;

public:
    CGovernanceObjectVoteFile();

    CGovernanceObjectVoteFile(const CGovernanceObjectVoteFile& other);

    /**
     * Add a vote to the file
     */
    void AddVote(const CGovernanceVote& vote);

    /**
     * Return true if the vote with this hash is currently cached in memory
     */
    bool HasVote(const uint256& nHash) const;

    /**
     * Retrieve a vote cached in memory
     */
    bool GetVote(const uint256& nHash, CGovernanceVote& vote) const;

    int GetVoteCount() {
        return nMemoryVotes;
    }

    std::vector<CGovernanceVote> GetVotes() const;

    CGovernanceObjectVoteFile& operator=(const CGovernanceObjectVoteFile& other);

    void RemoveVotesFromMasternode(const COutPoint& outpointMasternode);

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        READWRITE(nMemoryVotes);
        READWRITE(listVotes);
        if(ser_action.ForRead()) {
            RebuildIndex();
        }
    }
private:
    void RebuildIndex();

};

#endif
