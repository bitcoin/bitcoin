// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_LOAD_H
#define BITCOIN_WALLET_LOAD_H

#include <string>
#include <vector>

class ArgsManager;
class CScheduler;
struct WalletContext;

namespace interfaces {
class Chain;
} // namespace interfaces

//! Responsible for reading and validating the -wallet arguments and verifying the wallet database.
bool VerifyWallets(WalletContext& context);

//! Load wallet databases.
bool LoadWallets(WalletContext& context);

//! Complete startup of wallets.
void StartWallets(WalletContext& context, CScheduler& scheduler);

//! Flush all wallets in preparation for shutdown.
void FlushWallets(WalletContext& context);

//! Stop all wallets. Wallets will be flushed first.
void StopWallets(WalletContext& context);

//! Close all wallets.
void UnloadWallets(WalletContext& context);

#endif // BITCOIN_WALLET_LOAD_H
