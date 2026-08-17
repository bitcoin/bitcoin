// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INDEX_TX_LOOKUP_RESULT_H
#define BITCOIN_INDEX_TX_LOOKUP_RESULT_H

#include <primitives/transaction.h>
#include <uint256.h>

#include <vector>

/// Result of a transaction lookup that may be answered by the txindex.
struct TxLookupResult {
    /// The transaction, if found. nullptr otherwise.
    CTransactionRef tx;
    /// Hash of the block containing the transaction. null if the transaction
    /// was found in the mempool or was not found.
    uint256 block_hash;
    /// Only set when the transaction was not found but may exist in a pruned block.
    /// Note that these blocks may be false positives and not contain the transaction.
    std::vector<uint256> pruned_block_hashes;

    TxLookupResult() = default;
    TxLookupResult(CTransactionRef tx, uint256 block_hash = {})
        : tx{std::move(tx)}, block_hash{block_hash} {}
    explicit TxLookupResult(std::vector<uint256> pruned_block_hashes)
        : pruned_block_hashes{std::move(pruned_block_hashes)} {}
};

#endif // BITCOIN_INDEX_TX_LOOKUP_RESULT_H
