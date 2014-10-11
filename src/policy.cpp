// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// NOTE: This file is intended to be customised by the end user, and includes only local node policy logic

#include "policy.h"

#include "tinyformat.h"
#include "ui_interface.h"
#include "utilstrencodings.h"

/** Declaration of Standard Policy implementing CPolicy */
class CStandardPolicy : public CPolicy
{
public:
    virtual void InitFromArgs(const std::map<std::string, std::string>&);
    virtual bool ValidateScript(const CScript&, txnouttype&) const;
};

/** Global variables and their interfaces */

static CStandardPolicy standardPolicy;

static CPolicy* pCurrentPolicy = 0;

CPolicy& Policy(std::string policy)
{
    if (policy == "standard")
        return standardPolicy;
    throw std::runtime_error(strprintf(_("Unknown policy '%s'"), policy));
}

void SelectPolicy(std::string policy)
{
    pCurrentPolicy = &Policy(policy);
}

const CPolicy& Policy()
{
    assert(pCurrentPolicy);
    return *pCurrentPolicy;
}

std::string GetPolicyUsageStr()
{
    std::string strUsage = "";
    strUsage += "  -policy                " + strprintf(_("Select a specific type of policy (default: %s)"), "standard") + "\n";
    return strUsage;
}

void InitPolicyFromArgs(const std::map<std::string, std::string>& mapArgs)
{
    SelectPolicy(GetArg("-policy", "standard", mapArgs));
    pCurrentPolicy->InitFromArgs(mapArgs);
}

/** CStandardPolicy implementation */

void CStandardPolicy::InitFromArgs(const std::map<std::string, std::string>& mapArgs)
{
}

bool CStandardPolicy::ValidateScript(const CScript& scriptPubKey, txnouttype& whichType) const
{
    std::vector<std::vector<unsigned char> > vSolutions;
    if (!Solver(scriptPubKey, whichType, vSolutions))
        return false;

    if (whichType == TX_MULTISIG)
    {
        unsigned char m = vSolutions.front()[0];
        unsigned char n = vSolutions.back()[0];
        // Support up to x-of-3 multisig txns as standard
        if (n < 1 || n > 3)
            return false;
        if (m < 1 || m > n)
            return false;
    }

    return whichType != TX_NONSTANDARD;
}
