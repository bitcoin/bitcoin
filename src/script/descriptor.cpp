// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <script/descriptor.h>

#include <key_io.h>
#include <pubkey.h>
#include <script/script.h>
#include <script/standard.h>

#include <span.h>
#include <util/bip32.h>
#include <util/system.h>
#include <util/strencodings.h>

#include <memory>
#include <string>
#include <vector>

namespace {

////////////////////////////////////////////////////////////////////////////
// Checksum                                                               //
////////////////////////////////////////////////////////////////////////////

// This section implements a checksum algorithm for descriptors with the
// following properties:
// * Mistakes in a descriptor string are measured in "symbol errors". The higher
//   the number of symbol errors, the harder it is to detect:
//   * An error substituting a character from 0123456789()[],'/*abcdefgh@:$%{} for
//     another in that set always counts as 1 symbol error.
//     * Note that hex encoded keys are covered by these characters. Xprvs and
//       xpubs use other characters too, but already have their own checksum
//       mechanism.
//     * Function names like "multi()" use other characters, but mistakes in
//       these would generally result in an unparseable descriptor.
//   * A case error always counts as 1 symbol error.
//   * Any other 1 character substitution error counts as 1 or 2 symbol errors.
// * Any 1 symbol error is always detected.
// * Any 2 or 3 symbol error in a descriptor of up to 49154 characters is always detected.
// * Any 4 symbol error in a descriptor of up to 507 characters is always detected.
// * Any 5 symbol error in a descriptor of up to 77 characters is always detected.
// * Is optimized to minimize the chance a 5 symbol error in a descriptor up to 387 characters is undetected
// * Random errors have a chance of 1 in 2**40 of being undetected.
//
// These properties are achieved by expanding every group of 3 (non checksum) characters into
// 4 GF(32) symbols, over which a cyclic code is defined.

/*
 * Interprets c as 8 groups of 5 bits which are the coefficients of a degree 8 polynomial over GF(32),
 * multiplies that polynomial by x, computes its remainder modulo a generator, and adds the constant term val.
 *
 * This generator is G(x) = x^8 + {30}x^7 + {23}x^6 + {15}x^5 + {14}x^4 + {10}x^3 + {6}x^2 + {12}x + {9}.
 * It is chosen to define an cyclic error detecting code which is selected by:
 * - Starting from all BCH codes over GF(32) of degree 8 and below, which by construction guarantee detecting
 *   3 errors in windows up to 19000 symbols.
 * - Taking all those generators, and for degree 7 ones, extend them to degree 8 by adding all degree-1 factors.
 * - Selecting just the set of generators that guarantee detecting 4 errors in a window of length 512.
 * - Selecting one of those with best worst-case behavior for 5 errors in windows of length up to 512.
 *
 * The generator and the constants to implement it can be verified using this Sage code:
 *   B = GF(2) # Binary field
 *   BP.<b> = B[] # Polynomials over the binary field
 *   F_mod = b**5 + b**3 + 1
 *   F.<f> = GF(32, modulus=F_mod, repr='int') # GF(32) definition
 *   FP.<x> = F[] # Polynomials over GF(32)
 *   E_mod = x**3 + x + F.fetch_int(8)
 *   E.<e> = F.extension(E_mod) # Extension field definition
 *   alpha = e**2743 # Choice of an element in extension field
 *   for p in divisors(E.order() - 1): # Verify alpha has order 32767.
 *       assert((alpha**p == 1) == (p % 32767 == 0))
 *   G = lcm([(alpha**i).minpoly() for i in [1056,1057,1058]] + [x + 1])
 *   print(G) # Print out the generator
 *   for i in [1,2,4,8,16]: # Print out {1,2,4,8,16}*(G mod x^8), packed in hex integers.
 *       v = 0
 *       for coef in reversed((F.fetch_int(i)*(G % x**8)).coefficients(sparse=True)):
 *           v = v*32 + coef.integer_representation()
 *       print("0x%x" % v)
 */
uint64_t PolyMod(uint64_t c, int val)
{
    uint8_t c0 = c >> 35;
    c = ((c & 0x7ffffffff) << 5) ^ val;
    if (c0 & 1) c ^= 0xf5dee51989;
    if (c0 & 2) c ^= 0xa9fdca3312;
    if (c0 & 4) c ^= 0x1bab10e32d;
    if (c0 & 8) c ^= 0x3706b1677a;
    if (c0 & 16) c ^= 0x644d626ffd;
    return c;
}

std::string DescriptorChecksum(const Span<const char>& span)
{
    /** A character set designed such that:
     *  - The most common 'unprotected' descriptor characters (hex, keypaths) are in the first group of 32.
     *  - Case errors cause an offset that's a multiple of 32.
     *  - As many alphabetic characters are in the same group (while following the above restrictions).
     *
     * If p(x) gives the position of a character c in this character set, every group of 3 characters
     * (a,b,c) is encoded as the 4 symbols (p(a) & 31, p(b) & 31, p(c) & 31, (p(a) / 32) + 3 * (p(b) / 32) + 9 * (p(c) / 32).
     * This means that changes that only affect the lower 5 bits of the position, or only the higher 2 bits, will just
     * affect a single symbol.
     *
     * As a result, within-group-of-32 errors count as 1 symbol, as do cross-group errors that don't affect
     * the position within the groups.
     */
    static std::string INPUT_CHARSET =
        "0123456789()[],'/*abcdefgh@:$%{}"
        "IJKLMNOPQRSTUVWXYZ&+-.;<=>?!^_|~"
        "ijklmnopqrstuvwxyzABCDEFGH`#\"\\ ";

    /** The character set for the checksum itself (same as bech32). */
    static std::string CHECKSUM_CHARSET = "qpzry9x8gf2tvdw0s3jn54khce6mua7l";

    uint64_t c = 1;
    int cls = 0;
    int clscount = 0;
    for (auto ch : span) {
        auto pos = INPUT_CHARSET.find(ch);
        if (pos == std::string::npos) return "";
        c = PolyMod(c, pos & 31); // Emit a symbol for the position inside the group, for every character.
        cls = cls * 3 + (pos >> 5); // Accumulate the group numbers
        if (++clscount == 3) {
            // Emit an extra symbol representing the group numbers, for every 3 characters.
            c = PolyMod(c, cls);
            cls = 0;
            clscount = 0;
        }
    }
    if (clscount > 0) c = PolyMod(c, cls);
    for (int j = 0; j < 8; ++j) c = PolyMod(c, 0); // Shift further to determine the checksum.
    c ^= 1; // Prevent appending zeroes from not affecting the checksum.

    std::string ret(8, ' ');
    for (int j = 0; j < 8; ++j) ret[j] = CHECKSUM_CHARSET[(c >> (5 * (7 - j))) & 31];
    return ret;
}

std::string AddChecksum(const std::string& str) { return str + "#" + DescriptorChecksum(MakeSpan(str)); }

////////////////////////////////////////////////////////////////////////////
// Internal representation                                                //
////////////////////////////////////////////////////////////////////////////

typedef std::vector<uint32_t> KeyPath;

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

    /** Derive a private key, if private data is available in arg. */
    virtual bool GetPrivKey(int pos, const SigningProvider& arg, CKey& key) const = 0;
};

class OriginPubkeyProvider final : public PubkeyProvider
{
    KeyOriginInfo m_origin;
    std::unique_ptr<PubkeyProvider> m_provider;

    std::string OriginString() const
    {
        return HexStr(std::begin(m_origin.fingerprint), std::end(m_origin.fingerprint)) + FormatHDKeypath(m_origin.path);
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
    bool GetPrivKey(int pos, const SigningProvider& arg, CKey& key) const override
    {
        return m_provider->GetPrivKey(pos, arg, key);
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
    bool GetPrivKey(int pos, const SigningProvider& arg, CKey& key) const override
    {
        return arg.GetKey(m_pubkey.GetID(), key);
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
                CKey priv_key;
                if (!GetPrivKey(pos, arg, priv_key)) return false;
                *key = priv_key.GetPubKey();
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
        std::string ret = EncodeExtPubKey(m_extkey) + FormatHDKeypath(m_path);
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
        out = EncodeExtKey(key) + FormatHDKeypath(m_path);
        if (IsRange()) {
            out += "/*";
            if (m_derive == DeriveType::HARDENED) out += '\'';
        }
        return true;
    }
    bool GetPrivKey(int pos, const SigningProvider& arg, CKey& key) const override
    {
        CExtKey extkey;
        if (!GetExtKey(arg, extkey)) return false;
        for (auto entry : m_path) {
            extkey.Derive(extkey, entry);
        }
        if (m_derive == DeriveType::UNHARDENED) extkey.Derive(extkey, pos);
        if (m_derive == DeriveType::HARDENED) extkey.Derive(extkey, pos | 0x80000000UL);
        key = extkey.key;
        return true;
    }
};

/** Base class for all Descriptor implementations. */
class DescriptorImpl : public Descriptor
{
    //! Public key arguments for this descriptor (size 1 for PK, PKH, WPKH; any size for Multisig).
    const std::vector<std::unique_ptr<PubkeyProvider>> m_pubkey_args;
    //! The sub-descriptor argument (nullptr for everything but SH and WSH).
    //! In doc/descriptors.m this is referred to as SCRIPT expressions sh(SCRIPT)
    //! and wsh(SCRIPT), and distinct from KEY expressions and ADDR expressions.
    const std::unique_ptr<DescriptorImpl> m_subdescriptor_arg;
    //! The string name of the descriptor function.
    const std::string m_name;

protected:
    //! Return a serialization of anything except pubkey and script arguments, to be prepended to those.
    virtual std::string ToStringExtra() const { return ""; }

    /** A helper function to construct the scripts for this descriptor.
     *
     *  This function is invoked once for every CScript produced by evaluating
     *  m_subdescriptor_arg, or just once in case m_subdescriptor_arg is nullptr.

     *  @param pubkeys The evaluations of the m_pubkey_args field.
     *  @param script The evaluation of m_subdescriptor_arg (or nullptr when m_subdescriptor_arg is nullptr).
     *  @param out A FlatSigningProvider to put scripts or public keys in that are necessary to the solver.
     *             The script arguments to this function are automatically added, as is the origin info of the provided pubkeys.
     *  @return A vector with scriptPubKeys for this descriptor.
     */
    virtual std::vector<CScript> MakeScripts(const std::vector<CPubKey>& pubkeys, const CScript* script, FlatSigningProvider& out) const = 0;

public:
    DescriptorImpl(std::vector<std::unique_ptr<PubkeyProvider>> pubkeys, std::unique_ptr<DescriptorImpl> script, const std::string& name) : m_pubkey_args(std::move(pubkeys)), m_subdescriptor_arg(std::move(script)), m_name(name) {}

    bool IsSolvable() const override
    {
        if (m_subdescriptor_arg) {
            if (!m_subdescriptor_arg->IsSolvable()) return false;
        }
        return true;
    }

    bool IsRange() const final
    {
        for (const auto& pubkey : m_pubkey_args) {
            if (pubkey->IsRange()) return true;
        }
        if (m_subdescriptor_arg) {
            if (m_subdescriptor_arg->IsRange()) return true;
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
        if (m_subdescriptor_arg) {
            if (pos++) ret += ",";
            std::string tmp;
            if (!m_subdescriptor_arg->ToStringHelper(arg, tmp, priv)) return false;
            ret += std::move(tmp);
        }
        out = std::move(ret) + ")";
        return true;
    }

    std::string ToString() const final
    {
        std::string ret;
        ToStringHelper(nullptr, ret, false);
        return AddChecksum(ret);
    }

    bool ToPrivateString(const SigningProvider& arg, std::string& out) const override final
    {
        bool ret = ToStringHelper(&arg, out, true);
        out = AddChecksum(out);
        return ret;
    }

    bool ExpandHelper(int pos, const SigningProvider& arg, Span<const unsigned char>* cache_read, std::vector<CScript>& output_scripts, FlatSigningProvider& out, std::vector<unsigned char>* cache_write) const
    {
        std::vector<std::pair<CPubKey, KeyOriginInfo>> entries;
        entries.reserve(m_pubkey_args.size());

        // Construct temporary data in `entries` and `subscripts`, to avoid producing output in case of failure.
        for (const auto& p : m_pubkey_args) {
            entries.emplace_back();
            // If we have a cache, we don't need GetPubKey to compute the public key.
            // Pass in nullptr to signify only origin info is desired.
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
        if (m_subdescriptor_arg) {
            FlatSigningProvider subprovider;
            if (!m_subdescriptor_arg->ExpandHelper(pos, arg, cache_read, subscripts, subprovider, cache_write)) return false;
            out = Merge(out, subprovider);
        }

        std::vector<CPubKey> pubkeys;
        pubkeys.reserve(entries.size());
        for (auto& entry : entries) {
            pubkeys.push_back(entry.first);
            out.origins.emplace(entry.first.GetID(), std::make_pair<CPubKey, KeyOriginInfo>(CPubKey(entry.first), std::move(entry.second)));
        }
        if (m_subdescriptor_arg) {
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

    void ExpandPrivate(int pos, const SigningProvider& provider, FlatSigningProvider& out) const final
    {
        for (const auto& p : m_pubkey_args) {
            CKey key;
            if (!p->GetPrivKey(pos, provider, key)) continue;
            out.keys.emplace(key.GetPubKey().GetID(), key);
        }
        if (m_subdescriptor_arg) {
            FlatSigningProvider subprovider;
            m_subdescriptor_arg->ExpandPrivate(pos, provider, subprovider);
            out = Merge(out, subprovider);
        }
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
    std::vector<CScript> MakeScripts(const std::vector<CPubKey>& keys, const CScript*, FlatSigningProvider& out) const override
    {
        CKeyID id = keys[0].GetID();
        out.pubkeys.emplace(id, keys[0]);
        return Singleton(GetScriptForDestination(PKHash(id)));
    }
public:
    PKHDescriptor(std::unique_ptr<PubkeyProvider> prov) : DescriptorImpl(Singleton(std::move(prov)), {}, "pkh") {}
};

/** A parsed wpkh(P) descriptor. */
class WPKHDescriptor final : public DescriptorImpl
{
protected:
    std::vector<CScript> MakeScripts(const std::vector<CPubKey>& keys, const CScript*, FlatSigningProvider& out) const override
    {
        CKeyID id = keys[0].GetID();
        out.pubkeys.emplace(id, keys[0]);
        return Singleton(GetScriptForDestination(WitnessV0KeyHash(id)));
    }
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
        out.pubkeys.emplace(id, keys[0]);
        ret.emplace_back(GetScriptForRawPubKey(keys[0])); // P2PK
        ret.emplace_back(GetScriptForDestination(PKHash(id))); // P2PKH
        if (keys[0].IsCompressed()) {
            CScript p2wpkh = GetScriptForDestination(WitnessV0KeyHash(id));
            out.scripts.emplace(CScriptID(p2wpkh), p2wpkh);
            ret.emplace_back(p2wpkh);
            ret.emplace_back(GetScriptForDestination(ScriptHash(p2wpkh))); // P2SH-P2WPKH
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
    std::vector<CScript> MakeScripts(const std::vector<CPubKey>&, const CScript* script, FlatSigningProvider&) const override { return Singleton(GetScriptForDestination(ScriptHash(*script))); }
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
NODISCARD bool ParseKeyPath(const std::vector<Span<const char>>& split, KeyPath& out, std::string& error)
{
    for (size_t i = 1; i < split.size(); ++i) {
        Span<const char> elem = split[i];
        bool hardened = false;
        if (elem.size() > 0 && (elem[elem.size() - 1] == '\'' || elem[elem.size() - 1] == 'h')) {
            elem = elem.first(elem.size() - 1);
            hardened = true;
        }
        uint32_t p;
        if (!ParseUInt32(std::string(elem.begin(), elem.end()), &p)) {
            error = strprintf("Key path value '%s' is not a valid uint32", std::string(elem.begin(), elem.end()).c_str());
            return false;
        } else if (p > 0x7FFFFFFFUL) {
            error = strprintf("Key path value %u is out of range", p);
            return false;
        }
        out.push_back(p | (((uint32_t)hardened) << 31));
    }
    return true;
}

/** Parse a public key that excludes origin information. */
std::unique_ptr<PubkeyProvider> ParsePubkeyInner(const Span<const char>& sp, bool permit_uncompressed, FlatSigningProvider& out, std::string& error)
{
    auto split = Split(sp, '/');
    std::string str(split[0].begin(), split[0].end());
    if (str.size() == 0) {
        error = "No key provided";
        return nullptr;
    }
    if (split.size() == 1) {
        if (IsHex(str)) {
            std::vector<unsigned char> data = ParseHex(str);
            CPubKey pubkey(data);
            if (pubkey.IsFullyValid()) {
                if (permit_uncompressed || pubkey.IsCompressed()) {
                    return MakeUnique<ConstPubkeyProvider>(pubkey);
                } else {
                    error = "Uncompressed keys are not allowed";
                    return nullptr;
                }
            }
            error = strprintf("Pubkey '%s' is invalid", str);
            return nullptr;
        }
        CKey key = DecodeSecret(str);
        if (key.IsValid()) {
            if (permit_uncompressed || key.IsCompressed()) {
                CPubKey pubkey = key.GetPubKey();
                out.keys.emplace(pubkey.GetID(), key);
                return MakeUnique<ConstPubkeyProvider>(pubkey);
            } else {
                error = "Uncompressed keys are not allowed";
                return nullptr;
            }
        }
    }
    CExtKey extkey = DecodeExtKey(str);
    CExtPubKey extpubkey = DecodeExtPubKey(str);
    if (!extkey.key.IsValid() && !extpubkey.pubkey.IsValid()) {
        error = strprintf("key '%s' is not valid", str);
        return nullptr;
    }
    KeyPath path;
    DeriveType type = DeriveType::NO;
    if (split.back() == MakeSpan("*").first(1)) {
        split.pop_back();
        type = DeriveType::UNHARDENED;
    } else if (split.back() == MakeSpan("*'").first(2) || split.back() == MakeSpan("*h").first(2)) {
        split.pop_back();
        type = DeriveType::HARDENED;
    }
    if (!ParseKeyPath(split, path, error)) return nullptr;
    if (extkey.key.IsValid()) {
        extpubkey = extkey.Neuter();
        out.keys.emplace(extpubkey.pubkey.GetID(), extkey.key);
    }
    return MakeUnique<BIP32PubkeyProvider>(extpubkey, std::move(path), type);
}

/** Parse a public key including origin information (if enabled). */
std::unique_ptr<PubkeyProvider> ParsePubkey(const Span<const char>& sp, bool permit_uncompressed, FlatSigningProvider& out, std::string& error)
{
    auto origin_split = Split(sp, ']');
    if (origin_split.size() > 2) {
        error = "Multiple ']' characters found for a single pubkey";
        return nullptr;
    }
    if (origin_split.size() == 1) return ParsePubkeyInner(origin_split[0], permit_uncompressed, out, error);
    if (origin_split[0].size() < 1 || origin_split[0][0] != '[') {
        error = strprintf("Key origin start '[ character expected but not found, got '%c' instead", origin_split[0][0]);
        return nullptr;
    }
    auto slash_split = Split(origin_split[0].subspan(1), '/');
    if (slash_split[0].size() != 8) {
        error = strprintf("Fingerprint is not 4 bytes (%u characters instead of 8 characters)", slash_split[0].size());
        return nullptr;
    }
    std::string fpr_hex = std::string(slash_split[0].begin(), slash_split[0].end());
    if (!IsHex(fpr_hex)) {
        error = strprintf("Fingerprint '%s' is not hex", fpr_hex);
        return nullptr;
    }
    auto fpr_bytes = ParseHex(fpr_hex);
    KeyOriginInfo info;
    static_assert(sizeof(info.fingerprint) == 4, "Fingerprint must be 4 bytes");
    assert(fpr_bytes.size() == 4);
    std::copy(fpr_bytes.begin(), fpr_bytes.end(), info.fingerprint);
    if (!ParseKeyPath(slash_split, info.path, error)) return nullptr;
    auto provider = ParsePubkeyInner(origin_split[1], permit_uncompressed, out, error);
    if (!provider) return nullptr;
    return MakeUnique<OriginPubkeyProvider>(std::move(info), std::move(provider));
}

/** Parse a script in a particular context. */
std::unique_ptr<DescriptorImpl> ParseScript(Span<const char>& sp, ParseScriptContext ctx, FlatSigningProvider& out, std::string& error)
{
    auto expr = Expr(sp);
    if (Func("pk", expr)) {
        auto pubkey = ParsePubkey(expr, ctx != ParseScriptContext::P2WSH, out, error);
        if (!pubkey) return nullptr;
        return MakeUnique<PKDescriptor>(std::move(pubkey));
    }
    if (Func("pkh", expr)) {
        auto pubkey = ParsePubkey(expr, ctx != ParseScriptContext::P2WSH, out, error);
        if (!pubkey) return nullptr;
        return MakeUnique<PKHDescriptor>(std::move(pubkey));
    }
    if (ctx == ParseScriptContext::TOP && Func("combo", expr)) {
        auto pubkey = ParsePubkey(expr, true, out, error);
        if (!pubkey) return nullptr;
        return MakeUnique<ComboDescriptor>(std::move(pubkey));
    } else if (ctx != ParseScriptContext::TOP && Func("combo", expr)) {
        error = "Cannot have combo in non-top level";
        return nullptr;
    }
    if (Func("multi", expr)) {
        auto threshold = Expr(expr);
        uint32_t thres;
        std::vector<std::unique_ptr<PubkeyProvider>> providers;
        if (!ParseUInt32(std::string(threshold.begin(), threshold.end()), &thres)) {
            error = strprintf("Multi threshold '%s' is not valid", std::string(threshold.begin(), threshold.end()).c_str());
            return nullptr;
        }
        size_t script_size = 0;
        while (expr.size()) {
            if (!Const(",", expr)) {
                error = strprintf("Multi: expected ',', got '%c'", expr[0]);
                return nullptr;
            }
            auto arg = Expr(expr);
            auto pk = ParsePubkey(arg, ctx != ParseScriptContext::P2WSH, out, error);
            if (!pk) return nullptr;
            script_size += pk->GetSize() + 1;
            providers.emplace_back(std::move(pk));
        }
        if (providers.size() < 1 || providers.size() > 16) {
            error = strprintf("Cannot have %u keys in multisig; must have between 1 and 16 keys, inclusive", providers.size());
            return nullptr;
        } else if (thres < 1) {
            error = strprintf("Multisig threshold cannot be %d, must be at least 1", thres);
            return nullptr;
        } else if (thres > providers.size()) {
            error = strprintf("Multisig threshold cannot be larger than the number of keys; threshold is %d but only %u keys specified", thres, providers.size());
            return nullptr;
        }
        if (ctx == ParseScriptContext::TOP) {
            if (providers.size() > 3) {
                error = strprintf("Cannot have %u pubkeys in bare multisig; only at most 3 pubkeys", providers.size());
                return nullptr;
            }
        }
        if (ctx == ParseScriptContext::P2SH) {
            if (script_size + 3 > 520) {
                error = strprintf("P2SH script is too large, %d bytes is larger than 520 bytes", script_size + 3);
                return nullptr;
            }
        }
        return MakeUnique<MultisigDescriptor>(thres, std::move(providers));
    }
    if (ctx != ParseScriptContext::P2WSH && Func("wpkh", expr)) {
        auto pubkey = ParsePubkey(expr, false, out, error);
        if (!pubkey) return nullptr;
        return MakeUnique<WPKHDescriptor>(std::move(pubkey));
    } else if (ctx == ParseScriptContext::P2WSH && Func("wpkh", expr)) {
        error = "Cannot have wpkh within wsh";
        return nullptr;
    }
    if (ctx == ParseScriptContext::TOP && Func("sh", expr)) {
        auto desc = ParseScript(expr, ParseScriptContext::P2SH, out, error);
        if (!desc || expr.size()) return nullptr;
        return MakeUnique<SHDescriptor>(std::move(desc));
    } else if (ctx != ParseScriptContext::TOP && Func("sh", expr)) {
        error = "Cannot have sh in non-top level";
        return nullptr;
    }
    if (ctx != ParseScriptContext::P2WSH && Func("wsh", expr)) {
        auto desc = ParseScript(expr, ParseScriptContext::P2WSH, out, error);
        if (!desc || expr.size()) return nullptr;
        return MakeUnique<WSHDescriptor>(std::move(desc));
    } else if (ctx == ParseScriptContext::P2WSH && Func("wsh", expr)) {
        error = "Cannot have wsh within wsh";
        return nullptr;
    }
    if (ctx == ParseScriptContext::TOP && Func("addr", expr)) {
        CTxDestination dest = DecodeDestination(std::string(expr.begin(), expr.end()));
        if (!IsValidDestination(dest)) {
            error = "Address is not valid";
            return nullptr;
        }
        return MakeUnique<AddressDescriptor>(std::move(dest));
    }
    if (ctx == ParseScriptContext::TOP && Func("raw", expr)) {
        std::string str(expr.begin(), expr.end());
        if (!IsHex(str)) {
            error = "Raw script is not hex";
            return nullptr;
        }
        auto bytes = ParseHex(str);
        return MakeUnique<RawDescriptor>(CScript(bytes.begin(), bytes.end()));
    }
    if (ctx == ParseScriptContext::P2SH) {
        error = "A function is needed within P2SH";
        return nullptr;
    } else if (ctx == ParseScriptContext::P2WSH) {
        error = "A function is needed within P2WSH";
        return nullptr;
    }
    error = strprintf("%s is not a valid descriptor function", std::string(expr.begin(), expr.end()));
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

/** Check a descriptor checksum, and update desc to be the checksum-less part. */
bool CheckChecksum(Span<const char>& sp, bool require_checksum, std::string& error, std::string* out_checksum = nullptr)
{
    auto check_split = Split(sp, '#');
    if (check_split.size() > 2) {
        error = "Multiple '#' symbols";
        return false;
    }
    if (check_split.size() == 1 && require_checksum){
        error = "Missing checksum";
        return false;
    }
    if (check_split.size() == 2) {
        if (check_split[1].size() != 8) {
            error = strprintf("Expected 8 character checksum, not %u characters", check_split[1].size());
            return false;
        }
    }
    auto checksum = DescriptorChecksum(check_split[0]);
    if (checksum.empty()) {
        error = "Invalid characters in payload";
        return false;
    }
    if (check_split.size() == 2) {
        if (!std::equal(checksum.begin(), checksum.end(), check_split[1].begin())) {
            error = strprintf("Provided checksum '%s' does not match computed checksum '%s'", std::string(check_split[1].begin(), check_split[1].end()), checksum);
            return false;
        }
    }
    if (out_checksum) *out_checksum = std::move(checksum);
    sp = check_split[0];
    return true;
}

std::unique_ptr<Descriptor> Parse(const std::string& descriptor, FlatSigningProvider& out, std::string& error, bool require_checksum)
{
    Span<const char> sp(descriptor.data(), descriptor.size());
    if (!CheckChecksum(sp, require_checksum, error)) return nullptr;
    auto ret = ParseScript(sp, ParseScriptContext::TOP, out, error);
    if (sp.size() == 0 && ret) return std::unique_ptr<Descriptor>(std::move(ret));
    return nullptr;
}

std::string GetDescriptorChecksum(const std::string& descriptor)
{
    std::string ret;
    std::string error;
    Span<const char> sp(descriptor.data(), descriptor.size());
    if (!CheckChecksum(sp, false, error, &ret)) return "";
    return ret;
}

std::unique_ptr<Descriptor> InferDescriptor(const CScript& script, const SigningProvider& provider)
{
    return InferScript(script, ParseScriptContext::TOP, provider);
}
