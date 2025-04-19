// Copyright (c) 2009-2021 The Bitcoin Core developers
// Copyright (c) 2014-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_COINJOIN_H
#define BITCOIN_WALLET_COINJOIN_H

// Use a macro instead of a function for conditional logging to prevent
// evaluating arguments when logging for the category is not enabled.
#define WalletCJLogPrint(wallet, ...)               \
    do {                                            \
        if (LogAcceptDebug(BCLog::COINJOIN)) {      \
            wallet->WalletLogPrintf(__VA_ARGS__);   \
        }                                           \
    } while (0)

#endif // BITCOIN_WALLET_COINJOIN_H
