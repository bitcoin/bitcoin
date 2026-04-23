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

#include <policy/feerate.h>
#include <primitives/transaction.h>
#include <util/fees.h>

namespace wallet {
struct MinimumFeeRateResult {
    CFeeRate fee_rate;
    FeeSource fee_source = FeeSource::USER_SPECIFIED;
    int returned_target = 0;
};

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
    FeeSource fee_source;
    std::optional<unsigned int> change_pos;

    CreatedTransactionResult(CTransactionRef _tx, CAmount _fee, std::optional<unsigned int> _change_pos, FeeSource _fee_source)
        : tx(_tx), fee(_fee), fee_source(_fee_source), change_pos(_change_pos) {}
};

} // namespace wallet

#endif // BITCOIN_WALLET_TYPES_H
