// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COREWALLT_COREWALLET_H
#define BITCOIN_COREWALLT_COREWALLET_H

#include "corewallet/corewallet_wallet.h"
#include "validationinterface.h"


namespace CoreWallet {
    class Manager : public CValidationInterface
    {
    public:
        Manager();
        ~Manager() { }
        Wallet* GetWalletWithID(const std::string& walletIDIn);
        std::vector<std::string> GetWalletIDs();
        void AddNewWallet(const std::string& walletID);
        void SyncTransaction(const CTransaction& tx, const CBlock* pblock);
    protected:
        std::map<std::string, WalletModel> mapWallets;
        void WriteWalletList();
        void ReadWalletLists();
    };

    void RegisterSignals();
    void UnregisterSignals();
    Manager* GetManager();
};

#endif // BITCOIN_COREWALLT_COREWALLET_H
