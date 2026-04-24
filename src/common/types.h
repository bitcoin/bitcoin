// Copyright (c) 2010-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

//! @file common/types.h is a home for simple enum and struct type definitions
//! that can be used internally by functions in the libbitcoin_common library,
//! but also used externally by node, wallet, and GUI code.
//!
//! This file is intended to define only simple types that do not have external
//! dependencies. More complicated types should be defined in dedicated header
//! files.

#ifndef BITCOIN_COMMON_TYPES_H
#define BITCOIN_COMMON_TYPES_H

#include <optional>

namespace common {
enum class PSBTError {
    MISSING_INPUTS,
    SIGHASH_MISMATCH,
    EXTERNAL_SIGNER_NOT_FOUND,
    EXTERNAL_SIGNER_FAILED,
    UNSUPPORTED,
    INCOMPLETE,
    OK,
};
/**
 * Instructions for how a PSBT should be signed or filled with information.
 */
struct PSBTFillOptions {
    /**
     * Whether to sign or not.
     */
    bool sign{true};

    /**
     * The sighash type to use when signing (if PSBT does not specify).
     */
    std::optional<int> sighash_type{std::nullopt};

    /**
     * Whether to create the final scriptSig or scriptWitness if possible.
     */
    bool finalize{true};

    /**
     * Whether to fill in bip32 derivation information if available.
     */
    bool bip32_derivs{true};

    /**
     * Only sign the key path (for taproot inputs).
     */
    bool avoid_script_path{false};
};

} // namespace common

#endif // BITCOIN_COMMON_TYPES_H
