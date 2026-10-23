// Copyright (c) 2010-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

//! @file node/types.h is a home for public enum and struct type definitions
//! that are used internally by node code, but also used externally by wallet,
//! mining or GUI code.
//!
//! This file is intended to define only simple types that do not have external
//! dependencies. More complicated types should be defined in dedicated header
//! files.

#ifndef BITCOIN_NODE_TYPES_H
#define BITCOIN_NODE_TYPES_H

#include <cstdint>
#include <string>
#include <utility>

namespace node {
enum class TransactionError {
    MISSING_INPUTS,
    ALREADY_IN_UTXO_SET,
    MEMPOOL_REJECTED,
    MEMPOOL_ERROR,
    MAX_FEE_EXCEEDED,
    MAX_FEE_RATE_EXCEEDED,
    MAX_BURN_EXCEEDED,
    INVALID_PACKAGE,
    PRIVATE_BROADCAST_FULL,
    SUBMISSION_SKIPPED, //!< The wallet did not submit the transaction (e.g. broadcasting disabled or abandoned)
};

//! Error returned when a transaction cannot be broadcast: a machine-readable
//! code and an untranslated error message.
struct TransactionBroadcastError {
    TransactionError code;
    std::string message;

    TransactionBroadcastError(TransactionError code, std::string message)
        : code{code}, message{std::move(message)} {}
};

/**
 * How to broadcast a local transaction.
 * Used to influence `BroadcastTransaction()` and its callers.
 */
enum class TxBroadcast : uint8_t {
    /// Add the transaction to the mempool and broadcast to all peers for which tx relay is enabled.
    MEMPOOL_AND_BROADCAST_TO_ALL,
    /// Add the transaction to the mempool, but don't broadcast to anybody.
    MEMPOOL_NO_BROADCAST,
    /// Omit the mempool and directly send the transaction via a few dedicated connections to
    /// peers on privacy networks.
    NO_MEMPOOL_PRIVATE_BROADCAST,
};

} // namespace node

#endif // BITCOIN_NODE_TYPES_H
