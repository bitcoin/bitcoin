// Copyright (c) 2017-2018 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_RPC_RAWTRANSACTION_UTIL_H
#define SYSCOIN_RPC_RAWTRANSACTION_UTIL_H

class CBasicKeyStore;
class CTransaction;
class UniValue;
class uint256;
struct CMutableTransaction;

namespace interfaces {
class Chain;
} // namespace interfaces

/** Sign a transaction with the given keystore and previous transactions */
UniValue SignTransaction(interfaces::Chain& chain, CMutableTransaction& mtx, const UniValue& prevTxs, CBasicKeyStore *keystore, bool tempKeystore, const UniValue& hashType);

/** Create a transaction from univalue parameters */
CMutableTransaction ConstructTransaction(const UniValue& inputs_in, const UniValue& outputs_in, const UniValue& locktime, const UniValue& rbf);

void TxToJSON(const CTransaction& tx, const uint256 hashBlock, UniValue& entry);

#endif // SYSCOIN_RPC_RAWTRANSACTION_UTIL_H
