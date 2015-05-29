// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COREWALLT_WALLET_H
#define BITCOIN_COREWALLT_WALLET_H

#include "corewallet/corewallet_basics.h"
#include "corewallet/corewallet_db.h"
#include "corewallet/crypter.h"
#include "key.h"
#include "keystore.h"
#include "validationinterface.h"

namespace CoreWallet {
    
class Wallet : public CCryptoKeyStore, public CValidationInterface{
public:
    mutable CCriticalSection cs_coreWallet;
    std::map<CKeyID, CKeyMetadata> mapKeyMetadata;
    std::map<CTxDestination, CAddressBookMetadata> mapAddressBook;
    int64_t nTimeFirstKey;
    FileDB *walletPrivateDB;
    FileDB *walletCacheDB;

    std::string strChainPath;
    std::string strMasterseedHex;
    CExtPubKey internalPubKey;
    CExtPubKey externalPubKey;


    std::set<int> externalUsedKeysCache;

    unsigned int internalNextPos;
    unsigned int externalNextPos;

    Wallet(std::string strWalletFileIn)
    {
        //instantiate a wallet backend object and maps the stored values
        walletPrivateDB = new FileDB(strWalletFileIn+".private.logdb");
        walletPrivateDB->LoadWallet(this);

        walletCacheDB = new FileDB(strWalletFileIn+".cache.logdb");
        walletCacheDB->LoadWallet(this);
    }
    
    bool GenerateBip32Structure(const std::string& chainpath, unsigned char (&vchOut)[32], bool useSeed=false);
    CPubKey GenerateNewKey(int index=-1);
    CPubKey GetNextUnusedKey(unsigned int& usedIndex);
    CPubKey GetKeyAtIndex(unsigned int index, bool internal = false);
    
    bool AddKeyPubKey(const CKey& key, const CPubKey &pubkey);
    bool LoadKeyMetadata(const CPubKey &pubkey, const CoreWallet::CKeyMetadata &metadata);
    bool LoadKey(const CKey& key, const CPubKey &pubkey);
    bool SetAddressBook(const CTxDestination& address, const std::string& purpose);
    
    CKeyID masterKeyID;
    unsigned char masterSeed[32];
};

// WalletModel: a wallet metadata class
class WalletModel
{
public:
    static const int CURRENT_VERSION=1;
    int nVersion;
    
    Wallet* pWallet; //no persistance
    std::string walletID; //only A-Za-z0-9._-
    std::string strWalletFilename;
    int64_t nCreateTime; // 0 means unknown
    
    WalletModel()
    {
        SetNull();
    }
    
    WalletModel(const std::string& filenameIn, Wallet *pWalletIn)
    {
        SetNull();
        
        strWalletFilename = filenameIn;
        pWallet = pWalletIn;
    }
    
    void SetNull()
    {
        nVersion = CURRENT_VERSION;
        nCreateTime = 0;
        pWallet = NULL;
    }
    
    ADD_SERIALIZE_METHODS;
    
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(this->nVersion);
        nVersion = this->nVersion;
        READWRITE(nCreateTime);
        READWRITE(strWalletFilename);
    }
};

}

#endif // BITCOIN_COREWALLT_WALLET_H
