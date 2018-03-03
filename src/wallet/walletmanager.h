// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_WALLETMANAGER_H
#define BITCOIN_WALLET_WALLETMANAGER_H

#include <wallet/wallet.h>

#include <vector>
#include <util.h>

typedef CWallet* CWalletRef;
extern std::vector<CWalletRef> vpwallets;

//! Load wallet databases.
bool OpenWallets();

//! Complete startup of wallets.
void StartWallets(CScheduler& scheduler);

//! Flush all wallets in preparation for shutdown.
void FlushWallets();

//! Stop all wallets. Wallets will be flushed first.
void StopWallets();

//! Close all wallets.
void CloseWallets();

void AddWallet(CWallet *wallet);
void DeallocWallet(unsigned int pos);

bool HasWallets();

unsigned int CountWallets();

template<typename Callable>
void ForEachWallet(Callable&& func)
{
    for (auto&& wallet : vpwallets) {
        func(wallet);
    }
};

CWallet* GetWalletAtPos(int pos);

CWallet* FindWalletByName(const std::string &name);

#endif // BITCOIN_WALLET_WALLETMANAGER_H
