// Copyright (c) 2017-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_TRANSACTION_H
#define BITCOIN_NODE_TRANSACTION_H

#include <primitives/transaction.h>
#include <uint256.h>

/** Broadcast a transaction */
uint256 BroadcastTransaction(CTransactionRef tx, bool allowhighfees = false);

#endif // BITCOIN_NODE_TRANSACTION_H
