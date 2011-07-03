// Copyright (c) 2009-2011 Satoshi Nakamoto & Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_KEYSTORE_H
#define BITCOIN_KEYSTORE_H

#include "crypter.h"

class CKeyStore
{
public:
    mutable CCriticalSection cs_KeyStore;

    virtual bool AddKey(const CKey& key) =0;
    virtual bool HaveKey(const std::vector<unsigned char> &vchPubKey) const =0;
    virtual bool GetPrivKey(const std::vector<unsigned char> &vchPubKey, CKey& keyOut) const =0;
    virtual std::vector<unsigned char> GenerateNewKey();
};

typedef std::map<std::vector<unsigned char>, CPrivKey> KeyMap;

class CBasicKeyStore : public CKeyStore
{
protected:
    KeyMap mapKeys;

public:
    bool AddKey(const CKey& key);
    bool HaveKey(const std::vector<unsigned char> &vchPubKey) const
    {
        return (mapKeys.count(vchPubKey) > 0);
    }
    bool GetPrivKey(const std::vector<unsigned char> &vchPubKey, CKey& keyOut) const
    {
        std::map<std::vector<unsigned char>, CPrivKey>::const_iterator mi = mapKeys.find(vchPubKey);
        if (mi != mapKeys.end())
        {
            keyOut.SetPrivKey((*mi).second);
            return true;
        }
        return false;
    }
};

class CCryptoKeyStore : public CBasicKeyStore
{
private:
    std::map<std::vector<unsigned char>, std::vector<unsigned char> > mapCryptedKeys;

    CKeyingMaterial vMasterKey;

    // if fUseCrypto is true, mapKeys must be empty
    // if fUseCrypto is false, vMasterKey must be empty
    bool fUseCrypto;

protected:
    bool SetCrypted()
    {
        if (fUseCrypto)
            return true;
        if (!mapKeys.empty())
            return false;
        fUseCrypto = true;
        return true;
    }

    // will encrypt previously unencrypted keys
    bool EncryptKeys(CKeyingMaterial& vMasterKeyIn);

    bool Unlock(const CKeyingMaterial& vMasterKeyIn);

public:
    mutable CCriticalSection cs_vMasterKey; //No guarantees master key wont get locked before you can use it, so lock this first

    CCryptoKeyStore() : fUseCrypto(false)
    {
    }

    bool IsCrypted() const
    {
        return fUseCrypto;
    }

    bool IsLocked() const
    {
        if (!IsCrypted())
            return false;
        return vMasterKey.empty();
    }

    bool Lock()
    {
        CRITICAL_BLOCK(cs_vMasterKey)
        {
            if (!SetCrypted())
                return false;

            vMasterKey.clear();
        }
        return true;
    }

    virtual bool AddCryptedKey(const std::vector<unsigned char> &vchPubKey, const std::vector<unsigned char> &vchCryptedSecret);
    std::vector<unsigned char> GenerateNewKey();
    bool AddKey(const CKey& key);
    bool HaveKey(const std::vector<unsigned char> &vchPubKey) const
    {
        if (!IsCrypted())
            return CBasicKeyStore::HaveKey(vchPubKey);
        return mapCryptedKeys.count(vchPubKey) > 0;
    }
    bool GetPrivKey(const std::vector<unsigned char> &vchPubKey, CKey& keyOut) const;
};

#endif
