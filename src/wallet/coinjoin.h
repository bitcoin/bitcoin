// Copyright (c) 2009-2021 The Bitcoin Core developers
// Copyright (c) 2014-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_COINJOIN_H
#define BITCOIN_WALLET_COINJOIN_H

#include <consensus/amount.h>
#include <threadsafety.h>
#include <wallet/wallet.h>

// Use a macro instead of a function for conditional logging to prevent
// evaluating arguments when logging for the category is not enabled.
#define WalletCJLogPrint(wallet, ...)             \
    do {                                          \
        if (LogAcceptDebug(BCLog::COINJOIN)) {    \
            wallet->WalletLogPrintf(__VA_ARGS__); \
        }                                         \
    } while (0)

namespace wallet {
class CCoinControl;
class CWallet;
class CWalletTx;

CAmount GetBalanceAnonymized(const CWallet& wallet, const CCoinControl& coinControl);

CAmount CachedTxGetAnonymizedCredit(const CWallet& wallet, const CWalletTx& wtx, const CCoinControl& coinControl)
    EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet);

struct CoinJoinCredits {
    CAmount m_anonymized{0};
    CAmount m_denominated{0};
    bool is_unconfirmed{false};
};

CoinJoinCredits CachedTxGetAvailableCoinJoinCredits(const CWallet& wallet, const CWalletTx& wtx)
    EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet);
} // namespace wallet

#endif // BITCOIN_WALLET_COINJOIN_H
