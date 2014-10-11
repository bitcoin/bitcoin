// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_H
#define BITCOIN_POLICY_H

#include "script/standard.h"

#include <map>
#include <string>

/** Abstract interface for Policy */
class CPolicy
{
public:
    virtual void InitFromArgs(const std::map<std::string, std::string>&) = 0;
    virtual bool ValidateScript(const CScript&, txnouttype&) const = 0;
};

/** Return a CPolicy of the type described in the parameter string */
CPolicy& Policy(std::string);
/** Returns the current CPolicy. Requires calling SelectPolicy() or InitPolicyFromArgs() first */
const CPolicy& Policy();
/** Selects the current CPolicy of the type described in the parameter string */
void SelectPolicy(std::string);
/** Returns a HelpMessage string with policy options */
std::string GetPolicyUsageStr();
/** 
 * Selects the current CPolicy of the type described in the string on key "-policy" mapArgs
 * and calls CPolicy::InitFromArgs() with mapArgs.
 */
void InitPolicyFromArgs(const std::map<std::string, std::string>& mapArgs);

#endif // BITCOIN_POLICY_H
