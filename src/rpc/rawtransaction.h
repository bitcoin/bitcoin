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

/** Retrieve a transaction (from memory pool, or from disk, if possible) */
bool GetTransaction(const uint256& hash, CTransactionRef& tx, uint256& hashBlock, bool fAllowSlow = false, CBlockIndex* blockIndex = nullptr);

#endif // BITCOIN_RPC_RAWTRANSACTION_H
