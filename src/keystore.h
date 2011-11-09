// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2011 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_KEYSTORE_H
#define BITCOIN_KEYSTORE_H

#include "crypter.h"

// A virtual base class for key stores
class CKeyStore
{
protected:
    mutable CCriticalSection cs_KeyStore;

public:
    // Add a key to the store.
    virtual bool AddKey(const CKey& key) =0;

    // Check whether a key corresponding to a given address is present in the store.
    virtual bool HaveKey(const CBitcoinAddress &address) const =0;

    // Retrieve a key corresponding to a given address from the store.
    // Return true if succesful.
    virtual bool GetKey(const CBitcoinAddress &address, CKey& keyOut) const =0;

    // Retrieve only the public key corresponding to a given address.
    // This may succeed even if GetKey fails (e.g., encrypted wallets)
    virtual bool GetPubKey(const CBitcoinAddress &address, std::vector<unsigned char>& vchPubKeyOut) const;

    // Generate a new key, and add it to the store
    virtual std::vector<unsigned char> GenerateNewKey();
};

typedef std::map<CBitcoinAddress, CSecret> KeyMap;

// Basic key store, that keeps keys in an address->secret map
class CBasicKeyStore : public CKeyStore
{
protected:
    KeyMap mapKeys;

public:
    bool AddKey(const CKey& key);
    bool HaveKey(const CBitcoinAddress &address) const
    {
        bool result;
        CRITICAL_BLOCK(cs_KeyStore)
            result = (mapKeys.count(address) > 0);
        return result;
    }
    bool GetKey(const CBitcoinAddress &address, CKey& keyOut) const
    {
        CRITICAL_BLOCK(cs_KeyStore)
        {
            KeyMap::const_iterator mi = mapKeys.find(address);
            if (mi != mapKeys.end())
            {
                keyOut.SetSecret((*mi).second);
                return true;
            }
        }
        return false;
    }
};

typedef std::map<CBitcoinAddress, std::pair<std::vector<unsigned char>, std::vector<unsigned char> > > CryptedKeyMap;

// Keystore which keeps the private keys encrypted
// It derives from the basic key store, which is used if no encryption is active.
class CCryptoKeyStore : public CBasicKeyStore
{
private:
    CryptedKeyMap mapCryptedKeys;

    CKeyingMaterial vMasterKey;

    // if fUseCrypto is true, mapKeys must be empty
    // if fUseCrypto is false, vMasterKey must be empty
    bool fUseCrypto;

protected:
    bool SetCrypted();

    // will encrypt previously unencrypted keys
    bool EncryptKeys(CKeyingMaterial& vMasterKeyIn);

    bool Unlock(const CKeyingMaterial& vMasterKeyIn);

public:
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
        bool result;
        CRITICAL_BLOCK(cs_KeyStore)
            result = vMasterKey.empty();
        return result;
    }

    bool Lock()
    {
        if (!SetCrypted())
            return false;

        CRITICAL_BLOCK(cs_KeyStore)
            vMasterKey.clear();

        return true;
    }

    virtual bool AddCryptedKey(const std::vector<unsigned char> &vchPubKey, const std::vector<unsigned char> &vchCryptedSecret);
    std::vector<unsigned char> GenerateNewKey();
    bool AddKey(const CKey& key);
    bool HaveKey(const CBitcoinAddress &address) const
    {
        CRITICAL_BLOCK(cs_KeyStore)
        {
            if (!IsCrypted())
                return CBasicKeyStore::HaveKey(address);
            return mapCryptedKeys.count(address) > 0;
        }
        return false;
    }
    bool GetKey(const CBitcoinAddress &address, CKey& keyOut) const;
    bool GetPubKey(const CBitcoinAddress &address, std::vector<unsigned char>& vchPubKeyOut) const;
};

#endif
