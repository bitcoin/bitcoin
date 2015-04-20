// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// NOTE: This file is intended to be customised by the end user, and includes only local node policy logic

#include "policy/policy.h"

#include "amount.h"
#include "tinyformat.h"
#include "ui_interface.h"
#include "util.h"
#include "utilstrencodings.h"

bool fIsBareMultisigStd = true;
/** Fees smaller than this (in satoshi) are considered zero fee (for relaying and mining) */
CFeeRate minRelayTxFee = CFeeRate(1000);

/** Declaration of Standard Policy implementing CPolicy */
class CStandardPolicy : public CPolicy
{
public:
    virtual std::vector<std::pair<std::string, std::string> > GetOptionsHelp() const;
    virtual void InitFromArgs(const std::map<std::string, std::string>&);
    virtual bool ApproveScript(const CScript&, txnouttype&) const;
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

std::string GetPolicyUsageStr(std::string selectedPolicy)
{
    CPolicy& policy = standardPolicy;
    try {
        policy = Policy(selectedPolicy);
    } catch(std::exception &e) {
        selectedPolicy = "standard";
    }
    std::string strUsage = HelpMessageGroup(strprintf(_("Policy options (for policy: %s):"), selectedPolicy));
    strUsage += HelpMessageOpt("-policy", strprintf(_("Select a specific type of policy (default: %s)"), "standard"));
    strUsage += HelpMessagesOpt(policy.GetOptionsHelp());
    return strUsage;
}

void InitPolicyFromArgs(const std::map<std::string, std::string>& mapArgs, std::string defaultPolicy)
{
    SelectPolicy(GetArg("-policy", defaultPolicy, mapArgs));
    pCurrentPolicy->InitFromArgs(mapArgs);
}

/** CStandardPolicy implementation */

std::vector<std::pair<std::string, std::string> > CStandardPolicy::GetOptionsHelp() const
{
    std::vector<std::pair<std::string, std::string> > optionsHelp;
    return optionsHelp;
}

void CStandardPolicy::InitFromArgs(const std::map<std::string, std::string>& mapArgs)
{
}

bool CStandardPolicy::ApproveScript(const CScript& scriptPubKey, txnouttype& whichType) const
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
