// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_PRIMITIVES_TX_TYPES_H
#define BITCOIN_PRIMITIVES_TX_TYPES_H

#include <memory>

// Forward declarations and typedefs to be used as replacement for the full
// transaction.h or block.h header

class CTransaction;

template <bool WithHash>
class Transaction;

using PureTransaction = Transaction</* WithHash */ false>;
using CMutableTransaction = Transaction</* WithHash */ true>;

// Shared poiners to a transaction or the Block type are not defined for CMutableTransaction. They are only defined for
// PureTransaction or CTransaction, so that the transaction hash is either not available or cached.
using PureTransactionRef = std::shared_ptr<const PureTransaction>;
using CTransactionRef = std::shared_ptr<const CTransaction>;

template <typename TxRef>
class Block;

using PureBlock = Block<PureTransactionRef>;
using CBlock = Block<CTransactionRef>;

#endif // BITCOIN_PRIMITIVES_TX_TYPES_H
