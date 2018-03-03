// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_WALLETMANAGER_H
#define BITCOIN_WALLET_WALLETMANAGER_H

#include <wallet/wallet.h>

#include <vector>
#include <util.h>

typedef CWallet* CWalletRef;

class CWalletManager {
private:
    std::vector<CWalletRef> m_vpwallets;

public:
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
        for (auto&& wallet : m_vpwallets) {
            func(wallet);
        }
    };

    CWallet* GetWalletAtPos(int pos);
    CWallet* FindWalletByName(const std::string &name);
};

// global wallet manager instance
extern std::unique_ptr<CWalletManager> g_wallet_manager;

#endif // BITCOIN_WALLET_WALLETMANAGER_H
