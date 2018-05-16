// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_WALLETMANAGER_H
#define BITCOIN_WALLET_WALLETMANAGER_H

#include <sync.h>

#include <string>
#include <vector>

class CWallet;

//! A thread safe wallet manager.
class WalletManager final
{
    mutable CCriticalSection m_cs;
    std::vector<CWallet*> m_wallets GUARDED_BY(m_cs);

public:
    WalletManager() = default;
    WalletManager(const WalletManager&) = delete;
    WalletManager& operator=(const WalletManager&) = delete;
    ~WalletManager() = default;

    //! Add wallet to the registered wallets.
    bool AddWallet(CWallet* wallet);

    //! Remove wallet from the registered wallets.
    bool RemoveWallet(CWallet* wallet);

    //! Check if there are registered wallets.
    bool HasWallets() const;

    //! Retrieve all registered wallets.
    std::vector<CWallet*> GetWallets() const;

    //! Retrieve wallet by name or nullptr if not found.
    CWallet* GetWallet(const std::string& name) const;
};

//! Global wallet manager.
extern WalletManager g_wallet_manager;

#endif // BITCOIN_WALLET_WALLETMANAGER_H
