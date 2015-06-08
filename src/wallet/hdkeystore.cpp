// Copyright (c) 2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "wallet/hdkeystore.h"

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

bool CHDKeyStore::GetMasterSeed(const HDChainID& hash, CKeyingMaterial& seedOut) const
{
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
    }
    return false;
}