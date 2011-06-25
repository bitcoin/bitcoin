// Copyright (c) 2009-2011 Satoshi Nakamoto & Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#include "headers.h"
#include "db.h"

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
    CRITICAL_BLOCK(cs_KeyStore)
    {
        mapKeys[key.GetPubKey()] = key.GetPrivKey();
        mapPubKeys[Hash160(key.GetPubKey())] = key.GetPubKey();
    }
    return true;
}

bool CCryptoKeyStore::Unlock(const CMasterKey& vMasterKeyIn)
{
    if (!SetCrypted())
        return false;

    std::map<std::vector<unsigned char>, std::vector<unsigned char> >::const_iterator mi = mapCryptedKeys.begin();
    for (; mi != mapCryptedKeys.end(); ++mi)
    {
        const std::vector<unsigned char> &vchPubKey = (*mi).first;
        const std::vector<unsigned char> &vchCryptedSecret = (*mi).second;
        CSecret vchSecret;
        // decrypt vchCryptedSecret using vMasterKeyIn, into vchSecret
        CKey key;
        key.SetSecret(vchSecret);
        if (key.GetPubKey() == vchPubKey)
            break;
        return false;
    }
    vMasterKey = vMasterKeyIn;
    return true;
}

bool CCryptoKeyStore::AddKey(const CKey& key)
{
    CRITICAL_BLOCK(cs_KeyStore)
    {
        if (!IsCrypted())
            return CBasicKeyStore::AddKey(key);

        if (IsLocked())
            return false;

        CSecret vchSecret = key.GetSecret();

        std::vector<unsigned char> vchCryptedSecret;
        // encrypt vchSecret using vMasterKey, into vchCryptedSecret

        AddCryptedKey(key.GetPubKey(), vchCryptedSecret);
    }
    return true;
}


bool CCryptoKeyStore::AddCryptedKey(const std::vector<unsigned char> &vchPubKey, const std::vector<unsigned char> &vchCryptedSecret)
{
    CRITICAL_BLOCK(cs_KeyStore)
    {
        if (!SetCrypted())
            return false;

        mapCryptedKeys[vchPubKey] = vchCryptedSecret;
        mapPubKeys[Hash160(vchPubKey)] = vchPubKey;
    }
    return true;
}

bool CCryptoKeyStore::GetPrivKey(const std::vector<unsigned char> &vchPubKey, CPrivKey& keyOut) const
{
    if (!IsCrypted())
        return CBasicKeyStore::GetPrivKey(vchPubKey, keyOut);

    std::map<std::vector<unsigned char>, std::vector<unsigned char> >::const_iterator mi = mapCryptedKeys.find(vchPubKey);
    if (mi != mapCryptedKeys.end())
    {
        const std::vector<unsigned char> &vchCryptedSecret = (*mi).second;
        CSecret vchSecret;
        // decrypt vchCryptedSecret using vMasterKey into vchSecret;
        CKey key;
        key.SetSecret(vchSecret);
        keyOut = key.GetPrivKey();
        return true;
    }
    return false;
}

bool CCryptoKeyStore::GenerateMasterKey()
{
    if (!mapCryptedKeys.empty())
        return false;

    RandAddSeedPerfmon();

    vMasterKey.resize(32);
    RAND_bytes(&vMasterKey[0], 32);

    if (!IsCrypted())
    {
        // upgrade wallet
        fUseCrypto = true;
    }

    return true;
}
