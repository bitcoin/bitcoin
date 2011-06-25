// Copyright (c) 2009-2011 Satoshi Nakamoto & Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_KEYSTORE_H
#define BITCOIN_KEYSTORE_H

typedef std::vector<unsigned char, secure_allocator<unsigned char> > CMasterKey;

class CKeyStore
{
public:
    mutable CCriticalSection cs_KeyStore;

    virtual bool AddKey(const CKey& key) =0;
    virtual bool HaveKey(const std::vector<unsigned char> &vchPubKey) const =0;
    virtual bool GetPrivKey(const std::vector<unsigned char> &vchPubKey, CPrivKey& keyOut) const =0;
    virtual std::vector<unsigned char> GenerateNewKey();
};

class CBasicKeyStore : public CKeyStore
{
protected:
    std::map<std::vector<unsigned char>, CPrivKey> mapKeys;

public:
    bool AddKey(const CKey& key);
    bool HaveKey(const std::vector<unsigned char> &vchPubKey) const
    {
        return (mapKeys.count(vchPubKey) > 0);
    }
    bool GetPrivKey(const std::vector<unsigned char> &vchPubKey, CPrivKey& keyOut) const
    {
        std::map<std::vector<unsigned char>, CPrivKey>::const_iterator mi = mapKeys.find(vchPubKey);
        if (mi != mapKeys.end())
        {
            keyOut = (*mi).second;
            return true;
        }
        return false;
    }
};

class CCryptoKeyStore : public CBasicKeyStore
{
private:
    std::map<std::vector<unsigned char>, std::vector<unsigned char> > mapCryptedKeys;

    CMasterKey vMasterKey;

    // if fUseCrypto is true, mapKeys must be empty
    // if fUseCrypto is false, vMasterKey must be empty
    bool fUseCrypto;

protected:
    bool IsCrypted() const
    {
        return fUseCrypto;
    }

    bool SetCrypted()
    {
        if (fUseCrypto)
            return true;
        if (!mapKeys.empty())
            return false;
        fUseCrypto = true;
    }

    // will encrypt previously unencrypted keys
    bool GenerateMasterKey();

    bool GetMasterKey(CMasterKey &vMasterKeyOut) const
    {
        if (!IsCrypted())
            return false;
        if (IsLocked())
            return false;
        vMasterKeyOut = vMasterKey;
        return true;
    }
    bool Unlock(const CMasterKey& vMasterKeyIn);

public:
    CCryptoKeyStore() : fUseCrypto(false)
    {
    }

    bool IsLocked() const
    {
        if (!IsCrypted())
            return false;
        return vMasterKey.empty();
    }

    bool Lock()
    {
        if (!SetCrypted())
            return false;
        vMasterKey.clear();
    }

    virtual bool AddCryptedKey(const std::vector<unsigned char> &vchPubKey, const std::vector<unsigned char> &vchCryptedSecret);
    bool AddKey(const CKey& key);
    bool HaveKey(const std::vector<unsigned char> &vchPubKey) const
    {
        if (!IsCrypted())
            return CBasicKeyStore::HaveKey(vchPubKey);
        return mapCryptedKeys.count(vchPubKey) > 0;
    }
    bool GetPrivKey(const std::vector<unsigned char> &vchPubKey, CPrivKey& keyOut) const;
};

#endif
