// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_INTERFACE_H
#define BITCOIN_POLICY_INTERFACE_H

#include <string>

class CCoinsViewCache;
class CScript;
class CTransaction;

/**
 * \class CPolicy
 * Interface class for non-consensus-critical policy logic, like whether or not
 * a transaction should be relayed and/or included in blocks created.
 */
class CPolicy
{
public:
    virtual ~CPolicy() {};
    virtual bool ApproveScript(const CScript& scriptPubKey) const = 0;
    /**
     * Check for standard transaction types
     * @return True if all outputs (scriptPubKeys) use only standard transaction forms
     */
    virtual bool ApproveTx(const CTransaction& tx, std::string& reason) const = 0;
    /**
     * Check for standard transaction types
     * @param[in] mapInputs    Map of previous transactions that have outputs we're spending
     * @return True if all inputs (scriptSigs) use only standard transaction forms
     */
    virtual bool ApproveTxInputs(const CTransaction& tx, const CCoinsViewCache& mapInputs) const = 0;
};

#endif // BITCOIN_POLICY_INTERFACE_H
