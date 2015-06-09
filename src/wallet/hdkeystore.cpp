// Copyright (c) 2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "wallet/hdkeystore.h"

#include "base58.h"
#include "util.h"
#include "utilstrencodings.h"

#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>

bool CHDKeyStore::AddMasterSeed(const HDChainID& hash, const CKeyingMaterial& masterSeed)
{
    LOCK(cs_KeyStore);
    if (IsCrypted())
    {
        std::vector<unsigned char> vchCryptedSecret;
        if (!EncryptSeed(masterSeed, hash, vchCryptedSecret))
            return false;

        mapHDCryptedMasterSeeds[hash] = vchCryptedSecret;
        return true;
    }
    mapHDMasterSeeds[hash] = masterSeed;
    return true;
}

bool CHDKeyStore::AddCryptedMasterSeed(const HDChainID& hash, const std::vector<unsigned char>& vchCryptedSecret)
{
    LOCK(cs_KeyStore);
    mapHDCryptedMasterSeeds[hash] = vchCryptedSecret;
    return true;
}

bool CHDKeyStore::GetMasterSeed(const HDChainID& hash, CKeyingMaterial& seedOut) const
{
    LOCK(cs_KeyStore);
    if (!IsCrypted())
    {
        std::map<HDChainID, CKeyingMaterial >::const_iterator it=mapHDMasterSeeds.find(hash);
        if (it == mapHDMasterSeeds.end())
            return false;

        seedOut = it->second;
        return true;
    }
    else
    {
        std::map<HDChainID, std::vector<unsigned char> >::const_iterator it=mapHDCryptedMasterSeeds.find(hash);
        if (it == mapHDCryptedMasterSeeds.end())
            return false;

        std::vector<unsigned char> vchCryptedSecret = it->second;
        if (!DecryptSeed(vchCryptedSecret, hash, seedOut))
            return false;

        return true;
    }
    return false;
}

bool CHDKeyStore::EncryptSeeds()
{
    LOCK(cs_KeyStore);
    for (std::map<HDChainID, CKeyingMaterial >::iterator it = mapHDMasterSeeds.begin(); it != mapHDMasterSeeds.end(); ++it)
    {
        std::vector<unsigned char> vchCryptedSecret;
        if (!EncryptSeed(it->second, it->first, vchCryptedSecret))
            return false;
        AddCryptedMasterSeed(it->first, vchCryptedSecret);
    }
    mapHDMasterSeeds.clear();
    return true;
}

bool CHDKeyStore::GetCryptedMasterSeed(const HDChainID& hash, std::vector<unsigned char>& vchCryptedSecret) const
{
    LOCK(cs_KeyStore);
    if (!IsCrypted())
        return false;

    std::map<HDChainID, std::vector<unsigned char> >::const_iterator it=mapHDCryptedMasterSeeds.find(hash);
    if (it == mapHDCryptedMasterSeeds.end())
        return false;

    vchCryptedSecret = it->second;
    return true;
}

bool CHDKeyStore::HaveKey(const CKeyID &address) const
{
    LOCK(cs_KeyStore);
    if (mapHDPubKeys.count(address) > 0)
        return true;

    return CCryptoKeyStore::HaveKey(address);
}

bool CHDKeyStore::LoadHDPubKey(const CHDPubKey &pubkey)
{
    LOCK(cs_KeyStore);
    mapHDPubKeys[pubkey.pubkey.GetID()] = pubkey;
    return true;
}

bool CHDKeyStore::GetAvailableChainIDs(std::vector<HDChainID>& chainIDs)
{
    LOCK(cs_KeyStore);
    chainIDs.clear();

    if (IsCrypted())
    {
        for(std::map<HDChainID, std::vector<unsigned char> >::iterator it = mapHDCryptedMasterSeeds.begin(); it != mapHDCryptedMasterSeeds.end(); ++it) {
            chainIDs.push_back(it->first);
        }
    }
    else
    {
        for(std::map<HDChainID, CKeyingMaterial >::iterator it = mapHDMasterSeeds.begin(); it != mapHDMasterSeeds.end(); ++it) {
            chainIDs.push_back(it->first);
        }
    }
    
    return true;
}

bool CHDKeyStore::GetKey(const CKeyID &address, CKey &keyOut) const
{
    LOCK(cs_KeyStore);

    std::map<CKeyID, CHDPubKey>::const_iterator mi = mapHDPubKeys.find(address);
    if (mi != mapHDPubKeys.end())
    {
        if (!DeriveKey(mi->second, keyOut))
            return false;

        return true;
    }

    return CCryptoKeyStore::GetKey(address, keyOut);
}

bool CHDKeyStore::DeriveKey(const CHDPubKey hdPubKey, CKey& keyOut) const
{
    //this methode required no locking
    
    std::string chainPath = hdPubKey.chainPath;
    std::vector<std::string> pathFragments;
    boost::split(pathFragments, chainPath, boost::is_any_of("/"));

    CExtKey extKey;
    CExtKey parentKey;
    BOOST_FOREACH(std::string fragment, pathFragments)
    {
        bool harden = false;
        if (fragment.back() == '\'')
        {
            harden = true;
            fragment = fragment.substr(0,fragment.size()-1);
        }

        if (fragment == "m")
        {
            CExtKey bip32MasterKey;
            CKeyingMaterial masterSeed;

            // get master seed
            if (!GetMasterSeed(hdPubKey.chainHash, masterSeed))
                return false;

            bip32MasterKey.SetMaster(&masterSeed[0], masterSeed.size());
            parentKey = bip32MasterKey;
        }
        else if (fragment == "c")
        {
            return false;
        }
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
    keyOut = parentKey.key;
    return true;
}

bool CHDKeyStore::DeriveHDPubKeyAtIndex(const HDChainID chainId, CHDPubKey& hdPubKeyOut, unsigned int nIndex, bool internal) const
{
    CHDChain hdChain;
    if (!GetChain(chainId, hdChain))
        return false;

    if ( (internal && !hdChain.internalPubKey.pubkey.IsValid()) || !hdChain.externalPubKey.pubkey.IsValid())
        throw std::runtime_error("CHDKeyStore::HDGetChildPubKeyAtIndex(): Missing HD extended pubkey!");

    if (nIndex >= 0x80000000)
        throw std::runtime_error("CHDKeyStore::HDGetChildPubKeyAtIndex(): No more available keys!");

    CExtPubKey useExtKey = internal ? hdChain.internalPubKey : hdChain.externalPubKey;
    CExtPubKey childKey;
    if (!useExtKey.Derive(childKey, nIndex))
        throw std::runtime_error("CHDKeyStore::HDGetChildPubKeyAtIndex(): Key deriving failed!");

    hdPubKeyOut.pubkey = childKey.pubkey;
    hdPubKeyOut.chainHash = chainId;
    hdPubKeyOut.nChild = nIndex;
    hdPubKeyOut.chainPath = hdChain.chainPath;
    boost::replace_all(hdPubKeyOut.chainPath, "c", itostr(internal)); //replace the chain switch index
    hdPubKeyOut.chainPath += "/"+itostr(nIndex);

    return true;
}

unsigned int CHDKeyStore::GetNextChildIndex(const HDChainID& chainId, bool internal)
{
    std::vector<unsigned int> vIndices;

    {
        LOCK(cs_KeyStore);
        //get next unused child index
        for(std::map<CKeyID, CHDPubKey>::iterator it = mapHDPubKeys.begin(); it != mapHDPubKeys.end(); ++it)
            if (it->second.chainHash == chainId)
                vIndices.push_back(it->second.nChild);
    }

    for(unsigned int i=0;i<0x80000000;i++)
        if (std::find(vIndices.begin(), vIndices.end(), i) == vIndices.end())
            return i;

    return 0;
}

bool CHDKeyStore::AddChain(const CHDChain& chain)
{
    LOCK(cs_KeyStore);
    mapChains[chain.chainHash] = chain;
    return true;
}

bool CHDKeyStore::GetChain(const HDChainID chainId, CHDChain& chainOut) const
{
    LOCK(cs_KeyStore);
    std::map<HDChainID, CHDChain>::const_iterator it=mapChains.find(hash);
    if (it == mapChains.end())
        return false;

    chainOut = it->second;
    return true;
}