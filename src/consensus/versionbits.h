// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_VERSIONBITS_H
#define BITCOIN_VERSIONBITS_H

#include <map>
#include <set>
#include <stdexcept>
#include <stdint.h>

namespace Consensus
{

const int           NO_RULE      = -1;

namespace VersionBits
{

const int           VERSION_HIGH_BITS   = 0x20000000;
const int           VERSION_BITS_MASK   = 0x1fffffff;
const char          MIN_BIT             = 0;
const char          MAX_BIT             = 28;

enum RuleState { UNDEFINED, DEFINED, LOCKED_IN, ACTIVE, FAILED };

const char* GetRuleStateText(int ruleState, bool bUseCaps = false);

bool UsesVersionBits(int nVersion);

class SoftFork
{
public:
    SoftFork(int bit, int rule, int threshold, uint32_t deployTime, uint32_t expireTime) :
        m_bit(bit),
        m_rule(rule),
        m_threshold(threshold),
        m_deployTime(deployTime),
        m_expireTime(expireTime) { }

    int         GetBit() const { return m_bit; }
    int         GetRule() const { return m_rule; }
    int         GetThreshold() const { return m_threshold; }
    uint32_t    GetDeployTime() const { return m_deployTime; }
    uint32_t    GetExpireTime() const { return m_expireTime; }

private:
    int m_bit;
    int m_rule;
    int m_threshold;
    uint32_t m_deployTime;
    uint32_t m_expireTime;
};

typedef std::multimap<int /*bit*/, int /*rule*/> RuleMap;
typedef std::map<int /*rule*/, SoftFork*> SoftForkMap;

class SoftForkDeployments
{
public:
    ~SoftForkDeployments();

    // Creates and adds a new soft fork deployment
    void AddSoftFork(int bit, int rule, int threshold, uint32_t deployTime, uint32_t expireTime);

    // Returns true if the specified bit has not been assigned yet for the given time interval
    bool IsBitAvailable(int bit, uint32_t deployTime, uint32_t expireTime) const;

    // Returns true if the specified rule is assigned at a given time
    bool IsRuleAssigned(int rule, uint32_t time) const;

    // Returns the soft fork object for a given rule
    const SoftFork* GetSoftFork(int rule) const;

    // Returns the soft fork object to which the bit is assigned at a given time
    const SoftFork* GetAssignedSoftFork(int bit, uint32_t time) const;

    // Returns the rule for the soft fork to which the bit is assigned at a given time
    int GetAssignedRule(int bit, uint32_t time) const;

    // Returns the soft fork objects to which bits assigned at a given time
    std::set<const SoftFork*> GetAssignedSoftForks(uint32_t time) const;

    // Returns all the bits assigned at a given time
    std::set<int> GetAssignedBits(uint32_t time) const;

    // Returns all the soft fork rules to which bits are assigned at a given time
    std::set<int> GetAssignedRules(uint32_t time) const;

    // Clears all internal structures
    void Clear();

private:
    RuleMap m_rules;
    SoftForkMap m_softForks;
};

}
}

#endif // BITCOIN_VERSIONBITS_H
