// Copyright (c) 2012-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_COIN_AGE_PRIORITY_H
#define BITCOIN_POLICY_COIN_AGE_PRIORITY_H

#include <consensus/amount.h>

class CCoinsViewCache;
class CTransaction;

// Compute modified tx size for priority calculation (optionally given tx size)
unsigned int CalculateModifiedSize(const CTransaction& tx, unsigned int nTxSize=0);

// Compute priority, given sum coin-age of inputs and (optionally) tx size
double ComputePriority(const CTransaction& tx, double dPriorityInputs, unsigned int nTxSize=0);

/**
 * Return priority of tx at height nHeight. Also calculate the sum of the values of the inputs
 * that are already in the chain.  These are the inputs that will age and increase priority as
 * new blocks are added to the chain.
 */
double GetPriority(const CTransaction &tx, const CCoinsViewCache& view, int nHeight, CAmount &inChainInputValue);

#endif // BITCOIN_POLICY_COIN_AGE_PRIORITY_H
