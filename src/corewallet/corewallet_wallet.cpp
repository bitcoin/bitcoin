// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "corewallet/corewallet_wallet.h"

#include "base58.h"
#include "eccryptoverify.h"
#include "random.h"
#include "timedata.h"
#include "util.h"
#include "utilstrencodings.h"

#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>

namespace CoreWallet {
  
CPubKey Wallet::GenerateBip32Structure(const std::string& chainPathIn, unsigned char (&vch)[32], bool useSeed)
{
    //currently seed size is fixed to 256bit
    if (!useSeed)
    {
        RandAddSeedPerfmon();
        do {
            GetRandBytes(vch, sizeof(vch));
        } while (!eccrypto::Check(vch));
    }
    else
    {
        LogPrintf("using given seed with first 4 chars: %s", HexStr(vch, vch+4));
    }
    CExtKey Bip32MasterKey;
    CExtPubKey extPubkey;
    Bip32MasterKey.SetMaster(vch, sizeof(vch));
    
    CBitcoinExtKey b58key;
    b58key.SetKey(Bip32MasterKey);
    LogPrintf("key: %s", b58key.ToString());
    
    
    extPubkey = Bip32MasterKey.Neuter();
    
    masterKeyID = extPubkey.pubkey.GetID();
    
    int64_t nCreationTime = GetTime();
    CKeyMetadata meta(nCreationTime);
    mapKeyMetadata[masterKeyID] = meta;
    walletDB->Write(std::make_pair(std::string("keymeta"), extPubkey.pubkey), meta, false);
    
//    if (!AddKeyPubKey(Bip32MasterKey.key, extPubkey.pubkey))
//        throw std::runtime_error("CWallet::GenerateNewKey(): AddKey failed");

    if (!walletDB->Write(std::string("masterseed"), HexStr(vch, vch+sizeof(vch)), true)) //for easy serialization store the unsigned char[32] as hex string. //TODO: use 32byte binary ser.
        throw std::runtime_error("CWallet::GenerateBip32Structure(): Writing masterkeyid failed!");

    //take the given chainpath and extend it with /0 for external keys and /1 for internal
    //example: input chainpath "m/44'/0'/0'/" will result in m/44'/0'/0'/0/<nChild> for external and m/44'/0'/0'/1/<nChild> for internal keypair generation
    //disk cache the internal and external parent pubkey for faster pubkey generation
    std::string chainPath = chainPathIn;
    boost::to_lower(chainPath);
    boost::erase_all(chainPath, " ");
    if (chainPath.size() > 0 && chainPath.back() == '/')
        chainPath.resize(chainPath.size() - 1);
    
    std::vector<std::string> pathFragments;
    boost::split(pathFragments, chainPath, boost::is_any_of("/"));
    
    
    CExtKey parentKey = Bip32MasterKey;
    
    BOOST_FOREACH(std::string fragment, pathFragments)
    {
        if (fragment == "m") //todo, implement optional creation of masterkey and possibility for wallets with ext childkeys on the top
            continue;
        
        CExtKey childKey;
        
        bool harden = false;
        if (fragment.back() == '\'')
            harden = true;
        
        int nIndex = atoi(fragment.c_str());
        parentKey.Derive(childKey, (harden ? 0x80000000 : 0)+nIndex);
        
        if(pathFragments.back() == fragment)
        {
            // last fragment
            CExtKey internalKey;
            CExtKey externalKey;
            childKey.Derive(internalKey, 1);
            childKey.Derive(externalKey, 0);
            
            internalPubKey = internalKey.Neuter();
            externalPubKey = externalKey.Neuter();
            walletDB->Write(std::string("bip32intpubkey"), internalPubKey.pubkey, true);
            walletDB->Write(std::string("bip32extpubkey"), externalPubKey.pubkey, true);
            
            CExtPubKey childPubKey = childKey.Neuter();
            int64_t nCreationTime = GetTime();
            
            CKeyMetadata metaInternal(nCreationTime);
            metaInternal.keyidParent = childPubKey.pubkey.GetID();
            metaInternal.nDepth = internalPubKey.nDepth;
            metaInternal.nDerivationIndex = internalPubKey.nChild;
            mapKeyMetadata[internalPubKey.pubkey.GetID()] = metaInternal;
            
            CKeyMetadata metaExternal(nCreationTime);
            metaExternal.keyidParent = childPubKey.pubkey.GetID();
            metaExternal.nDepth = internalPubKey.nDepth;
            metaExternal.nDerivationIndex = internalPubKey.nChild;
            mapKeyMetadata[externalPubKey.pubkey.GetID()] = metaExternal;
        }
        
        parentKey = childKey;
    }
    
//    AddKey(extKey.key);
//    AddMeta(keyid, CKeyID(), 0, extKey.chaincode, 0);
//    
//    if (!walletDB->Write(std::make_pair(std::string("keymeta"), extPubkey.pubkey), mapKeyMetadata[extPubkey.pubkey.GetID()], true))
//        throw std::runtime_error("CWallet::GenerateBip32Structure(): Writing metadata failed!");
//    
//    if (!walletDB->Write(std::make_pair(std::string("extpubkey"), extPubkey.pubkey), extPubkey.pubkey, true))
//        throw std::runtime_error("CWallet::GenerateBip32Structure(): Writing pubkey failed!");
//    
    return extPubkey.pubkey;
}
    
CPubKey Wallet::GenerateNewKey(int indexIn)
{
    unsigned int useIndex = 0;
    if (index >= 0)
        useIndex = indexIn;
    
    CExtPubKey newExtPubKey;
    internalPubKey.Derive(newExtPubKey, useIndex);
    
    int64_t nCreationTime = GetTime();
    
    CKeyMetadata meta(nCreationTime);
    meta.keyidParent = internalPubKey.pubkey.GetID();
    meta.nDepth = newExtPubKey.nDepth;
    meta.nDerivationIndex = newExtPubKey.nChild;
    mapKeyMetadata[newExtPubKey.pubkey.GetID()] = meta;
    
    return newExtPubKey.pubkey;
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
    
    mapAddressBook[address].purpose = strPurpose;
    
    return walletDB->Write(make_pair(std::string("adrmeta"), CBitcoinAddress(address).ToString()), mapAddressBook[address]);
}

}; //end namespace