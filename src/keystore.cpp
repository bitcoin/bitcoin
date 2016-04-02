// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "keystore.h"
#include "script.h"
#include "base58.h"

extern bool fWalletUnlockMintOnly;

bool CKeyStore::GetPubKey(const CKeyID &address, CPubKey &vchPubKeyOut) const
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
    {
        LOCK(cs_KeyStore);
        mapKeys[key.GetPubKey().GetID()] = make_pair(secret, fCompressed);
    }
    return true;
}

bool CBasicKeyStore::AddMalleableKey(const CMalleableKeyView& keyView, const CSecret& vchSecretH)
{
    {
        LOCK(cs_KeyStore);
        mapMalleableKeys[CMalleableKeyView(keyView)] = vchSecretH;
    }
    return true;
}

bool CBasicKeyStore::AddCScript(const CScript& redeemScript)
{
    if (redeemScript.size() > MAX_SCRIPT_ELEMENT_SIZE)
        return error("CBasicKeyStore::AddCScript() : redeemScripts > %i bytes are invalid", MAX_SCRIPT_ELEMENT_SIZE);

    {
        LOCK(cs_KeyStore);
        mapScripts[redeemScript.GetID()] = redeemScript;
    }
    return true;
}

bool CBasicKeyStore::HaveCScript(const CScriptID& hash) const
{
    bool result;
    {
        LOCK(cs_KeyStore);
        result = (mapScripts.count(hash) > 0);
    }
    return result;
}


bool CBasicKeyStore::GetCScript(const CScriptID &hash, CScript& redeemScriptOut) const
{
    {
        LOCK(cs_KeyStore);
        ScriptMap::const_iterator mi = mapScripts.find(hash);
        if (mi != mapScripts.end())
        {
            redeemScriptOut = (*mi).second;
            return true;
        }
    }
    return false;
}

bool CBasicKeyStore::AddWatchOnly(const CScript &dest)
{
    LOCK(cs_KeyStore);

    CTxDestination address;
    if (ExtractDestination(dest, address)) {
        CKeyID keyID;
        CBitcoinAddress(address).GetKeyID(keyID);
        if (HaveKey(keyID))
            return false;
    }

    setWatchOnly.insert(dest);
    return true;
}


bool CBasicKeyStore::RemoveWatchOnly(const CScript &dest)
{
    LOCK(cs_KeyStore);
    setWatchOnly.erase(dest);
    return true;
}

bool CBasicKeyStore::HaveWatchOnly(const CScript &dest) const
{
    LOCK(cs_KeyStore);
    return setWatchOnly.count(dest) > 0;
}

bool CBasicKeyStore::HaveWatchOnly() const
{
    LOCK(cs_KeyStore);
    return (!setWatchOnly.empty());
}

bool CCryptoKeyStore::SetCrypted()
{
    {
        LOCK(cs_KeyStore);
        if (fUseCrypto)
            return true;
        if (!mapKeys.empty())
            return false;
        fUseCrypto = true;
    }
    return true;
}

bool CCryptoKeyStore::Lock()
{
    if (!SetCrypted())
        return false;

    {
        LOCK(cs_KeyStore);
        vMasterKey.clear();
        fWalletUnlockMintOnly = false;
    }

    NotifyStatusChanged(this);
    return true;
}

bool CCryptoKeyStore::Unlock(const CKeyingMaterial& vMasterKeyIn)
{
    {
        LOCK(cs_KeyStore);
        if (!SetCrypted())
            return false;

        CryptedKeyMap::const_iterator mi = mapCryptedKeys.begin();
        for (; mi != mapCryptedKeys.end(); ++mi)
        {
            const CPubKey &vchPubKey = (*mi).second.first;
            const std::vector<unsigned char> &vchCryptedSecret = (*mi).second.second;
            CSecret vchSecret;
            if(!DecryptSecret(vMasterKeyIn, vchCryptedSecret, vchPubKey.GetHash(), vchSecret))
                return false;
            if (vchSecret.size() != 32)
                return false;
            CKey key;
            key.SetSecret(vchSecret);
            key.SetCompressedPubKey(vchPubKey.IsCompressed());
            if (key.GetPubKey() == vchPubKey)
                break;
            return false;
        }

        vMasterKey = vMasterKeyIn;
    }
    NotifyStatusChanged(this);
    return true;
}

bool CCryptoKeyStore::AddKey(const CKey& key)
{
    {
        LOCK(cs_KeyStore);

        CScript script;
        script.SetDestination(key.GetPubKey().GetID());

        if (HaveWatchOnly(script))
            return false;

        if (!IsCrypted())
            return CBasicKeyStore::AddKey(key);

        if (IsLocked())
            return false;

        std::vector<unsigned char> vchCryptedSecret;
        CPubKey vchPubKey = key.GetPubKey();
        bool fCompressed;
        if (!EncryptSecret(vMasterKey, key.GetSecret(fCompressed), vchPubKey.GetHash(), vchCryptedSecret))
            return false;

        if (!AddCryptedKey(key.GetPubKey(), vchCryptedSecret))
            return false;
    }
    return true;
}

bool CCryptoKeyStore::AddMalleableKey(const CMalleableKeyView& keyView, const CSecret &vchSecretH)
{
    {
        LOCK(cs_KeyStore);
        if (!SetCrypted())
            return CBasicKeyStore::AddMalleableKey(keyView, vchSecretH);

        if (IsLocked())
            return false;

        CKey keyH;
        keyH.SetSecret(vchSecretH, true);

        std::vector<unsigned char> vchCryptedSecretH;
        if (!EncryptSecret(vMasterKey, vchSecretH, keyH.GetPubKey().GetHash(), vchCryptedSecretH))
            return false;

        if (!AddCryptedMalleableKey(keyView, vchCryptedSecretH))
            return false;
    }
    return true;
}

bool CCryptoKeyStore::AddCryptedKey(const CPubKey &vchPubKey, const std::vector<unsigned char> &vchCryptedSecret)
{
    {
        LOCK(cs_KeyStore);
        if (!SetCrypted())
            return false;

        mapCryptedKeys[vchPubKey.GetID()] = make_pair(vchPubKey, vchCryptedSecret);
    }
    return true;
}

bool CCryptoKeyStore::AddCryptedMalleableKey(const CMalleableKeyView& keyView, const std::vector<unsigned char>  &vchCryptedSecretH)
{
    {
        LOCK(cs_KeyStore);
        if (!SetCrypted())
            return false;

        mapCryptedMalleableKeys[CMalleableKeyView(keyView)] = vchCryptedSecretH;
    }
    return true;
}

bool CCryptoKeyStore::CreatePrivKey(const CPubKey &pubKeyVariant, const CPubKey &R, CKey &privKey) const
{
    {
        LOCK(cs_KeyStore);
        if (!IsCrypted())
            return CBasicKeyStore::CreatePrivKey(pubKeyVariant, R, privKey);

        for (CryptedMalleableKeyMap::const_iterator mi = mapCryptedMalleableKeys.begin(); mi != mapCryptedMalleableKeys.end(); mi++)
        {
            if (mi->first.CheckKeyVariant(R, pubKeyVariant))
            {
                const CPubKey H = mi->first.GetMalleablePubKey().GetH();

                CSecret vchSecretH;
                if (!DecryptSecret(vMasterKey, mi->second, H.GetHash(), vchSecretH))
                    return false;
                if (vchSecretH.size() != 32)
                    return false;

                CMalleableKey mKey = mi->first.GetMalleableKey(vchSecretH);
                return mKey.CheckKeyVariant(R, pubKeyVariant, privKey);;
            }
        }

    }
    return true;
}

bool CCryptoKeyStore::GetMalleableKey(const CMalleableKeyView &keyView, CMalleableKey &mKey) const
{
    {
        LOCK(cs_KeyStore);
        if (!IsCrypted())
            return CBasicKeyStore::GetMalleableKey(keyView, mKey);
        CryptedMalleableKeyMap::const_iterator mi = mapCryptedMalleableKeys.find(keyView);
        if (mi != mapCryptedMalleableKeys.end())
        {
            const CPubKey H = keyView.GetMalleablePubKey().GetH();

            CSecret vchSecretH;
            if (!DecryptSecret(vMasterKey, mi->second, H.GetHash(), vchSecretH))
                return false;

            if (vchSecretH.size() != 32)
                return false;
            mKey = mi->first.GetMalleableKey(vchSecretH);

            return true;
        }
    }
    return false;
}

bool CCryptoKeyStore::GetKey(const CKeyID &address, CKey& keyOut) const
{
    {
        LOCK(cs_KeyStore);
        if (!IsCrypted())
            return CBasicKeyStore::GetKey(address, keyOut);

        CryptedKeyMap::const_iterator mi = mapCryptedKeys.find(address);
        if (mi != mapCryptedKeys.end())
        {
            const CPubKey &vchPubKey = (*mi).second.first;
            const std::vector<unsigned char> &vchCryptedSecret = (*mi).second.second;
            CSecret vchSecret;
            if (!DecryptSecret(vMasterKey, vchCryptedSecret, vchPubKey.GetHash(), vchSecret))
                return false;
            if (vchSecret.size() != 32)
                return false;
            keyOut.SetSecret(vchSecret);
            keyOut.SetCompressedPubKey(vchPubKey.IsCompressed());
            return true;
        }
    }
    return false;
}

bool CCryptoKeyStore::GetPubKey(const CKeyID &address, CPubKey& vchPubKeyOut) const
{
    {
        LOCK(cs_KeyStore);
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
    {
        LOCK(cs_KeyStore);
        if (!mapCryptedKeys.empty() || IsCrypted())
            return false;

        fUseCrypto = true;
        BOOST_FOREACH(KeyMap::value_type& mKey, mapKeys)
        {
            CKey key;
            if (!key.SetSecret(mKey.second.first, mKey.second.second))
                return false;
            const CPubKey vchPubKey = key.GetPubKey();
            std::vector<unsigned char> vchCryptedSecret;
            bool fCompressed;
            if (!EncryptSecret(vMasterKeyIn, key.GetSecret(fCompressed), vchPubKey.GetHash(), vchCryptedSecret))
                return false;
            if (!AddCryptedKey(vchPubKey, vchCryptedSecret))
                return false;
        }
        mapKeys.clear();

        BOOST_FOREACH(MalleableKeyMap::value_type& mKey, mapMalleableKeys)
        {
            const CPubKey vchPubKeyH = mKey.first.GetMalleablePubKey().GetH();
            std::vector<unsigned char> vchCryptedSecretH;
            if (!EncryptSecret(vMasterKeyIn, mKey.second, vchPubKeyH.GetHash(), vchCryptedSecretH))
                return false;
            if (!AddCryptedMalleableKey(mKey.first, vchCryptedSecretH))
                return false;
        }
        mapMalleableKeys.clear();
    }
    return true;
}

bool CCryptoKeyStore::DecryptKeys(const CKeyingMaterial& vMasterKeyIn)
{
    {
        LOCK(cs_KeyStore);
        if (!IsCrypted())
            return false;

        CryptedKeyMap::const_iterator mi = mapCryptedKeys.begin();
        for (; mi != mapCryptedKeys.end(); ++mi)
        {
            const CPubKey &vchPubKey = (*mi).second.first;
            const std::vector<unsigned char> &vchCryptedSecret = (*mi).second.second;
            CSecret vchSecret;
            if(!DecryptSecret(vMasterKeyIn, vchCryptedSecret, vchPubKey.GetHash(), vchSecret))
                return false;
            if (vchSecret.size() != 32)
                return false;
            CKey key;
            key.SetSecret(vchSecret);
            key.SetCompressedPubKey(vchPubKey.IsCompressed());
            if (!CBasicKeyStore::AddKey(key))
                return false;
        }

        mapCryptedKeys.clear();

        CryptedMalleableKeyMap::const_iterator mi2 = mapCryptedMalleableKeys.begin();
        for(; mi2 != mapCryptedMalleableKeys.end(); ++mi2)
        {
            const CPubKey vchPubKeyH = mi2->first.GetMalleablePubKey().GetH();

            CSecret vchSecretH;
            if(!DecryptSecret(vMasterKeyIn, mi2->second, vchPubKeyH.GetHash(), vchSecretH))
                return false;
            if (vchSecretH.size() != 32)
                return false;

            if (!CBasicKeyStore::AddMalleableKey(mi2->first, vchSecretH))
                return false;
        }
        mapCryptedMalleableKeys.clear();
    }

    return true;
}
