// Copyright (c) 2014-2019 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_GOVERNANCE_GOVERNANCEVOTEDB_H
#define SYSCOIN_GOVERNANCE_GOVERNANCEVOTEDB_H

#include <list>
#include <map>

#include <governance/governancevote.h>
#include <serialize.h>
#include <streams.h>
#include <uint256.h>

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
    using vote_l_t = std::list<CGovernanceVote>;

    using vote_m_t = std::map<uint256, vote_l_t::iterator>;

private:
    int nMemoryVotes{0};

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
    bool SerializeVoteToStream(const uint256& nHash, CDataStream& ss) const;

    int GetVoteCount() const 
    {
        return nMemoryVotes;
    }

    std::vector<CGovernanceVote> GetVotes() const;

    void RemoveVotesFromMasternode(const COutPoint& outpointMasternode);
    std::set<uint256> RemoveInvalidVotes(const CBlockIndex *pindex, const COutPoint& outpointMasternode, bool fProposal);

    template<typename Stream>
    void Serialize(Stream& s) const
    {
        s << nMemoryVotes;
        s << listVotes;
   
    }

    template<typename Stream>
    void Unserialize(Stream& s)
    {
        s >> nMemoryVotes;
        s >> listVotes;
        RebuildIndex();
    }

private:
    // Drop older votes for the same gobject from the same masternode
    void RemoveOldVotes(const CGovernanceVote& vote);

    void RebuildIndex();
};

#endif // SYSCOIN_GOVERNANCE_GOVERNANCEVOTEDB_H
