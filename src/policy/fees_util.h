// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license. See the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_FEES_UTIL_H
#define BITCOIN_POLICY_FEES_UTIL_H

#include <kernel/mempool_entry.h>

#include <map>
#include <set>
#include <tuple>
#include <vector>

using TxAncestorsAndDescendants = std::map<Txid, std::tuple<std::set<Txid>, std::set<Txid>>>;

/* GetTxAncestorsAndDescendants takes the vector of transactions removed from the mempool after a block is connected.
 * The function assumes the order the transactions were included in the block was maintained; that is, all transaction
 * parents was added into the vector first before descendants.
 *
 * GetTxAncestorsAndDescendants computes all the ancestors and descendants of the transactions, the transaction is
 * also included as a descendant and ancestor of itself.
 */
TxAncestorsAndDescendants GetTxAncestorsAndDescendants(const std::vector<RemovedMempoolTransactionInfo>& transactions);

#endif // BITCOIN_POLICY_FEES_UTIL_H
