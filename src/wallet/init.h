// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_INIT_H
#define BITCOIN_WALLET_INIT_H

#include <string>

//! Return the wallets help message.
std::string GetWalletHelpString(bool showDebug);

//! Wallets parameter interaction
bool WalletParameterInteraction();

//! Responsible for reading and validating the -wallet arguments and verifying the wallet database.
//  This function will perform salvage on the wallet if requested, as long as only one wallet is
//  being loaded (CWallet::ParameterInteraction forbids -salvagewallet, -zapwallettxes or -upgradewallet with multiwallet).
bool WalletVerify();

//! Load wallet databases.
bool InitLoadWallet();

#endif // BITCOIN_WALLET_INIT_H
