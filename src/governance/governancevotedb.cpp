// Copyright (c) 2014-2021 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <governance/governancevotedb.h>

CGovernanceObjectVoteFile::CGovernanceObjectVoteFile() :
    nMemoryVotes(0),
    listVotes(),
    mapVoteIndex()
{
}

CGovernanceObjectVoteFile::CGovernanceObjectVoteFile(const CGovernanceObjectVoteFile& other) :
    nMemoryVotes(other.nMemoryVotes),
    listVotes(other.listVotes),
    mapVoteIndex()
{
    RebuildIndex();
}

void CGovernanceObjectVoteFile::AddVote(const CGovernanceVote& vote)
{
    const uint256 &nHash = vote.GetHash();
    // make sure to never add/update already known votes
    if (HasVote(nHash))
        return;
    listVotes.push_front(vote);
    mapVoteIndex.emplace(nHash, listVotes.begin());
    ++nMemoryVotes;
    RemoveOldVotes(vote);
}

bool CGovernanceObjectVoteFile::HasVote(const uint256& nHash) const
{
    return mapVoteIndex.find(nHash) != mapVoteIndex.end();
}

bool CGovernanceObjectVoteFile::SerializeVoteToStream(const uint256& nHash, CDataStream& ss) const
{
    auto it = mapVoteIndex.find(nHash);
    if (it == mapVoteIndex.end()) {
        return false;
    }
    ss << *(it->second);
    return true;
}

std::vector<CGovernanceVote> CGovernanceObjectVoteFile::GetVotes() const
{
    std::vector<CGovernanceVote> vecResult;
    vecResult.reserve(listVotes.size());
    std::copy(std::begin(listVotes), std::end(listVotes), std::back_inserter(vecResult));
    return vecResult;
}

void CGovernanceObjectVoteFile::RemoveVotesFromMasternode(const COutPoint& outpointMasternode)
{
    auto it = listVotes.begin();
    while (it != listVotes.end()) {
        if (it->GetMasternodeOutpoint() == outpointMasternode) {
            --nMemoryVotes;
            mapVoteIndex.erase(it->GetHash());
            listVotes.erase(it++);
        } else {
            ++it;
        }
    }
}

std::set<uint256> CGovernanceObjectVoteFile::RemoveInvalidVotes(const CDeterministicMNList& tip_mn_list, const COutPoint& outpointMasternode, bool fProposal)
{
    std::set<uint256> removedVotes;

    auto it = listVotes.begin();
    while (it != listVotes.end()) {
        if (it->GetMasternodeOutpoint() == outpointMasternode) {
            bool useVotingKey = fProposal && (it->GetSignal() == VOTE_SIGNAL_FUNDING);
            if (!it->IsValid(tip_mn_list, useVotingKey)) {
                removedVotes.emplace(it->GetHash());
                --nMemoryVotes;
                mapVoteIndex.erase(it->GetHash());
                listVotes.erase(it++);
                continue;
            }
        }
        ++it;
    }

    return removedVotes;
}

void CGovernanceObjectVoteFile::RemoveOldVotes(const CGovernanceVote& vote)
{
    auto it = listVotes.begin();
    while (it != listVotes.end()) {
        if (it->GetMasternodeOutpoint() == vote.GetMasternodeOutpoint() // same masternode
            && it->GetParentHash() == vote.GetParentHash() // same governance object (e.g. same proposal)
            && it->GetSignal() == vote.GetSignal() // same signal (e.g. "funding", "delete", etc.)
            && it->GetTimestamp() < vote.GetTimestamp()) // older than new vote
        {
            --nMemoryVotes;
            mapVoteIndex.erase(it->GetHash());
            listVotes.erase(it++);
        } else {
            ++it;
        }
    }
}

void CGovernanceObjectVoteFile::RebuildIndex()
{
    mapVoteIndex.clear();
    nMemoryVotes = 0;
    auto it = listVotes.begin();
    while (it != listVotes.end()) {
        const CGovernanceVote& vote = *it;
        const uint256 &nHash = vote.GetHash();
        if (mapVoteIndex.find(nHash) == mapVoteIndex.end()) {
            mapVoteIndex[nHash] = it;
            ++nMemoryVotes;
            ++it;
        } else {
            listVotes.erase(it++);
        }
    }
}
