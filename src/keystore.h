// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_KEYSTORE_H
#define BITCOIN_KEYSTORE_H

#include "crypter.h"
#include "util.h"
#include "base58.h"

class CScript;

/** A virtual base class for key stores */
class CKeyStore
{
protected:
    mutable CCriticalSection cs_KeyStore;

public:
    virtual ~CKeyStore() {}

    // Add a key to the store.
    virtual bool AddKey(const CKey& key) =0;

    // Check whether a key corresponding to a given address is present in the store.
    virtual bool HaveKey(const CBitcoinAddress &address) const =0;
    virtual bool GetKey(const CBitcoinAddress &address, CKey& keyOut) const =0;
    virtual void GetKeys(std::set<CBitcoinAddress> &setAddress) const =0;
    virtual bool GetPubKey(const CBitcoinAddress &address, std::vector<unsigned char>& vchPubKeyOut) const;

    // Support for BIP 0013 : see https://en.bitcoin.it/wiki/BIP_0013
    virtual bool AddCScript(const CScript& redeemScript) =0;
    virtual bool HaveCScript(const uint160 &hash) const =0;
    virtual bool GetCScript(const uint160 &hash, CScript& redeemScriptOut) const =0;

    virtual bool GetSecret(const CBitcoinAddress &address, CSecret& vchSecret, bool &fCompressed) const
    {
        CKey key;
        if (!GetKey(address, key))
            return false;
        vchSecret = key.GetSecret(fCompressed);
        return true;
    }
};

typedef std::map<CBitcoinAddress, std::pair<CSecret, bool> > KeyMap;
typedef std::map<uint160, CScript > ScriptMap;

/** Basic key store, that keeps keys in an address->secret map */
class CBasicKeyStore : public CKeyStore
{
protected:
    KeyMap mapKeys;
    ScriptMap mapScripts;

public:
    bool AddKey(const CKey& key);
    bool HaveKey(const CBitcoinAddress &address) const
    {
        bool result;
        {
            LOCK(cs_KeyStore);
            result = (mapKeys.count(address) > 0);
        }
        return result;
    }
    void GetKeys(std::set<CBitcoinAddress> &setAddress) const
    {
        setAddress.clear();
        {
            LOCK(cs_KeyStore);
            KeyMap::const_iterator mi = mapKeys.begin();
            while (mi != mapKeys.end())
            {
                setAddress.insert((*mi).first);
                mi++;
            }
        }
    }
    bool GetKey(const CBitcoinAddress &address, CKey &keyOut) const
    {
        {
            LOCK(cs_KeyStore);
            KeyMap::const_iterator mi = mapKeys.find(address);
            if (mi != mapKeys.end())
            {
                keyOut.Reset();
                keyOut.SetSecret((*mi).second.first, (*mi).second.second);
                return true;
            }
        }
        return false;
    }
    virtual bool AddCScript(const CScript& redeemScript);
    virtual bool HaveCScript(const uint160 &hash) const;
    virtual bool GetCScript(const uint160 &hash, CScript& redeemScriptOut) const;
};

typedef std::map<CBitcoinAddress, std::pair<std::vector<unsigned char>, std::vector<unsigned char> > > CryptedKeyMap;

/** Keystore which keeps the private keys encrypted.
 * It derives from the basic key store, which is used if no encryption is active.
 */
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
        {
            LOCK(cs_KeyStore);
            result = vMasterKey.empty();
        }
        return result;
    }

    bool Lock()
    {
        if (!SetCrypted())
            return false;

        {
            LOCK(cs_KeyStore);
            vMasterKey.clear();
        }

        return true;
    }

    virtual bool AddCryptedKey(const std::vector<unsigned char> &vchPubKey, const std::vector<unsigned char> &vchCryptedSecret);
    bool AddKey(const CKey& key);
    bool HaveKey(const CBitcoinAddress &address) const
    {
        {
            LOCK(cs_KeyStore);
            if (!IsCrypted())
                return CBasicKeyStore::HaveKey(address);
            return mapCryptedKeys.count(address) > 0;
        }
        return false;
    }
    bool GetKey(const CBitcoinAddress &address, CKey& keyOut) const;
    bool GetPubKey(const CBitcoinAddress &address, std::vector<unsigned char>& vchPubKeyOut) const;
    void GetKeys(std::set<CBitcoinAddress> &setAddress) const
    {
        if (!IsCrypted())
        {
            CBasicKeyStore::GetKeys(setAddress);
            return;
        }
        setAddress.clear();
        CryptedKeyMap::const_iterator mi = mapCryptedKeys.begin();
        while (mi != mapCryptedKeys.end())
        {
            setAddress.insert((*mi).first);
            mi++;
        }
    }
};

#endif
