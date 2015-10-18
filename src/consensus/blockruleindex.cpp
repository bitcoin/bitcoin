// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "blockruleindex.h"
#include "chain.h"

using namespace Consensus::VersionBits;


bool BlockRuleIndex::IsIntervalStart(const CBlockIndex* pblockIndex) const
{
    return m_activationInterval && pblockIndex && (pblockIndex->nHeight % m_activationInterval == 0);
}

const CBlockIndex* BlockRuleIndex::GetIntervalStart(const CBlockIndex* pblockIndex) const
{
    if (!m_activationInterval || !pblockIndex)
        return NULL;

    int nHeight = pblockIndex->nHeight - (pblockIndex->nHeight % m_activationInterval);

    return pblockIndex->GetAncestor(nHeight);
}

void BlockRuleIndex::SetSoftForkDeployments(int activationInterval, const SoftForkDeployments* deployments)
{
    m_activationInterval = activationInterval;
    m_deployments = deployments;
    m_ruleStateMap.clear();
}

int BlockRuleIndex::CreateBlockVersion(uint32_t nTime, CBlockIndex* pprev, const std::set<int>& disabledRules) const
{
    int nVersion = VERSION_HIGH_BITS;

    if (!m_deployments)
        return nVersion;

    std::set<int> setRules;
    std::set<int> finalizedRules;

    CBlockIndex blockIndex;
    blockIndex.nTime = nTime;
    blockIndex.pprev = pprev;
    uint32_t medianTime = blockIndex.GetMedianTimePast();

    {
        // Set bits for all defined soft forks that haven't activated, failed, or have been requested disabled
        RuleStates ruleStates = GetRuleStates(pprev);
        RuleStates::const_iterator it = ruleStates.begin();
        for (; it != ruleStates.end(); ++it)
        {
            switch (it->second)
            {
            case DEFINED:
                // Set assigned bits we're not disabling
                if (!disabledRules.count(it->first) &&
                    m_deployments->IsRuleAssigned(it->first, medianTime))
                        setRules.insert(it->first);
                break;

            case LOCKED_IN:
                // Always set bits for locked in rules
                setRules.insert(it->first);
                break;

            case ACTIVE:
                // Do not set bits for active and failed rules

            case FAILED:
                finalizedRules.insert(it->first);

            default:
                break;
            }
        }
    }

    {
        // Also set bits for any new deployments that have not been requested disabled
        std::set<int> assignedRules = m_deployments->GetAssignedRules(nTime);
        std::set<int>::const_iterator it = assignedRules.begin();
        for (; it != assignedRules.end(); ++it)
        {
            if (!(finalizedRules.count(*it) || disabledRules.count(*it)))
                setRules.insert(*it);
        }
    }

    {
        std::set<int>::const_iterator it = setRules.begin();
        for (; it != setRules.end(); ++it)
        {
            const SoftFork* psoftFork = m_deployments->GetSoftFork(*it);
            if (psoftFork)
                nVersion |= (0x1 << psoftFork->GetBit());
        }
    }

    return nVersion;
}

RuleState BlockRuleIndex::GetRuleState(int rule, const CBlockIndex* pblockIndex) const
{
    if (!m_activationInterval || !m_deployments)
        return UNDEFINED;

    pblockIndex = GetIntervalStart(pblockIndex);
    if (!pblockIndex)
        return UNDEFINED;

    RuleStateMap::const_iterator rit = m_ruleStateMap.find(pblockIndex);
    if (rit == m_ruleStateMap.end())
        return UNDEFINED;

    const RuleStates& ruleStates = rit->second;
    RuleStates::const_iterator it = ruleStates.find(rule);
    if (it == ruleStates.end())
        return UNDEFINED;

    return it->second;
}

RuleStates BlockRuleIndex::GetRuleStates(const CBlockIndex* pblockIndex) const
{
    RuleStates ruleStates;
    if (!m_activationInterval || !m_deployments)
        return ruleStates;

    pblockIndex = GetIntervalStart(pblockIndex);
    if (!pblockIndex)
        return ruleStates;

    RuleStateMap::const_iterator rit = m_ruleStateMap.find(pblockIndex);
    if (rit == m_ruleStateMap.end())
        return ruleStates;

    return rit->second;
}

bool BlockRuleIndex::AreVersionBitsRecognized(const CBlockIndex* pblockIndex, const CBlockIndex* pprev) const
{
    if (!m_activationInterval || !pblockIndex || !UsesVersionBits(pblockIndex->nVersion))
        return false;

    if (!pprev)
        pprev = pblockIndex->pprev;

    // Get the start of the interval
    const CBlockIndex* pstart = GetIntervalStart(pprev);
    if (!pstart)
        return false;

    uint32_t currentMedianTime = pblockIndex->GetMedianTimePast();
    RuleStates startStates = GetRuleStates(pstart);

    for (int b = MIN_BIT; b <= MAX_BIT; b++)
    {
        if ((pblockIndex->nVersion >> b) & 0x1)
        {
            int rule = m_deployments->GetAssignedRule(b, currentMedianTime);

            // Bit should not be set if it is not assigned
            if (rule == NO_RULE)
                return false;

            RuleStates::const_iterator it = startStates.find(rule);
            if (it != startStates.end())
            {
                // Bits for active and failed rules should not be set
                RuleState state = it->second;
                if ((state == ACTIVE) || (state == FAILED))
                    return false;
            }
        }
    }

    return true;
}

bool BlockRuleIndex::InsertBlockIndex(const CBlockIndex* pblockIndex, const CBlockIndex* pprev)
{
    if (!m_activationInterval || !IsIntervalStart(pblockIndex))
        return false;

    if (m_ruleStateMap.count(pblockIndex))
        return true;

    RuleStates newRuleStates;
    if (!m_deployments)
    {
        m_ruleStateMap[pblockIndex] = newRuleStates;
        return true;
    }

    if (!pprev)
        pprev = pblockIndex->pprev;

    uint32_t currentMedianTime = pblockIndex->GetMedianTimePast();

    // Get the start of the interval just completed.
    const CBlockIndex* pstart = (pblockIndex->nHeight > 0) ? pblockIndex->GetAncestor(pblockIndex->nHeight - m_activationInterval) : NULL;

    RuleStates prevRuleStates;
    if (pstart && m_ruleStateMap.count(pstart))
        prevRuleStates = m_ruleStateMap[pstart];

    // Assign the rule states for the new block
    //   1) Set all assigned rules for new block to DEFINED (we'll check whether they are active next)
    std::set<int> assignedRules = m_deployments->GetAssignedRules(currentMedianTime);
    for (std::set<int>::iterator it = assignedRules.begin(); it != assignedRules.end(); ++it)
    {
        newRuleStates[*it] = DEFINED;
    }

    //   2) Add all failed and active rules
    std::set<int> failedRules;
    std::set<int> activeRules;
    for (RuleStates::iterator it = prevRuleStates.begin(); it != prevRuleStates.end(); ++it)
    {
        switch (it->second)
        {
        case FAILED:
            newRuleStates[it->first] = FAILED;
            failedRules.insert(it->first);
            break;

        case LOCKED_IN:
            // Locked in rules become active at this block

        case ACTIVE:
            newRuleStates[it->first] = ACTIVE;
            activeRules.insert(it->first);
            break;

        default:
            break;
        }
    }

    //   3) Count set bits for interval
    typedef std::map<int /*rule*/, int /*count*/> RuleCountMap;
    RuleCountMap ruleCountMap;
    while (pprev)
    {
        if (UsesVersionBits(pprev->nVersion))
        {
            for (int b = MIN_BIT; b <= MAX_BIT; b++)
            {
                if ((pprev->nVersion >> b) & 0x1)
                {
                    int rule = m_deployments->GetAssignedRule(b, pprev->GetMedianTimePast());

                    // Only count bits for soft forks that are assigned and are not failed and not already active
                    if ((rule == NO_RULE) || failedRules.count(rule) || activeRules.count(rule))
                        continue;

                    if (ruleCountMap.count(rule))
                        ruleCountMap[rule]++;
                    else
                        ruleCountMap[rule] = 1;
                }
            }
        }

        if (pprev == pstart)
            break;

        pprev = pprev->pprev;
    }

    //   4) Set newly locked in and failed states
    RuleCountMap::iterator it = ruleCountMap.begin();
    for (; it != ruleCountMap.end(); ++it)
    {
        int rule = it->first;
        int count = it->second;
        const SoftFork* softFork = m_deployments->GetSoftFork(rule);

        if (softFork && (count >= softFork->GetThreshold()))
            newRuleStates[rule] = LOCKED_IN;
        else
            if (!m_deployments->IsRuleAssigned(rule, currentMedianTime))
                newRuleStates[rule] = FAILED;
    }

    m_ruleStateMap[pblockIndex] = newRuleStates;

    return true;
}

void BlockRuleIndex::Clear()
{
    m_ruleStateMap.clear();
}
