// Copyright (c) 2010-2021 The Bitcoin Core developers
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

#include <consensus/amount.h>
#include <cstddef>
#include <cstdint>
#include <policy/policy.h>
#include <script/script.h>
#include <uint256.h>
#include <util/time.h>

namespace node {
enum class TransactionError {
    OK, //!< No error
    MISSING_INPUTS,
    ALREADY_IN_UTXO_SET,
    MEMPOOL_REJECTED,
    MEMPOOL_ERROR,
    MAX_FEE_EXCEEDED,
    MAX_BURN_EXCEEDED,
    INVALID_PACKAGE,
};

struct BlockCreateOptions {
    /**
     * Set false to omit mempool transactions
     */
    bool use_mempool{true};
    /**
     * The default reserved weight for the fixed-size block header,
     * transaction count and coinbase transaction.
     */
    size_t block_reserved_weight{DEFAULT_BLOCK_RESERVED_WEIGHT};
    /**
     * The maximum additional sigops which the pool will add in coinbase
     * transaction outputs.
     */
    size_t coinbase_output_max_additional_sigops{400};
    /**
     * Script to put in the coinbase transaction. The default is an
     * anyone-can-spend dummy.
     *
     * Should only be used for tests, when the default doesn't suffice.
     *
     * Note that higher level code like the getblocktemplate RPC may omit the
     * coinbase transaction entirely. It's instead constructed by pool software
     * using fields like coinbasevalue, coinbaseaux and default_witness_commitment.
     * This software typically also controls the payout outputs, even for solo
     * mining.
     *
     * The size and sigops are not checked against
     * coinbase_max_additional_weight and coinbase_output_max_additional_sigops.
     */
    CScript coinbase_output_script{CScript() << OP_TRUE};
};

struct BlockWaitOptions {
    /**
     * How long to wait before returning nullptr instead of a new template.
     * Default is to wait forever.
     */
    MillisecondsDouble timeout{MillisecondsDouble::max()};

    /**
     * The wait method will not return a new template unless it has fees at
     * least fee_threshold sats higher than the current template, or unless
     * the chain tip changes and the previous template is no longer valid.
     *
     * A caller may not be interested in templates with higher fees, and
     * determining whether fee_threshold is reached is also expensive. So as
     * an optimization, when fee_threshold is set to MAX_MONEY (default), the
     * implementation is able to be much more efficient, skipping expensive
     * checks and only returning new templates when the chain tip changes.
     */
    CAmount fee_threshold{MAX_MONEY};
};

struct BlockCheckOptions {
    /**
     * Set false to omit the merkle root check
     */
    bool check_merkle_root{true};

    /**
     * Set false to omit the proof-of-work check
     */
    bool check_pow{true};
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
};

} // namespace node

#endif // BITCOIN_NODE_TYPES_H
