// Copyright (c) 2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "wallet/hdkeystore.h"

#include "base58.h"
#include "util.h"
#include "utilstrencodings.h"

#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>

bool CHDKeyStore::AddMasterSeed(const HDChainID& chainID, const CKeyingMaterial& masterSeed)
{
    LOCK(cs_KeyStore);
    if (IsCrypted())
    {
        if (IsLocked())
            return false;

        std::vector<unsigned char> vchCryptedSecret;
        CKeyingMaterial emptyKey; //an empty key will tell EncryptSeed() to use the internal vMasterKey
        if (!EncryptSeed(emptyKey, masterSeed, chainID, vchCryptedSecret))
            return false;

        mapHDCryptedMasterSeeds[chainID] = vchCryptedSecret;
        return true;
    }
    mapHDMasterSeeds[chainID] = masterSeed;
    return true;
}

bool CHDKeyStore::AddCryptedMasterSeed(const HDChainID& chainID, const std::vector<unsigned char>& vchCryptedSecret)
{
    LOCK(cs_KeyStore);
    mapHDCryptedMasterSeeds[chainID] = vchCryptedSecret;
    return true;
}

bool CHDKeyStore::GetMasterSeed(const HDChainID& chainID, CKeyingMaterial& seedOut) const
{
    LOCK(cs_KeyStore);
    if (!IsCrypted())
    {
        std::map<HDChainID, CKeyingMaterial >::const_iterator it=mapHDMasterSeeds.find(chainID);
        if (it == mapHDMasterSeeds.end())
            return false;

        seedOut = it->second;
        return true;
    }
    else
    {
        if (IsLocked())
            return false;

        std::map<HDChainID, std::vector<unsigned char> >::const_iterator it=mapHDCryptedMasterSeeds.find(chainID);
        if (it == mapHDCryptedMasterSeeds.end())
            return false;

        std::vector<unsigned char> vchCryptedSecret = it->second;
        CKeyingMaterial emptyKey; //an empty key will tell DecryptSeed() to use the internal vMasterKey
        if (!DecryptSeed(emptyKey, vchCryptedSecret, chainID, seedOut))
            return false;

        return true;
    }
    return false;
}

bool CHDKeyStore::EncryptSeeds(CKeyingMaterial& vMasterKeyIn)
{
    LOCK(cs_KeyStore);
    for (std::map<HDChainID, CKeyingMaterial >::iterator it = mapHDMasterSeeds.begin(); it != mapHDMasterSeeds.end(); ++it)
    {
        std::vector<unsigned char> vchCryptedSecret;
        if (!EncryptSeed(vMasterKeyIn, it->second, it->first, vchCryptedSecret))
            return false;
        AddCryptedMasterSeed(it->first, vchCryptedSecret);
    }
    mapHDMasterSeeds.clear();
    if (!SetCrypted())
        return false;

    return true;
}

bool CHDKeyStore::GetCryptedMasterSeed(const HDChainID& chainID, std::vector<unsigned char>& vchCryptedSecret) const
{
    LOCK(cs_KeyStore);
    if (!IsCrypted())
        return false;

    std::map<HDChainID, std::vector<unsigned char> >::const_iterator it=mapHDCryptedMasterSeeds.find(chainID);
    if (it == mapHDCryptedMasterSeeds.end())
        return false;

    vchCryptedSecret = it->second;
    return true;
}

void CHDKeyStore::GetAvailableChainIDs(std::vector<HDChainID>& chainIDs)
{
    LOCK(cs_KeyStore);
    chainIDs.clear();

    if (IsCrypted())
    {
        for (std::map<HDChainID, std::vector<unsigned char> >::iterator it = mapHDCryptedMasterSeeds.begin(); it != mapHDCryptedMasterSeeds.end(); ++it) {
            chainIDs.push_back(it->first);
        }
    }
    else
    {
        for (std::map<HDChainID, CKeyingMaterial >::iterator it = mapHDMasterSeeds.begin(); it != mapHDMasterSeeds.end(); ++it) {
            chainIDs.push_back(it->first);
        }
    }
}

bool CHDKeyStore::PrivateKeyDerivation(const std::string keypath, const HDChainID& chainID, CExtKey& extKeyOut) const
{
    std::vector<std::string> pathFragments;
    boost::split(pathFragments, keypath, boost::is_any_of("/"));

    CExtKey extKey;
    CExtKey parentKey;
    BOOST_FOREACH(std::string fragment, pathFragments)
    {
        bool harden = false;
        if (*fragment.rbegin() == '\'' || *fragment.rbegin() == 'h')
        {
            harden = true;
            fragment = fragment.substr(0,fragment.size()-1);
        }

        if (fragment == "m")
        {
            CExtKey bip32MasterKey;
            CKeyingMaterial masterSeed;

            // get master seed
            if (!GetMasterSeed(chainID, masterSeed))
                return false;

            bip32MasterKey.SetMaster(&masterSeed[0], masterSeed.size());
            parentKey = bip32MasterKey;
        }
        else if (fragment == "c")
            return false;
        else
        {
            CExtKey childKey;
            int32_t nIndex;
            if (!ParseInt32(fragment,&nIndex))
                return false;
            parentKey.Derive(childKey, (harden ? 0x80000000 : 0)+nIndex);
            parentKey = childKey;
        }
    }
    extKeyOut = parentKey;
    return true;
}

bool CHDKeyStore::DeriveKey(const HDChainID chainID, const std::string keypath, CKey& keyOut) const
{
    //this methode requires no locking
    CExtKey extKeyOut;
    if (!PrivateKeyDerivation(keypath, chainID, extKeyOut))
        return false;

    keyOut = extKeyOut.key;
    return true;
}

bool CHDKeyStore::DeriveKeyAtIndex(const HDChainID chainID, CKey& keyOut, std::string& keypathOut, unsigned int nIndex, bool internal) const
{
    CHDChain hdChain;
    if (!GetChain(chainID, hdChain))
        return false;

    if (nIndex >= 0x80000000)
        throw std::runtime_error("CHDKeyStore::DeriveKeyAtIndex(): No more available keys!");

    keypathOut = hdChain.keypathTemplate;
    boost::replace_all(keypathOut, "c", itostr(internal)); //replace the chain switch index

    keypathOut += "/"+itostr(nIndex)+"'"; //add hardened flag

    CExtKey extKeyOut;
    if (!PrivateKeyDerivation(keypathOut, chainID, extKeyOut))
        throw std::runtime_error("CHDKeyStore::DeriveKeyAtIndex(): Private Key Derivation failed!");
    keyOut = extKeyOut.key;

    return true;
}

unsigned int CHDKeyStore::GetNextChildIndex(const HDChainID& chainID, bool internal)
{
    std::vector<unsigned int> vIndices;

    {
        LOCK(cs_KeyStore);

        CHDChain hdChain;
        if (!GetChain(chainID, hdChain))
            return false;

        std::string keypathBase = hdChain.keypathTemplate;
        boost::replace_all(keypathBase, "c", itostr(internal)); //replace the chain switch index

        //get next unused child index
        for (std::map<CKeyID, CKeyMetadata>::iterator it = mapKeyMetadata.begin(); it != mapKeyMetadata.end(); ++it)
        {
            //skip non hd keys
            if (it->second.keypath.size() == 0)
                continue;

            std::string keysBaseKeypath = it->second.keypath.substr(0, it->second.keypath.find_last_of("/"));
            std::string childStr = it->second.keypath.substr(it->second.keypath.find_last_of("/") + 1);
            boost::erase_all(childStr, "'");
            int32_t nChild;
            if(it->second.chainID == chainID &&
               it->second.keypath.substr(0, it->second.keypath.find_last_of("/")) == keypathBase &&
               ParseInt32(childStr, &nChild))
            {
                vIndices.push_back(nChild);
            }
        }
    }

    for (unsigned int i=0;i<0x80000000;i++)
        if (std::find(vIndices.begin(), vIndices.end(), i) == vIndices.end())
            return i;

    return 0;
}

bool CHDKeyStore::AddHDChain(const CHDChain& chain)
{
    LOCK(cs_KeyStore);
    mapChains[chain.chainID] = chain;
    return true;
}

bool CHDKeyStore::GetChain(const HDChainID chainID, CHDChain& chainOut) const
{
    LOCK(cs_KeyStore);
    std::map<HDChainID, CHDChain>::const_iterator it=mapChains.find(chainID);
    if (it == mapChains.end())
        return false;

    chainOut = it->second;
    return true;
}
