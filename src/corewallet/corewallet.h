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
        virtual ~Manager() {}

        //!will return a wallet with given walletid, "" (empty string) will return the default wallet (if exists)
        Wallet* GetWalletWithID(const std::string& walletIDIn);

        //!will return all existing walletids
        std::vector<std::string> GetWalletIDs();

        //!add a new wallet without any keys, hdchains, encryption
        Wallet* AddNewWallet(const std::string& walletID);

        //CValidationInterface listener, will ask all wallets to sync a given transaction
        void SyncTransaction(const CTransaction& tx, const CBlock* pblock);
    protected:
        CCriticalSection cs_mapWallets;
        std::map<std::string, WalletModel> mapWallets; //!map with wallet metadata

        //!store wallets metadata map from memory to disk (multiwallet.dat)
        void WriteWalletList();

        //!read wallet metadata file and map in into memory
        void ReadWalletLists();
    };


    //!global function (namespace CoreWallet) to register the corewallet bitcoin-core module
    void RegisterSignals();
    void UnregisterSignals();

    //!singleton pattern
    Manager* GetManager();
};

#endif // BITCOIN_COREWALLT_COREWALLET_H
