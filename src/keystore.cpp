// Copyright (c) 2009-2011 Satoshi Nakamoto & Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#include "headers.h"
#include "db.h"
#include "crypter.h"

std::vector<unsigned char> CKeyStore::GenerateNewKey()
{
    RandAddSeedPerfmon();
    CKey key;
    key.MakeNewKey();
    if (!AddKey(key))
        throw std::runtime_error("CKeyStore::GenerateNewKey() : AddKey failed");
    return key.GetPubKey();
}

bool CBasicKeyStore::AddKey(const CKey& key)
{
    CRITICAL_BLOCK(cs_mapPubKeys)
    CRITICAL_BLOCK(cs_KeyStore)
    {
        mapKeys[key.GetPubKey()] = key.GetPrivKey();
        mapPubKeys[Hash160(key.GetPubKey())] = key.GetPubKey();
    }
    return true;
}

std::vector<unsigned char> CCryptoKeyStore::GenerateNewKey()
{
    RandAddSeedPerfmon();
    CKey key;
    key.MakeNewKey();
    if (!AddKey(key))
        throw std::runtime_error("CCryptoKeyStore::GenerateNewKey() : AddKey failed");
    return key.GetPubKey();
}

bool CCryptoKeyStore::Unlock(const CKeyingMaterial& vMasterKeyIn)
{
    CRITICAL_BLOCK(cs_vMasterKey)
    {
        if (!SetCrypted())
            return false;

        std::map<std::vector<unsigned char>, std::vector<unsigned char> >::const_iterator mi = mapCryptedKeys.begin();
        for (; mi != mapCryptedKeys.end(); ++mi)
        {
            const std::vector<unsigned char> &vchPubKey = (*mi).first;
            const std::vector<unsigned char> &vchCryptedSecret = (*mi).second;
            CSecret vchSecret;
            if(!DecryptSecret(vMasterKeyIn, vchCryptedSecret, Hash(vchPubKey.begin(), vchPubKey.end()), vchSecret))
                return false;
            CKey key;
            key.SetSecret(vchSecret);
            if (key.GetPubKey() == vchPubKey)
                break;
            return false;
        }
        vMasterKey = vMasterKeyIn;
    }
    return true;
}

bool CCryptoKeyStore::AddKey(const CKey& key)
{
    CRITICAL_BLOCK(cs_KeyStore)
    CRITICAL_BLOCK(cs_vMasterKey)
    {
        if (!IsCrypted())
            return CBasicKeyStore::AddKey(key);

        if (IsLocked())
            return false;

        std::vector<unsigned char> vchCryptedSecret;
        std::vector<unsigned char> vchPubKey = key.GetPubKey();
        if (!EncryptSecret(vMasterKey, key.GetSecret(), Hash(vchPubKey.begin(), vchPubKey.end()), vchCryptedSecret))
            return false;

        if (!AddCryptedKey(key.GetPubKey(), vchCryptedSecret))
            return false;
    }
    return true;
}


bool CCryptoKeyStore::AddCryptedKey(const std::vector<unsigned char> &vchPubKey, const std::vector<unsigned char> &vchCryptedSecret)
{
    CRITICAL_BLOCK(cs_mapPubKeys)
    CRITICAL_BLOCK(cs_KeyStore)
    {
        if (!SetCrypted())
            return false;

        mapCryptedKeys[vchPubKey] = vchCryptedSecret;
        mapPubKeys[Hash160(vchPubKey)] = vchPubKey;
    }
    return true;
}

bool CCryptoKeyStore::GetPrivKey(const std::vector<unsigned char> &vchPubKey, CKey& keyOut) const
{
    CRITICAL_BLOCK(cs_vMasterKey)
    {
        if (!IsCrypted())
            return CBasicKeyStore::GetPrivKey(vchPubKey, keyOut);

        std::map<std::vector<unsigned char>, std::vector<unsigned char> >::const_iterator mi = mapCryptedKeys.find(vchPubKey);
        if (mi != mapCryptedKeys.end())
        {
            const std::vector<unsigned char> &vchCryptedSecret = (*mi).second;
            CSecret vchSecret;
            if (!DecryptSecret(vMasterKey, (*mi).second, Hash((*mi).first.begin(), (*mi).first.end()), vchSecret))
                return false;
            keyOut.SetSecret(vchSecret);
            return true;
        }
    }
    return false;
}

bool CCryptoKeyStore::EncryptKeys(CKeyingMaterial& vMasterKeyIn)
{
    CRITICAL_BLOCK(cs_KeyStore)
    CRITICAL_BLOCK(cs_vMasterKey)
    {
        if (!mapCryptedKeys.empty() || IsCrypted())
            return false;

        fUseCrypto = true;
        CKey key;
        BOOST_FOREACH(KeyMap::value_type& mKey, mapKeys)
        {
            if (!key.SetPrivKey(mKey.second))
                return false;
            std::vector<unsigned char> vchCryptedSecret;
            if (!EncryptSecret(vMasterKeyIn, key.GetSecret(), Hash(mKey.first.begin(), mKey.first.end()), vchCryptedSecret))
                return false;
            if (!AddCryptedKey(mKey.first, vchCryptedSecret))
                return false;
        }
        mapKeys.clear();
    }
    return true;
}
