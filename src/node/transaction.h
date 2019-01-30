// Copyright (c) 2017-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_TRANSACTION_H
#define BITCOIN_NODE_TRANSACTION_H

#include <primitives/transaction.h>
#include <uint256.h>

enum class TransactionError {
    OK = 0,
    UNKNOWN_ERROR,

    MISSING_INPUTS,
    ALREADY_IN_CHAIN,
    P2P_DISABLED,
    MEMPOOL_REJECTED,
    MEMPOOL_ERROR,
    INVALID_PSBT,
    PSBT_MISMATCH,
    SIGHASH_MISMATCH,

    ERROR_COUNT
};

#define TRANSACTION_ERR_LAST TransactionError::ERROR_COUNT

const char* TransactionErrorString(const TransactionError error);

/**
 * Broadcast a transaction
 *
 * @param[in]  tx the transaction to broadcast
 * @param[out] &txid the txid of the transaction, if successfully broadcast
 * @param[out] &error reference to UniValue to fill with error info on failure
 * @param[out] &err_string reference to std::string to fill with error string if available
 * @param[in]  allowhighfees whether to allow fees exceeding maxTxFee
 * return true on success, false on error (and fills in `error`)
 */
bool BroadcastTransaction(const CTransactionRef tx, uint256& txid, TransactionError& error, std::string& err_string, const bool allowhighfees = false, const bool bypass_limits = false);

#endif // BITCOIN_NODE_TRANSACTION_H
