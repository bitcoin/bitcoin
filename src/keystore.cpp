// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2011 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#include "headers.h"
#include "crypter.h"
#include "db.h"
#include "script.h"

std::vector<unsigned char> CKeyStore::GenerateNewKey()
{
    RandAddSeedPerfmon();
    CKey key;
    key.MakeNewKey();
    if (!AddKey(key))
        throw std::runtime_error("CKeyStore::GenerateNewKey() : AddKey failed");
    return key.GetPubKey();
}

bool CKeyStore::GetPubKey(const CBitcoinAddress &address, std::vector<unsigned char> &vchPubKeyOut) const
{
    CKey key;
    if (!GetKey(address, key))
        return false;
    vchPubKeyOut = key.GetPubKey();
    return true;
}

bool CBasicKeyStore::AddKey(const CKey& key)
{
    bool fCompressed = false;
    CSecret secret = key.GetSecret(fCompressed);
    CRITICAL_BLOCK(cs_KeyStore)
        mapKeys[CBitcoinAddress(key.GetPubKey())] = make_pair(secret, fCompressed);
    return true;
}

bool CBasicKeyStore::AddCScript(const CScript& redeemScript)
{
    CRITICAL_BLOCK(cs_KeyStore)
        mapScripts[Hash160(redeemScript)] = redeemScript;
    return true;
}

bool CBasicKeyStore::HaveCScript(const uint160& hash) const
{
    bool result;
    CRITICAL_BLOCK(cs_KeyStore)
        result = (mapScripts.count(hash) > 0);
    return result;
}


bool CBasicKeyStore::GetCScript(const uint160 &hash, CScript& redeemScriptOut) const
{
    CRITICAL_BLOCK(cs_KeyStore)
    {
        ScriptMap::const_iterator mi = mapScripts.find(hash);
        if (mi != mapScripts.end())
        {
            redeemScriptOut = (*mi).second;
            return true;
        }
    }
    return false;
}

bool CCryptoKeyStore::SetCrypted()
{
    CRITICAL_BLOCK(cs_KeyStore)
    {
        if (fUseCrypto)
            return true;
        if (!mapKeys.empty())
            return false;
        fUseCrypto = true;
    }
    return true;
}

bool CCryptoKeyStore::Unlock(const CKeyingMaterial& vMasterKeyIn)
{
    CRITICAL_BLOCK(cs_KeyStore)
    {
        if (!SetCrypted())
            return false;

        CryptedKeyMap::const_iterator mi = mapCryptedKeys.begin();
        for (; mi != mapCryptedKeys.end(); ++mi)
        {
            const std::vector<unsigned char> &vchPubKey = (*mi).second.first;
            const std::vector<unsigned char> &vchCryptedSecret = (*mi).second.second;
            CSecret vchSecret;
            if(!DecryptSecret(vMasterKeyIn, vchCryptedSecret, Hash(vchPubKey.begin(), vchPubKey.end()), vchSecret))
                return false;
            CKey key;
            key.SetPubKey(vchPubKey);
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
    {
        if (!IsCrypted())
            return CBasicKeyStore::AddKey(key);

        if (IsLocked())
            return false;

        std::vector<unsigned char> vchCryptedSecret;
        std::vector<unsigned char> vchPubKey = key.GetPubKey();
        bool fCompressed;
        if (!EncryptSecret(vMasterKey, key.GetSecret(fCompressed), Hash(vchPubKey.begin(), vchPubKey.end()), vchCryptedSecret))
            return false;

        if (!AddCryptedKey(key.GetPubKey(), vchCryptedSecret))
            return false;
    }
    return true;
}


bool CCryptoKeyStore::AddCryptedKey(const std::vector<unsigned char> &vchPubKey, const std::vector<unsigned char> &vchCryptedSecret)
{
    CRITICAL_BLOCK(cs_KeyStore)
    {
        if (!SetCrypted())
            return false;

        mapCryptedKeys[CBitcoinAddress(vchPubKey)] = make_pair(vchPubKey, vchCryptedSecret);
    }
    return true;
}

bool CCryptoKeyStore::GetKey(const CBitcoinAddress &address, CKey& keyOut) const
{
    CRITICAL_BLOCK(cs_KeyStore)
    {
        if (!IsCrypted())
            return CBasicKeyStore::GetKey(address, keyOut);

        CryptedKeyMap::const_iterator mi = mapCryptedKeys.find(address);
        if (mi != mapCryptedKeys.end())
        {
            const std::vector<unsigned char> &vchPubKey = (*mi).second.first;
            const std::vector<unsigned char> &vchCryptedSecret = (*mi).second.second;
            CSecret vchSecret;
            if (!DecryptSecret(vMasterKey, vchCryptedSecret, Hash(vchPubKey.begin(), vchPubKey.end()), vchSecret))
                return false;
            keyOut.SetPubKey(vchPubKey);
            keyOut.SetSecret(vchSecret);
            return true;
        }
    }
    return false;
}

bool CCryptoKeyStore::GetPubKey(const CBitcoinAddress &address, std::vector<unsigned char>& vchPubKeyOut) const
{
    CRITICAL_BLOCK(cs_KeyStore)
    {
        if (!IsCrypted())
            return CKeyStore::GetPubKey(address, vchPubKeyOut);

        CryptedKeyMap::const_iterator mi = mapCryptedKeys.find(address);
        if (mi != mapCryptedKeys.end())
        {
            vchPubKeyOut = (*mi).second.first;
            return true;
        }
    }
    return false;
}

bool CCryptoKeyStore::EncryptKeys(CKeyingMaterial& vMasterKeyIn)
{
    CRITICAL_BLOCK(cs_KeyStore)
    {
        if (!mapCryptedKeys.empty() || IsCrypted())
            return false;

        fUseCrypto = true;
        BOOST_FOREACH(KeyMap::value_type& mKey, mapKeys)
        {
            CKey key;
            if (!key.SetSecret(mKey.second.first, false))
                return false;
            const std::vector<unsigned char> vchPubKey = key.GetPubKey();
            std::vector<unsigned char> vchCryptedSecret;
            bool fCompressed;
            if (!EncryptSecret(vMasterKeyIn, key.GetSecret(fCompressed), Hash(vchPubKey.begin(), vchPubKey.end()), vchCryptedSecret))
                return false;
            if (!AddCryptedKey(vchPubKey, vchCryptedSecret))
                return false;
        }
        mapKeys.clear();
    }
    return true;
}
