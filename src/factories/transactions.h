// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_FACTORIES_TRANSACTIONS_H
#define BITCOIN_FACTORIES_TRANSACTIONS_H

#include <primitives/transaction.h>

CMutableTransaction BuildSpendingTransaction(const CScript& scriptSig, int nValue, const uint256& prevout_hash = uint256());
CMutableTransaction BuildSpendingTransaction(const CScript& scriptSig, const CScriptWitness& scriptWitness, int nValue, const uint256& prevout_hash = uint256());
CMutableTransaction BuildSpendingTransaction(const CScript& scriptSig, const CMutableTransaction& txCredit);
CMutableTransaction BuildSpendingTransaction(const CScript& scriptSig, const CScriptWitness& scriptWitness, const CMutableTransaction& txCredit);

CMutableTransaction BuildCreditingTransaction(const CScript& scriptPubKey, int nValue);

#endif
