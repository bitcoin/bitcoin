// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_WALLETTOOL_H
#define BITCOIN_WALLET_WALLETTOOL_H

#include "script/ismine.h"
#include "wallet/wallet.h"

namespace WalletTool {
CWallet* CreateWallet(const std::string walletFile);
CWallet* LoadWallet(const std::string walletFile, bool *fFirstRun);
void WalletShowInfo(CWallet *walletInstance);
bool executeWalletToolFunc(const std::string& strMethod, const std::string& walletFile);
}


#endif // BITCOIN_WALLET_WALLETTOOL_H
