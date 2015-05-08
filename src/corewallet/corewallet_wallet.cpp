// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "corewallet/corewallet_wallet.h"

#include "base58.h"
#include "timedata.h"
#include "util.h"
    
CPubKey CoreWallet::Wallet::GenerateNewKey()
{
    CKey secret;
    secret.MakeNewKey(true);
    
    CPubKey pubkey = secret.GetPubKey();
    assert(secret.VerifyPubKey(pubkey));
    
    // Create new metadata
    int64_t nCreationTime = GetTime();
    mapKeyMetadata[pubkey.GetID()] = CoreWallet::CKeyMetadata(nCreationTime);
    if (!nTimeFirstKey || nCreationTime < nTimeFirstKey)
        nTimeFirstKey = nCreationTime;
    
    if (!AddKeyPubKey(secret, pubkey))
        throw std::runtime_error("CWallet::GenerateNewKey(): AddKey failed");
    return pubkey;
}

bool CoreWallet::Wallet::LoadKey(const CKey& key, const CPubKey &pubkey)
{
    return CCryptoKeyStore::AddKeyPubKey(key, pubkey);
}

bool CoreWallet::Wallet::AddKeyPubKey(const CKey& secret, const CPubKey &pubkey)
{
    if (!CCryptoKeyStore::AddKeyPubKey(secret, pubkey))
        return false;
    
    return walletDB->WriteKey(pubkey, secret.GetPrivKey(), mapKeyMetadata[pubkey.GetID()]);
}

bool CoreWallet::Wallet::LoadKeyMetadata(const CPubKey &pubkey, const CoreWallet::CKeyMetadata &meta)
{
    if (meta.nCreateTime && (!nTimeFirstKey || meta.nCreateTime < nTimeFirstKey))
        nTimeFirstKey = meta.nCreateTime;
    
    mapKeyMetadata[pubkey.GetID()] = meta;
    return true;
}

bool CoreWallet::Wallet::SetAddressBook(const CTxDestination& address, const std::string& strPurpose)
{
    CoreWallet::CAddressBookMetadata metadata;
    {
        LOCK(cs_coreWallet);
        std::map<CTxDestination, CoreWallet::CAddressBookMetadata> ::iterator mi = mapAddressBook.find(address);
        if(mi != mapAddressBook.end())
        {
            metadata = mapAddressBook[address];
        }
    }
    metadata.purpose = strPurpose;
    return walletDB->Write(make_pair(std::string("keymeta"), CBitcoinAddress(address).ToString()), metadata);
}