// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

//! @file wallet/types.h is a home for public enum and struct type definitions
//! that are used by internally by wallet code, but also used externally by node
//! or GUI code.
//!
//! This file is intended to define only simple types that do not have external
//! dependencies. More complicated public wallet types like CCoinControl should
//! be defined in dedicated header files.

#ifndef BITCOIN_WALLET_TYPES_H
#define BITCOIN_WALLET_TYPES_H

#include <type_traits>

namespace wallet {
/**
 * IsMine() return codes, which depend on ScriptPubKeyMan implementation.
 * Not every ScriptPubKeyMan covers all types, please refer to
 * https://github.com/bitcoin/bitcoin/blob/master/doc/release-notes/release-notes-0.21.0.md#ismine-semantics
 * for better understanding.
 *
 * For LegacyScriptPubKeyMan,
 * ISMINE_NO: the scriptPubKey is not in the wallet;
 * ISMINE_WATCH_ONLY: the scriptPubKey has been imported into the wallet;
 * ISMINE_SPENDABLE: the scriptPubKey corresponds to an address owned by the wallet user (can spend with the private key);
 * ISMINE_USED: the scriptPubKey corresponds to a used address owned by the wallet user;
 * ISMINE_ALL: all ISMINE flags except for USED;
 * ISMINE_ALL_USED: all ISMINE flags including USED;
 * ISMINE_ENUM_ELEMENTS: the number of isminetype enum elements.
 *
 * For DescriptorScriptPubKeyMan and future ScriptPubKeyMan,
 * ISMINE_NO: the scriptPubKey is not in the wallet;
 * ISMINE_SPENDABLE: the scriptPubKey matches a scriptPubKey in the wallet.
 * ISMINE_USED: the scriptPubKey corresponds to a used address owned by the wallet user.
 *
 */
enum isminetype : unsigned int {
    ISMINE_NO         = 0,
    ISMINE_WATCH_ONLY = 1 << 0,
    ISMINE_SPENDABLE  = 1 << 1,
    ISMINE_USED       = 1 << 2,
    ISMINE_ALL        = ISMINE_WATCH_ONLY | ISMINE_SPENDABLE,
    ISMINE_ALL_USED   = ISMINE_ALL | ISMINE_USED,
    ISMINE_ENUM_ELEMENTS,
};
/** used for bitflags of isminetype */
using isminefilter = std::underlying_type<isminetype>::type;

/**
 * Address purpose field that has been been stored with wallet sending and
 * receiving addresses since BIP70 payment protocol support was added in
 * https://github.com/bitcoin/bitcoin/pull/2539. This field is not currently
 * used for any logic inside the wallet, but it is still shown in RPC and GUI
 * interfaces and saved for new addresses. It is basically redundant with an
 * address's IsMine() result.
 */
enum class AddressPurpose {
    RECEIVE,
    SEND,
    REFUND, //!< Never set in current code may be present in older wallet databases
};
} // namespace wallet

#endif // BITCOIN_WALLET_TYPES_H
