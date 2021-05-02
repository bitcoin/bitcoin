// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2019 The XBit Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef XBIT_WALLET_LOAD_H
#define XBIT_WALLET_LOAD_H

#include <string>
#include <vector>

class ArgsManager;
class CScheduler;

namespace interfaces {
class Chain;
} // namespace interfaces

//! Responsible for reading and validating the -wallet arguments and verifying the wallet database.
bool VerifyWallets(interfaces::Chain& chain);

//! Load wallet databases.
bool LoadWallets(interfaces::Chain& chain);

//! Complete startup of wallets.
void StartWallets(CScheduler& scheduler, const ArgsManager& args);

//! Flush all wallets in preparation for shutdown.
void FlushWallets();

//! Stop all wallets. Wallets will be flushed first.
void StopWallets();

//! Close all wallets.
void UnloadWallets();

#endif // XBIT_WALLET_LOAD_H
