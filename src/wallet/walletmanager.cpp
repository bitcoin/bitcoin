// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/wallet.h>
#include <wallet/walletmanager.h>
#include <wallet/walletutil.h>

std::unique_ptr<CWalletManager> g_wallet_manager = std::unique_ptr<CWalletManager>(new CWalletManager());;

bool CWalletManager::OpenWallets()
{
    if (gArgs.GetBoolArg("-disablewallet", DEFAULT_DISABLE_WALLET)) {
        LogPrintf("Wallet disabled!\n");
        return true;
    }

    for (const std::string& walletFile : gArgs.GetArgs("-wallet")) {
        CWallet * const pwallet = CWallet::CreateWalletFromFile(walletFile, fs::absolute(walletFile, GetWalletDir()));
        if (!pwallet) {
            return false;
        }
        m_vpwallets.push_back(pwallet);
    }

    return true;
}

void CWalletManager::StartWallets(CScheduler& scheduler) {
    for (CWalletRef pwallet : m_vpwallets) {
        pwallet->postInitProcess(scheduler);
    }
}

void CWalletManager::FlushWallets() {
    for (CWalletRef pwallet : m_vpwallets) {
        pwallet->Flush(false);
    }
}

void CWalletManager::StopWallets() {
    for (CWalletRef pwallet : m_vpwallets) {
        pwallet->Flush(true);
    }
}

void CWalletManager::CloseWallets() {
    for (CWalletRef pwallet : m_vpwallets) {
        delete pwallet;
    }
    m_vpwallets.clear();
}

void CWalletManager::AddWallet(CWallet *wallet) {
    m_vpwallets.insert(m_vpwallets.begin(), wallet);
}

void CWalletManager::DeallocWallet(unsigned int pos) {
    m_vpwallets.erase(m_vpwallets.begin()+pos);
}

bool CWalletManager::HasWallets() {
    return !m_vpwallets.empty();
}

CWallet * CWalletManager::GetWalletAtPos(int pos) {
    return m_vpwallets[pos];
}

unsigned int CWalletManager::CountWallets() {
    return m_vpwallets.size();
}

CWallet* CWalletManager::FindWalletByName(const std::string &name) {
    for (CWalletRef pwallet : m_vpwallets) {
        if (pwallet->GetName() == name) {
            return pwallet;
        }
    }
    return nullptr;
}
