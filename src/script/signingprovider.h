// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_SCRIPT_SIGNINGPROVIDER_H
#define SYSCOIN_SCRIPT_SIGNINGPROVIDER_H

#include <key.h>
#include <pubkey.h>
#include <script/script.h>
#include <script/standard.h>
#include <sync.h>

struct KeyOriginInfo;

/** An interface to be implemented by keystores that support signing. */
class SigningProvider
{
public:
    virtual ~SigningProvider() {}
    virtual bool GetCScript(const CScriptID &scriptid, CScript& script) const { return false; }
    virtual bool HaveCScript(const CScriptID &scriptid) const { return false; }
    virtual bool GetPubKey(const CKeyID &address, CPubKey& pubkey) const { return false; }
    virtual bool GetKey(const CKeyID &address, CKey& key) const { return false; }
    virtual bool HaveKey(const CKeyID &address) const { return false; }
    virtual bool GetKeyOrigin(const CKeyID& keyid, KeyOriginInfo& info) const { return false; }
    virtual bool GetTaprootSpendData(const XOnlyPubKey& output_key, TaprootSpendData& spenddata) const { return false; }
};

extern const SigningProvider& DUMMY_SIGNING_PROVIDER;

class HidingSigningProvider : public SigningProvider
{
private:
    const bool m_hide_secret;
    const bool m_hide_origin;
    const SigningProvider* m_provider;

public:
    HidingSigningProvider(const SigningProvider* provider, bool hide_secret, bool hide_origin) : m_hide_secret(hide_secret), m_hide_origin(hide_origin), m_provider(provider) {}
    bool GetCScript(const CScriptID& scriptid, CScript& script) const override;
    bool GetPubKey(const CKeyID& keyid, CPubKey& pubkey) const override;
    bool GetKey(const CKeyID& keyid, CKey& key) const override;
    bool GetKeyOrigin(const CKeyID& keyid, KeyOriginInfo& info) const override;
    bool GetTaprootSpendData(const XOnlyPubKey& output_key, TaprootSpendData& spenddata) const override;
};

struct FlatSigningProvider final : public SigningProvider
{
    std::map<CScriptID, CScript> scripts;
    std::map<CKeyID, CPubKey> pubkeys;
    std::map<CKeyID, std::pair<CPubKey, KeyOriginInfo>> origins;
    std::map<CKeyID, CKey> keys;
    std::map<XOnlyPubKey, TaprootSpendData> tr_spenddata; /** Map from output key to spend data. */

    bool GetCScript(const CScriptID& scriptid, CScript& script) const override;
    bool GetPubKey(const CKeyID& keyid, CPubKey& pubkey) const override;
    bool GetKeyOrigin(const CKeyID& keyid, KeyOriginInfo& info) const override;
    bool GetKey(const CKeyID& keyid, CKey& key) const override;
    bool GetTaprootSpendData(const XOnlyPubKey& output_key, TaprootSpendData& spenddata) const override;
};

FlatSigningProvider Merge(const FlatSigningProvider& a, const FlatSigningProvider& b);

/** Fillable signing provider that keeps keys in an address->secret map */
class FillableSigningProvider : public SigningProvider
{
protected:
    using KeyMap = std::map<CKeyID, CKey>;
    using ScriptMap = std::map<CScriptID, CScript>;

    /**
     * Map of key id to unencrypted private keys known by the signing provider.
     * Map may be empty if the provider has another source of keys, like an
     * encrypted store.
     */
    KeyMap mapKeys GUARDED_BY(cs_KeyStore);

    /**
     * Map of script id to scripts known by the signing provider.
     *
     * This map originally just held P2SH redeemScripts, and was used by wallet
     * code to look up script ids referenced in "OP_HASH160 <script id>
     * OP_EQUAL" P2SH outputs. Later in 605e8473a7d it was extended to hold
     * P2WSH witnessScripts as well, and used to look up nested scripts
     * referenced in "OP_0 <script hash>" P2WSH outputs. Later in commits
     * f4691ab3a9d and 248f3a76a82, it was extended once again to hold segwit
     * "OP_0 <key or script hash>" scriptPubKeys, in order to give the wallet a
     * way to distinguish between segwit outputs that it generated addresses for
     * and wanted to receive payments from, and segwit outputs that it never
     * generated addresses for, but it could spend just because of having keys.
     * (Before segwit activation it was also important to not treat segwit
     * outputs to arbitrary wallet keys as payments, because these could be
     * spent by anyone without even needing to sign with the keys.)
     *
     * Some of the scripts stored in mapScripts are memory-only and
     * intentionally not saved to disk. Specifically, scripts added by
     * ImplicitlyLearnRelatedKeyScripts(pubkey) calls are not written to disk so
     * future wallet code can have flexibility to be more selective about what
     * transaction outputs it recognizes as payments, instead of having to treat
     * all outputs spending to keys it knows as payments. By contrast,
     * mapScripts entries added by AddCScript(script),
     * LearnRelatedScripts(pubkey, type), and LearnAllRelatedScripts(pubkey)
     * calls are saved because they are all intentionally used to receive
     * payments.
     *
     * The FillableSigningProvider::mapScripts script map should not be confused
     * with LegacyScriptPubKeyMan::setWatchOnly script set. The two collections
     * can hold the same scripts, but they serve different purposes. The
     * setWatchOnly script set is intended to expand the set of outputs the
     * wallet considers payments. Every output with a script it contains is
     * considered to belong to the wallet, regardless of whether the script is
     * solvable or signable. By contrast, the scripts in mapScripts are only
     * used for solving, and to restrict which outputs are considered payments
     * by the wallet. An output with a script in mapScripts, unlike
     * setWatchOnly, is not automatically considered to belong to the wallet if
     * it can't be solved and signed for.
     */
    ScriptMap mapScripts GUARDED_BY(cs_KeyStore);

    void ImplicitlyLearnRelatedKeyScripts(const CPubKey& pubkey) EXCLUSIVE_LOCKS_REQUIRED(cs_KeyStore);

public:
    mutable RecursiveMutex cs_KeyStore;

    virtual bool AddKeyPubKey(const CKey& key, const CPubKey &pubkey);
    virtual bool AddKey(const CKey &key) { return AddKeyPubKey(key, key.GetPubKey()); }
    virtual bool GetPubKey(const CKeyID &address, CPubKey& vchPubKeyOut) const override;
    virtual bool HaveKey(const CKeyID &address) const override;
    virtual std::set<CKeyID> GetKeys() const;
    virtual bool GetKey(const CKeyID &address, CKey &keyOut) const override;
    virtual bool AddCScript(const CScript& redeemScript);
    virtual bool HaveCScript(const CScriptID &hash) const override;
    virtual std::set<CScriptID> GetCScripts() const;
    virtual bool GetCScript(const CScriptID &hash, CScript& redeemScriptOut) const override;
};

/** Return the CKeyID of the key involved in a script (if there is a unique one). */
CKeyID GetKeyForDestination(const SigningProvider& store, const CTxDestination& dest);

#endif // SYSCOIN_SCRIPT_SIGNINGPROVIDER_H
