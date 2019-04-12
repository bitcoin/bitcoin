// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_UTIL_FEES_H
#define BITCOIN_UTIL_FEES_H

#include <amount.h>
#include <coins.h>
#include <primitives/transaction.h>

#include <string>

enum class FeeEstimateMode;
enum class FeeReason;

bool FeeModeFromString(const std::string& mode_string, FeeEstimateMode& fee_estimate_mode);
std::string StringForFeeReason(FeeReason reason);
bool GetTransactionFee(const CTransactionRef& tx, const std::vector<Coin>& coins, CAmount& fee);

#endif // BITCOIN_UTIL_FEES_H
