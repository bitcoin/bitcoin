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

#include <stdint.h>

#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>

namespace CoreWallet {
  
bool Wallet::GenerateBip32Structure(const std::string& chainPathIn, unsigned char (&vch)[32], bool useSeed)
{
    //take the given chainpath and extend it with /0 for external keys and /1 for internal
    //example: input chainpath "m/44'/0'/0'/" will result in m/44'/0'/0'/0/<nChild> for external and m/44'/0'/0'/1/<nChild> for internal keypair generation
    //disk cache the internal and external parent pubkey for faster pubkey generation
    if (!walletCacheDB->Write(std::string("chainpath"), chainPathIn, true)) //for easy serialization store the unsigned char[32] as hex string. //TODO: use 32byte binary ser.
        throw std::runtime_error("CWallet::GenerateBip32Structure(): write chainpath failed!");

    strChainPath = chainPathIn;
    std::string chainPath = chainPathIn;
    boost::to_lower(chainPath);
    boost::erase_all(chainPath, " ");
    if (chainPath.size() > 0 && chainPath.back() == '/')
        chainPath.resize(chainPath.size() - 1);
    
    std::vector<std::string> pathFragments;
    boost::split(pathFragments, chainPath, boost::is_any_of("/"));
    
    
    CExtKey parentKey;
    
    BOOST_FOREACH(std::string fragment, pathFragments)
    {
        bool harden = false;
        if (fragment.back() == '\'')
            harden = true;

        if (fragment == "m")
        {
            // lets generate a master key seed
            //currently seed size is fixed to 256bit
            if (!useSeed)
            {
                RandAddSeedPerfmon();
                do {
                    GetRandBytes(vch, sizeof(vch));
                } while (!eccrypto::Check(vch));
            }

            CExtKey bip32MasterKey;
            bip32MasterKey.SetMaster(vch, sizeof(vch));

            CBitcoinExtKey b58key;
            b58key.SetKey(bip32MasterKey);
            LogPrintf("key: %s", b58key.ToString());

            strMasterseedHex = HexStr(vch, vch+sizeof(vch));
            uint32_t seedNum = 0;
            if (!walletPrivateDB->Write(std::make_pair(std::string("masterseed"),seedNum), strMasterseedHex, true)) //for easy serialization store the unsigned char[32] as hex string. //TODO: use 32byte binary ser.
                throw std::runtime_error("CWallet::GenerateBip32Structure(): Writing masterkeyid failed!");

            parentKey = bip32MasterKey;
        }
        else if (fragment == "k")
        {
            harden = false;
        }
        else if (fragment == "c")
        {
            harden = false;
            CExtPubKey parentExtPubKey = parentKey.Neuter();
            parentExtPubKey.Derive(externalPubKey, 0);
            parentExtPubKey.Derive(internalPubKey, 1);

            uint32_t keyRingNum = 0;
            if (!walletPrivateDB->Write(std::make_pair(std::string("externalpubkey"),keyRingNum), externalPubKey, true))
                throw std::runtime_error("CWallet::GenerateBip32Structure(): Writing masterkeyid failed!");

            if (!walletPrivateDB->Write(std::make_pair(std::string("internalpubkey"),keyRingNum), internalPubKey, true))
                throw std::runtime_error("CWallet::GenerateBip32Structure(): Writing masterkeyid failed!");
        }
        else
        {
            CExtKey childKey;
            int nIndex = atoi(fragment.c_str());
            parentKey.Derive(childKey, (harden ? 0x80000000 : 0)+nIndex);
            parentKey = childKey;
        }
    }

    return true;
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

CPubKey Wallet::GetNextUnusedKey(unsigned int& usedIndex)
{

//
//            internalPubKey = internalKey.Neuter();
//            externalPubKey = externalKey.Neuter();
//
//            CExtPubKey childPubKey = childKey.Neuter();
//            int64_t nCreationTime = GetTime();
//
//            CKeyMetadata metaInternal(nCreationTime);
//            metaInternal.keyidParent = childPubKey.pubkey.GetID();
//            metaInternal.nDepth = internalPubKey.nDepth;
//            metaInternal.nDerivationIndex = internalPubKey.nChild;
//            mapKeyMetadata[internalPubKey.pubkey.GetID()] = metaInternal;
//
//            CKeyMetadata metaExternal(nCreationTime);
//            metaExternal.keyidParent = childPubKey.pubkey.GetID();
//            metaExternal.nDepth = internalPubKey.nDepth;
//            metaExternal.nDerivationIndex = internalPubKey.nChild;
//            mapKeyMetadata[externalPubKey.pubkey.GetID()] = metaExternal;


    usedIndex = internalNextPos;
    CPubKey pubKey = GetKeyAtIndex(internalNextPos++);
    return pubKey;
}

CPubKey Wallet::GetKeyAtIndex(unsigned int index, bool internal)
{
    //check if this key is already generated

    CExtPubKey childKey;
    externalPubKey.Derive(childKey,index);

    int64_t nCreationTime = GetTime();
    CKeyMetadata meta(nCreationTime);
    meta.keyidParent = externalPubKey.pubkey.GetID();
    meta.nDepth = childKey.nDepth;
    meta.nDerivationIndex = childKey.nChild;
    mapKeyMetadata[childKey.pubkey.GetID()] = meta;

    if (!walletCacheDB->Write(std::make_pair(std::string("extpubkey"),childKey.pubkey.GetID()), childKey, true))
        throw std::runtime_error("CWallet::GenerateBip32Structure(): Writing masterkeyid failed!");

    externalUsedKeysCache.insert(childKey.nDepth);

    return childKey.pubkey;
}

bool Wallet::LoadKey(const CKey& key, const CPubKey &pubkey)
{
    return CCryptoKeyStore::AddKeyPubKey(key, pubkey);
}

bool Wallet::AddKeyPubKey(const CKey& secret, const CPubKey &pubkey)
{
    if (!CCryptoKeyStore::AddKeyPubKey(secret, pubkey))
        return false;
    
    return walletCacheDB->WriteKey(pubkey, secret.GetPrivKey(), mapKeyMetadata[pubkey.GetID()]);
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
    
    return walletCacheDB->Write(make_pair(std::string("adrmeta"), CBitcoinAddress(address).ToString()), mapAddressBook[address]);
}

}; //end namespace