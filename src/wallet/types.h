// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
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

#include <policy/fees/block_policy_estimator.h>
#include <util/translation.h>

namespace wallet {
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

struct CreatedTransactionResult
{
    CTransactionRef tx;
    CAmount fee;
    FeeCalculation fee_calc;
    std::optional<unsigned int> change_pos;

    CreatedTransactionResult(CTransactionRef _tx, CAmount _fee, std::optional<unsigned int> _change_pos, const FeeCalculation& _fee_calc)
            : tx(_tx), fee(_fee), fee_calc(_fee_calc), change_pos(_change_pos) {}
};

//! Machine-readable wallet error codes.
//!
//! @note Add new codes only when callers need to handle the condition
//! differently. For errors that should only be displayed to the user, use
//! WalletErrorCode::GenericError and provide the user-facing details in WalletError::message.
enum class WalletErrorCode {
    //! Generic wallet error. Callers may present the accompanying message to
    //! the user.
    GenericError,

    //! The wallet is locked and the operation requires access to private keys.
    //! Callers may ask the user to unlock the wallet and retry the operation.
    UnlockNeeded,
};

//! Wallet-layer error with both programmatic and user-facing information.
//!
//! Wallet methods should return a specific WalletErrorCode only when callers
//! can handle that condition differently. Otherwise, use WalletErrorCode::GenericError
//! and describe the failure in `message`.
struct WalletError {
    //! Machine-readable error code for callers that need programmatic handling.
    WalletErrorCode code;
    //! User-facing translated error message
    bilingual_str message;
};

} // namespace wallet

#endif // BITCOIN_WALLET_TYPES_H
