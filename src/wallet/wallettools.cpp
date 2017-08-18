// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "base58.h"
#include "util.h"
#include "wallet/wallet.h"

#include <boost/filesystem.hpp>

namespace WalletTool {

static CWallet* CreateWallet(const std::string walletFile)
{
    boost::filesystem::path filePath(walletFile);
    if (boost::filesystem::exists(filePath))
    {
        fprintf(stderr, "Error: File exists already\n");
        return NULL;
    }
    std::unique_ptr<CWalletDBWrapper> dbw(new CWalletDBWrapper(&bitdb, walletFile));
    CWallet *walletInstance = new CWallet(std::move(dbw));
    bool fFirstRun = true;
    DBErrors nLoadWalletRet = walletInstance->LoadWallet(fFirstRun);
    if (nLoadWalletRet != DB_LOAD_OK)
    {
        fprintf(stderr, "Error creating %s", walletFile.c_str());
        return NULL;
    }

    if (GetBoolArg("-usehd", DEFAULT_USE_HD_WALLET) && !walletInstance->IsHDEnabled()) {
        walletInstance->SetMinVersion(FEATURE_HD_SPLIT);

        // generate a new master key
        CPubKey masterPubKey = walletInstance->GenerateNewHDMasterKey();
        if (!walletInstance->SetHDMasterKey(masterPubKey))
            throw std::runtime_error(std::string(__func__) + ": Storing master key failed");
    }

    fprintf(stdout, "Topping up keypool...\n");
    walletInstance->TopUpKeyPool();
    return walletInstance;
}

static CWallet* LoadWallet(const std::string walletFile, bool *fFirstRun)
{
    boost::filesystem::path filePath(walletFile);
    if (!boost::filesystem::exists(filePath))
    {
        fprintf(stderr, "Error: Wallet files does not exist\n");
        return NULL;
    }

    std::unique_ptr<CWalletDBWrapper> dbw(new CWalletDBWrapper(&bitdb, walletFile));
    CWallet *walletInstance = new CWallet(std::move(dbw));
    DBErrors nLoadWalletRet = walletInstance->LoadWallet(*fFirstRun);
    if (nLoadWalletRet != DB_LOAD_OK)
    {
        walletInstance = NULL;
        if (nLoadWalletRet == DB_CORRUPT)
        {
            fprintf(stderr, "Error loading %s: Wallet corrupted", walletFile.c_str());
            return NULL;
        }
        else if (nLoadWalletRet == DB_NONCRITICAL_ERROR)
        {
            fprintf(stderr, "Error reading %s! All keys read correctly, but transaction data"
                    " or address book entries might be missing or incorrect.",
                    walletFile.c_str());
        }
        else if (nLoadWalletRet == DB_TOO_NEW)
        {
            fprintf(stderr, "Error loading %s: Wallet requires newer version of %s",
                    walletFile.c_str(), PACKAGE_NAME);
            return NULL;
        }
        else if (nLoadWalletRet == DB_NEED_REWRITE)
        {
            fprintf(stderr, "Wallet needed to be rewritten: restart %s to complete", PACKAGE_NAME);
            return NULL;
        }
        else
        {
            fprintf(stderr, "Error loading %s", walletFile.c_str());
            return NULL;
        }
    }

    return walletInstance;
}

static void WalletShowInfo(CWallet *walletInstance)
{
    // lock required because of some AssertLockHeld()
    LOCK(walletInstance->cs_wallet);

    fprintf(stdout, "Wallet info\n===========\n");
    fprintf(stdout, "Encrypted: %s\n",      walletInstance->IsCrypted() ? "yes" : "no");
    fprintf(stdout, "HD (hd seed available): %s\n",             walletInstance->GetHDChain().masterKeyID.IsNull() ? "no" : "yes");
    fprintf(stdout, "Keypool Size: %lu\n",  (unsigned long)walletInstance->GetKeyPoolSize());
    fprintf(stdout, "Transactions: %lu\n",  (unsigned long)walletInstance->mapWallet.size());
    fprintf(stdout, "Address Book: %lu\n",  (unsigned long)walletInstance->mapAddressBook.size());
}

bool executeWalletToolFunc(const std::string& strMethod, const std::string& walletFile)
{

    if (strMethod == "create")
    {
        CWallet *walletInstance = CreateWallet(walletFile);
        if (walletInstance)
            WalletShowInfo(walletInstance);
    }
    else if (strMethod == "info")
    {
        bool fFirstRun;
        CWallet *walletInstance = LoadWallet(walletFile, &fFirstRun);
        if (!walletInstance)
            return false;
        WalletShowInfo(walletInstance);
    }
    else if (strMethod == "salvage")
    {
        // Recover readable keypairs:
        std::string strError;
        fs::path walletFilePath(walletFile);
        if (!CWalletDB::VerifyEnvironment(walletFilePath.filename().string(), walletFilePath.parent_path().string()+"/", strError)) {
            fprintf(stderr, "CWalletDB::VerifyEnvironment Error: %s\n", strError.c_str());
        }

        CWallet dummyWallet;
        if (!CWalletDB::Recover(walletFilePath.filename().string(), (void *)&dummyWallet, CWalletDB::RecoverKeysOnlyFilter)) {
            fprintf(stderr, "Salvage failed\n");
            return false;
        }
        //TODO, set wallets best block to genesis to enforce a rescan
        fprintf(stdout, "Salvage successful. Please rescan your wallet.");
    }
    else if (strMethod == "zaptxes")
    {
        std::vector<CWalletTx> vWtx;
        std::unique_ptr<CWalletDBWrapper> dbw(new CWalletDBWrapper(&bitdb, walletFile));
        CWallet *tempWallet = new CWallet(std::move(dbw));
        DBErrors nZapWalletRet = tempWallet->ZapWalletTx(vWtx);
        if (nZapWalletRet != DB_LOAD_OK) {
            fprintf(stderr, "Error loading %s: Wallet corrupted", walletFile.c_str());
            return false;
        }
        fprintf(stdout, "Zaptxes successful executed. Please rescan your wallet.");
    }
    else {
        fprintf(stderr, "Unknown command\n");
    }

    return true;
}
}
