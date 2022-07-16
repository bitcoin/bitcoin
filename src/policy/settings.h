// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_SETTINGS_H
#define BITCOIN_POLICY_SETTINGS_H

#include <policy/feerate.h>
#include <policy/policy.h>

#include <cstdint>
#include <string>

class CTransaction;

// Policy settings which are configurable at runtime.
extern CFeeRate incrementalRelayFee;
extern CFeeRate dustRelayFee;
/** A fee rate smaller than this is considered zero fee (for relaying, mining and transaction creation) */
extern CFeeRate minRelayTxFee;
extern unsigned int nBytesPerSigOp;
extern bool fIsBareMultisigStd;

static inline bool IsStandardTx(const CTransaction& tx, std::string& reason)
{
    return IsStandardTx(tx, ::fIsBareMultisigStd, ::dustRelayFee, reason);
}

static inline int64_t GetVirtualTransactionSize(int64_t weight, int64_t sigop_cost)
{
    return GetVirtualTransactionSize(weight, sigop_cost, ::nBytesPerSigOp);
}

static inline int64_t GetVirtualTransactionSize(const CTransaction& tx, int64_t sigop_cost)
{
    return GetVirtualTransactionSize(tx, sigop_cost, ::nBytesPerSigOp);
}

#endif // BITCOIN_POLICY_SETTINGS_H
