// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/wallet.h>
#include <wallet/walletmanager.h>
#include <wallet/walletutil.h>

std::vector<CWalletRef> vpwallets;

bool OpenWallets()
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
        vpwallets.push_back(pwallet);
    }

    return true;
}

void StartWallets(CScheduler& scheduler) {
    for (CWalletRef pwallet : vpwallets) {
        pwallet->postInitProcess(scheduler);
    }
}

void FlushWallets() {
    for (CWalletRef pwallet : vpwallets) {
        pwallet->Flush(false);
    }
}

void StopWallets() {
    for (CWalletRef pwallet : vpwallets) {
        pwallet->Flush(true);
    }
}

void CloseWallets() {
    for (CWalletRef pwallet : vpwallets) {
        delete pwallet;
    }
    vpwallets.clear();
}

void AddWallet(CWallet *wallet) {
    vpwallets.insert(vpwallets.begin(), wallet);
}

void DeallocWallet(unsigned int pos) {
    vpwallets.erase(vpwallets.begin()+pos);
}

bool HasWallets() {
    return !vpwallets.empty();
}

CWallet * GetWalletAtPos(int pos) {
    return vpwallets[pos];
}

unsigned int CountWallets() {
    return vpwallets.size();
}

CWallet* FindWalletByName(const std::string &name) {
    for (CWalletRef pwallet : ::vpwallets) {
        if (pwallet->GetName() == name) {
            return pwallet;
        }
    }
    return nullptr;
}
