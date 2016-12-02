// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_INTERFACE_H
#define BITCOIN_POLICY_INTERFACE_H

#include <map>
#include <string>
#include <vector>

class CFeeRate;
class CTxOut;

/**
 * Abstract interface for extensible policy.
 */
class CPolicy
{
public:
    virtual ~CPolicy() {}; // Extend before instantiate, this is an interface

    /** A fee rate smaller than this is considered zero fee (for relaying, mining and transaction creation) */
    virtual CFeeRate GetMinRelayFee() const = 0;
    //! Some policies forbid output amounts below the dust threshold, defined internally.
    virtual CAmount GetDustThreshold(const CTxOut& txout) const = 0;
    virtual bool AcceptDust(const CTxOut& txout) const = 0;

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
