// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <script/descriptor.h>

#include <key_io.h>
#include <pubkey.h>
#include <script/script.h>
#include <script/standard.h>

#include <span.h>
#include <util/system.h>
#include <util/strencodings.h>

#include <memory>
#include <string>
#include <vector>

namespace {

////////////////////////////////////////////////////////////////////////////
// Internal representation                                                //
////////////////////////////////////////////////////////////////////////////

typedef std::vector<uint32_t> KeyPath;

std::string FormatKeyPath(const KeyPath& path)
{
    std::string ret;
    for (auto i : path) {
        ret += strprintf("/%i", (i << 1) >> 1);
        if (i >> 31) ret += '\'';
    }
    return ret;
}

/** Interface for public key objects in descriptors. */
struct PubkeyProvider
{
    virtual ~PubkeyProvider() = default;

    /** Derive a public key. If key==nullptr, only info is desired. */
    virtual bool GetPubKey(int pos, const SigningProvider& arg, CPubKey* key, KeyOriginInfo& info) const = 0;

    /** Whether this represent multiple public keys at different positions. */
    virtual bool IsRange() const = 0;

    /** Get the size of the generated public key(s) in bytes (33 or 65). */
    virtual size_t GetSize() const = 0;

    /** Get the descriptor string form. */
    virtual std::string ToString() const = 0;

    /** Get the descriptor string form including private data (if available in arg). */
    virtual bool ToPrivateString(const SigningProvider& arg, std::string& out) const = 0;
};

class OriginPubkeyProvider final : public PubkeyProvider
{
    KeyOriginInfo m_origin;
    std::unique_ptr<PubkeyProvider> m_provider;

    std::string OriginString() const
    {
        return HexStr(std::begin(m_origin.fingerprint), std::end(m_origin.fingerprint)) + FormatKeyPath(m_origin.path);
    }

public:
    OriginPubkeyProvider(KeyOriginInfo info, std::unique_ptr<PubkeyProvider> provider) : m_origin(std::move(info)), m_provider(std::move(provider)) {}
    bool GetPubKey(int pos, const SigningProvider& arg, CPubKey* key, KeyOriginInfo& info) const override
    {
        if (!m_provider->GetPubKey(pos, arg, key, info)) return false;
        std::copy(std::begin(m_origin.fingerprint), std::end(m_origin.fingerprint), info.fingerprint);
        info.path.insert(info.path.begin(), m_origin.path.begin(), m_origin.path.end());
        return true;
    }
    bool IsRange() const override { return m_provider->IsRange(); }
    size_t GetSize() const override { return m_provider->GetSize(); }
    std::string ToString() const override { return "[" + OriginString() + "]" + m_provider->ToString(); }
    bool ToPrivateString(const SigningProvider& arg, std::string& ret) const override
    {
        std::string sub;
        if (!m_provider->ToPrivateString(arg, sub)) return false;
        ret = "[" + OriginString() + "]" + std::move(sub);
        return true;
    }
};

/** An object representing a parsed constant public key in a descriptor. */
class ConstPubkeyProvider final : public PubkeyProvider
{
    CPubKey m_pubkey;

public:
    ConstPubkeyProvider(const CPubKey& pubkey) : m_pubkey(pubkey) {}
    bool GetPubKey(int pos, const SigningProvider& arg, CPubKey* key, KeyOriginInfo& info) const override
    {
        if (key) *key = m_pubkey;
        info.path.clear();
        CKeyID keyid = m_pubkey.GetID();
        std::copy(keyid.begin(), keyid.begin() + sizeof(info.fingerprint), info.fingerprint);
        return true;
    }
    bool IsRange() const override { return false; }
    size_t GetSize() const override { return m_pubkey.size(); }
    std::string ToString() const override { return HexStr(m_pubkey.begin(), m_pubkey.end()); }
    bool ToPrivateString(const SigningProvider& arg, std::string& ret) const override
    {
        CKey key;
        if (!arg.GetKey(m_pubkey.GetID(), key)) return false;
        ret = EncodeSecret(key);
        return true;
    }
};

enum class DeriveType {
    NO,
    UNHARDENED,
    HARDENED,
};

/** An object representing a parsed extended public key in a descriptor. */
class BIP32PubkeyProvider final : public PubkeyProvider
{
    CExtPubKey m_extkey;
    KeyPath m_path;
    DeriveType m_derive;

    bool GetExtKey(const SigningProvider& arg, CExtKey& ret) const
    {
        CKey key;
        if (!arg.GetKey(m_extkey.pubkey.GetID(), key)) return false;
        ret.nDepth = m_extkey.nDepth;
        std::copy(m_extkey.vchFingerprint, m_extkey.vchFingerprint + sizeof(ret.vchFingerprint), ret.vchFingerprint);
        ret.nChild = m_extkey.nChild;
        ret.chaincode = m_extkey.chaincode;
        ret.key = key;
        return true;
    }

    bool IsHardened() const
    {
        if (m_derive == DeriveType::HARDENED) return true;
        for (auto entry : m_path) {
            if (entry >> 31) return true;
        }
        return false;
    }

public:
    BIP32PubkeyProvider(const CExtPubKey& extkey, KeyPath path, DeriveType derive) : m_extkey(extkey), m_path(std::move(path)), m_derive(derive) {}
    bool IsRange() const override { return m_derive != DeriveType::NO; }
    size_t GetSize() const override { return 33; }
    bool GetPubKey(int pos, const SigningProvider& arg, CPubKey* key, KeyOriginInfo& info) const override
    {
        if (key) {
            if (IsHardened()) {
                CExtKey extkey;
                if (!GetExtKey(arg, extkey)) return false;
                for (auto entry : m_path) {
                    extkey.Derive(extkey, entry);
                }
                if (m_derive == DeriveType::UNHARDENED) extkey.Derive(extkey, pos);
                if (m_derive == DeriveType::HARDENED) extkey.Derive(extkey, pos | 0x80000000UL);
                *key = extkey.Neuter().pubkey;
            } else {
                // TODO: optimize by caching
                CExtPubKey extkey = m_extkey;
                for (auto entry : m_path) {
                    extkey.Derive(extkey, entry);
                }
                if (m_derive == DeriveType::UNHARDENED) extkey.Derive(extkey, pos);
                assert(m_derive != DeriveType::HARDENED);
                *key = extkey.pubkey;
            }
        }
        CKeyID keyid = m_extkey.pubkey.GetID();
        std::copy(keyid.begin(), keyid.begin() + sizeof(info.fingerprint), info.fingerprint);
        info.path = m_path;
        if (m_derive == DeriveType::UNHARDENED) info.path.push_back((uint32_t)pos);
        if (m_derive == DeriveType::HARDENED) info.path.push_back(((uint32_t)pos) | 0x80000000L);
        return true;
    }
    std::string ToString() const override
    {
        std::string ret = EncodeExtPubKey(m_extkey) + FormatKeyPath(m_path);
        if (IsRange()) {
            ret += "/*";
            if (m_derive == DeriveType::HARDENED) ret += '\'';
        }
        return ret;
    }
    bool ToPrivateString(const SigningProvider& arg, std::string& out) const override
    {
        CExtKey key;
        if (!GetExtKey(arg, key)) return false;
        out = EncodeExtKey(key) + FormatKeyPath(m_path);
        if (IsRange()) {
            out += "/*";
            if (m_derive == DeriveType::HARDENED) out += '\'';
        }
        return true;
    }
};

/** Base class for all Descriptor implementations. */
class DescriptorImpl : public Descriptor
{
    //! Public key arguments for this descriptor (size 1 for PK, PKH, WPKH; any size of Multisig).
    const std::vector<std::unique_ptr<PubkeyProvider>> m_pubkey_args;
    //! The sub-descriptor argument (nullptr for everything but SH and WSH).
    const std::unique_ptr<DescriptorImpl> m_script_arg;
    //! The string name of the descriptor function.
    const std::string m_name;

protected:
    //! Return a serialization of anything except pubkey and script arguments, to be prepended to those.
    virtual std::string ToStringExtra() const { return ""; }

    /** A helper function to construct the scripts for this descriptor.
     *
     *  This function is invoked once for every CScript produced by evaluating
     *  m_script_arg, or just once in case m_script_arg is nullptr.

     *  @param pubkeys The evaluations of the m_pubkey_args field.
     *  @param script The evaluation of m_script_arg (or nullptr when m_script_arg is nullptr).
     *  @param out A FlatSigningProvider to put scripts or public keys in that are necessary to the solver.
     *             The script and pubkeys argument to this function are automatically added.
     *  @return A vector with scriptPubKeys for this descriptor.
     */
    virtual std::vector<CScript> MakeScripts(const std::vector<CPubKey>& pubkeys, const CScript* script, FlatSigningProvider& out) const = 0;

public:
    DescriptorImpl(std::vector<std::unique_ptr<PubkeyProvider>> pubkeys, std::unique_ptr<DescriptorImpl> script, const std::string& name) : m_pubkey_args(std::move(pubkeys)), m_script_arg(std::move(script)), m_name(name) {}

    bool IsSolvable() const override
    {
        if (m_script_arg) {
            if (!m_script_arg->IsSolvable()) return false;
        }
        return true;
    }

    bool IsRange() const final
    {
        for (const auto& pubkey : m_pubkey_args) {
            if (pubkey->IsRange()) return true;
        }
        if (m_script_arg) {
            if (m_script_arg->IsRange()) return true;
        }
        return false;
    }

    bool ToStringHelper(const SigningProvider* arg, std::string& out, bool priv) const
    {
        std::string extra = ToStringExtra();
        size_t pos = extra.size() > 0 ? 1 : 0;
        std::string ret = m_name + "(" + extra;
        for (const auto& pubkey : m_pubkey_args) {
            if (pos++) ret += ",";
            std::string tmp;
            if (priv) {
                if (!pubkey->ToPrivateString(*arg, tmp)) return false;
            } else {
                tmp = pubkey->ToString();
            }
            ret += std::move(tmp);
        }
        if (m_script_arg) {
            if (pos++) ret += ",";
            std::string tmp;
            if (!m_script_arg->ToStringHelper(arg, tmp, priv)) return false;
            ret += std::move(tmp);
        }
        out = std::move(ret) + ")";
        return true;
    }

    std::string ToString() const final
    {
        std::string ret;
        ToStringHelper(nullptr, ret, false);
        return ret;
    }

    bool ToPrivateString(const SigningProvider& arg, std::string& out) const override final { return ToStringHelper(&arg, out, true); }

    bool ExpandHelper(int pos, const SigningProvider& arg, Span<const unsigned char>* cache_read, std::vector<CScript>& output_scripts, FlatSigningProvider& out, std::vector<unsigned char>* cache_write) const
    {
        std::vector<std::pair<CPubKey, KeyOriginInfo>> entries;
        entries.reserve(m_pubkey_args.size());

        // Construct temporary data in `entries` and `subscripts`, to avoid producing output in case of failure.
        for (const auto& p : m_pubkey_args) {
            entries.emplace_back();
            if (!p->GetPubKey(pos, arg, cache_read ? nullptr : &entries.back().first, entries.back().second)) return false;
            if (cache_read) {
                // Cached expanded public key exists, use it.
                if (cache_read->size() == 0) return false;
                bool compressed = ((*cache_read)[0] == 0x02 || (*cache_read)[0] == 0x03) && cache_read->size() >= 33;
                bool uncompressed = ((*cache_read)[0] == 0x04) && cache_read->size() >= 65;
                if (!(compressed || uncompressed)) return false;
                CPubKey pubkey(cache_read->begin(), cache_read->begin() + (compressed ? 33 : 65));
                entries.back().first = pubkey;
                *cache_read = cache_read->subspan(compressed ? 33 : 65);
            }
            if (cache_write) {
                cache_write->insert(cache_write->end(), entries.back().first.begin(), entries.back().first.end());
            }
        }
        std::vector<CScript> subscripts;
        if (m_script_arg) {
            FlatSigningProvider subprovider;
            if (!m_script_arg->ExpandHelper(pos, arg, cache_read, subscripts, subprovider, cache_write)) return false;
            out = Merge(out, subprovider);
        }

        std::vector<CPubKey> pubkeys;
        pubkeys.reserve(entries.size());
        for (auto& entry : entries) {
            pubkeys.push_back(entry.first);
            out.origins.emplace(entry.first.GetID(), std::move(entry.second));
            out.pubkeys.emplace(entry.first.GetID(), entry.first);
        }
        if (m_script_arg) {
            for (const auto& subscript : subscripts) {
                out.scripts.emplace(CScriptID(subscript), subscript);
                std::vector<CScript> addscripts = MakeScripts(pubkeys, &subscript, out);
                for (auto& addscript : addscripts) {
                    output_scripts.push_back(std::move(addscript));
                }
            }
        } else {
            output_scripts = MakeScripts(pubkeys, nullptr, out);
        }
        return true;
    }

    bool Expand(int pos, const SigningProvider& provider, std::vector<CScript>& output_scripts, FlatSigningProvider& out, std::vector<unsigned char>* cache = nullptr) const final
    {
        return ExpandHelper(pos, provider, nullptr, output_scripts, out, cache);
    }

    bool ExpandFromCache(int pos, const std::vector<unsigned char>& cache, std::vector<CScript>& output_scripts, FlatSigningProvider& out) const final
    {
        Span<const unsigned char> span = MakeSpan(cache);
        return ExpandHelper(pos, DUMMY_SIGNING_PROVIDER, &span, output_scripts, out, nullptr) && span.size() == 0;
    }
};

/** Construct a vector with one element, which is moved into it. */
template<typename T>
std::vector<T> Singleton(T elem)
{
    std::vector<T> ret;
    ret.emplace_back(std::move(elem));
    return ret;
}

/** A parsed addr(A) descriptor. */
class AddressDescriptor final : public DescriptorImpl
{
    const CTxDestination m_destination;
protected:
    std::string ToStringExtra() const override { return EncodeDestination(m_destination); }
    std::vector<CScript> MakeScripts(const std::vector<CPubKey>&, const CScript*, FlatSigningProvider&) const override { return Singleton(GetScriptForDestination(m_destination)); }
public:
    AddressDescriptor(CTxDestination destination) : DescriptorImpl({}, {}, "addr"), m_destination(std::move(destination)) {}
    bool IsSolvable() const final { return false; }
};

/** A parsed raw(H) descriptor. */
class RawDescriptor final : public DescriptorImpl
{
    const CScript m_script;
protected:
    std::string ToStringExtra() const override { return HexStr(m_script.begin(), m_script.end()); }
    std::vector<CScript> MakeScripts(const std::vector<CPubKey>&, const CScript*, FlatSigningProvider&) const override { return Singleton(m_script); }
public:
    RawDescriptor(CScript script) : DescriptorImpl({}, {}, "raw"), m_script(std::move(script)) {}
    bool IsSolvable() const final { return false; }
};

/** A parsed pk(P) descriptor. */
class PKDescriptor final : public DescriptorImpl
{
protected:
    std::vector<CScript> MakeScripts(const std::vector<CPubKey>& keys, const CScript*, FlatSigningProvider&) const override { return Singleton(GetScriptForRawPubKey(keys[0])); }
public:
    PKDescriptor(std::unique_ptr<PubkeyProvider> prov) : DescriptorImpl(Singleton(std::move(prov)), {}, "pk") {}
};

/** A parsed pkh(P) descriptor. */
class PKHDescriptor final : public DescriptorImpl
{
protected:
    std::vector<CScript> MakeScripts(const std::vector<CPubKey>& keys, const CScript*, FlatSigningProvider&) const override { return Singleton(GetScriptForDestination(keys[0].GetID())); }
public:
    PKHDescriptor(std::unique_ptr<PubkeyProvider> prov) : DescriptorImpl(Singleton(std::move(prov)), {}, "pkh") {}
};

/** A parsed wpkh(P) descriptor. */
class WPKHDescriptor final : public DescriptorImpl
{
protected:
    std::vector<CScript> MakeScripts(const std::vector<CPubKey>& keys, const CScript*, FlatSigningProvider&) const override { return Singleton(GetScriptForDestination(WitnessV0KeyHash(keys[0].GetID()))); }
public:
    WPKHDescriptor(std::unique_ptr<PubkeyProvider> prov) : DescriptorImpl(Singleton(std::move(prov)), {}, "wpkh") {}
};

/** A parsed combo(P) descriptor. */
class ComboDescriptor final : public DescriptorImpl
{
protected:
    std::vector<CScript> MakeScripts(const std::vector<CPubKey>& keys, const CScript*, FlatSigningProvider& out) const override
    {
        std::vector<CScript> ret;
        CKeyID id = keys[0].GetID();
        ret.emplace_back(GetScriptForRawPubKey(keys[0])); // P2PK
        ret.emplace_back(GetScriptForDestination(id)); // P2PKH
        if (keys[0].IsCompressed()) {
            CScript p2wpkh = GetScriptForDestination(WitnessV0KeyHash(id));
            out.scripts.emplace(CScriptID(p2wpkh), p2wpkh);
            ret.emplace_back(p2wpkh);
            ret.emplace_back(GetScriptForDestination(CScriptID(p2wpkh))); // P2SH-P2WPKH
        }
        return ret;
    }
public:
    ComboDescriptor(std::unique_ptr<PubkeyProvider> prov) : DescriptorImpl(Singleton(std::move(prov)), {}, "combo") {}
};

/** A parsed multi(...) descriptor. */
class MultisigDescriptor final : public DescriptorImpl
{
    const int m_threshold;
protected:
    std::string ToStringExtra() const override { return strprintf("%i", m_threshold); }
    std::vector<CScript> MakeScripts(const std::vector<CPubKey>& keys, const CScript*, FlatSigningProvider&) const override { return Singleton(GetScriptForMultisig(m_threshold, keys)); }
public:
    MultisigDescriptor(int threshold, std::vector<std::unique_ptr<PubkeyProvider>> providers) : DescriptorImpl(std::move(providers), {}, "multi"), m_threshold(threshold) {}
};

/** A parsed sh(...) descriptor. */
class SHDescriptor final : public DescriptorImpl
{
protected:
    std::vector<CScript> MakeScripts(const std::vector<CPubKey>&, const CScript* script, FlatSigningProvider&) const override { return Singleton(GetScriptForDestination(CScriptID(*script))); }
public:
    SHDescriptor(std::unique_ptr<DescriptorImpl> desc) : DescriptorImpl({}, std::move(desc), "sh") {}
};

/** A parsed wsh(...) descriptor. */
class WSHDescriptor final : public DescriptorImpl
{
protected:
    std::vector<CScript> MakeScripts(const std::vector<CPubKey>&, const CScript* script, FlatSigningProvider&) const override { return Singleton(GetScriptForDestination(WitnessV0ScriptHash(*script))); }
public:
    WSHDescriptor(std::unique_ptr<DescriptorImpl> desc) : DescriptorImpl({}, std::move(desc), "wsh") {}
};

////////////////////////////////////////////////////////////////////////////
// Parser                                                                 //
////////////////////////////////////////////////////////////////////////////

enum class ParseScriptContext {
    TOP,
    P2SH,
    P2WSH,
};

/** Parse a constant. If successful, sp is updated to skip the constant and return true. */
bool Const(const std::string& str, Span<const char>& sp)
{
    if ((size_t)sp.size() >= str.size() && std::equal(str.begin(), str.end(), sp.begin())) {
        sp = sp.subspan(str.size());
        return true;
    }
    return false;
}

/** Parse a function call. If successful, sp is updated to be the function's argument(s). */
bool Func(const std::string& str, Span<const char>& sp)
{
    if ((size_t)sp.size() >= str.size() + 2 && sp[str.size()] == '(' && sp[sp.size() - 1] == ')' && std::equal(str.begin(), str.end(), sp.begin())) {
        sp = sp.subspan(str.size() + 1, sp.size() - str.size() - 2);
        return true;
    }
    return false;
}

/** Return the expression that sp begins with, and update sp to skip it. */
Span<const char> Expr(Span<const char>& sp)
{
    int level = 0;
    auto it = sp.begin();
    while (it != sp.end()) {
        if (*it == '(') {
            ++level;
        } else if (level && *it == ')') {
            --level;
        } else if (level == 0 && (*it == ')' || *it == ',')) {
            break;
        }
        ++it;
    }
    Span<const char> ret = sp.first(it - sp.begin());
    sp = sp.subspan(it - sp.begin());
    return ret;
}

/** Split a string on every instance of sep, returning a vector. */
std::vector<Span<const char>> Split(const Span<const char>& sp, char sep)
{
    std::vector<Span<const char>> ret;
    auto it = sp.begin();
    auto start = it;
    while (it != sp.end()) {
        if (*it == sep) {
            ret.emplace_back(start, it);
            start = it + 1;
        }
        ++it;
    }
    ret.emplace_back(start, it);
    return ret;
}

/** Parse a key path, being passed a split list of elements (the first element is ignored). */
NODISCARD bool ParseKeyPath(const std::vector<Span<const char>>& split, KeyPath& out)
{
    for (size_t i = 1; i < split.size(); ++i) {
        Span<const char> elem = split[i];
        bool hardened = false;
        if (elem.size() > 0 && (elem[elem.size() - 1] == '\'' || elem[elem.size() - 1] == 'h')) {
            elem = elem.first(elem.size() - 1);
            hardened = true;
        }
        uint32_t p;
        if (!ParseUInt32(std::string(elem.begin(), elem.end()), &p) || p > 0x7FFFFFFFUL) return false;
        out.push_back(p | (((uint32_t)hardened) << 31));
    }
    return true;
}

/** Parse a public key that excludes origin information. */
std::unique_ptr<PubkeyProvider> ParsePubkeyInner(const Span<const char>& sp, bool permit_uncompressed, FlatSigningProvider& out)
{
    auto split = Split(sp, '/');
    std::string str(split[0].begin(), split[0].end());
    if (split.size() == 1) {
        if (IsHex(str)) {
            std::vector<unsigned char> data = ParseHex(str);
            CPubKey pubkey(data);
            if (pubkey.IsFullyValid() && (permit_uncompressed || pubkey.IsCompressed())) return MakeUnique<ConstPubkeyProvider>(pubkey);
        }
        CKey key = DecodeSecret(str);
        if (key.IsValid() && (permit_uncompressed || key.IsCompressed())) {
            CPubKey pubkey = key.GetPubKey();
            out.keys.emplace(pubkey.GetID(), key);
            return MakeUnique<ConstPubkeyProvider>(pubkey);
        }
    }
    CExtKey extkey = DecodeExtKey(str);
    CExtPubKey extpubkey = DecodeExtPubKey(str);
    if (!extkey.key.IsValid() && !extpubkey.pubkey.IsValid()) return nullptr;
    KeyPath path;
    DeriveType type = DeriveType::NO;
    if (split.back() == MakeSpan("*").first(1)) {
        split.pop_back();
        type = DeriveType::UNHARDENED;
    } else if (split.back() == MakeSpan("*'").first(2) || split.back() == MakeSpan("*h").first(2)) {
        split.pop_back();
        type = DeriveType::HARDENED;
    }
    if (!ParseKeyPath(split, path)) return nullptr;
    if (extkey.key.IsValid()) {
        extpubkey = extkey.Neuter();
        out.keys.emplace(extpubkey.pubkey.GetID(), extkey.key);
    }
    return MakeUnique<BIP32PubkeyProvider>(extpubkey, std::move(path), type);
}

/** Parse a public key including origin information (if enabled). */
std::unique_ptr<PubkeyProvider> ParsePubkey(const Span<const char>& sp, bool permit_uncompressed, FlatSigningProvider& out)
{
    auto origin_split = Split(sp, ']');
    if (origin_split.size() > 2) return nullptr;
    if (origin_split.size() == 1) return ParsePubkeyInner(origin_split[0], permit_uncompressed, out);
    if (origin_split[0].size() < 1 || origin_split[0][0] != '[') return nullptr;
    auto slash_split = Split(origin_split[0].subspan(1), '/');
    if (slash_split[0].size() != 8) return nullptr;
    std::string fpr_hex = std::string(slash_split[0].begin(), slash_split[0].end());
    if (!IsHex(fpr_hex)) return nullptr;
    auto fpr_bytes = ParseHex(fpr_hex);
    KeyOriginInfo info;
    static_assert(sizeof(info.fingerprint) == 4, "Fingerprint must be 4 bytes");
    assert(fpr_bytes.size() == 4);
    std::copy(fpr_bytes.begin(), fpr_bytes.end(), info.fingerprint);
    if (!ParseKeyPath(slash_split, info.path)) return nullptr;
    auto provider = ParsePubkeyInner(origin_split[1], permit_uncompressed, out);
    if (!provider) return nullptr;
    return MakeUnique<OriginPubkeyProvider>(std::move(info), std::move(provider));
}

/** Parse a script in a particular context. */
std::unique_ptr<DescriptorImpl> ParseScript(Span<const char>& sp, ParseScriptContext ctx, FlatSigningProvider& out)
{
    auto expr = Expr(sp);
    if (Func("pk", expr)) {
        auto pubkey = ParsePubkey(expr, ctx != ParseScriptContext::P2WSH, out);
        if (!pubkey) return nullptr;
        return MakeUnique<PKDescriptor>(std::move(pubkey));
    }
    if (Func("pkh", expr)) {
        auto pubkey = ParsePubkey(expr, ctx != ParseScriptContext::P2WSH, out);
        if (!pubkey) return nullptr;
        return MakeUnique<PKHDescriptor>(std::move(pubkey));
    }
    if (ctx == ParseScriptContext::TOP && Func("combo", expr)) {
        auto pubkey = ParsePubkey(expr, true, out);
        if (!pubkey) return nullptr;
        return MakeUnique<ComboDescriptor>(std::move(pubkey));
    }
    if (Func("multi", expr)) {
        auto threshold = Expr(expr);
        uint32_t thres;
        std::vector<std::unique_ptr<PubkeyProvider>> providers;
        if (!ParseUInt32(std::string(threshold.begin(), threshold.end()), &thres)) return nullptr;
        size_t script_size = 0;
        while (expr.size()) {
            if (!Const(",", expr)) return nullptr;
            auto arg = Expr(expr);
            auto pk = ParsePubkey(arg, ctx != ParseScriptContext::P2WSH, out);
            if (!pk) return nullptr;
            script_size += pk->GetSize() + 1;
            providers.emplace_back(std::move(pk));
        }
        if (providers.size() < 1 || providers.size() > 16 || thres < 1 || thres > providers.size()) return nullptr;
        if (ctx == ParseScriptContext::TOP) {
            if (providers.size() > 3) return nullptr; // Not more than 3 pubkeys for raw multisig
        }
        if (ctx == ParseScriptContext::P2SH) {
            if (script_size + 3 > 520) return nullptr; // Enforce P2SH script size limit
        }
        return MakeUnique<MultisigDescriptor>(thres, std::move(providers));
    }
    if (ctx != ParseScriptContext::P2WSH && Func("wpkh", expr)) {
        auto pubkey = ParsePubkey(expr, false, out);
        if (!pubkey) return nullptr;
        return MakeUnique<WPKHDescriptor>(std::move(pubkey));
    }
    if (ctx == ParseScriptContext::TOP && Func("sh", expr)) {
        auto desc = ParseScript(expr, ParseScriptContext::P2SH, out);
        if (!desc || expr.size()) return nullptr;
        return MakeUnique<SHDescriptor>(std::move(desc));
    }
    if (ctx != ParseScriptContext::P2WSH && Func("wsh", expr)) {
        auto desc = ParseScript(expr, ParseScriptContext::P2WSH, out);
        if (!desc || expr.size()) return nullptr;
        return MakeUnique<WSHDescriptor>(std::move(desc));
    }
    if (ctx == ParseScriptContext::TOP && Func("addr", expr)) {
        CTxDestination dest = DecodeDestination(std::string(expr.begin(), expr.end()));
        if (!IsValidDestination(dest)) return nullptr;
        return MakeUnique<AddressDescriptor>(std::move(dest));
    }
    if (ctx == ParseScriptContext::TOP && Func("raw", expr)) {
        std::string str(expr.begin(), expr.end());
        if (!IsHex(str)) return nullptr;
        auto bytes = ParseHex(str);
        return MakeUnique<RawDescriptor>(CScript(bytes.begin(), bytes.end()));
    }
    return nullptr;
}

std::unique_ptr<PubkeyProvider> InferPubkey(const CPubKey& pubkey, ParseScriptContext, const SigningProvider& provider)
{
    std::unique_ptr<PubkeyProvider> key_provider = MakeUnique<ConstPubkeyProvider>(pubkey);
    KeyOriginInfo info;
    if (provider.GetKeyOrigin(pubkey.GetID(), info)) {
        return MakeUnique<OriginPubkeyProvider>(std::move(info), std::move(key_provider));
    }
    return key_provider;
}

std::unique_ptr<DescriptorImpl> InferScript(const CScript& script, ParseScriptContext ctx, const SigningProvider& provider)
{
    std::vector<std::vector<unsigned char>> data;
    txnouttype txntype = Solver(script, data);

    if (txntype == TX_PUBKEY) {
        CPubKey pubkey(data[0].begin(), data[0].end());
        if (pubkey.IsValid()) {
            return MakeUnique<PKDescriptor>(InferPubkey(pubkey, ctx, provider));
        }
    }
    if (txntype == TX_PUBKEYHASH) {
        uint160 hash(data[0]);
        CKeyID keyid(hash);
        CPubKey pubkey;
        if (provider.GetPubKey(keyid, pubkey)) {
            return MakeUnique<PKHDescriptor>(InferPubkey(pubkey, ctx, provider));
        }
    }
    if (txntype == TX_WITNESS_V0_KEYHASH && ctx != ParseScriptContext::P2WSH) {
        uint160 hash(data[0]);
        CKeyID keyid(hash);
        CPubKey pubkey;
        if (provider.GetPubKey(keyid, pubkey)) {
            return MakeUnique<WPKHDescriptor>(InferPubkey(pubkey, ctx, provider));
        }
    }
    if (txntype == TX_MULTISIG) {
        std::vector<std::unique_ptr<PubkeyProvider>> providers;
        for (size_t i = 1; i + 1 < data.size(); ++i) {
            CPubKey pubkey(data[i].begin(), data[i].end());
            providers.push_back(InferPubkey(pubkey, ctx, provider));
        }
        return MakeUnique<MultisigDescriptor>((int)data[0][0], std::move(providers));
    }
    if (txntype == TX_SCRIPTHASH && ctx == ParseScriptContext::TOP) {
        uint160 hash(data[0]);
        CScriptID scriptid(hash);
        CScript subscript;
        if (provider.GetCScript(scriptid, subscript)) {
            auto sub = InferScript(subscript, ParseScriptContext::P2SH, provider);
            if (sub) return MakeUnique<SHDescriptor>(std::move(sub));
        }
    }
    if (txntype == TX_WITNESS_V0_SCRIPTHASH && ctx != ParseScriptContext::P2WSH) {
        CScriptID scriptid;
        CRIPEMD160().Write(data[0].data(), data[0].size()).Finalize(scriptid.begin());
        CScript subscript;
        if (provider.GetCScript(scriptid, subscript)) {
            auto sub = InferScript(subscript, ParseScriptContext::P2WSH, provider);
            if (sub) return MakeUnique<WSHDescriptor>(std::move(sub));
        }
    }

    CTxDestination dest;
    if (ExtractDestination(script, dest)) {
        if (GetScriptForDestination(dest) == script) {
            return MakeUnique<AddressDescriptor>(std::move(dest));
        }
    }

    return MakeUnique<RawDescriptor>(script);
}

} // namespace

std::unique_ptr<Descriptor> Parse(const std::string& descriptor, FlatSigningProvider& out)
{
    Span<const char> sp(descriptor.data(), descriptor.size());
    auto ret = ParseScript(sp, ParseScriptContext::TOP, out);
    if (sp.size() == 0 && ret) return std::unique_ptr<Descriptor>(std::move(ret));
    return nullptr;
}

std::unique_ptr<Descriptor> InferDescriptor(const CScript& script, const SigningProvider& provider)
{
    return InferScript(script, ParseScriptContext::TOP, provider);
}
