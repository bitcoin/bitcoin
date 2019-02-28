#include "prooftracker.h"
#include "blockwitness.h"

#define REQUIRED_WITNESS_SIGS 6
#define STAKE_REPACKAGE_THRESHOLD 3

void ProofTracker::AddNewStake(const STAKEHASH& hashStake, const BLOCKHASH& hashBlock)
{
    if (!m_mapStakes.count(hashStake))
        m_mapStakes.emplace(std::make_pair(hashStake, std::set<BLOCKHASH>()));
    m_mapStakes.at(hashStake).emplace(hashBlock);
}

void ProofTracker::AddWitness(const BlockWitness& witness)
{
    if (!m_mapBlockWitness.count(witness.m_hashBlock))
        m_mapBlockWitness.emplace(std::make_pair(witness.m_hashBlock, std::set<BlockWitness>()));
    m_mapBlockWitness.at(witness.m_hashBlock).emplace(witness);
}

bool ProofTracker::IsSuspicious(const STAKEHASH& hashStake, const BLOCKHASH& hashBlock)
{
    if (!m_mapStakes.count(hashStake)) {
        AddNewStake(hashStake, hashBlock);
        return false;
    }

    if (m_mapStakes.at(hashStake).size() >= STAKE_REPACKAGE_THRESHOLD) {
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
    AddNewStake(hashStake, hashBlock);
    return false;
}