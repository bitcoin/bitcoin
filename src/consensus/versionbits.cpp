// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "versionbits.h"

using namespace Consensus::VersionBits;

typedef std::multimap<int /*bit*/, int /*rule*/> RuleMap;
typedef std::map<int /*rule*/, const SoftFork*> SoftForkMap;

const char* Consensus::VersionBits::GetRuleStateText(int ruleState, bool bUseCaps)
{
    switch (ruleState)
    {
    case UNDEFINED:
        return bUseCaps ? "UNDEFINED" : "undefined";

    case DEFINED:
        return bUseCaps ? "DEFINED" : "defined";

    case LOCKED_IN:
        return bUseCaps ? "LOCKED IN" : "locked in";

    case ACTIVE:
        return bUseCaps ? "ACTIVE" : "active";

    case FAILED:
        return bUseCaps ? "FAILED" : "failed";

    default:
        return bUseCaps ? "N/A" : "n/a";
    }
}

bool Consensus::VersionBits::UsesVersionBits(int nVersion)
{
    return (nVersion & ~VERSION_BITS_MASK) == VERSION_HIGH_BITS;
}


SoftForkDeployments::~SoftForkDeployments()
{
    Clear();
}

void SoftForkDeployments::AddSoftFork(int bit, int rule, int threshold, uint32_t deployTime, uint32_t expireTime)
{
    if (bit < MIN_BIT || bit > MAX_BIT)
        throw std::runtime_error("VersionBits::SoftForkDeployments::AddSoftFork() - invalid bit.");

    if (threshold < 0)
        throw std::runtime_error("VersionBits::SoftForkDeployments::AddSoftFork() - invalid threshold.");

    if (deployTime >= expireTime)
        throw std::runtime_error("VersionBits::SoftForkDeployments::AddSoftFork() - invalid time range.");

    if (m_softForks.count(rule))
        throw std::runtime_error("VersionBits::SoftForkDeployments::AddSoftFork() - rule already assigned.");

    if (!IsBitAvailable(bit, deployTime, expireTime))
        throw std::runtime_error("VersionBits::SoftForkDeployments::AddSoftFork() - bit conflicts with existing softFork.");

    SoftFork* softFork = new SoftFork(bit, rule, threshold, deployTime, expireTime);
    m_softForks.insert(std::pair<int, SoftFork*>(rule, softFork));
    m_rules.insert(std::pair<int, int>(bit, rule));
}

bool SoftForkDeployments::IsBitAvailable(int bit, uint32_t deployTime, uint32_t expireTime) const
{
    std::pair<RuleMap::const_iterator, RuleMap::const_iterator> range;
    range = m_rules.equal_range(bit);
    for (RuleMap::const_iterator rit = range.first; rit != range.second; ++rit)
    {
        SoftForkMap::const_iterator it = m_softForks.find(rit->second);
        if (it == m_softForks.end())
            throw std::runtime_error("VersionBits::SoftForkDeployments::IsBitAvailable() - inconsistent internal state.");

        // Do softFork times overlap?
        const SoftFork* softFork = it->second;
        if (((deployTime >= softFork->GetDeployTime()) && (deployTime <  softFork->GetExpireTime())) ||
            ((expireTime >  softFork->GetDeployTime()) && (expireTime <= softFork->GetExpireTime())) ||
            ((deployTime <= softFork->GetDeployTime()) && (expireTime >= softFork->GetExpireTime())))
                return false;
    }

    return true;
}

bool SoftForkDeployments::IsRuleAssigned(int rule, uint32_t time) const
{
    SoftForkMap::const_iterator it = m_softForks.find(rule);
    if (it == m_softForks.end())
        return false;

    const SoftFork* softFork = it->second;

    return ((time >= softFork->GetDeployTime()) && (time < softFork->GetExpireTime()));
}

const SoftFork* SoftForkDeployments::GetSoftFork(int rule) const
{
    SoftForkMap::const_iterator it = m_softForks.find(rule);

    return (it != m_softForks.end()) ? it->second : NULL;
}

const SoftFork* SoftForkDeployments::GetAssignedSoftFork(int bit, uint32_t time) const
{
    std::pair<RuleMap::const_iterator, RuleMap::const_iterator> range;
    range = m_rules.equal_range(bit);
    for (RuleMap::const_iterator rit = range.first; rit != range.second; ++rit)
    {
        SoftForkMap::const_iterator it = m_softForks.find(rit->second);
        if (it == m_softForks.end())
            throw std::runtime_error("VersionBits::SoftForkDeployments::GetAssignedSoftFork() - inconsistent internal state.");
        const SoftFork* softFork = it->second;
        if ((time >= softFork->GetDeployTime()) && (time < softFork->GetExpireTime()))
            return softFork;
    }

    return NULL;
}

int SoftForkDeployments::GetAssignedRule(int bit, uint32_t time) const
{
    const SoftFork* softFork = GetAssignedSoftFork(bit, time);

    return softFork ? softFork->GetRule() : NO_RULE;
}

std::set<const SoftFork*> SoftForkDeployments::GetAssignedSoftForks(uint32_t time) const
{
    std::set<const SoftFork*> softForks;
    for (SoftForkMap::const_iterator it = m_softForks.begin(); it != m_softForks.end(); ++it)
    {
        const SoftFork* softFork = it->second;
        if ((time >= softFork->GetDeployTime()) && (time < softFork->GetExpireTime()))
            softForks.insert(softFork);
    }

    return softForks;
}

std::set<int> SoftForkDeployments::GetAssignedBits(uint32_t time) const
{
    std::set<int> bits;
    for (SoftForkMap::const_iterator it = m_softForks.begin(); it != m_softForks.end(); ++it)
    {
        const SoftFork* softFork = it->second;
        if ((time >= softFork->GetDeployTime()) && (time < softFork->GetExpireTime()))
            bits.insert(softFork->GetBit());
    }

    return bits;
}

std::set<int> SoftForkDeployments::GetAssignedRules(uint32_t time) const
{
    std::set<int> rules;
    for (SoftForkMap::const_iterator it = m_softForks.begin(); it != m_softForks.end(); ++it)
    {
        const SoftFork* softFork = it->second;
        if ((time >= softFork->GetDeployTime()) && (time < softFork->GetExpireTime()))
            rules.insert(softFork->GetRule());
    }

    return rules;
}

void SoftForkDeployments::Clear()
{
    SoftForkMap::iterator it = m_softForks.begin();
    for (; it != m_softForks.end(); ++it)
        if (it->second) delete it->second;

    m_softForks.clear();
    m_rules.clear();
}
