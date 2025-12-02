// Copyright (c) 2018-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <script/descriptor.h>

#include <hash.h>
#include <key_io.h>
#include <pubkey.h>
#include <musig.h>
#include <script/miniscript.h>
#include <script/parsing.h>
#include <script/script.h>
#include <script/signingprovider.h>
#include <script/solver.h>
#include <uint256.h>

#include <common/args.h>
#include <span.h>
#include <util/bip32.h>
#include <util/check.h>
#include <util/strencodings.h>
#include <util/vector.h>

#include <algorithm>
#include <memory>
#include <numeric>
#include <optional>
#include <string>
#include <vector>

using util::Split;

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
//       these would generally result in an unparsable descriptor.
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

std::string DescriptorChecksum(const std::span<const char>& span)
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
    static const std::string INPUT_CHARSET =
        "0123456789()[],'/*abcdefgh@:$%{}"
        "IJKLMNOPQRSTUVWXYZ&+-.;<=>?!^_|~"
        "ijklmnopqrstuvwxyzABCDEFGH`#\"\\ ";

    /** The character set for the checksum itself (same as bech32). */
    static const std::string CHECKSUM_CHARSET = "qpzry9x8gf2tvdw0s3jn54khce6mua7l";

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

std::string AddChecksum(const std::string& str) { return str + "#" + DescriptorChecksum(str); }

////////////////////////////////////////////////////////////////////////////
// Internal representation                                                //
////////////////////////////////////////////////////////////////////////////

typedef std::vector<uint32_t> KeyPath;

/** Interface for public key objects in descriptors. */
struct PubkeyProvider
{
protected:
    //! Index of this key expression in the descriptor
    //! E.g. If this PubkeyProvider is key1 in multi(2, key1, key2, key3), then m_expr_index = 0
    uint32_t m_expr_index;

public:
    explicit PubkeyProvider(uint32_t exp_index) : m_expr_index(exp_index) {}

    virtual ~PubkeyProvider() = default;

    /** Compare two public keys represented by this provider.
     * Used by the Miniscript descriptors to check for duplicate keys in the script.
     */
    bool operator<(PubkeyProvider& other) const {
        FlatSigningProvider dummy;

        std::optional<CPubKey> a = GetPubKey(0, dummy, dummy);
        std::optional<CPubKey> b = other.GetPubKey(0, dummy, dummy);

        return a < b;
    }

    /** Derive a public key and put it into out.
     *  read_cache is the cache to read keys from (if not nullptr)
     *  write_cache is the cache to write keys to (if not nullptr)
     *  Caches are not exclusive but this is not tested. Currently we use them exclusively
     */
    virtual std::optional<CPubKey> GetPubKey(int pos, const SigningProvider& arg, FlatSigningProvider& out, const DescriptorCache* read_cache = nullptr, DescriptorCache* write_cache = nullptr) const = 0;

    /** Whether this represent multiple public keys at different positions. */
    virtual bool IsRange() const = 0;

    /** Get the size of the generated public key(s) in bytes (33 or 65). */
    virtual size_t GetSize() const = 0;

    enum class StringType {
        PUBLIC,
        COMPAT // string calculation that mustn't change over time to stay compatible with previous software versions
    };

    /** Get the descriptor string form. */
    virtual std::string ToString(StringType type=StringType::PUBLIC) const = 0;

    /** Get the descriptor string form including private data (if available in arg). */
    virtual bool ToPrivateString(const SigningProvider& arg, std::string& out) const = 0;

    /** Get the descriptor string form with the xpub at the last hardened derivation,
     *  and always use h for hardened derivation.
     */
    virtual bool ToNormalizedString(const SigningProvider& arg, std::string& out, const DescriptorCache* cache = nullptr) const = 0;

    /** Derive a private key, if private data is available in arg and put it into out. */
    virtual void GetPrivKey(int pos, const SigningProvider& arg, FlatSigningProvider& out) const = 0;

    /** Return the non-extended public key for this PubkeyProvider, if it has one. */
    virtual std::optional<CPubKey> GetRootPubKey() const = 0;
    /** Return the extended public key for this PubkeyProvider, if it has one. */
    virtual std::optional<CExtPubKey> GetRootExtPubKey() const = 0;

    /** Make a deep copy of this PubkeyProvider */
    virtual std::unique_ptr<PubkeyProvider> Clone() const = 0;

    /** Whether this PubkeyProvider is a BIP 32 extended key that can be derived from */
    virtual bool IsBIP32() const = 0;
};

class OriginPubkeyProvider final : public PubkeyProvider
{
    KeyOriginInfo m_origin;
    std::unique_ptr<PubkeyProvider> m_provider;
    bool m_apostrophe;

    std::string OriginString(StringType type, bool normalized=false) const
    {
        // If StringType==COMPAT, always use the apostrophe to stay compatible with previous versions
        bool use_apostrophe = (!normalized && m_apostrophe) || type == StringType::COMPAT;
        return HexStr(m_origin.fingerprint) + FormatHDKeypath(m_origin.path, use_apostrophe);
    }

public:
    OriginPubkeyProvider(uint32_t exp_index, KeyOriginInfo info, std::unique_ptr<PubkeyProvider> provider, bool apostrophe) : PubkeyProvider(exp_index), m_origin(std::move(info)), m_provider(std::move(provider)), m_apostrophe(apostrophe) {}
    std::optional<CPubKey> GetPubKey(int pos, const SigningProvider& arg, FlatSigningProvider& out, const DescriptorCache* read_cache = nullptr, DescriptorCache* write_cache = nullptr) const override
    {
        std::optional<CPubKey> pub = m_provider->GetPubKey(pos, arg, out, read_cache, write_cache);
        if (!pub) return std::nullopt;
        Assert(out.pubkeys.contains(pub->GetID()));
        auto& [pubkey, suborigin] = out.origins[pub->GetID()];
        Assert(pubkey == *pub); // m_provider must have a valid origin by this point.
        std::copy(std::begin(m_origin.fingerprint), std::end(m_origin.fingerprint), suborigin.fingerprint);
        suborigin.path.insert(suborigin.path.begin(), m_origin.path.begin(), m_origin.path.end());
        return pub;
    }
    bool IsRange() const override { return m_provider->IsRange(); }
    size_t GetSize() const override { return m_provider->GetSize(); }
    bool IsBIP32() const override { return m_provider->IsBIP32(); }
    std::string ToString(StringType type) const override { return "[" + OriginString(type) + "]" + m_provider->ToString(type); }
    bool ToPrivateString(const SigningProvider& arg, std::string& ret) const override
    {
        std::string sub;
        if (!m_provider->ToPrivateString(arg, sub)) return false;
        ret = "[" + OriginString(StringType::PUBLIC) + "]" + std::move(sub);
        return true;
    }
    bool ToNormalizedString(const SigningProvider& arg, std::string& ret, const DescriptorCache* cache) const override
    {
        std::string sub;
        if (!m_provider->ToNormalizedString(arg, sub, cache)) return false;
        // If m_provider is a BIP32PubkeyProvider, we may get a string formatted like a OriginPubkeyProvider
        // In that case, we need to strip out the leading square bracket and fingerprint from the substring,
        // and append that to our own origin string.
        if (sub[0] == '[') {
            sub = sub.substr(9);
            ret = "[" + OriginString(StringType::PUBLIC, /*normalized=*/true) + std::move(sub);
        } else {
            ret = "[" + OriginString(StringType::PUBLIC, /*normalized=*/true) + "]" + std::move(sub);
        }
        return true;
    }
    void GetPrivKey(int pos, const SigningProvider& arg, FlatSigningProvider& out) const override
    {
        m_provider->GetPrivKey(pos, arg, out);
    }
    std::optional<CPubKey> GetRootPubKey() const override
    {
        return m_provider->GetRootPubKey();
    }
    std::optional<CExtPubKey> GetRootExtPubKey() const override
    {
        return m_provider->GetRootExtPubKey();
    }
    std::unique_ptr<PubkeyProvider> Clone() const override
    {
        return std::make_unique<OriginPubkeyProvider>(m_expr_index, m_origin, m_provider->Clone(), m_apostrophe);
    }
};

/** An object representing a parsed constant public key in a descriptor. */
class ConstPubkeyProvider final : public PubkeyProvider
{
    CPubKey m_pubkey;
    bool m_xonly;

    std::optional<CKey> GetPrivKey(const SigningProvider& arg) const
    {
        CKey key;
        if (!(m_xonly ? arg.GetKeyByXOnly(XOnlyPubKey(m_pubkey), key) :
                        arg.GetKey(m_pubkey.GetID(), key))) return std::nullopt;
        return key;
    }

public:
    ConstPubkeyProvider(uint32_t exp_index, const CPubKey& pubkey, bool xonly) : PubkeyProvider(exp_index), m_pubkey(pubkey), m_xonly(xonly) {}
    std::optional<CPubKey> GetPubKey(int pos, const SigningProvider&, FlatSigningProvider& out, const DescriptorCache* read_cache = nullptr, DescriptorCache* write_cache = nullptr) const override
    {
        KeyOriginInfo info;
        CKeyID keyid = m_pubkey.GetID();
        std::copy(keyid.begin(), keyid.begin() + sizeof(info.fingerprint), info.fingerprint);
        out.origins.emplace(keyid, std::make_pair(m_pubkey, info));
        out.pubkeys.emplace(keyid, m_pubkey);
        return m_pubkey;
    }
    bool IsRange() const override { return false; }
    size_t GetSize() const override { return m_pubkey.size(); }
    bool IsBIP32() const override { return false; }
    std::string ToString(StringType type) const override { return m_xonly ? HexStr(m_pubkey).substr(2) : HexStr(m_pubkey); }
    bool ToPrivateString(const SigningProvider& arg, std::string& ret) const override
    {
        std::optional<CKey> key = GetPrivKey(arg);
        if (!key) return false;
        ret = EncodeSecret(*key);
        return true;
    }
    bool ToNormalizedString(const SigningProvider& arg, std::string& ret, const DescriptorCache* cache) const override
    {
        ret = ToString(StringType::PUBLIC);
        return true;
    }
    void GetPrivKey(int pos, const SigningProvider& arg, FlatSigningProvider& out) const override
    {
        std::optional<CKey> key = GetPrivKey(arg);
        if (!key) return;
        out.keys.emplace(key->GetPubKey().GetID(), *key);
    }
    std::optional<CPubKey> GetRootPubKey() const override
    {
        return m_pubkey;
    }
    std::optional<CExtPubKey> GetRootExtPubKey() const override
    {
        return std::nullopt;
    }
    std::unique_ptr<PubkeyProvider> Clone() const override
    {
        return std::make_unique<ConstPubkeyProvider>(m_expr_index, m_pubkey, m_xonly);
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
    // Root xpub, path, and final derivation step type being used, if any
    CExtPubKey m_root_extkey;
    KeyPath m_path;
    DeriveType m_derive;
    // Whether ' or h is used in harded derivation
    bool m_apostrophe;

    bool GetExtKey(const SigningProvider& arg, CExtKey& ret) const
    {
        CKey key;
        if (!arg.GetKey(m_root_extkey.pubkey.GetID(), key)) return false;
        ret.nDepth = m_root_extkey.nDepth;
        std::copy(m_root_extkey.vchFingerprint, m_root_extkey.vchFingerprint + sizeof(ret.vchFingerprint), ret.vchFingerprint);
        ret.nChild = m_root_extkey.nChild;
        ret.chaincode = m_root_extkey.chaincode;
        ret.key = key;
        return true;
    }

    // Derives the last xprv
    bool GetDerivedExtKey(const SigningProvider& arg, CExtKey& xprv, CExtKey& last_hardened) const
    {
        if (!GetExtKey(arg, xprv)) return false;
        for (auto entry : m_path) {
            if (!xprv.Derive(xprv, entry)) return false;
            if (entry >> 31) {
                last_hardened = xprv;
            }
        }
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
    BIP32PubkeyProvider(uint32_t exp_index, const CExtPubKey& extkey, KeyPath path, DeriveType derive, bool apostrophe) : PubkeyProvider(exp_index), m_root_extkey(extkey), m_path(std::move(path)), m_derive(derive), m_apostrophe(apostrophe) {}
    bool IsRange() const override { return m_derive != DeriveType::NO; }
    size_t GetSize() const override { return 33; }
    bool IsBIP32() const override { return true; }
    std::optional<CPubKey> GetPubKey(int pos, const SigningProvider& arg, FlatSigningProvider& out, const DescriptorCache* read_cache = nullptr, DescriptorCache* write_cache = nullptr) const override
    {
        KeyOriginInfo info;
        CKeyID keyid = m_root_extkey.pubkey.GetID();
        std::copy(keyid.begin(), keyid.begin() + sizeof(info.fingerprint), info.fingerprint);
        info.path = m_path;
        if (m_derive == DeriveType::UNHARDENED) info.path.push_back((uint32_t)pos);
        if (m_derive == DeriveType::HARDENED) info.path.push_back(((uint32_t)pos) | 0x80000000L);

        // Derive keys or fetch them from cache
        CExtPubKey final_extkey = m_root_extkey;
        CExtPubKey parent_extkey = m_root_extkey;
        CExtPubKey last_hardened_extkey;
        bool der = true;
        if (read_cache) {
            if (!read_cache->GetCachedDerivedExtPubKey(m_expr_index, pos, final_extkey)) {
                if (m_derive == DeriveType::HARDENED) return std::nullopt;
                // Try to get the derivation parent
                if (!read_cache->GetCachedParentExtPubKey(m_expr_index, parent_extkey)) return std::nullopt;
                final_extkey = parent_extkey;
                if (m_derive == DeriveType::UNHARDENED) der = parent_extkey.Derive(final_extkey, pos);
            }
        } else if (IsHardened()) {
            CExtKey xprv;
            CExtKey lh_xprv;
            if (!GetDerivedExtKey(arg, xprv, lh_xprv)) return std::nullopt;
            parent_extkey = xprv.Neuter();
            if (m_derive == DeriveType::UNHARDENED) der = xprv.Derive(xprv, pos);
            if (m_derive == DeriveType::HARDENED) der = xprv.Derive(xprv, pos | 0x80000000UL);
            final_extkey = xprv.Neuter();
            if (lh_xprv.key.IsValid()) {
                last_hardened_extkey = lh_xprv.Neuter();
            }
        } else {
            for (auto entry : m_path) {
                if (!parent_extkey.Derive(parent_extkey, entry)) return std::nullopt;
            }
            final_extkey = parent_extkey;
            if (m_derive == DeriveType::UNHARDENED) der = parent_extkey.Derive(final_extkey, pos);
            assert(m_derive != DeriveType::HARDENED);
        }
        if (!der) return std::nullopt;

        out.origins.emplace(final_extkey.pubkey.GetID(), std::make_pair(final_extkey.pubkey, info));
        out.pubkeys.emplace(final_extkey.pubkey.GetID(), final_extkey.pubkey);

        if (write_cache) {
            // Only cache parent if there is any unhardened derivation
            if (m_derive != DeriveType::HARDENED) {
                write_cache->CacheParentExtPubKey(m_expr_index, parent_extkey);
                // Cache last hardened xpub if we have it
                if (last_hardened_extkey.pubkey.IsValid()) {
                    write_cache->CacheLastHardenedExtPubKey(m_expr_index, last_hardened_extkey);
                }
            } else if (info.path.size() > 0) {
                write_cache->CacheDerivedExtPubKey(m_expr_index, pos, final_extkey);
            }
        }

        return final_extkey.pubkey;
    }
    std::string ToString(StringType type, bool normalized) const
    {
        // If StringType==COMPAT, always use the apostrophe to stay compatible with previous versions
        const bool use_apostrophe = (!normalized && m_apostrophe) || type == StringType::COMPAT;
        std::string ret = EncodeExtPubKey(m_root_extkey) + FormatHDKeypath(m_path, /*apostrophe=*/use_apostrophe);
        if (IsRange()) {
            ret += "/*";
            if (m_derive == DeriveType::HARDENED) ret += use_apostrophe ? '\'' : 'h';
        }
        return ret;
    }
    std::string ToString(StringType type=StringType::PUBLIC) const override
    {
        return ToString(type, /*normalized=*/false);
    }
    bool ToPrivateString(const SigningProvider& arg, std::string& out) const override
    {
        CExtKey key;
        if (!GetExtKey(arg, key)) return false;
        out = EncodeExtKey(key) + FormatHDKeypath(m_path, /*apostrophe=*/m_apostrophe);
        if (IsRange()) {
            out += "/*";
            if (m_derive == DeriveType::HARDENED) out += m_apostrophe ? '\'' : 'h';
        }
        return true;
    }
    bool ToNormalizedString(const SigningProvider& arg, std::string& out, const DescriptorCache* cache) const override
    {
        if (m_derive == DeriveType::HARDENED) {
            out = ToString(StringType::PUBLIC, /*normalized=*/true);

            return true;
        }
        // Step backwards to find the last hardened step in the path
        int i = (int)m_path.size() - 1;
        for (; i >= 0; --i) {
            if (m_path.at(i) >> 31) {
                break;
            }
        }
        // Either no derivation or all unhardened derivation
        if (i == -1) {
            out = ToString();
            return true;
        }
        // Get the path to the last hardened stup
        KeyOriginInfo origin;
        int k = 0;
        for (; k <= i; ++k) {
            // Add to the path
            origin.path.push_back(m_path.at(k));
        }
        // Build the remaining path
        KeyPath end_path;
        for (; k < (int)m_path.size(); ++k) {
            end_path.push_back(m_path.at(k));
        }
        // Get the fingerprint
        CKeyID id = m_root_extkey.pubkey.GetID();
        std::copy(id.begin(), id.begin() + 4, origin.fingerprint);

        CExtPubKey xpub;
        CExtKey lh_xprv;
        // If we have the cache, just get the parent xpub
        if (cache != nullptr) {
            cache->GetCachedLastHardenedExtPubKey(m_expr_index, xpub);
        }
        if (!xpub.pubkey.IsValid()) {
            // Cache miss, or nor cache, or need privkey
            CExtKey xprv;
            if (!GetDerivedExtKey(arg, xprv, lh_xprv)) return false;
            xpub = lh_xprv.Neuter();
        }
        assert(xpub.pubkey.IsValid());

        // Build the string
        std::string origin_str = HexStr(origin.fingerprint) + FormatHDKeypath(origin.path);
        out = "[" + origin_str + "]" + EncodeExtPubKey(xpub) + FormatHDKeypath(end_path);
        if (IsRange()) {
            out += "/*";
            assert(m_derive == DeriveType::UNHARDENED);
        }
        return true;
    }
    void GetPrivKey(int pos, const SigningProvider& arg, FlatSigningProvider& out) const override
    {
        CExtKey extkey;
        CExtKey dummy;
        if (!GetDerivedExtKey(arg, extkey, dummy)) return;
        if (m_derive == DeriveType::UNHARDENED && !extkey.Derive(extkey, pos)) return;
        if (m_derive == DeriveType::HARDENED && !extkey.Derive(extkey, pos | 0x80000000UL)) return;
        out.keys.emplace(extkey.key.GetPubKey().GetID(), extkey.key);
    }
    std::optional<CPubKey> GetRootPubKey() const override
    {
        return std::nullopt;
    }
    std::optional<CExtPubKey> GetRootExtPubKey() const override
    {
        return m_root_extkey;
    }
    std::unique_ptr<PubkeyProvider> Clone() const override
    {
        return std::make_unique<BIP32PubkeyProvider>(m_expr_index, m_root_extkey, m_path, m_derive, m_apostrophe);
    }
};

/** PubkeyProvider for a musig() expression */
class MuSigPubkeyProvider final : public PubkeyProvider
{
private:
    //! PubkeyProvider for the participants
    const std::vector<std::unique_ptr<PubkeyProvider>> m_participants;
    //! Derivation path
    const KeyPath m_path;
    //! PubkeyProvider for the aggregate pubkey if it can be cached (i.e. participants are not ranged)
    mutable std::unique_ptr<PubkeyProvider> m_aggregate_provider;
    mutable std::optional<CPubKey> m_aggregate_pubkey;
    const DeriveType m_derive;
    const bool m_ranged_participants;

    bool IsRangedDerivation() const { return m_derive != DeriveType::NO; }

public:
    MuSigPubkeyProvider(
        uint32_t exp_index,
        std::vector<std::unique_ptr<PubkeyProvider>> providers,
        KeyPath path,
        DeriveType derive
    )
        : PubkeyProvider(exp_index),
        m_participants(std::move(providers)),
        m_path(std::move(path)),
        m_derive(derive),
        m_ranged_participants(std::any_of(m_participants.begin(), m_participants.end(), [](const auto& pubkey) { return pubkey->IsRange(); }))
    {
        if (!Assume(!(m_ranged_participants && IsRangedDerivation()))) {
            throw std::runtime_error("musig(): Cannot have both ranged participants and ranged derivation");
        }
        if (!Assume(m_derive != DeriveType::HARDENED)) {
            throw std::runtime_error("musig(): Cannot have hardened derivation");
        }
    }

    std::optional<CPubKey> GetPubKey(int pos, const SigningProvider& arg, FlatSigningProvider& out, const DescriptorCache* read_cache = nullptr, DescriptorCache* write_cache = nullptr) const override
    {
        FlatSigningProvider dummy;
        // If the participants are not ranged, we can compute and cache the aggregate pubkey by creating a PubkeyProvider for it
        if (!m_aggregate_provider && !m_ranged_participants) {
            // Retrieve the pubkeys from the providers
            std::vector<CPubKey> pubkeys;
            for (const auto& prov : m_participants) {
                std::optional<CPubKey> pubkey = prov->GetPubKey(0, arg, dummy, read_cache, write_cache);
                if (!pubkey.has_value()) {
                    return std::nullopt;
                }
                pubkeys.push_back(pubkey.value());
            }
            std::sort(pubkeys.begin(), pubkeys.end());

            // Aggregate the pubkey
            m_aggregate_pubkey = MuSig2AggregatePubkeys(pubkeys);
            if (!Assume(m_aggregate_pubkey.has_value())) return std::nullopt;

            // Make our pubkey provider
            if (IsRangedDerivation() || !m_path.empty()) {
                // Make the synthetic xpub and construct the BIP32PubkeyProvider
                CExtPubKey extpub = CreateMuSig2SyntheticXpub(m_aggregate_pubkey.value());
                m_aggregate_provider = std::make_unique<BIP32PubkeyProvider>(m_expr_index, extpub, m_path, m_derive, /*apostrophe=*/false);
            } else {
                m_aggregate_provider = std::make_unique<ConstPubkeyProvider>(m_expr_index, m_aggregate_pubkey.value(), /*xonly=*/false);
            }
        }

        // Retrieve all participant pubkeys
        std::vector<CPubKey> pubkeys;
        for (const auto& prov : m_participants) {
            std::optional<CPubKey> pub = prov->GetPubKey(pos, arg, out, read_cache, write_cache);
            if (!pub) return std::nullopt;
            pubkeys.emplace_back(*pub);
        }
        std::sort(pubkeys.begin(), pubkeys.end());

        CPubKey pubout;
        if (m_aggregate_provider) {
            // When we have a cached aggregate key, we are either returning it or deriving from it
            // Either way, we can passthrough to its GetPubKey
            // Use a dummy signing provider as private keys do not exist for the aggregate pubkey
            std::optional<CPubKey> pub = m_aggregate_provider->GetPubKey(pos, dummy, out, read_cache, write_cache);
            if (!pub) return std::nullopt;
            pubout = *pub;
            out.aggregate_pubkeys.emplace(m_aggregate_pubkey.value(), pubkeys);
        } else {
            if (!Assume(m_ranged_participants) || !Assume(m_path.empty())) return std::nullopt;
            // Compute aggregate key from derived participants
            std::optional<CPubKey> aggregate_pubkey = MuSig2AggregatePubkeys(pubkeys);
            if (!aggregate_pubkey) return std::nullopt;
            pubout = *aggregate_pubkey;

            std::unique_ptr<ConstPubkeyProvider> this_agg_provider = std::make_unique<ConstPubkeyProvider>(m_expr_index, aggregate_pubkey.value(), /*xonly=*/false);
            this_agg_provider->GetPubKey(0, dummy, out, read_cache, write_cache);
            out.aggregate_pubkeys.emplace(pubout, pubkeys);
        }

        if (!Assume(pubout.IsValid())) return std::nullopt;
        return pubout;
    }
    bool IsRange() const override { return IsRangedDerivation() || m_ranged_participants; }
    // musig() expressions can only be used in tr() contexts which have 32 byte xonly pubkeys
    size_t GetSize() const override { return 32; }

    std::string ToString(StringType type=StringType::PUBLIC) const override
    {
        std::string out = "musig(";
        for (size_t i = 0; i < m_participants.size(); ++i) {
            const auto& pubkey = m_participants.at(i);
            if (i) out += ",";
            out += pubkey->ToString(type);
        }
        out += ")";
        out += FormatHDKeypath(m_path);
        if (IsRangedDerivation()) {
            out += "/*";
        }
        return out;
    }
    bool ToPrivateString(const SigningProvider& arg, std::string& out) const override
    {
        bool any_privkeys = false;
        out = "musig(";
        for (size_t i = 0; i < m_participants.size(); ++i) {
            const auto& pubkey = m_participants.at(i);
            if (i) out += ",";
            std::string tmp;
            if (pubkey->ToPrivateString(arg, tmp)) {
                any_privkeys = true;
                out += tmp;
            } else {
                out += pubkey->ToString();
            }
        }
        out += ")";
        out += FormatHDKeypath(m_path);
        if (IsRangedDerivation()) {
            out += "/*";
        }
        if (!any_privkeys) out.clear();
        return any_privkeys;
    }
    bool ToNormalizedString(const SigningProvider& arg, std::string& out, const DescriptorCache* cache = nullptr) const override
    {
        out = "musig(";
        for (size_t i = 0; i < m_participants.size(); ++i) {
            const auto& pubkey = m_participants.at(i);
            if (i) out += ",";
            std::string tmp;
            if (!pubkey->ToNormalizedString(arg, tmp, cache)) {
                return false;
            }
            out += tmp;
        }
        out += ")";
        out += FormatHDKeypath(m_path);
        if (IsRangedDerivation()) {
            out += "/*";
        }
        return true;
    }

    void GetPrivKey(int pos, const SigningProvider& arg, FlatSigningProvider& out) const override
    {
        // Get the private keys for any participants that we have
        // If there is participant derivation, it will be done.
        // If there is not, then the participant privkeys will be included directly
        for (const auto& prov : m_participants) {
            prov->GetPrivKey(pos, arg, out);
        }
    }

    // Get RootPubKey and GetRootExtPubKey are used to return the single pubkey underlying the pubkey provider
    // to be presented to the user in gethdkeys. As this is a multisig construction, there is no single underlying
    // pubkey hence nothing should be returned.
    // While the aggregate pubkey could be returned as the root (ext)pubkey, it is not a pubkey that anyone should
    // be using by itself in a descriptor as it is unspendable without knowing its participants.
    std::optional<CPubKey> GetRootPubKey() const override
    {
        return std::nullopt;
    }
    std::optional<CExtPubKey> GetRootExtPubKey() const override
    {
        return std::nullopt;
    }

    std::unique_ptr<PubkeyProvider> Clone() const override
    {
        std::vector<std::unique_ptr<PubkeyProvider>> providers;
        providers.reserve(m_participants.size());
        for (const std::unique_ptr<PubkeyProvider>& p : m_participants) {
            providers.emplace_back(p->Clone());
        }
        return std::make_unique<MuSigPubkeyProvider>(m_expr_index, std::move(providers), m_path, m_derive);
    }
    bool IsBIP32() const override
    {
        // musig() can only be a BIP 32 key if all participants are bip32 too
        return std::all_of(m_participants.begin(), m_participants.end(), [](const auto& pubkey) { return pubkey->IsBIP32(); });
    }
};

/** Base class for all Descriptor implementations. */
class DescriptorImpl : public Descriptor
{
protected:
    //! Public key arguments for this descriptor (size 1 for PK, PKH, WPKH; any size for WSH and Multisig).
    const std::vector<std::unique_ptr<PubkeyProvider>> m_pubkey_args;
    //! The string name of the descriptor function.
    const std::string m_name;

    //! The sub-descriptor arguments (empty for everything but SH and WSH).
    //! In doc/descriptors.m this is referred to as SCRIPT expressions sh(SCRIPT)
    //! and wsh(SCRIPT), and distinct from KEY expressions and ADDR expressions.
    //! Subdescriptors can only ever generate a single script.
    const std::vector<std::unique_ptr<DescriptorImpl>> m_subdescriptor_args;

    //! Return a serialization of anything except pubkey and script arguments, to be prepended to those.
    virtual std::string ToStringExtra() const { return ""; }

    /** A helper function to construct the scripts for this descriptor.
     *
     *  This function is invoked once by ExpandHelper.
     *
     *  @param pubkeys The evaluations of the m_pubkey_args field.
     *  @param scripts The evaluations of m_subdescriptor_args (one for each m_subdescriptor_args element).
     *  @param out A FlatSigningProvider to put scripts or public keys in that are necessary to the solver.
     *             The origin info of the provided pubkeys is automatically added.
     *  @return A vector with scriptPubKeys for this descriptor.
     */
    virtual std::vector<CScript> MakeScripts(const std::vector<CPubKey>& pubkeys, std::span<const CScript> scripts, FlatSigningProvider& out) const = 0;

public:
    DescriptorImpl(std::vector<std::unique_ptr<PubkeyProvider>> pubkeys, const std::string& name) : m_pubkey_args(std::move(pubkeys)), m_name(name), m_subdescriptor_args() {}
    DescriptorImpl(std::vector<std::unique_ptr<PubkeyProvider>> pubkeys, std::unique_ptr<DescriptorImpl> script, const std::string& name) : m_pubkey_args(std::move(pubkeys)), m_name(name), m_subdescriptor_args(Vector(std::move(script))) {}
    DescriptorImpl(std::vector<std::unique_ptr<PubkeyProvider>> pubkeys, std::vector<std::unique_ptr<DescriptorImpl>> scripts, const std::string& name) : m_pubkey_args(std::move(pubkeys)), m_name(name), m_subdescriptor_args(std::move(scripts)) {}

    enum class StringType
    {
        PUBLIC,
        PRIVATE,
        NORMALIZED,
        COMPAT, // string calculation that mustn't change over time to stay compatible with previous software versions
    };

    // NOLINTNEXTLINE(misc-no-recursion)
    bool IsSolvable() const override
    {
        for (const auto& arg : m_subdescriptor_args) {
            if (!arg->IsSolvable()) return false;
        }
        return true;
    }

    // NOLINTNEXTLINE(misc-no-recursion)
    bool IsRange() const final
    {
        for (const auto& pubkey : m_pubkey_args) {
            if (pubkey->IsRange()) return true;
        }
        for (const auto& arg : m_subdescriptor_args) {
            if (arg->IsRange()) return true;
        }
        return false;
    }

    // NOLINTNEXTLINE(misc-no-recursion)
    virtual bool ToStringSubScriptHelper(const SigningProvider* arg, std::string& ret, const StringType type, const DescriptorCache* cache = nullptr) const
    {
        size_t pos = 0;
        for (const auto& scriptarg : m_subdescriptor_args) {
            if (pos++) ret += ",";
            std::string tmp;
            if (!scriptarg->ToStringHelper(arg, tmp, type, cache)) return false;
            ret += tmp;
        }
        return true;
    }

    // NOLINTNEXTLINE(misc-no-recursion)
    virtual bool ToStringHelper(const SigningProvider* arg, std::string& out, const StringType type, const DescriptorCache* cache = nullptr) const
    {
        std::string extra = ToStringExtra();
        size_t pos = extra.size() > 0 ? 1 : 0;
        std::string ret = m_name + "(" + extra;
        for (const auto& pubkey : m_pubkey_args) {
            if (pos++) ret += ",";
            std::string tmp;
            switch (type) {
                case StringType::NORMALIZED:
                    if (!pubkey->ToNormalizedString(*arg, tmp, cache)) return false;
                    break;
                case StringType::PRIVATE:
                    if (!pubkey->ToPrivateString(*arg, tmp)) return false;
                    break;
                case StringType::PUBLIC:
                    tmp = pubkey->ToString();
                    break;
                case StringType::COMPAT:
                    tmp = pubkey->ToString(PubkeyProvider::StringType::COMPAT);
                    break;
            }
            ret += tmp;
        }
        std::string subscript;
        if (!ToStringSubScriptHelper(arg, subscript, type, cache)) return false;
        if (pos && subscript.size()) ret += ',';
        out = std::move(ret) + std::move(subscript) + ")";
        return true;
    }

    std::string ToString(bool compat_format) const final
    {
        std::string ret;
        ToStringHelper(nullptr, ret, compat_format ? StringType::COMPAT : StringType::PUBLIC);
        return AddChecksum(ret);
    }

    bool ToPrivateString(const SigningProvider& arg, std::string& out) const override
    {
        bool ret = ToStringHelper(&arg, out, StringType::PRIVATE);
        out = AddChecksum(out);
        return ret;
    }

    bool ToNormalizedString(const SigningProvider& arg, std::string& out, const DescriptorCache* cache) const override final
    {
        bool ret = ToStringHelper(&arg, out, StringType::NORMALIZED, cache);
        out = AddChecksum(out);
        return ret;
    }

    // NOLINTNEXTLINE(misc-no-recursion)
    bool ExpandHelper(int pos, const SigningProvider& arg, const DescriptorCache* read_cache, std::vector<CScript>& output_scripts, FlatSigningProvider& out, DescriptorCache* write_cache) const
    {
        FlatSigningProvider subprovider;
        std::vector<CPubKey> pubkeys;
        pubkeys.reserve(m_pubkey_args.size());

        // Construct temporary data in `pubkeys`, `subscripts`, and `subprovider` to avoid producing output in case of failure.
        for (const auto& p : m_pubkey_args) {
            std::optional<CPubKey> pubkey = p->GetPubKey(pos, arg, subprovider, read_cache, write_cache);
            if (!pubkey) return false;
            pubkeys.push_back(pubkey.value());
        }
        std::vector<CScript> subscripts;
        for (const auto& subarg : m_subdescriptor_args) {
            std::vector<CScript> outscripts;
            if (!subarg->ExpandHelper(pos, arg, read_cache, outscripts, subprovider, write_cache)) return false;
            assert(outscripts.size() == 1);
            subscripts.emplace_back(std::move(outscripts[0]));
        }
        out.Merge(std::move(subprovider));

        output_scripts = MakeScripts(pubkeys, std::span{subscripts}, out);
        return true;
    }

    bool Expand(int pos, const SigningProvider& provider, std::vector<CScript>& output_scripts, FlatSigningProvider& out, DescriptorCache* write_cache = nullptr) const final
    {
        return ExpandHelper(pos, provider, nullptr, output_scripts, out, write_cache);
    }

    bool ExpandFromCache(int pos, const DescriptorCache& read_cache, std::vector<CScript>& output_scripts, FlatSigningProvider& out) const final
    {
        return ExpandHelper(pos, DUMMY_SIGNING_PROVIDER, &read_cache, output_scripts, out, nullptr);
    }

    // NOLINTNEXTLINE(misc-no-recursion)
    void ExpandPrivate(int pos, const SigningProvider& provider, FlatSigningProvider& out) const final
    {
        for (const auto& p : m_pubkey_args) {
            p->GetPrivKey(pos, provider, out);
        }
        for (const auto& arg : m_subdescriptor_args) {
            arg->ExpandPrivate(pos, provider, out);
        }
    }

    std::optional<OutputType> GetOutputType() const override { return std::nullopt; }

    std::optional<int64_t> ScriptSize() const override { return {}; }

    /** A helper for MaxSatisfactionWeight.
     *
     * @param use_max_sig Whether to assume ECDSA signatures will have a high-r.
     * @return The maximum size of the satisfaction in raw bytes (with no witness meaning).
     */
    virtual std::optional<int64_t> MaxSatSize(bool use_max_sig) const { return {}; }

    std::optional<int64_t> MaxSatisfactionWeight(bool) const override { return {}; }

    std::optional<int64_t> MaxSatisfactionElems() const override { return {}; }

    // NOLINTNEXTLINE(misc-no-recursion)
    void GetPubKeys(std::set<CPubKey>& pubkeys, std::set<CExtPubKey>& ext_pubs) const override
    {
        for (const auto& p : m_pubkey_args) {
            std::optional<CPubKey> pub = p->GetRootPubKey();
            if (pub) pubkeys.insert(*pub);
            std::optional<CExtPubKey> ext_pub = p->GetRootExtPubKey();
            if (ext_pub) ext_pubs.insert(*ext_pub);
        }
        for (const auto& arg : m_subdescriptor_args) {
            arg->GetPubKeys(pubkeys, ext_pubs);
        }
    }

    virtual std::unique_ptr<DescriptorImpl> Clone() const = 0;
};

/** A parsed addr(A) descriptor. */
class AddressDescriptor final : public DescriptorImpl
{
    const CTxDestination m_destination;
protected:
    std::string ToStringExtra() const override { return EncodeDestination(m_destination); }
    std::vector<CScript> MakeScripts(const std::vector<CPubKey>&, std::span<const CScript>, FlatSigningProvider&) const override { return Vector(GetScriptForDestination(m_destination)); }
public:
    AddressDescriptor(CTxDestination destination) : DescriptorImpl({}, "addr"), m_destination(std::move(destination)) {}
    bool IsSolvable() const final { return false; }

    std::optional<OutputType> GetOutputType() const override
    {
        return OutputTypeFromDestination(m_destination);
    }
    bool IsSingleType() const final { return true; }
    bool ToPrivateString(const SigningProvider& arg, std::string& out) const final { return false; }

    std::optional<int64_t> ScriptSize() const override { return GetScriptForDestination(m_destination).size(); }
    std::unique_ptr<DescriptorImpl> Clone() const override
    {
        return std::make_unique<AddressDescriptor>(m_destination);
    }
};

/** A parsed raw(H) descriptor. */
class RawDescriptor final : public DescriptorImpl
{
    const CScript m_script;
protected:
    std::string ToStringExtra() const override { return HexStr(m_script); }
    std::vector<CScript> MakeScripts(const std::vector<CPubKey>&, std::span<const CScript>, FlatSigningProvider&) const override { return Vector(m_script); }
public:
    RawDescriptor(CScript script) : DescriptorImpl({}, "raw"), m_script(std::move(script)) {}
    bool IsSolvable() const final { return false; }

    std::optional<OutputType> GetOutputType() const override
    {
        CTxDestination dest;
        ExtractDestination(m_script, dest);
        return OutputTypeFromDestination(dest);
    }
    bool IsSingleType() const final { return true; }
    bool ToPrivateString(const SigningProvider& arg, std::string& out) const final { return false; }

    std::optional<int64_t> ScriptSize() const override { return m_script.size(); }

    std::unique_ptr<DescriptorImpl> Clone() const override
    {
        return std::make_unique<RawDescriptor>(m_script);
    }
};

/** A parsed pk(P) descriptor. */
class PKDescriptor final : public DescriptorImpl
{
private:
    const bool m_xonly;
protected:
    std::vector<CScript> MakeScripts(const std::vector<CPubKey>& keys, std::span<const CScript>, FlatSigningProvider&) const override
    {
        if (m_xonly) {
            CScript script = CScript() << ToByteVector(XOnlyPubKey(keys[0])) << OP_CHECKSIG;
            return Vector(std::move(script));
        } else {
            return Vector(GetScriptForRawPubKey(keys[0]));
        }
    }
public:
    PKDescriptor(std::unique_ptr<PubkeyProvider> prov, bool xonly = false) : DescriptorImpl(Vector(std::move(prov)), "pk"), m_xonly(xonly) {}
    bool IsSingleType() const final { return true; }

    std::optional<int64_t> ScriptSize() const override {
        return 1 + (m_xonly ? 32 : m_pubkey_args[0]->GetSize()) + 1;
    }

    std::optional<int64_t> MaxSatSize(bool use_max_sig) const override {
        const auto ecdsa_sig_size = use_max_sig ? 72 : 71;
        return 1 + (m_xonly ? 65 : ecdsa_sig_size);
    }

    std::optional<int64_t> MaxSatisfactionWeight(bool use_max_sig) const override {
        return *MaxSatSize(use_max_sig) * WITNESS_SCALE_FACTOR;
    }

    std::optional<int64_t> MaxSatisfactionElems() const override { return 1; }

    std::unique_ptr<DescriptorImpl> Clone() const override
    {
        return std::make_unique<PKDescriptor>(m_pubkey_args.at(0)->Clone(), m_xonly);
    }
};

/** A parsed pkh(P) descriptor. */
class PKHDescriptor final : public DescriptorImpl
{
protected:
    std::vector<CScript> MakeScripts(const std::vector<CPubKey>& keys, std::span<const CScript>, FlatSigningProvider&) const override
    {
        CKeyID id = keys[0].GetID();
        return Vector(GetScriptForDestination(PKHash(id)));
    }
public:
    PKHDescriptor(std::unique_ptr<PubkeyProvider> prov) : DescriptorImpl(Vector(std::move(prov)), "pkh") {}
    std::optional<OutputType> GetOutputType() const override { return OutputType::LEGACY; }
    bool IsSingleType() const final { return true; }

    std::optional<int64_t> ScriptSize() const override { return 1 + 1 + 1 + 20 + 1 + 1; }

    std::optional<int64_t> MaxSatSize(bool use_max_sig) const override {
        const auto sig_size = use_max_sig ? 72 : 71;
        return 1 + sig_size + 1 + m_pubkey_args[0]->GetSize();
    }

    std::optional<int64_t> MaxSatisfactionWeight(bool use_max_sig) const override {
        return *MaxSatSize(use_max_sig) * WITNESS_SCALE_FACTOR;
    }

    std::optional<int64_t> MaxSatisfactionElems() const override { return 2; }

    std::unique_ptr<DescriptorImpl> Clone() const override
    {
        return std::make_unique<PKHDescriptor>(m_pubkey_args.at(0)->Clone());
    }
};

/** A parsed wpkh(P) descriptor. */
class WPKHDescriptor final : public DescriptorImpl
{
protected:
    std::vector<CScript> MakeScripts(const std::vector<CPubKey>& keys, std::span<const CScript>, FlatSigningProvider&) const override
    {
        CKeyID id = keys[0].GetID();
        return Vector(GetScriptForDestination(WitnessV0KeyHash(id)));
    }
public:
    WPKHDescriptor(std::unique_ptr<PubkeyProvider> prov) : DescriptorImpl(Vector(std::move(prov)), "wpkh") {}
    std::optional<OutputType> GetOutputType() const override { return OutputType::BECH32; }
    bool IsSingleType() const final { return true; }

    std::optional<int64_t> ScriptSize() const override { return 1 + 1 + 20; }

    std::optional<int64_t> MaxSatSize(bool use_max_sig) const override {
        const auto sig_size = use_max_sig ? 72 : 71;
        return (1 + sig_size + 1 + 33);
    }

    std::optional<int64_t> MaxSatisfactionWeight(bool use_max_sig) const override {
        return MaxSatSize(use_max_sig);
    }

    std::optional<int64_t> MaxSatisfactionElems() const override { return 2; }

    std::unique_ptr<DescriptorImpl> Clone() const override
    {
        return std::make_unique<WPKHDescriptor>(m_pubkey_args.at(0)->Clone());
    }
};

/** A parsed combo(P) descriptor. */
class ComboDescriptor final : public DescriptorImpl
{
protected:
    std::vector<CScript> MakeScripts(const std::vector<CPubKey>& keys, std::span<const CScript>, FlatSigningProvider& out) const override
    {
        std::vector<CScript> ret;
        CKeyID id = keys[0].GetID();
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
    ComboDescriptor(std::unique_ptr<PubkeyProvider> prov) : DescriptorImpl(Vector(std::move(prov)), "combo") {}
    bool IsSingleType() const final { return false; }
    std::unique_ptr<DescriptorImpl> Clone() const override
    {
        return std::make_unique<ComboDescriptor>(m_pubkey_args.at(0)->Clone());
    }
};

/** A parsed multi(...) or sortedmulti(...) descriptor */
class MultisigDescriptor final : public DescriptorImpl
{
    const int m_threshold;
    const bool m_sorted;
protected:
    std::string ToStringExtra() const override { return strprintf("%i", m_threshold); }
    std::vector<CScript> MakeScripts(const std::vector<CPubKey>& keys, std::span<const CScript>, FlatSigningProvider&) const override {
        if (m_sorted) {
            std::vector<CPubKey> sorted_keys(keys);
            std::sort(sorted_keys.begin(), sorted_keys.end());
            return Vector(GetScriptForMultisig(m_threshold, sorted_keys));
        }
        return Vector(GetScriptForMultisig(m_threshold, keys));
    }
public:
    MultisigDescriptor(int threshold, std::vector<std::unique_ptr<PubkeyProvider>> providers, bool sorted = false) : DescriptorImpl(std::move(providers), sorted ? "sortedmulti" : "multi"), m_threshold(threshold), m_sorted(sorted) {}
    bool IsSingleType() const final { return true; }

    std::optional<int64_t> ScriptSize() const override {
        const auto n_keys = m_pubkey_args.size();
        auto op = [](int64_t acc, const std::unique_ptr<PubkeyProvider>& pk) { return acc + 1 + pk->GetSize();};
        const auto pubkeys_size{std::accumulate(m_pubkey_args.begin(), m_pubkey_args.end(), int64_t{0}, op)};
        return 1 + BuildScript(n_keys).size() + BuildScript(m_threshold).size() + pubkeys_size;
    }

    std::optional<int64_t> MaxSatSize(bool use_max_sig) const override {
        const auto sig_size = use_max_sig ? 72 : 71;
        return (1 + (1 + sig_size) * m_threshold);
    }

    std::optional<int64_t> MaxSatisfactionWeight(bool use_max_sig) const override {
        return *MaxSatSize(use_max_sig) * WITNESS_SCALE_FACTOR;
    }

    std::optional<int64_t> MaxSatisfactionElems() const override { return 1 + m_threshold; }

    std::unique_ptr<DescriptorImpl> Clone() const override
    {
        std::vector<std::unique_ptr<PubkeyProvider>> providers;
        providers.reserve(m_pubkey_args.size());
        std::transform(m_pubkey_args.begin(), m_pubkey_args.end(), providers.begin(), [](const std::unique_ptr<PubkeyProvider>& p) { return p->Clone(); });
        return std::make_unique<MultisigDescriptor>(m_threshold, std::move(providers), m_sorted);
    }
};

/** A parsed (sorted)multi_a(...) descriptor. Always uses x-only pubkeys. */
class MultiADescriptor final : public DescriptorImpl
{
    const int m_threshold;
    const bool m_sorted;
protected:
    std::string ToStringExtra() const override { return strprintf("%i", m_threshold); }
    std::vector<CScript> MakeScripts(const std::vector<CPubKey>& keys, std::span<const CScript>, FlatSigningProvider&) const override {
        CScript ret;
        std::vector<XOnlyPubKey> xkeys;
        xkeys.reserve(keys.size());
        for (const auto& key : keys) xkeys.emplace_back(key);
        if (m_sorted) std::sort(xkeys.begin(), xkeys.end());
        ret << ToByteVector(xkeys[0]) << OP_CHECKSIG;
        for (size_t i = 1; i < keys.size(); ++i) {
            ret << ToByteVector(xkeys[i]) << OP_CHECKSIGADD;
        }
        ret << m_threshold << OP_NUMEQUAL;
        return Vector(std::move(ret));
    }
public:
    MultiADescriptor(int threshold, std::vector<std::unique_ptr<PubkeyProvider>> providers, bool sorted = false) : DescriptorImpl(std::move(providers), sorted ? "sortedmulti_a" : "multi_a"), m_threshold(threshold), m_sorted(sorted) {}
    bool IsSingleType() const final { return true; }

    std::optional<int64_t> ScriptSize() const override {
        const auto n_keys = m_pubkey_args.size();
        return (1 + 32 + 1) * n_keys + BuildScript(m_threshold).size() + 1;
    }

    std::optional<int64_t> MaxSatSize(bool use_max_sig) const override {
        return (1 + 65) * m_threshold + (m_pubkey_args.size() - m_threshold);
    }

    std::optional<int64_t> MaxSatisfactionElems() const override { return m_pubkey_args.size(); }

    std::unique_ptr<DescriptorImpl> Clone() const override
    {
        std::vector<std::unique_ptr<PubkeyProvider>> providers;
        providers.reserve(m_pubkey_args.size());
        for (const auto& arg : m_pubkey_args) {
            providers.push_back(arg->Clone());
        }
        return std::make_unique<MultiADescriptor>(m_threshold, std::move(providers), m_sorted);
    }
};

/** A parsed sh(...) descriptor. */
class SHDescriptor final : public DescriptorImpl
{
protected:
    std::vector<CScript> MakeScripts(const std::vector<CPubKey>&, std::span<const CScript> scripts, FlatSigningProvider& out) const override
    {
        auto ret = Vector(GetScriptForDestination(ScriptHash(scripts[0])));
        if (ret.size()) out.scripts.emplace(CScriptID(scripts[0]), scripts[0]);
        return ret;
    }

    bool IsSegwit() const { return m_subdescriptor_args[0]->GetOutputType() == OutputType::BECH32; }

public:
    SHDescriptor(std::unique_ptr<DescriptorImpl> desc) : DescriptorImpl({}, std::move(desc), "sh") {}

    std::optional<OutputType> GetOutputType() const override
    {
        assert(m_subdescriptor_args.size() == 1);
        if (IsSegwit()) return OutputType::P2SH_SEGWIT;
        return OutputType::LEGACY;
    }
    bool IsSingleType() const final { return true; }

    std::optional<int64_t> ScriptSize() const override { return 1 + 1 + 20 + 1; }

    std::optional<int64_t> MaxSatisfactionWeight(bool use_max_sig) const override {
        if (const auto sat_size = m_subdescriptor_args[0]->MaxSatSize(use_max_sig)) {
            if (const auto subscript_size = m_subdescriptor_args[0]->ScriptSize()) {
                // The subscript is never witness data.
                const auto subscript_weight = (1 + *subscript_size) * WITNESS_SCALE_FACTOR;
                // The weight depends on whether the inner descriptor is satisfied using the witness stack.
                if (IsSegwit()) return subscript_weight + *sat_size;
                return subscript_weight + *sat_size * WITNESS_SCALE_FACTOR;
            }
        }
        return {};
    }

    std::optional<int64_t> MaxSatisfactionElems() const override {
        if (const auto sub_elems = m_subdescriptor_args[0]->MaxSatisfactionElems()) return 1 + *sub_elems;
        return {};
    }

    std::unique_ptr<DescriptorImpl> Clone() const override
    {
        return std::make_unique<SHDescriptor>(m_subdescriptor_args.at(0)->Clone());
    }
};

/** A parsed wsh(...) descriptor. */
class WSHDescriptor final : public DescriptorImpl
{
protected:
    std::vector<CScript> MakeScripts(const std::vector<CPubKey>&, std::span<const CScript> scripts, FlatSigningProvider& out) const override
    {
        auto ret = Vector(GetScriptForDestination(WitnessV0ScriptHash(scripts[0])));
        if (ret.size()) out.scripts.emplace(CScriptID(scripts[0]), scripts[0]);
        return ret;
    }
public:
    WSHDescriptor(std::unique_ptr<DescriptorImpl> desc) : DescriptorImpl({}, std::move(desc), "wsh") {}
    std::optional<OutputType> GetOutputType() const override { return OutputType::BECH32; }
    bool IsSingleType() const final { return true; }

    std::optional<int64_t> ScriptSize() const override { return 1 + 1 + 32; }

    std::optional<int64_t> MaxSatSize(bool use_max_sig) const override {
        if (const auto sat_size = m_subdescriptor_args[0]->MaxSatSize(use_max_sig)) {
            if (const auto subscript_size = m_subdescriptor_args[0]->ScriptSize()) {
                return GetSizeOfCompactSize(*subscript_size) + *subscript_size + *sat_size;
            }
        }
        return {};
    }

    std::optional<int64_t> MaxSatisfactionWeight(bool use_max_sig) const override {
        return MaxSatSize(use_max_sig);
    }

    std::optional<int64_t> MaxSatisfactionElems() const override {
        if (const auto sub_elems = m_subdescriptor_args[0]->MaxSatisfactionElems()) return 1 + *sub_elems;
        return {};
    }

    std::unique_ptr<DescriptorImpl> Clone() const override
    {
        return std::make_unique<WSHDescriptor>(m_subdescriptor_args.at(0)->Clone());
    }
};

/** A parsed tr(...) descriptor. */
class TRDescriptor final : public DescriptorImpl
{
    std::vector<int> m_depths;
protected:
    std::vector<CScript> MakeScripts(const std::vector<CPubKey>& keys, std::span<const CScript> scripts, FlatSigningProvider& out) const override
    {
        TaprootBuilder builder;
        assert(m_depths.size() == scripts.size());
        for (size_t pos = 0; pos < m_depths.size(); ++pos) {
            builder.Add(m_depths[pos], scripts[pos], TAPROOT_LEAF_TAPSCRIPT);
        }
        if (!builder.IsComplete()) return {};
        assert(keys.size() == 1);
        XOnlyPubKey xpk(keys[0]);
        if (!xpk.IsFullyValid()) return {};
        builder.Finalize(xpk);
        WitnessV1Taproot output = builder.GetOutput();
        out.tr_trees[output] = builder;
        return Vector(GetScriptForDestination(output));
    }
    bool ToStringSubScriptHelper(const SigningProvider* arg, std::string& ret, const StringType type, const DescriptorCache* cache = nullptr) const override
    {
        if (m_depths.empty()) return true;
        std::vector<bool> path;
        for (size_t pos = 0; pos < m_depths.size(); ++pos) {
            if (pos) ret += ',';
            while ((int)path.size() <= m_depths[pos]) {
                if (path.size()) ret += '{';
                path.push_back(false);
            }
            std::string tmp;
            if (!m_subdescriptor_args[pos]->ToStringHelper(arg, tmp, type, cache)) return false;
            ret += tmp;
            while (!path.empty() && path.back()) {
                if (path.size() > 1) ret += '}';
                path.pop_back();
            }
            if (!path.empty()) path.back() = true;
        }
        return true;
    }
public:
    TRDescriptor(std::unique_ptr<PubkeyProvider> internal_key, std::vector<std::unique_ptr<DescriptorImpl>> descs, std::vector<int> depths) :
        DescriptorImpl(Vector(std::move(internal_key)), std::move(descs), "tr"), m_depths(std::move(depths))
    {
        assert(m_subdescriptor_args.size() == m_depths.size());
    }
    std::optional<OutputType> GetOutputType() const override { return OutputType::BECH32M; }
    bool IsSingleType() const final { return true; }

    std::optional<int64_t> ScriptSize() const override { return 1 + 1 + 32; }

    std::optional<int64_t> MaxSatisfactionWeight(bool) const override {
        // FIXME: We assume keypath spend, which can lead to very large underestimations.
        return 1 + 65;
    }

    std::optional<int64_t> MaxSatisfactionElems() const override {
        // FIXME: See above, we assume keypath spend.
        return 1;
    }

    std::unique_ptr<DescriptorImpl> Clone() const override
    {
        std::vector<std::unique_ptr<DescriptorImpl>> subdescs;
        subdescs.reserve(m_subdescriptor_args.size());
        std::transform(m_subdescriptor_args.begin(), m_subdescriptor_args.end(), subdescs.begin(), [](const std::unique_ptr<DescriptorImpl>& d) { return d->Clone(); });
        return std::make_unique<TRDescriptor>(m_pubkey_args.at(0)->Clone(), std::move(subdescs), m_depths);
    }
};

/* We instantiate Miniscript here with a simple integer as key type.
 * The value of these key integers are an index in the
 * DescriptorImpl::m_pubkey_args vector.
 */

/**
 * The context for converting a Miniscript descriptor into a Script.
 */
class ScriptMaker {
    //! Keys contained in the Miniscript (the evaluation of DescriptorImpl::m_pubkey_args).
    const std::vector<CPubKey>& m_keys;
    //! The script context we're operating within (Tapscript or P2WSH).
    const miniscript::MiniscriptContext m_script_ctx;

    //! Get the ripemd160(sha256()) hash of this key.
    //! Any key that is valid in a descriptor serializes as 32 bytes within a Tapscript context. So we
    //! must not hash the sign-bit byte in this case.
    uint160 GetHash160(uint32_t key) const {
        if (miniscript::IsTapscript(m_script_ctx)) {
            return Hash160(XOnlyPubKey{m_keys[key]});
        }
        return m_keys[key].GetID();
    }

public:
    ScriptMaker(const std::vector<CPubKey>& keys LIFETIMEBOUND, const miniscript::MiniscriptContext script_ctx) : m_keys(keys), m_script_ctx{script_ctx} {}

    std::vector<unsigned char> ToPKBytes(uint32_t key) const {
        // In Tapscript keys always serialize as x-only, whether an x-only key was used in the descriptor or not.
        if (!miniscript::IsTapscript(m_script_ctx)) {
            return {m_keys[key].begin(), m_keys[key].end()};
        }
        const XOnlyPubKey xonly_pubkey{m_keys[key]};
        return {xonly_pubkey.begin(), xonly_pubkey.end()};
    }

    std::vector<unsigned char> ToPKHBytes(uint32_t key) const {
        auto id = GetHash160(key);
        return {id.begin(), id.end()};
    }
};

/**
 * The context for converting a Miniscript descriptor to its textual form.
 */
class StringMaker {
    //! To convert private keys for private descriptors.
    const SigningProvider* m_arg;
    //! Keys contained in the Miniscript (a reference to DescriptorImpl::m_pubkey_args).
    const std::vector<std::unique_ptr<PubkeyProvider>>& m_pubkeys;
    //! StringType to serialize keys
    const DescriptorImpl::StringType m_type;
    const DescriptorCache* m_cache;

public:
    StringMaker(const SigningProvider* arg LIFETIMEBOUND,
                const std::vector<std::unique_ptr<PubkeyProvider>>& pubkeys LIFETIMEBOUND,
                DescriptorImpl::StringType type,
                const DescriptorCache* cache LIFETIMEBOUND)
        : m_arg(arg), m_pubkeys(pubkeys), m_type(type), m_cache(cache) {}

    std::optional<std::string> ToString(uint32_t key) const
    {
        std::string ret;
        switch (m_type) {
        case DescriptorImpl::StringType::PUBLIC:
            ret = m_pubkeys[key]->ToString();
            break;
        case DescriptorImpl::StringType::PRIVATE:
            if (!m_pubkeys[key]->ToPrivateString(*m_arg, ret)) return {};
            break;
        case DescriptorImpl::StringType::NORMALIZED:
            if (!m_pubkeys[key]->ToNormalizedString(*m_arg, ret, m_cache)) return {};
            break;
        case DescriptorImpl::StringType::COMPAT:
            ret = m_pubkeys[key]->ToString(PubkeyProvider::StringType::COMPAT);
            break;
        }
        return ret;
    }
};

class MiniscriptDescriptor final : public DescriptorImpl
{
private:
    miniscript::NodeRef<uint32_t> m_node;

protected:
    std::vector<CScript> MakeScripts(const std::vector<CPubKey>& keys, std::span<const CScript> scripts,
                                     FlatSigningProvider& provider) const override
    {
        const auto script_ctx{m_node->GetMsCtx()};
        for (const auto& key : keys) {
            if (miniscript::IsTapscript(script_ctx)) {
                provider.pubkeys.emplace(Hash160(XOnlyPubKey{key}), key);
            } else {
                provider.pubkeys.emplace(key.GetID(), key);
            }
        }
        return Vector(m_node->ToScript(ScriptMaker(keys, script_ctx)));
    }

public:
    MiniscriptDescriptor(std::vector<std::unique_ptr<PubkeyProvider>> providers, miniscript::NodeRef<uint32_t> node)
        : DescriptorImpl(std::move(providers), "?"), m_node(std::move(node)) {}

    bool ToStringHelper(const SigningProvider* arg, std::string& out, const StringType type,
                        const DescriptorCache* cache = nullptr) const override
    {
        if (const auto res = m_node->ToString(StringMaker(arg, m_pubkey_args, type, cache))) {
            out = *res;
            return true;
        }
        return false;
    }

    bool IsSolvable() const override { return true; }
    bool IsSingleType() const final { return true; }

    std::optional<int64_t> ScriptSize() const override { return m_node->ScriptSize(); }

    std::optional<int64_t> MaxSatSize(bool) const override {
        // For Miniscript we always assume high-R ECDSA signatures.
        return m_node->GetWitnessSize();
    }

    std::optional<int64_t> MaxSatisfactionElems() const override {
        return m_node->GetStackSize();
    }

    std::unique_ptr<DescriptorImpl> Clone() const override
    {
        std::vector<std::unique_ptr<PubkeyProvider>> providers;
        providers.reserve(m_pubkey_args.size());
        for (const auto& arg : m_pubkey_args) {
            providers.push_back(arg->Clone());
        }
        return std::make_unique<MiniscriptDescriptor>(std::move(providers), m_node->Clone());
    }
};

/** A parsed rawtr(...) descriptor. */
class RawTRDescriptor final : public DescriptorImpl
{
protected:
    std::vector<CScript> MakeScripts(const std::vector<CPubKey>& keys, std::span<const CScript> scripts, FlatSigningProvider& out) const override
    {
        assert(keys.size() == 1);
        XOnlyPubKey xpk(keys[0]);
        if (!xpk.IsFullyValid()) return {};
        WitnessV1Taproot output{xpk};
        return Vector(GetScriptForDestination(output));
    }
public:
    RawTRDescriptor(std::unique_ptr<PubkeyProvider> output_key) : DescriptorImpl(Vector(std::move(output_key)), "rawtr") {}
    std::optional<OutputType> GetOutputType() const override { return OutputType::BECH32M; }
    bool IsSingleType() const final { return true; }

    std::optional<int64_t> ScriptSize() const override { return 1 + 1 + 32; }

    std::optional<int64_t> MaxSatisfactionWeight(bool) const override {
        // We can't know whether there is a script path, so assume key path spend.
        return 1 + 65;
    }

    std::optional<int64_t> MaxSatisfactionElems() const override {
        // See above, we assume keypath spend.
        return 1;
    }

    std::unique_ptr<DescriptorImpl> Clone() const override
    {
        return std::make_unique<RawTRDescriptor>(m_pubkey_args.at(0)->Clone());
    }
};

////////////////////////////////////////////////////////////////////////////
// Parser                                                                 //
////////////////////////////////////////////////////////////////////////////

enum class ParseScriptContext {
    TOP,     //!< Top-level context (script goes directly in scriptPubKey)
    P2SH,    //!< Inside sh() (script becomes P2SH redeemScript)
    P2WPKH,  //!< Inside wpkh() (no script, pubkey only)
    P2WSH,   //!< Inside wsh() (script becomes v0 witness script)
    P2TR,    //!< Inside tr() (either internal key, or BIP342 script leaf)
    MUSIG,   //!< Inside musig() (implies P2TR, cannot have nested musig())
};

std::optional<uint32_t> ParseKeyPathNum(std::span<const char> elem, bool& apostrophe, std::string& error, bool& has_hardened)
{
    bool hardened = false;
    if (elem.size() > 0) {
        const char last = elem[elem.size() - 1];
        if (last == '\'' || last == 'h') {
            elem = elem.first(elem.size() - 1);
            hardened = true;
            apostrophe = last == '\'';
        }
    }
    const auto p{ToIntegral<uint32_t>(std::string_view{elem.begin(), elem.end()})};
    if (!p) {
        error = strprintf("Key path value '%s' is not a valid uint32", std::string_view{elem.begin(), elem.end()});
        return std::nullopt;
    } else if (*p > 0x7FFFFFFFUL) {
        error = strprintf("Key path value %u is out of range", *p);
        return std::nullopt;
    }
    has_hardened = has_hardened || hardened;

    return std::make_optional<uint32_t>(*p | (((uint32_t)hardened) << 31));
}

/**
 * Parse a key path, being passed a split list of elements (the first element is ignored because it is always the key).
 *
 * @param[in] split BIP32 path string, using either ' or h for hardened derivation
 * @param[out] out Vector of parsed key paths
 * @param[out] apostrophe only updated if hardened derivation is found
 * @param[out] error parsing error message
 * @param[in] allow_multipath Allows the parsed path to use the multipath specifier
 * @param[out] has_hardened Records whether the path contains any hardened derivation
 * @returns false if parsing failed
 **/
[[nodiscard]] bool ParseKeyPath(const std::vector<std::span<const char>>& split, std::vector<KeyPath>& out, bool& apostrophe, std::string& error, bool allow_multipath, bool& has_hardened)
{
    KeyPath path;
    struct MultipathSubstitutes {
        size_t placeholder_index;
        std::vector<uint32_t> values;
    };
    std::optional<MultipathSubstitutes> substitutes;
    has_hardened = false;

    for (size_t i = 1; i < split.size(); ++i) {
        const std::span<const char>& elem = split[i];

        // Check if element contains multipath specifier
        if (!elem.empty() && elem.front() == '<' && elem.back() == '>') {
            if (!allow_multipath) {
                error = strprintf("Key path value '%s' specifies multipath in a section where multipath is not allowed", std::string(elem.begin(), elem.end()));
                return false;
            }
            if (substitutes) {
                error = "Multiple multipath key path specifiers found";
                return false;
            }

            // Parse each possible value
            std::vector<std::span<const char>> nums = Split(std::span(elem.begin()+1, elem.end()-1), ";");
            if (nums.size() < 2) {
                error = "Multipath key path specifiers must have at least two items";
                return false;
            }

            substitutes.emplace();
            std::unordered_set<uint32_t> seen_substitutes;
            for (const auto& num : nums) {
                const auto& op_num = ParseKeyPathNum(num, apostrophe, error, has_hardened);
                if (!op_num) return false;
                auto [_, inserted] = seen_substitutes.insert(*op_num);
                if (!inserted) {
                    error = strprintf("Duplicated key path value %u in multipath specifier", *op_num);
                    return false;
                }
                substitutes->values.emplace_back(*op_num);
            }

            path.emplace_back(); // Placeholder for multipath segment
            substitutes->placeholder_index = path.size() - 1;
        } else {
            const auto& op_num = ParseKeyPathNum(elem, apostrophe, error, has_hardened);
            if (!op_num) return false;
            path.emplace_back(*op_num);
        }
    }

    if (!substitutes) {
        out.emplace_back(std::move(path));
    } else {
        // Replace the multipath placeholder with each value while generating paths
        for (uint32_t substitute : substitutes->values) {
            KeyPath branch_path = path;
            branch_path[substitutes->placeholder_index] = substitute;
            out.emplace_back(std::move(branch_path));
        }
    }
    return true;
}

[[nodiscard]] bool ParseKeyPath(const std::vector<std::span<const char>>& split, std::vector<KeyPath>& out, bool& apostrophe, std::string& error, bool allow_multipath)
{
    bool dummy;
    return ParseKeyPath(split, out, apostrophe, error, allow_multipath, /*has_hardened=*/dummy);
}

static DeriveType ParseDeriveType(std::vector<std::span<const char>>& split, bool& apostrophe)
{
    DeriveType type = DeriveType::NO;
    if (std::ranges::equal(split.back(), std::span{"*"}.first(1))) {
        split.pop_back();
        type = DeriveType::UNHARDENED;
    } else if (std::ranges::equal(split.back(), std::span{"*'"}.first(2)) || std::ranges::equal(split.back(), std::span{"*h"}.first(2))) {
        apostrophe = std::ranges::equal(split.back(), std::span{"*'"}.first(2));
        split.pop_back();
        type = DeriveType::HARDENED;
    }
    return type;
}

/** Parse a public key that excludes origin information. */
std::vector<std::unique_ptr<PubkeyProvider>> ParsePubkeyInner(uint32_t key_exp_index, const std::span<const char>& sp, ParseScriptContext ctx, FlatSigningProvider& out, bool& apostrophe, std::string& error)
{
    std::vector<std::unique_ptr<PubkeyProvider>> ret;
    bool permit_uncompressed = ctx == ParseScriptContext::TOP || ctx == ParseScriptContext::P2SH;
    auto split = Split(sp, '/');
    std::string str(split[0].begin(), split[0].end());
    if (str.size() == 0) {
        error = "No key provided";
        return {};
    }
    if (IsSpace(str.front()) || IsSpace(str.back())) {
        error = strprintf("Key '%s' is invalid due to whitespace", str);
        return {};
    }
    if (split.size() == 1) {
        if (IsHex(str)) {
            std::vector<unsigned char> data = ParseHex(str);
            CPubKey pubkey(data);
            if (pubkey.IsValid() && !pubkey.IsValidNonHybrid()) {
                error = "Hybrid public keys are not allowed";
                return {};
            }
            if (pubkey.IsFullyValid()) {
                if (permit_uncompressed || pubkey.IsCompressed()) {
                    ret.emplace_back(std::make_unique<ConstPubkeyProvider>(key_exp_index, pubkey, false));
                    return ret;
                } else {
                    error = "Uncompressed keys are not allowed";
                    return {};
                }
            } else if (data.size() == 32 && ctx == ParseScriptContext::P2TR) {
                unsigned char fullkey[33] = {0x02};
                std::copy(data.begin(), data.end(), fullkey + 1);
                pubkey.Set(std::begin(fullkey), std::end(fullkey));
                if (pubkey.IsFullyValid()) {
                    ret.emplace_back(std::make_unique<ConstPubkeyProvider>(key_exp_index, pubkey, true));
                    return ret;
                }
            }
            error = strprintf("Pubkey '%s' is invalid", str);
            return {};
        }
        CKey key = DecodeSecret(str);
        if (key.IsValid()) {
            if (permit_uncompressed || key.IsCompressed()) {
                CPubKey pubkey = key.GetPubKey();
                out.keys.emplace(pubkey.GetID(), key);
                ret.emplace_back(std::make_unique<ConstPubkeyProvider>(key_exp_index, pubkey, ctx == ParseScriptContext::P2TR));
                return ret;
            } else {
                error = "Uncompressed keys are not allowed";
                return {};
            }
        }
    }
    CExtKey extkey = DecodeExtKey(str);
    CExtPubKey extpubkey = DecodeExtPubKey(str);
    if (!extkey.key.IsValid() && !extpubkey.pubkey.IsValid()) {
        error = strprintf("key '%s' is not valid", str);
        return {};
    }
    std::vector<KeyPath> paths;
    DeriveType type = ParseDeriveType(split, apostrophe);
    if (!ParseKeyPath(split, paths, apostrophe, error, /*allow_multipath=*/true)) return {};
    if (extkey.key.IsValid()) {
        extpubkey = extkey.Neuter();
        out.keys.emplace(extpubkey.pubkey.GetID(), extkey.key);
    }
    for (auto& path : paths) {
        ret.emplace_back(std::make_unique<BIP32PubkeyProvider>(key_exp_index, extpubkey, std::move(path), type, apostrophe));
    }
    return ret;
}

/** Parse a public key including origin information (if enabled). */
// NOLINTNEXTLINE(misc-no-recursion)
std::vector<std::unique_ptr<PubkeyProvider>> ParsePubkey(uint32_t& key_exp_index, const std::span<const char>& sp, ParseScriptContext ctx, FlatSigningProvider& out, std::string& error)
{
    std::vector<std::unique_ptr<PubkeyProvider>> ret;

    using namespace script;

    // musig cannot be nested inside of an origin
    std::span<const char> span = sp;
    if (Const("musig(", span, /*skip=*/false)) {
        if (ctx != ParseScriptContext::P2TR) {
            error = "musig() is only allowed in tr() and rawtr()";
            return {};
        }

        // Split the span on the end parentheses. The end parentheses must
        // be included in the resulting span so that Expr is happy.
        auto split = Split(sp, ')', /*include_sep=*/true);
        if (split.size() > 2) {
            error = "Too many ')' in musig() expression";
            return {};
        }
        std::span<const char> expr(split.at(0).begin(), split.at(0).end());
        if (!Func("musig", expr)) {
            error = "Invalid musig() expression";
            return {};
        }

        // Parse the participant pubkeys
        bool any_ranged = false;
        bool all_bip32 = true;
        std::vector<std::vector<std::unique_ptr<PubkeyProvider>>> providers;
        bool any_key_parsed = false;
        size_t max_multipath_len = 0;
        while (expr.size()) {
            if (any_key_parsed && !Const(",", expr)) {
                error = strprintf("musig(): expected ',', got '%c'", expr[0]);
                return {};
            }
            auto arg = Expr(expr);
            auto pk = ParsePubkey(key_exp_index, arg, ParseScriptContext::MUSIG, out, error);
            if (pk.empty()) {
                error = strprintf("musig(): %s", error);
                return {};
            }
            any_key_parsed = true;

            any_ranged = any_ranged || pk.at(0)->IsRange();
            all_bip32 = all_bip32 &&  pk.at(0)->IsBIP32();

            max_multipath_len = std::max(max_multipath_len, pk.size());

            providers.emplace_back(std::move(pk));
            key_exp_index++;
        }
        if (!any_key_parsed) {
            error = "musig(): Must contain key expressions";
            return {};
        }

        // Parse any derivation
        DeriveType deriv_type = DeriveType::NO;
        std::vector<KeyPath> derivation_multipaths;
        if (split.size() == 2 && Const("/", split.at(1), /*skip=*/false)) {
            if (!all_bip32) {
                error = "musig(): derivation requires all participants to be xpubs or xprvs";
                return {};
            }
            if (any_ranged) {
                error = "musig(): Cannot have ranged participant keys if musig() also has derivation";
                return {};
            }
            bool dummy = false;
            auto deriv_split = Split(split.at(1), '/');
            deriv_type = ParseDeriveType(deriv_split, dummy);
            if (deriv_type == DeriveType::HARDENED) {
                error = "musig(): Cannot have hardened child derivation";
                return {};
            }
            bool has_hardened = false;
            if (!ParseKeyPath(deriv_split, derivation_multipaths, dummy, error, /*allow_multipath=*/true, has_hardened)) {
                error = "musig(): " + error;
                return {};
            }
            if (has_hardened) {
                error = "musig(): cannot have hardened derivation steps";
                return {};
            }
        } else {
            derivation_multipaths.emplace_back();
        }

        // Makes sure that all providers vectors in providers are the given length, or exactly length 1
        // Length 1 vectors have the single provider cloned until it matches the given length.
        const auto& clone_providers = [&providers](size_t length) -> bool {
            for (auto& multipath_providers : providers) {
                if (multipath_providers.size() == 1) {
                    for (size_t i = 1; i < length; ++i) {
                        multipath_providers.emplace_back(multipath_providers.at(0)->Clone());
                    }
                } else if (multipath_providers.size() != length) {
                    return false;
                }
            }
            return true;
        };

        // Emplace the final MuSigPubkeyProvider into ret with the pubkey providers from the specified provider vectors index
        // and the path from the specified path index
        const auto& emplace_final_provider = [&ret, &key_exp_index, &deriv_type, &derivation_multipaths, &providers](size_t vec_idx, size_t path_idx) -> void {
            KeyPath& path = derivation_multipaths.at(path_idx);
            std::vector<std::unique_ptr<PubkeyProvider>> pubs;
            pubs.reserve(providers.size());
            for (auto& vec : providers) {
                pubs.emplace_back(std::move(vec.at(vec_idx)));
            }
            ret.emplace_back(std::make_unique<MuSigPubkeyProvider>(key_exp_index, std::move(pubs), path, deriv_type));
        };

        if (max_multipath_len > 1 && derivation_multipaths.size() > 1) {
            error = "musig(): Cannot have multipath participant keys if musig() is also multipath";
            return {};
        } else if (max_multipath_len > 1) {
            if (!clone_providers(max_multipath_len)) {
                error = strprintf("musig(): Multipath derivation paths have mismatched lengths");
                return {};
            }
            for (size_t i = 0; i < max_multipath_len; ++i) {
                // Final MuSigPubkeyProvider uses participant pubkey providers at each multipath position, and the first (and only) path
                emplace_final_provider(i, 0);
            }
        } else if (derivation_multipaths.size() > 1) {
            // All key provider vectors should be length 1. Clone them until they have the same length as paths
            if (!Assume(clone_providers(derivation_multipaths.size()))) {
                error = "musig(): Multipath derivation path with multipath participants is disallowed"; // This error is unreachable due to earlier check
                return {};
            }
            for (size_t i = 0; i < derivation_multipaths.size(); ++i) {
                // Final MuSigPubkeyProvider uses cloned participant pubkey providers, and the multipath derivation paths
                emplace_final_provider(i, i);
            }
        } else {
            // No multipath derivation, MuSigPubkeyProvider uses the first (and only) participant pubkey providers, and the first (and only) path
            emplace_final_provider(0, 0);
        }
        return ret;
    }

    auto origin_split = Split(sp, ']');
    if (origin_split.size() > 2) {
        error = "Multiple ']' characters found for a single pubkey";
        return {};
    }
    // This is set if either the origin or path suffix contains a hardened derivation.
    bool apostrophe = false;
    if (origin_split.size() == 1) {
        return ParsePubkeyInner(key_exp_index, origin_split[0], ctx, out, apostrophe, error);
    }
    if (origin_split[0].empty() || origin_split[0][0] != '[') {
        error = strprintf("Key origin start '[ character expected but not found, got '%c' instead",
                          origin_split[0].empty() ? /** empty, implies split char */ ']' : origin_split[0][0]);
        return {};
    }
    auto slash_split = Split(origin_split[0].subspan(1), '/');
    if (slash_split[0].size() != 8) {
        error = strprintf("Fingerprint is not 4 bytes (%u characters instead of 8 characters)", slash_split[0].size());
        return {};
    }
    std::string fpr_hex = std::string(slash_split[0].begin(), slash_split[0].end());
    if (!IsHex(fpr_hex)) {
        error = strprintf("Fingerprint '%s' is not hex", fpr_hex);
        return {};
    }
    auto fpr_bytes = ParseHex(fpr_hex);
    KeyOriginInfo info;
    static_assert(sizeof(info.fingerprint) == 4, "Fingerprint must be 4 bytes");
    assert(fpr_bytes.size() == 4);
    std::copy(fpr_bytes.begin(), fpr_bytes.end(), info.fingerprint);
    std::vector<KeyPath> path;
    if (!ParseKeyPath(slash_split, path, apostrophe, error, /*allow_multipath=*/false)) return {};
    info.path = path.at(0);
    auto providers = ParsePubkeyInner(key_exp_index, origin_split[1], ctx, out, apostrophe, error);
    if (providers.empty()) return {};
    ret.reserve(providers.size());
    for (auto& prov : providers) {
        ret.emplace_back(std::make_unique<OriginPubkeyProvider>(key_exp_index, info, std::move(prov), apostrophe));
    }
    return ret;
}

std::unique_ptr<PubkeyProvider> InferPubkey(const CPubKey& pubkey, ParseScriptContext ctx, const SigningProvider& provider)
{
    // Key cannot be hybrid
    if (!pubkey.IsValidNonHybrid()) {
        return nullptr;
    }
    // Uncompressed is only allowed in TOP and P2SH contexts
    if (ctx != ParseScriptContext::TOP && ctx != ParseScriptContext::P2SH && !pubkey.IsCompressed()) {
        return nullptr;
    }
    std::unique_ptr<PubkeyProvider> key_provider = std::make_unique<ConstPubkeyProvider>(0, pubkey, false);
    KeyOriginInfo info;
    if (provider.GetKeyOrigin(pubkey.GetID(), info)) {
        return std::make_unique<OriginPubkeyProvider>(0, std::move(info), std::move(key_provider), /*apostrophe=*/false);
    }
    return key_provider;
}

std::unique_ptr<PubkeyProvider> InferXOnlyPubkey(const XOnlyPubKey& xkey, ParseScriptContext ctx, const SigningProvider& provider)
{
    CPubKey pubkey{xkey.GetEvenCorrespondingCPubKey()};
    std::unique_ptr<PubkeyProvider> key_provider = std::make_unique<ConstPubkeyProvider>(0, pubkey, true);
    KeyOriginInfo info;
    if (provider.GetKeyOriginByXOnly(xkey, info)) {
        return std::make_unique<OriginPubkeyProvider>(0, std::move(info), std::move(key_provider), /*apostrophe=*/false);
    }
    return key_provider;
}

/**
 * The context for parsing a Miniscript descriptor (either from Script or from its textual representation).
 */
struct KeyParser {
    //! The Key type is an index in DescriptorImpl::m_pubkey_args
    using Key = uint32_t;
    //! Must not be nullptr if parsing from string.
    FlatSigningProvider* m_out;
    //! Must not be nullptr if parsing from Script.
    const SigningProvider* m_in;
    //! List of multipath expanded keys contained in the Miniscript.
    mutable std::vector<std::vector<std::unique_ptr<PubkeyProvider>>> m_keys;
    //! Used to detect key parsing errors within a Miniscript.
    mutable std::string m_key_parsing_error;
    //! The script context we're operating within (Tapscript or P2WSH).
    const miniscript::MiniscriptContext m_script_ctx;
    //! The number of keys that were parsed before starting to parse this Miniscript descriptor.
    uint32_t m_offset;

    KeyParser(FlatSigningProvider* out LIFETIMEBOUND, const SigningProvider* in LIFETIMEBOUND,
              miniscript::MiniscriptContext ctx, uint32_t offset = 0)
        : m_out(out), m_in(in), m_script_ctx(ctx), m_offset(offset) {}

    bool KeyCompare(const Key& a, const Key& b) const {
        return *m_keys.at(a).at(0) < *m_keys.at(b).at(0);
    }

    ParseScriptContext ParseContext() const {
        switch (m_script_ctx) {
            case miniscript::MiniscriptContext::P2WSH: return ParseScriptContext::P2WSH;
            case miniscript::MiniscriptContext::TAPSCRIPT: return ParseScriptContext::P2TR;
        }
        assert(false);
    }

    template<typename I> std::optional<Key> FromString(I begin, I end) const
    {
        assert(m_out);
        Key key = m_keys.size();
        uint32_t exp_index = m_offset + key;
        auto pk = ParsePubkey(exp_index, {&*begin, &*end}, ParseContext(), *m_out, m_key_parsing_error);
        if (pk.empty()) return {};
        m_keys.emplace_back(std::move(pk));
        return key;
    }

    std::optional<std::string> ToString(const Key& key) const
    {
        return m_keys.at(key).at(0)->ToString();
    }

    template<typename I> std::optional<Key> FromPKBytes(I begin, I end) const
    {
        assert(m_in);
        Key key = m_keys.size();
        if (miniscript::IsTapscript(m_script_ctx) && end - begin == 32) {
            XOnlyPubKey pubkey;
            std::copy(begin, end, pubkey.begin());
            if (auto pubkey_provider = InferXOnlyPubkey(pubkey, ParseContext(), *m_in)) {
                m_keys.emplace_back();
                m_keys.back().push_back(std::move(pubkey_provider));
                return key;
            }
        } else if (!miniscript::IsTapscript(m_script_ctx)) {
            CPubKey pubkey(begin, end);
            if (auto pubkey_provider = InferPubkey(pubkey, ParseContext(), *m_in)) {
                m_keys.emplace_back();
                m_keys.back().push_back(std::move(pubkey_provider));
                return key;
            }
        }
        return {};
    }

    template<typename I> std::optional<Key> FromPKHBytes(I begin, I end) const
    {
        assert(end - begin == 20);
        assert(m_in);
        uint160 hash;
        std::copy(begin, end, hash.begin());
        CKeyID keyid(hash);
        CPubKey pubkey;
        if (m_in->GetPubKey(keyid, pubkey)) {
            if (auto pubkey_provider = InferPubkey(pubkey, ParseContext(), *m_in)) {
                Key key = m_keys.size();
                m_keys.emplace_back();
                m_keys.back().push_back(std::move(pubkey_provider));
                return key;
            }
        }
        return {};
    }

    miniscript::MiniscriptContext MsContext() const {
        return m_script_ctx;
    }
};

/** Parse a script in a particular context. */
// NOLINTNEXTLINE(misc-no-recursion)
std::vector<std::unique_ptr<DescriptorImpl>> ParseScript(uint32_t& key_exp_index, std::span<const char>& sp, ParseScriptContext ctx, FlatSigningProvider& out, std::string& error)
{
    using namespace script;
    Assume(ctx == ParseScriptContext::TOP || ctx == ParseScriptContext::P2SH || ctx == ParseScriptContext::P2WSH || ctx == ParseScriptContext::P2TR);
    std::vector<std::unique_ptr<DescriptorImpl>> ret;
    auto expr = Expr(sp);
    if (Func("pk", expr)) {
        auto pubkeys = ParsePubkey(key_exp_index, expr, ctx, out, error);
        if (pubkeys.empty()) {
            error = strprintf("pk(): %s", error);
            return {};
        }
        ++key_exp_index;
        for (auto& pubkey : pubkeys) {
            ret.emplace_back(std::make_unique<PKDescriptor>(std::move(pubkey), ctx == ParseScriptContext::P2TR));
        }
        return ret;
    }
    if ((ctx == ParseScriptContext::TOP || ctx == ParseScriptContext::P2SH || ctx == ParseScriptContext::P2WSH) && Func("pkh", expr)) {
        auto pubkeys = ParsePubkey(key_exp_index, expr, ctx, out, error);
        if (pubkeys.empty()) {
            error = strprintf("pkh(): %s", error);
            return {};
        }
        ++key_exp_index;
        for (auto& pubkey : pubkeys) {
            ret.emplace_back(std::make_unique<PKHDescriptor>(std::move(pubkey)));
        }
        return ret;
    }
    if (ctx == ParseScriptContext::TOP && Func("combo", expr)) {
        auto pubkeys = ParsePubkey(key_exp_index, expr, ctx, out, error);
        if (pubkeys.empty()) {
            error = strprintf("combo(): %s", error);
            return {};
        }
        ++key_exp_index;
        for (auto& pubkey : pubkeys) {
            ret.emplace_back(std::make_unique<ComboDescriptor>(std::move(pubkey)));
        }
        return ret;
    } else if (Func("combo", expr)) {
        error = "Can only have combo() at top level";
        return {};
    }
    const bool multi = Func("multi", expr);
    const bool sortedmulti = !multi && Func("sortedmulti", expr);
    const bool multi_a = !(multi || sortedmulti) && Func("multi_a", expr);
    const bool sortedmulti_a = !(multi || sortedmulti || multi_a) && Func("sortedmulti_a", expr);
    if (((ctx == ParseScriptContext::TOP || ctx == ParseScriptContext::P2SH || ctx == ParseScriptContext::P2WSH) && (multi || sortedmulti)) ||
        (ctx == ParseScriptContext::P2TR && (multi_a || sortedmulti_a))) {
        auto threshold = Expr(expr);
        uint32_t thres;
        std::vector<std::vector<std::unique_ptr<PubkeyProvider>>> providers; // List of multipath expanded pubkeys
        if (const auto maybe_thres{ToIntegral<uint32_t>(std::string_view{threshold.begin(), threshold.end()})}) {
            thres = *maybe_thres;
        } else {
            error = strprintf("Multi threshold '%s' is not valid", std::string(threshold.begin(), threshold.end()));
            return {};
        }
        size_t script_size = 0;
        size_t max_providers_len = 0;
        while (expr.size()) {
            if (!Const(",", expr)) {
                error = strprintf("Multi: expected ',', got '%c'", expr[0]);
                return {};
            }
            auto arg = Expr(expr);
            auto pks = ParsePubkey(key_exp_index, arg, ctx, out, error);
            if (pks.empty()) {
                error = strprintf("Multi: %s", error);
                return {};
            }
            script_size += pks.at(0)->GetSize() + 1;
            max_providers_len = std::max(max_providers_len, pks.size());
            providers.emplace_back(std::move(pks));
            key_exp_index++;
        }
        if ((multi || sortedmulti) && (providers.empty() || providers.size() > MAX_PUBKEYS_PER_MULTISIG)) {
            error = strprintf("Cannot have %u keys in multisig; must have between 1 and %d keys, inclusive", providers.size(), MAX_PUBKEYS_PER_MULTISIG);
            return {};
        } else if ((multi_a || sortedmulti_a) && (providers.empty() || providers.size() > MAX_PUBKEYS_PER_MULTI_A)) {
            error = strprintf("Cannot have %u keys in multi_a; must have between 1 and %d keys, inclusive", providers.size(), MAX_PUBKEYS_PER_MULTI_A);
            return {};
        } else if (thres < 1) {
            error = strprintf("Multisig threshold cannot be %d, must be at least 1", thres);
            return {};
        } else if (thres > providers.size()) {
            error = strprintf("Multisig threshold cannot be larger than the number of keys; threshold is %d but only %u keys specified", thres, providers.size());
            return {};
        }
        if (ctx == ParseScriptContext::TOP) {
            if (providers.size() > 3) {
                error = strprintf("Cannot have %u pubkeys in bare multisig; only at most 3 pubkeys", providers.size());
                return {};
            }
        }
        if (ctx == ParseScriptContext::P2SH) {
            // This limits the maximum number of compressed pubkeys to 15.
            if (script_size + 3 > MAX_SCRIPT_ELEMENT_SIZE) {
                error = strprintf("P2SH script is too large, %d bytes is larger than %d bytes", script_size + 3, MAX_SCRIPT_ELEMENT_SIZE);
                return {};
            }
        }

        // Make sure all vecs are of the same length, or exactly length 1
        // For length 1 vectors, clone key providers until vector is the same length
        for (auto& vec : providers) {
            if (vec.size() == 1) {
                for (size_t i = 1; i < max_providers_len; ++i) {
                    vec.emplace_back(vec.at(0)->Clone());
                }
            } else if (vec.size() != max_providers_len) {
                error = strprintf("multi(): Multipath derivation paths have mismatched lengths");
                return {};
            }
        }

        // Build the final descriptors vector
        for (size_t i = 0; i < max_providers_len; ++i) {
            // Build final pubkeys vectors by retrieving the i'th subscript for each vector in subscripts
            std::vector<std::unique_ptr<PubkeyProvider>> pubs;
            pubs.reserve(providers.size());
            for (auto& pub : providers) {
                pubs.emplace_back(std::move(pub.at(i)));
            }
            if (multi || sortedmulti) {
                ret.emplace_back(std::make_unique<MultisigDescriptor>(thres, std::move(pubs), sortedmulti));
            } else {
                ret.emplace_back(std::make_unique<MultiADescriptor>(thres, std::move(pubs), sortedmulti_a));
            }
        }
        return ret;
    } else if (multi || sortedmulti) {
        error = "Can only have multi/sortedmulti at top level, in sh(), or in wsh()";
        return {};
    } else if (multi_a || sortedmulti_a) {
        error = "Can only have multi_a/sortedmulti_a inside tr()";
        return {};
    }
    if ((ctx == ParseScriptContext::TOP || ctx == ParseScriptContext::P2SH) && Func("wpkh", expr)) {
        auto pubkeys = ParsePubkey(key_exp_index, expr, ParseScriptContext::P2WPKH, out, error);
        if (pubkeys.empty()) {
            error = strprintf("wpkh(): %s", error);
            return {};
        }
        key_exp_index++;
        for (auto& pubkey : pubkeys) {
            ret.emplace_back(std::make_unique<WPKHDescriptor>(std::move(pubkey)));
        }
        return ret;
    } else if (Func("wpkh", expr)) {
        error = "Can only have wpkh() at top level or inside sh()";
        return {};
    }
    if (ctx == ParseScriptContext::TOP && Func("sh", expr)) {
        auto descs = ParseScript(key_exp_index, expr, ParseScriptContext::P2SH, out, error);
        if (descs.empty() || expr.size()) return {};
        std::vector<std::unique_ptr<DescriptorImpl>> ret;
        ret.reserve(descs.size());
        for (auto& desc : descs) {
            ret.push_back(std::make_unique<SHDescriptor>(std::move(desc)));
        }
        return ret;
    } else if (Func("sh", expr)) {
        error = "Can only have sh() at top level";
        return {};
    }
    if ((ctx == ParseScriptContext::TOP || ctx == ParseScriptContext::P2SH) && Func("wsh", expr)) {
        auto descs = ParseScript(key_exp_index, expr, ParseScriptContext::P2WSH, out, error);
        if (descs.empty() || expr.size()) return {};
        for (auto& desc : descs) {
            ret.emplace_back(std::make_unique<WSHDescriptor>(std::move(desc)));
        }
        return ret;
    } else if (Func("wsh", expr)) {
        error = "Can only have wsh() at top level or inside sh()";
        return {};
    }
    if (ctx == ParseScriptContext::TOP && Func("addr", expr)) {
        CTxDestination dest = DecodeDestination(std::string(expr.begin(), expr.end()));
        if (!IsValidDestination(dest)) {
            error = "Address is not valid";
            return {};
        }
        ret.emplace_back(std::make_unique<AddressDescriptor>(std::move(dest)));
        return ret;
    } else if (Func("addr", expr)) {
        error = "Can only have addr() at top level";
        return {};
    }
    if (ctx == ParseScriptContext::TOP && Func("tr", expr)) {
        auto arg = Expr(expr);
        auto internal_keys = ParsePubkey(key_exp_index, arg, ParseScriptContext::P2TR, out, error);
        if (internal_keys.empty()) {
            error = strprintf("tr(): %s", error);
            return {};
        }
        size_t max_providers_len = internal_keys.size();
        ++key_exp_index;
        std::vector<std::vector<std::unique_ptr<DescriptorImpl>>> subscripts; //!< list of multipath expanded script subexpressions
        std::vector<int> depths; //!< depth in the tree of each subexpression (same length subscripts)
        if (expr.size()) {
            if (!Const(",", expr)) {
                error = strprintf("tr: expected ',', got '%c'", expr[0]);
                return {};
            }
            /** The path from the top of the tree to what we're currently processing.
             * branches[i] == false: left branch in the i'th step from the top; true: right branch.
             */
            std::vector<bool> branches;
            // Loop over all provided scripts. In every iteration exactly one script will be processed.
            // Use a do-loop because inside this if-branch we expect at least one script.
            do {
                // First process all open braces.
                while (Const("{", expr)) {
                    branches.push_back(false); // new left branch
                    if (branches.size() > TAPROOT_CONTROL_MAX_NODE_COUNT) {
                        error = strprintf("tr() supports at most %i nesting levels", TAPROOT_CONTROL_MAX_NODE_COUNT);
                        return {};
                    }
                }
                // Process the actual script expression.
                auto sarg = Expr(expr);
                subscripts.emplace_back(ParseScript(key_exp_index, sarg, ParseScriptContext::P2TR, out, error));
                if (subscripts.back().empty()) return {};
                max_providers_len = std::max(max_providers_len, subscripts.back().size());
                depths.push_back(branches.size());
                // Process closing braces; one is expected for every right branch we were in.
                while (branches.size() && branches.back()) {
                    if (!Const("}", expr)) {
                        error = strprintf("tr(): expected '}' after script expression");
                        return {};
                    }
                    branches.pop_back(); // move up one level after encountering '}'
                }
                // If after that, we're at the end of a left branch, expect a comma.
                if (branches.size() && !branches.back()) {
                    if (!Const(",", expr)) {
                        error = strprintf("tr(): expected ',' after script expression");
                        return {};
                    }
                    branches.back() = true; // And now we're in a right branch.
                }
            } while (branches.size());
            // After we've explored a whole tree, we must be at the end of the expression.
            if (expr.size()) {
                error = strprintf("tr(): expected ')' after script expression");
                return {};
            }
        }
        assert(TaprootBuilder::ValidDepths(depths));

        // Make sure all vecs are of the same length, or exactly length 1
        // For length 1 vectors, clone subdescs until vector is the same length
        for (auto& vec : subscripts) {
            if (vec.size() == 1) {
                for (size_t i = 1; i < max_providers_len; ++i) {
                    vec.emplace_back(vec.at(0)->Clone());
                }
            } else if (vec.size() != max_providers_len) {
                error = strprintf("tr(): Multipath subscripts have mismatched lengths");
                return {};
            }
        }

        if (internal_keys.size() > 1 && internal_keys.size() != max_providers_len) {
            error = strprintf("tr(): Multipath internal key mismatches multipath subscripts lengths");
            return {};
        }

        while (internal_keys.size() < max_providers_len) {
            internal_keys.emplace_back(internal_keys.at(0)->Clone());
        }

        // Build the final descriptors vector
        for (size_t i = 0; i < max_providers_len; ++i) {
            // Build final subscripts vectors by retrieving the i'th subscript for each vector in subscripts
            std::vector<std::unique_ptr<DescriptorImpl>> this_subs;
            this_subs.reserve(subscripts.size());
            for (auto& subs : subscripts) {
                this_subs.emplace_back(std::move(subs.at(i)));
            }
            ret.emplace_back(std::make_unique<TRDescriptor>(std::move(internal_keys.at(i)), std::move(this_subs), depths));
        }
        return ret;


    } else if (Func("tr", expr)) {
        error = "Can only have tr at top level";
        return {};
    }
    if (ctx == ParseScriptContext::TOP && Func("rawtr", expr)) {
        auto arg = Expr(expr);
        if (expr.size()) {
            error = strprintf("rawtr(): only one key expected.");
            return {};
        }
        auto output_keys = ParsePubkey(key_exp_index, arg, ParseScriptContext::P2TR, out, error);
        if (output_keys.empty()) {
            error = strprintf("rawtr(): %s", error);
            return {};
        }
        ++key_exp_index;
        for (auto& pubkey : output_keys) {
            ret.emplace_back(std::make_unique<RawTRDescriptor>(std::move(pubkey)));
        }
        return ret;
    } else if (Func("rawtr", expr)) {
        error = "Can only have rawtr at top level";
        return {};
    }
    if (ctx == ParseScriptContext::TOP && Func("raw", expr)) {
        std::string str(expr.begin(), expr.end());
        if (!IsHex(str)) {
            error = "Raw script is not hex";
            return {};
        }
        auto bytes = ParseHex(str);
        ret.emplace_back(std::make_unique<RawDescriptor>(CScript(bytes.begin(), bytes.end())));
        return ret;
    } else if (Func("raw", expr)) {
        error = "Can only have raw() at top level";
        return {};
    }
    // Process miniscript expressions.
    {
        const auto script_ctx{ctx == ParseScriptContext::P2WSH ? miniscript::MiniscriptContext::P2WSH : miniscript::MiniscriptContext::TAPSCRIPT};
        KeyParser parser(/*out = */&out, /* in = */nullptr, /* ctx = */script_ctx, key_exp_index);
        auto node = miniscript::FromString(std::string(expr.begin(), expr.end()), parser);
        if (parser.m_key_parsing_error != "") {
            error = std::move(parser.m_key_parsing_error);
            return {};
        }
        if (node) {
            if (ctx != ParseScriptContext::P2WSH && ctx != ParseScriptContext::P2TR) {
                error = "Miniscript expressions can only be used in wsh or tr.";
                return {};
            }
            if (!node->IsSane() || node->IsNotSatisfiable()) {
                // Try to find the first insane sub for better error reporting.
                auto insane_node = node.get();
                if (const auto sub = node->FindInsaneSub()) insane_node = sub;
                if (const auto str = insane_node->ToString(parser)) error = *str;
                if (!insane_node->IsValid()) {
                    error += " is invalid";
                } else if (!node->IsSane()) {
                    error += " is not sane";
                    if (!insane_node->IsNonMalleable()) {
                        error += ": malleable witnesses exist";
                    } else if (insane_node == node.get() && !insane_node->NeedsSignature()) {
                        error += ": witnesses without signature exist";
                    } else if (!insane_node->CheckTimeLocksMix()) {
                        error += ": contains mixes of timelocks expressed in blocks and seconds";
                    } else if (!insane_node->CheckDuplicateKey()) {
                        error += ": contains duplicate public keys";
                    } else if (!insane_node->ValidSatisfactions()) {
                        error += ": needs witnesses that may exceed resource limits";
                    }
                } else {
                    error += " is not satisfiable";
                }
                return {};
            }
            // A signature check is required for a miniscript to be sane. Therefore no sane miniscript
            // may have an empty list of public keys.
            CHECK_NONFATAL(!parser.m_keys.empty());
            key_exp_index += parser.m_keys.size();
            // Make sure all vecs are of the same length, or exactly length 1
            // For length 1 vectors, clone subdescs until vector is the same length
            size_t num_multipath = std::max_element(parser.m_keys.begin(), parser.m_keys.end(),
                    [](const std::vector<std::unique_ptr<PubkeyProvider>>& a, const std::vector<std::unique_ptr<PubkeyProvider>>& b) {
                        return a.size() < b.size();
                    })->size();

            for (auto& vec : parser.m_keys) {
                if (vec.size() == 1) {
                    for (size_t i = 1; i < num_multipath; ++i) {
                        vec.emplace_back(vec.at(0)->Clone());
                    }
                } else if (vec.size() != num_multipath) {
                    error = strprintf("Miniscript: Multipath derivation paths have mismatched lengths");
                    return {};
                }
            }

            // Build the final descriptors vector
            for (size_t i = 0; i < num_multipath; ++i) {
                // Build final pubkeys vectors by retrieving the i'th subscript for each vector in subscripts
                std::vector<std::unique_ptr<PubkeyProvider>> pubs;
                pubs.reserve(parser.m_keys.size());
                for (auto& pub : parser.m_keys) {
                    pubs.emplace_back(std::move(pub.at(i)));
                }
                ret.emplace_back(std::make_unique<MiniscriptDescriptor>(std::move(pubs), node->Clone()));
            }
            return ret;
        }
    }
    if (ctx == ParseScriptContext::P2SH) {
        error = "A function is needed within P2SH";
        return {};
    } else if (ctx == ParseScriptContext::P2WSH) {
        error = "A function is needed within P2WSH";
        return {};
    }
    error = strprintf("'%s' is not a valid descriptor function", std::string(expr.begin(), expr.end()));
    return {};
}

std::unique_ptr<DescriptorImpl> InferMultiA(const CScript& script, ParseScriptContext ctx, const SigningProvider& provider)
{
    auto match = MatchMultiA(script);
    if (!match) return {};
    std::vector<std::unique_ptr<PubkeyProvider>> keys;
    keys.reserve(match->second.size());
    for (const auto keyspan : match->second) {
        if (keyspan.size() != 32) return {};
        auto key = InferXOnlyPubkey(XOnlyPubKey{keyspan}, ctx, provider);
        if (!key) return {};
        keys.push_back(std::move(key));
    }
    return std::make_unique<MultiADescriptor>(match->first, std::move(keys));
}

// NOLINTNEXTLINE(misc-no-recursion)
std::unique_ptr<DescriptorImpl> InferScript(const CScript& script, ParseScriptContext ctx, const SigningProvider& provider)
{
    if (ctx == ParseScriptContext::P2TR && script.size() == 34 && script[0] == 32 && script[33] == OP_CHECKSIG) {
        XOnlyPubKey key{std::span{script}.subspan(1, 32)};
        return std::make_unique<PKDescriptor>(InferXOnlyPubkey(key, ctx, provider), true);
    }

    if (ctx == ParseScriptContext::P2TR) {
        auto ret = InferMultiA(script, ctx, provider);
        if (ret) return ret;
    }

    std::vector<std::vector<unsigned char>> data;
    TxoutType txntype = Solver(script, data);

    if (txntype == TxoutType::PUBKEY && (ctx == ParseScriptContext::TOP || ctx == ParseScriptContext::P2SH || ctx == ParseScriptContext::P2WSH)) {
        CPubKey pubkey(data[0]);
        if (auto pubkey_provider = InferPubkey(pubkey, ctx, provider)) {
            return std::make_unique<PKDescriptor>(std::move(pubkey_provider));
        }
    }
    if (txntype == TxoutType::PUBKEYHASH && (ctx == ParseScriptContext::TOP || ctx == ParseScriptContext::P2SH || ctx == ParseScriptContext::P2WSH)) {
        uint160 hash(data[0]);
        CKeyID keyid(hash);
        CPubKey pubkey;
        if (provider.GetPubKey(keyid, pubkey)) {
            if (auto pubkey_provider = InferPubkey(pubkey, ctx, provider)) {
                return std::make_unique<PKHDescriptor>(std::move(pubkey_provider));
            }
        }
    }
    if (txntype == TxoutType::WITNESS_V0_KEYHASH && (ctx == ParseScriptContext::TOP || ctx == ParseScriptContext::P2SH)) {
        uint160 hash(data[0]);
        CKeyID keyid(hash);
        CPubKey pubkey;
        if (provider.GetPubKey(keyid, pubkey)) {
            if (auto pubkey_provider = InferPubkey(pubkey, ParseScriptContext::P2WPKH, provider)) {
                return std::make_unique<WPKHDescriptor>(std::move(pubkey_provider));
            }
        }
    }
    if (txntype == TxoutType::MULTISIG && (ctx == ParseScriptContext::TOP || ctx == ParseScriptContext::P2SH || ctx == ParseScriptContext::P2WSH)) {
        bool ok = true;
        std::vector<std::unique_ptr<PubkeyProvider>> providers;
        for (size_t i = 1; i + 1 < data.size(); ++i) {
            CPubKey pubkey(data[i]);
            if (auto pubkey_provider = InferPubkey(pubkey, ctx, provider)) {
                providers.push_back(std::move(pubkey_provider));
            } else {
                ok = false;
                break;
            }
        }
        if (ok) return std::make_unique<MultisigDescriptor>((int)data[0][0], std::move(providers));
    }
    if (txntype == TxoutType::SCRIPTHASH && ctx == ParseScriptContext::TOP) {
        uint160 hash(data[0]);
        CScriptID scriptid(hash);
        CScript subscript;
        if (provider.GetCScript(scriptid, subscript)) {
            auto sub = InferScript(subscript, ParseScriptContext::P2SH, provider);
            if (sub) return std::make_unique<SHDescriptor>(std::move(sub));
        }
    }
    if (txntype == TxoutType::WITNESS_V0_SCRIPTHASH && (ctx == ParseScriptContext::TOP || ctx == ParseScriptContext::P2SH)) {
        CScriptID scriptid{RIPEMD160(data[0])};
        CScript subscript;
        if (provider.GetCScript(scriptid, subscript)) {
            auto sub = InferScript(subscript, ParseScriptContext::P2WSH, provider);
            if (sub) return std::make_unique<WSHDescriptor>(std::move(sub));
        }
    }
    if (txntype == TxoutType::WITNESS_V1_TAPROOT && ctx == ParseScriptContext::TOP) {
        // Extract x-only pubkey from output.
        XOnlyPubKey pubkey;
        std::copy(data[0].begin(), data[0].end(), pubkey.begin());
        // Request spending data.
        TaprootSpendData tap;
        if (provider.GetTaprootSpendData(pubkey, tap)) {
            // If found, convert it back to tree form.
            auto tree = InferTaprootTree(tap, pubkey);
            if (tree) {
                // If that works, try to infer subdescriptors for all leaves.
                bool ok = true;
                std::vector<std::unique_ptr<DescriptorImpl>> subscripts; //!< list of script subexpressions
                std::vector<int> depths; //!< depth in the tree of each subexpression (same length subscripts)
                for (const auto& [depth, script, leaf_ver] : *tree) {
                    std::unique_ptr<DescriptorImpl> subdesc;
                    if (leaf_ver == TAPROOT_LEAF_TAPSCRIPT) {
                        subdesc = InferScript(CScript(script.begin(), script.end()), ParseScriptContext::P2TR, provider);
                    }
                    if (!subdesc) {
                        ok = false;
                        break;
                    } else {
                        subscripts.push_back(std::move(subdesc));
                        depths.push_back(depth);
                    }
                }
                if (ok) {
                    auto key = InferXOnlyPubkey(tap.internal_key, ParseScriptContext::P2TR, provider);
                    return std::make_unique<TRDescriptor>(std::move(key), std::move(subscripts), std::move(depths));
                }
            }
        }
        // If the above doesn't work, construct a rawtr() descriptor with just the encoded x-only pubkey.
        if (pubkey.IsFullyValid()) {
            auto key = InferXOnlyPubkey(pubkey, ParseScriptContext::P2TR, provider);
            if (key) {
                return std::make_unique<RawTRDescriptor>(std::move(key));
            }
        }
    }

    if (ctx == ParseScriptContext::P2WSH || ctx == ParseScriptContext::P2TR) {
        const auto script_ctx{ctx == ParseScriptContext::P2WSH ? miniscript::MiniscriptContext::P2WSH : miniscript::MiniscriptContext::TAPSCRIPT};
        KeyParser parser(/* out = */nullptr, /* in = */&provider, /* ctx = */script_ctx);
        auto node = miniscript::FromScript(script, parser);
        if (node && node->IsSane()) {
            std::vector<std::unique_ptr<PubkeyProvider>> keys;
            keys.reserve(parser.m_keys.size());
            for (auto& key : parser.m_keys) {
                keys.emplace_back(std::move(key.at(0)));
            }
            return std::make_unique<MiniscriptDescriptor>(std::move(keys), std::move(node));
        }
    }

    // The following descriptors are all top-level only descriptors.
    // So if we are not at the top level, return early.
    if (ctx != ParseScriptContext::TOP) return nullptr;

    CTxDestination dest;
    if (ExtractDestination(script, dest)) {
        if (GetScriptForDestination(dest) == script) {
            return std::make_unique<AddressDescriptor>(std::move(dest));
        }
    }

    return std::make_unique<RawDescriptor>(script);
}


} // namespace

/** Check a descriptor checksum, and update desc to be the checksum-less part. */
bool CheckChecksum(std::span<const char>& sp, bool require_checksum, std::string& error, std::string* out_checksum = nullptr)
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

std::vector<std::unique_ptr<Descriptor>> Parse(std::string_view descriptor, FlatSigningProvider& out, std::string& error, bool require_checksum)
{
    std::span<const char> sp{descriptor};
    if (!CheckChecksum(sp, require_checksum, error)) return {};
    uint32_t key_exp_index = 0;
    auto ret = ParseScript(key_exp_index, sp, ParseScriptContext::TOP, out, error);
    if (sp.empty() && !ret.empty()) {
        std::vector<std::unique_ptr<Descriptor>> descs;
        descs.reserve(ret.size());
        for (auto& r : ret) {
            descs.emplace_back(std::unique_ptr<Descriptor>(std::move(r)));
        }
        return descs;
    }
    return {};
}

std::string GetDescriptorChecksum(const std::string& descriptor)
{
    std::string ret;
    std::string error;
    std::span<const char> sp{descriptor};
    if (!CheckChecksum(sp, false, error, &ret)) return "";
    return ret;
}

std::unique_ptr<Descriptor> InferDescriptor(const CScript& script, const SigningProvider& provider)
{
    return InferScript(script, ParseScriptContext::TOP, provider);
}

uint256 DescriptorID(const Descriptor& desc)
{
    std::string desc_str = desc.ToString(/*compat_format=*/true);
    uint256 id;
    CSHA256().Write((unsigned char*)desc_str.data(), desc_str.size()).Finalize(id.begin());
    return id;
}

void DescriptorCache::CacheParentExtPubKey(uint32_t key_exp_pos, const CExtPubKey& xpub)
{
    m_parent_xpubs[key_exp_pos] = xpub;
}

void DescriptorCache::CacheDerivedExtPubKey(uint32_t key_exp_pos, uint32_t der_index, const CExtPubKey& xpub)
{
    auto& xpubs = m_derived_xpubs[key_exp_pos];
    xpubs[der_index] = xpub;
}

void DescriptorCache::CacheLastHardenedExtPubKey(uint32_t key_exp_pos, const CExtPubKey& xpub)
{
    m_last_hardened_xpubs[key_exp_pos] = xpub;
}

bool DescriptorCache::GetCachedParentExtPubKey(uint32_t key_exp_pos, CExtPubKey& xpub) const
{
    const auto& it = m_parent_xpubs.find(key_exp_pos);
    if (it == m_parent_xpubs.end()) return false;
    xpub = it->second;
    return true;
}

bool DescriptorCache::GetCachedDerivedExtPubKey(uint32_t key_exp_pos, uint32_t der_index, CExtPubKey& xpub) const
{
    const auto& key_exp_it = m_derived_xpubs.find(key_exp_pos);
    if (key_exp_it == m_derived_xpubs.end()) return false;
    const auto& der_it = key_exp_it->second.find(der_index);
    if (der_it == key_exp_it->second.end()) return false;
    xpub = der_it->second;
    return true;
}

bool DescriptorCache::GetCachedLastHardenedExtPubKey(uint32_t key_exp_pos, CExtPubKey& xpub) const
{
    const auto& it = m_last_hardened_xpubs.find(key_exp_pos);
    if (it == m_last_hardened_xpubs.end()) return false;
    xpub = it->second;
    return true;
}

DescriptorCache DescriptorCache::MergeAndDiff(const DescriptorCache& other)
{
    DescriptorCache diff;
    for (const auto& parent_xpub_pair : other.GetCachedParentExtPubKeys()) {
        CExtPubKey xpub;
        if (GetCachedParentExtPubKey(parent_xpub_pair.first, xpub)) {
            if (xpub != parent_xpub_pair.second) {
                throw std::runtime_error(std::string(__func__) + ": New cached parent xpub does not match already cached parent xpub");
            }
            continue;
        }
        CacheParentExtPubKey(parent_xpub_pair.first, parent_xpub_pair.second);
        diff.CacheParentExtPubKey(parent_xpub_pair.first, parent_xpub_pair.second);
    }
    for (const auto& derived_xpub_map_pair : other.GetCachedDerivedExtPubKeys()) {
        for (const auto& derived_xpub_pair : derived_xpub_map_pair.second) {
            CExtPubKey xpub;
            if (GetCachedDerivedExtPubKey(derived_xpub_map_pair.first, derived_xpub_pair.first, xpub)) {
                if (xpub != derived_xpub_pair.second) {
                    throw std::runtime_error(std::string(__func__) + ": New cached derived xpub does not match already cached derived xpub");
                }
                continue;
            }
            CacheDerivedExtPubKey(derived_xpub_map_pair.first, derived_xpub_pair.first, derived_xpub_pair.second);
            diff.CacheDerivedExtPubKey(derived_xpub_map_pair.first, derived_xpub_pair.first, derived_xpub_pair.second);
        }
    }
    for (const auto& lh_xpub_pair : other.GetCachedLastHardenedExtPubKeys()) {
        CExtPubKey xpub;
        if (GetCachedLastHardenedExtPubKey(lh_xpub_pair.first, xpub)) {
            if (xpub != lh_xpub_pair.second) {
                throw std::runtime_error(std::string(__func__) + ": New cached last hardened xpub does not match already cached last hardened xpub");
            }
            continue;
        }
        CacheLastHardenedExtPubKey(lh_xpub_pair.first, lh_xpub_pair.second);
        diff.CacheLastHardenedExtPubKey(lh_xpub_pair.first, lh_xpub_pair.second);
    }
    return diff;
}

ExtPubKeyMap DescriptorCache::GetCachedParentExtPubKeys() const
{
    return m_parent_xpubs;
}

std::unordered_map<uint32_t, ExtPubKeyMap> DescriptorCache::GetCachedDerivedExtPubKeys() const
{
    return m_derived_xpubs;
}

ExtPubKeyMap DescriptorCache::GetCachedLastHardenedExtPubKeys() const
{
    return m_last_hardened_xpubs;
}
