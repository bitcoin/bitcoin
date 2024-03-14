// Copyright (c) 2012-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_COIN_AGE_PRIORITY_H
#define BITCOIN_POLICY_COIN_AGE_PRIORITY_H

#include <consensus/amount.h>

class CCoinsViewCache;
class CTransaction;

// Compute modified tx vsize for priority calculation
unsigned int CalculateModifiedSize(const CTransaction& tx, unsigned int nTxSize);

// Compute priority, given sum coin-age of inputs and modified tx vsize
// CAUTION: Original ComputePriority accepted UNMODIFIED tx vsize and did the modification internally
double ComputePriority2(double inputs_coin_age, unsigned int mod_vsize);
double ReversePriority2(double coin_age_priority, unsigned int mod_vsize);

/**
 * Return sum coin-age of tx inputs at height nHeight. Also calculate the sum of the values of the inputs
 * that are already in the chain.  These are the inputs that will age and increase priority as
 * new blocks are added to the chain.
 * CAUTION: Original GetPriority also called ComputePriority and returned the final coin-age priority
 */
double GetCoinAge(const CTransaction &tx, const CCoinsViewCache& view, int nHeight, CAmount &inChainInputValue);

#endif // BITCOIN_POLICY_COIN_AGE_PRIORITY_H
