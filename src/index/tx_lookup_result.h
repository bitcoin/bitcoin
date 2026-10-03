// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INDEX_TX_LOOKUP_RESULT_H
#define BITCOIN_INDEX_TX_LOOKUP_RESULT_H

#include <primitives/transaction.h>
#include <uint256.h>

#include <variant>
#include <vector>

struct TxFound {
    /// The transaction that was found.
    CTransactionRef tx;
    /// Hash of the block containing the transaction. null if the transaction
    /// was found in the mempool.
    uint256 block_hash;
};

struct TxMiss {
    /// Only set when the transaction was not found but may exist in a pruned block.
    /// Note that these blocks may be false positives and not contain the transaction.
    std::vector<uint256> pruned_block_hashes;
};

/// Result of a transaction lookup that may be answered by the txindex.
using TxLookupResult = std::variant<TxFound, TxMiss>;

#endif // BITCOIN_INDEX_TX_LOOKUP_RESULT_H
