// Copyright (c) 2016-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_RBF_H
#define BITCOIN_POLICY_RBF_H

#include <txmempool.h>

static const uint32_t MAX_BIP125_RBF_SEQUENCE = 0xfffffffd;

enum class RBFTransactionState {
    UNKNOWN,
    REPLACEABLE_BIP125,
    FINAL
};

// Check whether the sequence numbers on this transaction are signaling
// opt-in to replace-by-fee, according to BIP 125
bool SignalsOptInRBF(const CTransaction &tx);

// Check whether the replacement policy has expired
bool ExpiredOptInRBFPolicy(const int64_t now, const int64_t accepted, const int64_t timeout);

// Determine whether an in-mempool transaction has hit the expired replacement policy or
// is signaling opt-in to RBF according to BIP 125
// This involves checking sequence numbers of the transaction, as well
// as the sequence numbers of all in-mempool ancestors.
RBFTransactionState IsRBFOptIn(const CTransaction& tx, const CTxMemPool& pool, const int64_t now, const int64_t timeout, const bool enabled_replacement_timeout) EXCLUSIVE_LOCKS_REQUIRED(pool.cs);

#endif // BITCOIN_POLICY_RBF_H
