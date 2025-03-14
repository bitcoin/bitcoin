// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

//! @file kernel/types.h is a home for simple enum and struct type definitions
//! that can be used internally by functions in the libbitcoin_kernel library,
//! but also used externally by node, wallet, and GUI code.
//!
//! This file is intended to define only simple types that do not have external
//! dependencies. More complicated types should be defined in dedicated header
//! files.

#ifndef BITCOIN_KERNEL_TYPES_H
#define BITCOIN_KERNEL_TYPES_H

namespace kernel {
//! Information about chainstate that notifications are sent from.
struct ChainstateRole {
    //! Whether this is a notification from chainstate that's been fully
    //! validated starting from the genesis block. False if is from an
    //! assumeutxo snapshot chainstate that has not been validated yet.
    bool validated{true};

    //! Whether this a historical chainstate downloading old blocks to validate
    //! an assumeutxo snapshot, instead of syncing to the network tip.
    bool historical{false};
};
} // namespace kernel

#endif // BITCOIN_KERNEL_TYPES_H
