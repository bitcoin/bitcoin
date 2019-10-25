// Copyright (c) 2017-2018 The Talkcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TALKCOIN_RPC_RAWTRANSACTION_UTIL_H
#define TALKCOIN_RPC_RAWTRANSACTION_UTIL_H

#include <map>

class FillableSigningProvider;
class UniValue;
struct CMutableTransaction;
class Coin;
class COutPoint;
class CTransaction;
class uint256; 
/**
 * Sign a transaction with the given keystore and previous transactions
 *
 * @param  mtx           The transaction to-be-signed
 * @param  prevTxs       Array of previous txns outputs that tx depends on but may not yet be in the block chain
 * @param  keystore      Temporary keystore containing signing keys
 * @param  coins         Map of unspent outputs - coins in mempool and current chain UTXO set, may be extended by previous txns outputs after call
 * @param  tempKeystore  Whether to use temporary keystore
 * @param  hashType      The signature hash type
 * @returns JSON object with details of signed transaction
 */
UniValue SignTransaction(CMutableTransaction& mtx, const UniValue& prevTxs, FillableSigningProvider* keystore, std::map<COutPoint, Coin>& coins, bool tempKeystore, const UniValue& hashType);

/** Create a transaction from univalue parameters */
CMutableTransaction ConstructTransaction(const UniValue& inputs_in, const UniValue& outputs_in, const UniValue& locktime, bool rbf);

extern void TxToJSON(const CTransaction& tx, const uint256 hashBlock, UniValue& entry);

#endif // TALKCOIN_RPC_RAWTRANSACTION_UTIL_H
