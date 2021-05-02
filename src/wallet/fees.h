// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The XBit Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef XBIT_WALLET_FEES_H
#define XBIT_WALLET_FEES_H

#include <amount.h>

class CCoinControl;
class CFeeRate;
class CWallet;
struct FeeCalculation;

/**
 * Return the minimum required absolute fee for this size
 * based on the required fee rate
 */
CAmount GetRequiredFee(const CWallet& wallet, unsigned int nTxBytes);

/**
 * Estimate the minimum fee considering user set parameters
 * and the required fee
 */
CAmount GetMinimumFee(const CWallet& wallet, unsigned int nTxBytes, const CCoinControl& coin_control, FeeCalculation* feeCalc);

/**
 * Return the minimum required feerate taking into account the
 * minimum relay feerate and user set minimum transaction feerate
 */
CFeeRate GetRequiredFeeRate(const CWallet& wallet);

/**
 * Estimate the minimum fee rate considering user set parameters
 * and the required fee
 */
CFeeRate GetMinimumFeeRate(const CWallet& wallet, const CCoinControl& coin_control, FeeCalculation* feeCalc);

/**
 * Return the maximum feerate for discarding change.
 */
CFeeRate GetDiscardRate(const CWallet& wallet);

#endif // XBIT_WALLET_FEES_H
