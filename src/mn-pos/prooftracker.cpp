#include "prooftracker.h"
#include "blockwitness.h"

#define REQUIRED_WITNESS_SIGS 6
#define STAKE_REPACKAGE_THRESHOLD 3

void ProofTracker::AddNewStake(const STAKEHASH& hashStake, const BLOCKHASH& hashBlock, int nHeight)
{
    if (!m_mapStakes.count(hashStake))
        m_mapStakes.emplace(std::make_pair(hashStake, std::make_pair(nHeight, std::set<BLOCKHASH>())));
    m_mapStakes.at(hashStake).second.emplace(hashBlock);
}

void ProofTracker::AddWitness(const BlockWitness& witness)
{
    if (!m_mapBlockWitness.count(witness.m_hashBlock))
        m_mapBlockWitness.emplace(std::make_pair(witness.m_hashBlock, std::set<BlockWitness>()));
    m_mapBlockWitness.at(witness.m_hashBlock).emplace(witness);
}

std::set<BlockWitness> ProofTracker::GetWitnesses(const uint256& hashBlock) const
{
    if (m_mapBlockWitness.count(hashBlock))
        return m_mapBlockWitness.at(hashBlock);
    return std::set<BlockWitness>();
}

int ProofTracker::GetWitnessCount(const BLOCKHASH& hashBlock) const
{
    if (!m_mapBlockWitness.count(hashBlock))
        return 0;
    return m_mapBlockWitness.at(hashBlock).size();
}

bool ProofTracker::HasSufficientProof(const BLOCKHASH& hashBlock) const
{
    return m_mapBlockWitness.count(hashBlock) && m_mapBlockWitness.at(hashBlock).size() >= REQUIRED_WITNESS_SIGS;
}

bool ProofTracker::IsSuspicious(const STAKEHASH& hashStake, const BLOCKHASH& hashBlock, int nHeight)
{
    if (!m_mapStakes.count(hashStake)) {
        AddNewStake(hashStake, hashBlock, nHeight);
        return false;
    }

    if (m_mapStakes.at(hashStake).second.size() >= STAKE_REPACKAGE_THRESHOLD) {
        //If there are enough masternode that has signed off on this hash then it is not suspicious
        if (!m_mapBlockWitness.count(hashBlock))
            return true; //suspicious because no records of mn signing this block

        // Factored out for legibility...
        const auto& vWitness = m_mapBlockWitness.at(hashBlock);
        bool isSuspicious = vWitness.size() < REQUIRED_WITNESS_SIGS;
        if (isSuspicious)
            return isSuspicious;
    }

    //Not suspicious, but still record knowledge of this stake so potential suspicious repackaging of this stake
    //can be tracked
    AddNewStake(hashStake, hashBlock, nHeight);
    return false;
}

void ProofTracker::EraseBeforeHeight(int nHeight)
{
    std::set<uint256> setErase;
    for (const auto& p : m_mapStakes) {
        int nBlockHeight = p.second.first;
        if (nBlockHeight < nHeight) {
            setErase.emplace(p.first);
            //Erase all witnesses associated with this stake hash
            const std::set<BLOCKHASH>& setWitness = p.second.second;
            for (const uint256& hashBlock : setWitness) {
                m_mapBlockWitness.erase(hashBlock);
            }
        }

    }

    for (const uint256& hashStake : setErase) {
        m_mapStakes.erase(hashStake);
    }
}