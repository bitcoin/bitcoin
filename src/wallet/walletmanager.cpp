// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/walletmanager.h>

#include <wallet/wallet.h>

#include <algorithm>


WalletManager g_wallet_manager;

bool WalletManager::AddWallet(CWallet* wallet)
{
    LOCK(m_cs);
    assert(wallet);
    std::vector<CWallet*>::const_iterator i = std::find(m_wallets.begin(), m_wallets.end(), wallet);
    if (i != m_wallets.end()) return false;
    m_wallets.push_back(wallet);
    return true;
}

bool WalletManager::RemoveWallet(CWallet* wallet)
{
    LOCK(m_cs);
    assert(wallet);
    std::vector<CWallet*>::iterator i = std::find(m_wallets.begin(), m_wallets.end(), wallet);
    if (i == m_wallets.end()) return false;
    m_wallets.erase(i);
    return true;
}

bool WalletManager::HasWallets() const
{
    LOCK(m_cs);
    return !m_wallets.empty();
}

std::vector<CWallet*> WalletManager::GetWallets() const
{
    LOCK(m_cs);
    return m_wallets;
}

CWallet* WalletManager::GetWallet(const std::string& name) const
{
    LOCK(m_cs);
    for (CWallet* wallet : m_wallets) {
        if (wallet->GetName() == name) return wallet;
    }
    return nullptr;
}
