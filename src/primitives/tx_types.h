// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_PRIMITIVES_TX_TYPES_H
#define BITCOIN_PRIMITIVES_TX_TYPES_H

#include <memory>

// Forward declarations and typedefs to be used as replacement for the full
// transaction.h or block.h header

class CPureTransaction;
class CTransaction;

using CPureTransactionRef = std::shared_ptr<const CPureTransaction>;
using CTransactionRef = std::shared_ptr<const CTransaction>;

template <typename TxRef>
class Block;

using CPureBlock = Block<CPureTransactionRef>;
using CBlock = Block<CTransactionRef>;

#endif // BITCOIN_PRIMITIVES_TX_TYPES_H
