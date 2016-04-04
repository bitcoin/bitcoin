// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_INTERFACE_H
#define BITCOIN_POLICY_INTERFACE_H

#include <map>
#include <string>
#include <vector>

/**
 * Abstract interface for extensible policy.
 */
class CPolicy
{
public:
    virtual ~CPolicy() {}; // Extend before instantiate, this is an interface

    /**
     * @return a vector with strings {"option", "description"} pairs, with the policy options.
     */
    virtual std::vector<std::pair<std::string, std::string> > GetOptionsHelp() const = 0;
    /**
     * @param argMap a map with options to read from.
     */
    virtual void InitFromArgs(const std::map<std::string, std::string>& argMap) = 0;
};

#endif // BITCOIN_POLICY_INTERFACE_H
