// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COREWALLT_WALLET_H
#define BITCOIN_COREWALLT_WALLET_H

#include "corewallet/corewallet_basics.h"
#include "corewallet/corewallet_db.h"
#include "corewallet/hdkeystore.h"
#include "key.h"
#include "keystore.h"
#include "validationinterface.h"

namespace CoreWallet {
    
class Wallet : public CHDKeyStore, public CValidationInterface{
public:
    mutable CCriticalSection cs_coreWallet;
    std::map<CKeyID, CKeyMetadata> mapKeyMetadata;
    std::map<CTxDestination, CAddressBookMetadata> mapAddressBook;
    int64_t nTimeFirstKey;
    FileDB *walletPrivateDB;
    FileDB *walletCacheDB;

    //! state: current active hd chain, must be protected over cs_coreWallet
    HDChainID activeHDChain;

    Wallet(std::string strWalletFileIn)
    {
        //instantiate a wallet backend object and maps the stored values
        walletPrivateDB = new FileDB(strWalletFileIn+".private.logdb");
        walletPrivateDB->LoadWallet(this);

        walletCacheDB = new FileDB(strWalletFileIn+".cache.logdb");
        walletCacheDB->LoadWallet(this);
    }

    //!adds a hd chain of keys to the wallet
    bool HDSetChainPath(const std::string& chainPath, bool generateMaster, CKeyingMaterial& vSeed, HDChainID& chainId, bool overwrite = false);

    //!gets a child key from the internal or external chain at given index
    bool HDGetChildPubKeyAtIndex(const HDChainID& chainID, CPubKey &pubKeyOut, std::string& newKeysChainpath, unsigned int nIndex, bool internal = false);

    //!get next free child key
    bool HDGetNextChildPubKey(const HDChainID& chainId, CPubKey &pubKeyOut, std::string& newKeysChainpathOut, bool internal = false);

    //!encrypt your master seeds
    bool EncryptHDSeeds(CKeyingMaterial& vMasterKeyIn);

    //!set the active chain of keys
    bool HDSetActiveChainID(const HDChainID& chainID, bool check = true);

    //!set the active chain of keys
    bool HDGetActiveChainID(HDChainID& chainID);
    
    bool AddKeyPubKey(const CKey& key, const CPubKey &pubkey);
    bool LoadKeyMetadata(const CPubKey &pubkey, const CoreWallet::CKeyMetadata &metadata);
    bool LoadKey(const CKey& key, const CPubKey &pubkey);
    bool SetAddressBook(const CTxDestination& address, const std::string& purpose);
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

/** A key allocated from the key pool. */
class CReserveKey
{
protected:
    Wallet* pwallet;
    int64_t nIndex;
    CPubKey vchPubKey;
public:
    CReserveKey(Wallet* pwalletIn)
    {
        nIndex = -1;
        pwallet = pwalletIn;
    }

    ~CReserveKey()
    {
        ReturnKey();
    }

    void ReturnKey();
    virtual bool GetReservedKey(CPubKey &pubkey);
    void KeepKey();
};

class CHDReserveKey : public CReserveKey
{
protected:
    CPubKey vchPubKey;
    HDChainID chainID;
public:
    CHDReserveKey(Wallet* pwalletIn) : CReserveKey(pwalletIn)
    {
        pwalletIn->HDGetActiveChainID(chainID);
    }

    ~CHDReserveKey()
    {
        ReturnKey();
    }

    void ReturnKey();
    bool GetReservedKey(CPubKey &pubkey);
    void KeepKey();
};

}

#endif // BITCOIN_COREWALLT_WALLET_H
