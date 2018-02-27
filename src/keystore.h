// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_KEYSTORE_H
#define BITCOIN_KEYSTORE_H

#include <key.h>
#include <key/extkey.h>
#include <key/stealth.h>
#include <pubkey.h>
#include <script/script.h>
#include <script/standard.h>
#include <sync.h>

#include <boost/signals2/signal.hpp>

enum isminetype : uint8_t;

/** A virtual base class for key stores */
class CKeyStore
{
protected:
    mutable CCriticalSection cs_KeyStore;

public:
    virtual ~CKeyStore() {}

    //! Add a key to the store.
    virtual bool AddKeyPubKey(const CKey &key, const CPubKey &pubkey) =0;
    virtual bool AddKey(const CKey &key);

    //! Check whether a key corresponding to a given address is present in the store.
    virtual isminetype IsMine(const CKeyID &address) const =0;
    virtual bool HaveKey(const CKeyID &address) const =0;
    virtual bool GetKey(const CKeyID &address, CKey& keyOut) const =0;
    virtual std::set<CKeyID> GetKeys() const =0;
    virtual bool GetPubKey(const CKeyID &address, CPubKey& vchPubKeyOut) const =0;

    //! Support for BIP 0013 : see https://github.com/bitcoin/bips/blob/master/bip-0013.mediawiki
    virtual bool AddCScript(const CScript& redeemScript) =0;
    virtual bool HaveCScript(const CScriptID &hash) const =0;
    virtual std::set<CScriptID> GetCScripts() const =0;
    virtual bool GetCScript(const CScriptID &hash, CScript& redeemScriptOut) const =0;

    //! Support for Watch-only addresses
    virtual bool AddWatchOnly(const CScript &dest) =0;
    virtual bool RemoveWatchOnly(const CScript &dest) =0;
    virtual bool HaveWatchOnly(const CScript &dest) const =0;
    virtual bool HaveWatchOnly() const =0;

    virtual size_t CountKeys() const =0;
};

typedef std::map<CKeyID, CKey> KeyMap;
typedef std::map<CKeyID, CPubKey> WatchKeyMap;
typedef std::map<CScriptID, CScript > ScriptMap;
typedef std::set<CScript> WatchOnlySet;

/** Basic key store, that keeps keys in an address->secret map */
class CBasicKeyStore : public CKeyStore
{
protected:
    KeyMap mapKeys;
    WatchKeyMap mapWatchKeys;
    ScriptMap mapScripts;
    WatchOnlySet setWatchOnly;

    void ImplicitlyLearnRelatedKeyScripts(const CPubKey& pubkey);

public:
    bool AddKeyPubKey(const CKey& key, const CPubKey &pubkey) override;
    bool GetPubKey(const CKeyID &address, CPubKey& vchPubKeyOut) const override;
    isminetype IsMine(const CKeyID &address) const override;
    bool HaveKey(const CKeyID &address) const override;
    std::set<CKeyID> GetKeys() const override;
    bool GetKey(const CKeyID &address, CKey &keyOut) const override;
    virtual bool AddCScript(const CScript& redeemScript) override;
    virtual bool HaveCScript(const CScriptID &hash) const override;
    virtual std::set<CScriptID> GetCScripts() const override;
    virtual bool GetCScript(const CScriptID &hash, CScript& redeemScriptOut) const override;

    virtual bool AddWatchOnly(const CScript &dest) override;
    virtual bool RemoveWatchOnly(const CScript &dest) override;
    virtual bool HaveWatchOnly(const CScript &dest) const override;
    virtual bool HaveWatchOnly() const override;

    virtual size_t CountKeys() const override
    {
        LOCK(cs_KeyStore);
        return mapKeys.size();
    };
};

typedef std::vector<unsigned char, secure_allocator<unsigned char> > CKeyingMaterial;
typedef std::map<CKeyID, std::pair<CPubKey, std::vector<unsigned char> > > CryptedKeyMap;

/** Return the CKeyID of the key involved in a script (if there is a unique one). */
CKeyID GetKeyForDestination(const CKeyStore& store, const CTxDestination& dest);

#endif // BITCOIN_KEYSTORE_H
