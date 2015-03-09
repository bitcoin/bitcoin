// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_H
#define BITCOIN_POLICY_H

#include "consensus/consensus.h"
#include "script/interpreter.h"
#include "script/standard.h"

#include <map>
#include <string>

class CCoinsViewEfficient;
class CFeeRate;
class CTransaction;
class CTxOut;
class CValidationState;

/** Default for -blockmaxsize and -blockminsize, which control the range of sizes the mining code will create **/
static const unsigned int DEFAULT_BLOCK_MAX_SIZE = 750000;
static const unsigned int DEFAULT_BLOCK_MIN_SIZE = 0;
/** Default for -blockprioritysize, maximum space for zero/low-fee transactions **/
static const unsigned int DEFAULT_BLOCK_PRIORITY_SIZE = 50000;
/** The maximum size for transactions we're willing to relay/mine */
static const unsigned int MAX_STANDARD_TX_SIZE = 100000;
/** Maximum number of signature check operations in an Standard P2SH script */
static const unsigned int MAX_P2SH_SIGOPS = 15;
/** The maximum number of sigops we're willing to relay/mine in a single tx */
static const unsigned int MAX_STANDARD_TX_SIGOPS = MAX_BLOCK_SIGOPS/5;

/** GLOBALS: These variables are supposed to become CStandardPolicy attributes */

extern CFeeRate minRelayTxFee;

inline double AllowFreeThreshold()
{
    return COIN * 144 / 250;
}

inline bool AllowFree(double dPriority)
{
    // Large (in bytes) low-priority (new, small-coin) transactions
    // need a fee.
    return dPriority > AllowFreeThreshold();
}

/** Abstract interface for Policy */
class CPolicy
{
public:
    virtual std::vector<std::pair<std::string, std::string> > GetOptionsHelp() const = 0;
    virtual void InitFromArgs(const std::map<std::string, std::string>&) = 0;
    virtual bool ApproveScript(const CScript&, txnouttype&) const = 0;
    virtual CAmount GetDustThreshold(const CTxOut& txout) const = 0;
    virtual bool ApproveOutput(const CTxOut& txout) const = 0;
    /**
     * Check for standard transaction types
     * @return True if all outputs (scriptPubKeys) use only standard transaction forms
     */
    virtual bool ApproveTx(const CTransaction&, CValidationState&) const = 0;
    /**
     * Check for standard transaction types
     * @param[in] mapInputs    Map of previous transactions that have outputs we're spending
     * @return True if all inputs (scriptSigs) use only standard transaction forms
     */
    virtual bool ApproveTxInputs(const CTransaction&, const CCoinsViewEfficient&) const = 0;
    /**
     * Check whether all inputs of this transaction are valid (scripts and sigs)
     * This does not modify the UTXO set. This does not check double spends and amounts.
     * Preconditions: tx.IsCoinBase() is false
     */
    virtual bool ApproveTxInputsScripts(const CTransaction&, CValidationState&, const CCoinsViewEfficient&, bool cacheStore) const = 0;
};

/** Return a CPolicy of the type described in the parameter string */
CPolicy& Policy(std::string);
/** Returns the current CPolicy. Requires calling SelectPolicy() or InitPolicyFromArgs() first */
const CPolicy& Policy();
/** Selects the current CPolicy of the type described in the parameter string */
void SelectPolicy(std::string);
/**
 * Returns a HelpMessage string with policy options
 *
 * @param selectedPolicy select a policy to show its options
 * @return the formatted string
 */
std::string GetPolicyUsageStr(std::string selectedPolicy);
/**
 * Selects the current CPolicy of the type described in the string on key "-policy" mapArgs
 * and calls CPolicy::InitFromArgs() with mapArgs.
 */
void InitPolicyFromArgs(const std::map<std::string, std::string>& mapArgs);

#endif // BITCOIN_POLICY_H
