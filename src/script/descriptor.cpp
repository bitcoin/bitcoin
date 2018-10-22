// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <script/descriptor.h>

#include <key_io.h>
#include <pubkey.h>
#include <script/script.h>
#include <script/standard.h>

#include <span.h>
#include <util.h>
#include <utilstrencodings.h>

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

    /** Derive a public key. */
    virtual bool GetPubKey(int pos, const SigningProvider& arg, CPubKey& key, KeyOriginInfo& info) const = 0;

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
    bool GetPubKey(int pos, const SigningProvider& arg, CPubKey& key, KeyOriginInfo& info) const override
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
    bool GetPubKey(int pos, const SigningProvider& arg, CPubKey& key, KeyOriginInfo& info) const override
    {
        key = m_pubkey;
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
    bool GetPubKey(int pos, const SigningProvider& arg, CPubKey& key, KeyOriginInfo& info) const override
    {
        if (IsHardened()) {
            CExtKey extkey;
            if (!GetExtKey(arg, extkey)) return false;
            for (auto entry : m_path) {
                extkey.Derive(extkey, entry);
            }
            if (m_derive == DeriveType::UNHARDENED) extkey.Derive(extkey, pos);
            if (m_derive == DeriveType::HARDENED) extkey.Derive(extkey, pos | 0x80000000UL);
            key = extkey.Neuter().pubkey;
        } else {
            // TODO: optimize by caching
            CExtPubKey extkey = m_extkey;
            for (auto entry : m_path) {
                extkey.Derive(extkey, entry);
            }
            if (m_derive == DeriveType::UNHARDENED) extkey.Derive(extkey, pos);
            assert(m_derive != DeriveType::HARDENED);
            key = extkey.pubkey;
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

/** A parsed addr(A) descriptor. */
class AddressDescriptor final : public Descriptor
{
    CTxDestination m_destination;

public:
    AddressDescriptor(CTxDestination destination) : m_destination(std::move(destination)) {}

    bool IsRange() const override { return false; }
    std::string ToString() const override { return "addr(" + EncodeDestination(m_destination) + ")"; }
    bool ToPrivateString(const SigningProvider& arg, std::string& out) const override { out = ToString(); return true; }
    bool Expand(int pos, const SigningProvider& arg, std::vector<CScript>& output_scripts, FlatSigningProvider& out) const override
    {
        output_scripts = std::vector<CScript>{GetScriptForDestination(m_destination)};
        return true;
    }
};

/** A parsed raw(H) descriptor. */
class RawDescriptor final : public Descriptor
{
    CScript m_script;

public:
    RawDescriptor(CScript script) : m_script(std::move(script)) {}

    bool IsRange() const override { return false; }
    std::string ToString() const override { return "raw(" + HexStr(m_script.begin(), m_script.end()) + ")"; }
    bool ToPrivateString(const SigningProvider& arg, std::string& out) const override { out = ToString(); return true; }
    bool Expand(int pos, const SigningProvider& arg, std::vector<CScript>& output_scripts, FlatSigningProvider& out) const override
    {
        output_scripts = std::vector<CScript>{m_script};
        return true;
    }
};

/** A parsed pk(P), pkh(P), or wpkh(P) descriptor. */
class SingleKeyDescriptor final : public Descriptor
{
    const std::function<CScript(const CPubKey&)> m_script_fn;
    const std::string m_fn_name;
    std::unique_ptr<PubkeyProvider> m_provider;

public:
    SingleKeyDescriptor(std::unique_ptr<PubkeyProvider> prov, const std::function<CScript(const CPubKey&)>& fn, const std::string& name) : m_script_fn(fn), m_fn_name(name), m_provider(std::move(prov)) {}

    bool IsRange() const override { return m_provider->IsRange(); }
    std::string ToString() const override { return m_fn_name + "(" + m_provider->ToString() + ")"; }
    bool ToPrivateString(const SigningProvider& arg, std::string& out) const override
    {
        std::string ret;
        if (!m_provider->ToPrivateString(arg, ret)) return false;
        out = m_fn_name + "(" + std::move(ret) + ")";
        return true;
    }
    bool Expand(int pos, const SigningProvider& arg, std::vector<CScript>& output_scripts, FlatSigningProvider& out) const override
    {
        CPubKey key;
        KeyOriginInfo info;
        if (!m_provider->GetPubKey(pos, arg, key, info)) return false;
        output_scripts = std::vector<CScript>{m_script_fn(key)};
        out.origins.emplace(key.GetID(), std::move(info));
        out.pubkeys.emplace(key.GetID(), key);
        return true;
    }
};

CScript P2PKHGetScript(const CPubKey& pubkey) { return GetScriptForDestination(pubkey.GetID()); }
CScript P2PKGetScript(const CPubKey& pubkey) { return GetScriptForRawPubKey(pubkey); }
CScript P2WPKHGetScript(const CPubKey& pubkey) { return GetScriptForDestination(WitnessV0KeyHash(pubkey.GetID())); }

/** A parsed multi(...) descriptor. */
class MultisigDescriptor : public Descriptor
{
    int m_threshold;
    std::vector<std::unique_ptr<PubkeyProvider>> m_providers;

public:
    MultisigDescriptor(int threshold, std::vector<std::unique_ptr<PubkeyProvider>> providers) : m_threshold(threshold), m_providers(std::move(providers)) {}

    bool IsRange() const override
    {
        for (const auto& p : m_providers) {
            if (p->IsRange()) return true;
        }
        return false;
    }

    std::string ToString() const override
    {
        std::string ret = strprintf("multi(%i", m_threshold);
        for (const auto& p : m_providers) {
            ret += "," + p->ToString();
        }
        return std::move(ret) + ")";
    }

    bool ToPrivateString(const SigningProvider& arg, std::string& out) const override
    {
        std::string ret = strprintf("multi(%i", m_threshold);
        for (const auto& p : m_providers) {
            std::string sub;
            if (!p->ToPrivateString(arg, sub)) return false;
            ret += "," + std::move(sub);
        }
        out = std::move(ret) + ")";
        return true;
    }

    bool Expand(int pos, const SigningProvider& arg, std::vector<CScript>& output_scripts, FlatSigningProvider& out) const override
    {
        std::vector<std::pair<CPubKey, KeyOriginInfo>> entries;
        entries.reserve(m_providers.size());
        // Construct temporary data in `entries`, to avoid producing output in case of failure.
        for (const auto& p : m_providers) {
            entries.emplace_back();
            if (!p->GetPubKey(pos, arg, entries.back().first, entries.back().second)) return false;
        }
        std::vector<CPubKey> pubkeys;
        pubkeys.reserve(entries.size());
        for (auto& entry : entries) {
            pubkeys.push_back(entry.first);
            out.origins.emplace(entry.first.GetID(), std::move(entry.second));
            out.pubkeys.emplace(entry.first.GetID(), entry.first);
        }
        output_scripts = std::vector<CScript>{GetScriptForMultisig(m_threshold, pubkeys)};
        return true;
    }
};

/** A parsed sh(S) or wsh(S) descriptor. */
class ConvertorDescriptor : public Descriptor
{
    const std::function<CScript(const CScript&)> m_convert_fn;
    const std::string m_fn_name;
    std::unique_ptr<Descriptor> m_descriptor;

public:
    ConvertorDescriptor(std::unique_ptr<Descriptor> descriptor, const std::function<CScript(const CScript&)>& fn, const std::string& name) : m_convert_fn(fn), m_fn_name(name), m_descriptor(std::move(descriptor)) {}

    bool IsRange() const override { return m_descriptor->IsRange(); }
    std::string ToString() const override { return m_fn_name + "(" + m_descriptor->ToString() + ")"; }
    bool ToPrivateString(const SigningProvider& arg, std::string& out) const override
    {
        std::string ret;
        if (!m_descriptor->ToPrivateString(arg, ret)) return false;
        out = m_fn_name + "(" + std::move(ret) + ")";
        return true;
    }
    bool Expand(int pos, const SigningProvider& arg, std::vector<CScript>& output_scripts, FlatSigningProvider& out) const override
    {
        std::vector<CScript> sub;
        if (!m_descriptor->Expand(pos, arg, sub, out)) return false;
        output_scripts.clear();
        for (const auto& script : sub) {
            CScriptID id(script);
            out.scripts.emplace(CScriptID(script), script);
            output_scripts.push_back(m_convert_fn(script));
        }
        return true;
    }
};

CScript ConvertP2SH(const CScript& script) { return GetScriptForDestination(CScriptID(script)); }
CScript ConvertP2WSH(const CScript& script) { return GetScriptForDestination(WitnessV0ScriptHash(script)); }

/** A parsed combo(P) descriptor. */
class ComboDescriptor final : public Descriptor
{
    std::unique_ptr<PubkeyProvider> m_provider;

public:
    ComboDescriptor(std::unique_ptr<PubkeyProvider> provider) : m_provider(std::move(provider)) {}

    bool IsRange() const override { return m_provider->IsRange(); }
    std::string ToString() const override { return "combo(" + m_provider->ToString() + ")"; }
    bool ToPrivateString(const SigningProvider& arg, std::string& out) const override
    {
        std::string ret;
        if (!m_provider->ToPrivateString(arg, ret)) return false;
        out = "combo(" + std::move(ret) + ")";
        return true;
    }
    bool Expand(int pos, const SigningProvider& arg, std::vector<CScript>& output_scripts, FlatSigningProvider& out) const override
    {
        CPubKey key;
        KeyOriginInfo info;
        if (!m_provider->GetPubKey(pos, arg, key, info)) return false;
        CKeyID keyid = key.GetID();
        {
            CScript p2pk = GetScriptForRawPubKey(key);
            CScript p2pkh = GetScriptForDestination(keyid);
            output_scripts = std::vector<CScript>{std::move(p2pk), std::move(p2pkh)};
            out.pubkeys.emplace(keyid, key);
            out.origins.emplace(keyid, std::move(info));
        }
        if (key.IsCompressed()) {
            CScript p2wpkh = GetScriptForDestination(WitnessV0KeyHash(keyid));
            CScriptID p2wpkh_id(p2wpkh);
            CScript p2sh_p2wpkh = GetScriptForDestination(p2wpkh_id);
            out.scripts.emplace(p2wpkh_id, p2wpkh);
            output_scripts.push_back(std::move(p2wpkh));
            output_scripts.push_back(std::move(p2sh_p2wpkh));
        }
        return true;
    }
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
bool ParseKeyPath(const std::vector<Span<const char>>& split, KeyPath& out)
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
std::unique_ptr<Descriptor> ParseScript(Span<const char>& sp, ParseScriptContext ctx, FlatSigningProvider& out)
{
    auto expr = Expr(sp);
    if (Func("pk", expr)) {
        auto pubkey = ParsePubkey(expr, ctx != ParseScriptContext::P2WSH, out);
        if (!pubkey) return nullptr;
        return MakeUnique<SingleKeyDescriptor>(std::move(pubkey), P2PKGetScript, "pk");
    }
    if (Func("pkh", expr)) {
        auto pubkey = ParsePubkey(expr, ctx != ParseScriptContext::P2WSH, out);
        if (!pubkey) return nullptr;
        return MakeUnique<SingleKeyDescriptor>(std::move(pubkey), P2PKHGetScript, "pkh");
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
        return MakeUnique<SingleKeyDescriptor>(std::move(pubkey), P2WPKHGetScript, "wpkh");
    }
    if (ctx == ParseScriptContext::TOP && Func("sh", expr)) {
        auto desc = ParseScript(expr, ParseScriptContext::P2SH, out);
        if (!desc || expr.size()) return nullptr;
        return MakeUnique<ConvertorDescriptor>(std::move(desc), ConvertP2SH, "sh");
    }
    if (ctx != ParseScriptContext::P2WSH && Func("wsh", expr)) {
        auto desc = ParseScript(expr, ParseScriptContext::P2WSH, out);
        if (!desc || expr.size()) return nullptr;
        return MakeUnique<ConvertorDescriptor>(std::move(desc), ConvertP2WSH, "wsh");
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

} // namespace

std::unique_ptr<Descriptor> Parse(const std::string& descriptor, FlatSigningProvider& out)
{
    Span<const char> sp(descriptor.data(), descriptor.size());
    auto ret = ParseScript(sp, ParseScriptContext::TOP, out);
    if (sp.size() == 0 && ret) return ret;
    return nullptr;
}
