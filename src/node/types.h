// Copyright (c) 2010-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

//! @file node/types.h is a home for public enum and struct type definitions
//! that are used by internally by node code, but also used externally by wallet
//! or GUI code.
//!
//! This file is intended to define only simple types that do not have external
//! dependencies. More complicated types should be defined in dedicated header
//! files.

#ifndef BITCOIN_NODE_TYPES_H
#define BITCOIN_NODE_TYPES_H

namespace node {
enum class TransactionError {
    OK, //!< No error
    INPUTS_MISSING_OR_SPENT,
    ALREADY_IN_UTXO_SET,
    MEMPOOL_REJECTED,
    MEMPOOL_ERROR,
    MAX_FEE_EXCEEDED,
    MAX_BURN_EXCEEDED,
    INVALID_PACKAGE,
};
} // namespace node

#endif // BITCOIN_NODE_TYPES_H
