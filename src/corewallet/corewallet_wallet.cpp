// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "corewallet/corewallet_wallet.h"

#include "base58.h"
#include "timedata.h"
#include "util.h"

namespace CoreWallet {
    
CPubKey Wallet::GenerateNewKey()
{
    CKey secret;
    secret.MakeNewKey(true);
    
    CPubKey pubkey = secret.GetPubKey();
    assert(secret.VerifyPubKey(pubkey));
    
    // Create new metadata
    int64_t nCreationTime = GetTime();
    mapKeyMetadata[pubkey.GetID()] = CKeyMetadata(nCreationTime);
    if (!nTimeFirstKey || nCreationTime < nTimeFirstKey)
        nTimeFirstKey = nCreationTime;
    
    if (!AddKeyPubKey(secret, pubkey))
        throw std::runtime_error("CWallet::GenerateNewKey(): AddKey failed");
    return pubkey;
}

bool Wallet::LoadKey(const CKey& key, const CPubKey &pubkey)
{
    return CCryptoKeyStore::AddKeyPubKey(key, pubkey);
}

bool Wallet::AddKeyPubKey(const CKey& secret, const CPubKey &pubkey)
{
    if (!CCryptoKeyStore::AddKeyPubKey(secret, pubkey))
        return false;
    
    return walletDB->WriteKey(pubkey, secret.GetPrivKey(), mapKeyMetadata[pubkey.GetID()]);
}

bool Wallet::LoadKeyMetadata(const CPubKey &pubkey, const CKeyMetadata &meta)
{
    if (meta.nCreateTime && (!nTimeFirstKey || meta.nCreateTime < nTimeFirstKey))
        nTimeFirstKey = meta.nCreateTime;
    
    mapKeyMetadata[pubkey.GetID()] = meta;
    return true;
}

bool Wallet::SetAddressBook(const CTxDestination& address, const std::string& strPurpose)
{
    LOCK(cs_coreWallet);
    std::map<CTxDestination, CAddressBookMetadata> ::iterator mi = mapAddressBook.find(address);
    if(mi == mapAddressBook.end())
        mapAddressBook[address] = CAddressBookMetadata();
    
    return walletDB->Write(make_pair(std::string("adrmeta"), CBitcoinAddress(address).ToString()), mapAddressBook[address]);
}

}; //end namespace