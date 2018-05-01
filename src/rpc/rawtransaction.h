// Copyright (c) 2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPC_RAWTRANSACTION_H
#define BITCOIN_RPC_RAWTRANSACTION_H

#include <primitives/transaction.h>
#include <uint256.h>

class CBasicKeyStore;
class CBlockIndex;
struct CMutableTransaction;
class UniValue;

/** Sign a transaction with the given keystore and previous transactions */
UniValue SignTransaction(CMutableTransaction& mtx, const UniValue& prevTxs, CBasicKeyStore *keystore, bool tempKeystore, const UniValue& hashType);

/**
 * Look up a transaction by hash. This checks for transactions in the mempool,
 * the tx index if enabled, and the UTXO set cache on a best-effort basis. If
 * the transaction is not found, this returns false.
 *
 * @param[in]   tx_hash  The hash of the transaction to be returned.
 * @param[out]  tx  The transaction itself.
 * @param[out]  block_hash  The hash of the block the transaction is found in.
 * @param[out]  error_code  RPC code explaining why transaction could not be found.
 * @param[out]  errmsg  Reason why transaction could not be found.
 * @param[in]   allow_slow  An option to search a slow, unreliable source for transactions.
 * @return  true if transaction is found, false otherwise
 */
bool GetTransaction(const uint256& tx_hash, CTransactionRef& tx, uint256& block_hash,
                    int& error_code, std::string& errmsg, bool allow_slow);

/**
 * Look up a raw transaction by hash within a specified block. This loads the
 * block itself from disk and return true if the transaction is contained within
 * it.
 *
 * @param[in]   tx_hash  The hash of the transaction to be returned.
 * @param[in]   block_index  The block index of the block to search.
 * @param[out]  tx  The raw transaction itself.
 * @return  true if transaction is found, false otherwise
 */
bool GetTransactionInBlock(const uint256& tx_hash, const CBlockIndex* block_index,
                           CTransactionRef& tx);

#endif // BITCOIN_RPC_RAWTRANSACTION_H
