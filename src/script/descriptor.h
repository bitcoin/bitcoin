// Copyright (c) 2018-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SCRIPT_DESCRIPTOR_H
#define BITCOIN_SCRIPT_DESCRIPTOR_H

#include <outputtype.h>
#include <pubkey.h>
#include <script/keyorigin.h>
#include <uint256.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

class CScript;
class SigningProvider;
struct FlatSigningProvider;

using ExtPubKeyMap = std::unordered_map<uint32_t, CExtPubKey>;

/** Cache for single descriptor's derived extended pubkeys */
class DescriptorCache {
private:
    /** Map key expression index -> map of (key derivation index -> xpub) */
    std::unordered_map<uint32_t, ExtPubKeyMap> m_derived_xpubs;
    /** Map key expression index -> parent xpub */
    ExtPubKeyMap m_parent_xpubs;
    /** Map key expression index -> last hardened xpub */
    ExtPubKeyMap m_last_hardened_xpubs;

public:
    /** Cache a parent xpub
     *
     * @param[in] key_exp_pos Position of the key expression within the descriptor
     * @param[in] xpub The CExtPubKey to cache
     */
    void CacheParentExtPubKey(uint32_t key_exp_pos, const CExtPubKey& xpub);
    /** Retrieve a cached parent xpub
     *
     * @param[in] key_exp_pos Position of the key expression within the descriptor
     * @param[out] xpub The CExtPubKey to get from cache
     */
    bool GetCachedParentExtPubKey(uint32_t key_exp_pos, CExtPubKey& xpub) const;
    /** Cache an xpub derived at an index
     *
     * @param[in] key_exp_pos Position of the key expression within the descriptor
     * @param[in] der_index Derivation index of the xpub
     * @param[in] xpub The CExtPubKey to cache
     */
    void CacheDerivedExtPubKey(uint32_t key_exp_pos, uint32_t der_index, const CExtPubKey& xpub);
    /** Retrieve a cached xpub derived at an index
     *
     * @param[in] key_exp_pos Position of the key expression within the descriptor
     * @param[in] der_index Derivation index of the xpub
     * @param[out] xpub The CExtPubKey to get from cache
     */
    bool GetCachedDerivedExtPubKey(uint32_t key_exp_pos, uint32_t der_index, CExtPubKey& xpub) const;
    /** Cache a last hardened xpub
     *
     * @param[in] key_exp_pos Position of the key expression within the descriptor
     * @param[in] xpub The CExtPubKey to cache
     */
    void CacheLastHardenedExtPubKey(uint32_t key_exp_pos, const CExtPubKey& xpub);
    /** Retrieve a cached last hardened xpub
     *
     * @param[in] key_exp_pos Position of the key expression within the descriptor
     * @param[out] xpub The CExtPubKey to get from cache
     */
    bool GetCachedLastHardenedExtPubKey(uint32_t key_exp_pos, CExtPubKey& xpub) const;

    /** Retrieve all cached parent xpubs */
    ExtPubKeyMap GetCachedParentExtPubKeys() const;
    /** Retrieve all cached derived xpubs */
    std::unordered_map<uint32_t, ExtPubKeyMap> GetCachedDerivedExtPubKeys() const;
    /** Retrieve all cached last hardened xpubs */
    ExtPubKeyMap GetCachedLastHardenedExtPubKeys() const;

    /** Combine another DescriptorCache into this one.
     * Returns a cache containing the items from the other cache unknown to current cache
     */
    DescriptorCache MergeAndDiff(const DescriptorCache& other);
};

/** Interface for public key objects in descriptors. */
struct PubkeyProvider
{
public:
    //! Index of this key expression in the descriptor
    //! E.g. If this PubkeyProvider is key1 in multi(2, key1, key2, key3), then m_expr_index = 0
    const uint32_t m_expr_index;

    explicit PubkeyProvider(uint32_t exp_index) : m_expr_index(exp_index) {}

    virtual ~PubkeyProvider() = default;

    /** Compare two public keys represented by this provider.
     * Used by the Miniscript descriptors to check for duplicate keys in the script.
     */
    bool operator<(PubkeyProvider& other) const;

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

    /** Get the descriptor string form including private data (if available in arg).
     *  If the private data is not available, the output string in the "out" parameter
     *  will not contain any private key information,
     *  and this function will return "false".
     */
    virtual bool ToPrivateString(const SigningProvider& arg, std::string& out) const = 0;

    /** Get the descriptor string form with the xpub at the last hardened derivation,
     *  and always use h for hardened derivation.
     */
    virtual bool ToNormalizedString(const SigningProvider& arg, std::string& out, const DescriptorCache* cache = nullptr) const = 0;

    /** Derive a private key, if private data is available in arg and put it into out. */
    virtual void GetPrivKey(int pos, const SigningProvider& arg, FlatSigningProvider& out) const = 0;

    /** Whether private data for this provider is available in arg. */
    virtual bool HavePrivateKeys(const SigningProvider& arg) const;

    /** Return the non-extended public key for this PubkeyProvider, if it has one. */
    virtual std::optional<CPubKey> GetRootPubKey() const = 0;
    /** Return the extended public key for this PubkeyProvider, if it has one. */
    virtual std::optional<CExtPubKey> GetRootExtPubKey() const = 0;

    /** Return explicitly provided key origin information, if any. */
    virtual std::optional<KeyOriginInfo> GetOriginInfo() const { return std::nullopt; }

    /** Make a deep copy of this PubkeyProvider */
    virtual std::unique_ptr<PubkeyProvider> Clone() const = 0;

    /** Whether this PubkeyProvider is a BIP 32 extended key that can be derived from */
    virtual bool IsBIP32() const = 0;

    /** Get the count of keys known by this PubkeyProvider. Usually one, but may be more for key aggregation schemes */
    virtual size_t GetKeyCount() const { return 1; }

    /** Whether this PubkeyProvider can always provide a public key without cache or private key arguments */
    virtual bool CanSelfExpand() const = 0;
};

/** Parse a key expression using P2TR key rules.
 *
 * Any private keys in the expression are added to `out`.
 * If parsing fails, `error` is set and an empty vector is returned.
 */
std::vector<std::unique_ptr<PubkeyProvider>> ParsePubkey(std::string_view key_expression, FlatSigningProvider& out, std::string& error);

/** \brief Interface for parsed descriptor objects.
 *
 * Descriptors are strings that describe a set of scriptPubKeys, together with
 * all information necessary to solve them. By combining all information into
 * one, they avoid the need to separately import keys and scripts.
 *
 * Descriptors may be ranged, which occurs when the public keys inside are
 * specified in the form of HD chains (xpubs).
 *
 * Descriptors always represent public information - public keys and scripts -
 * but in cases where private keys need to be conveyed along with a descriptor,
 * they can be included inside by changing public keys to private keys (WIF
 * format), and changing xpubs by xprvs.
 *
 * Reference documentation about the descriptor language can be found in
 * doc/descriptors.md.
 */
struct Descriptor {
    virtual ~Descriptor() = default;

    /** Whether the expansion of this descriptor depends on the position. */
    virtual bool IsRange() const = 0;

    /** Whether this descriptor has all information about signing ignoring lack of private keys.
     *  This is true for all descriptors except ones that use `raw` or `addr` constructions. */
    virtual bool IsSolvable() const = 0;

    /** Convert the descriptor back to a string, undoing parsing. */
    virtual std::string ToString(bool compat_format=false) const = 0;

    /** Whether this descriptor will return at most one scriptPubKey or multiple (aka is or is not combo) */
    virtual bool IsSingleType() const = 0;

    /** Whether the given provider has all private keys required by this descriptor.
     * @return `false` if the descriptor doesn't have any keys or subdescriptors,
     *         or if the provider does not have all private keys required by
     *         the descriptor.
     */
    virtual bool HavePrivateKeys(const SigningProvider& provider) const = 0;

    /** Convert the descriptor to a private string. This uses public keys if the relevant private keys are not in the SigningProvider.
     *  If none of the relevant private keys are available, the output string in the "out" parameter will not contain any private key information,
     *  and this function will return "false".
     *  @param[in] provider The SigningProvider to query for private keys.
     *  @param[out] out The resulting descriptor string, containing private keys if available.
     *  @returns true if at least one private key available.
     */
    virtual bool ToPrivateString(const SigningProvider& provider, std::string& out) const = 0;

    /** Convert the descriptor to a normalized string. Normalized descriptors have the xpub at the last hardened step. This fails if the provided provider does not have the private keys to derive that xpub. */
    virtual bool ToNormalizedString(const SigningProvider& provider, std::string& out, const DescriptorCache* cache = nullptr) const = 0;

    /** Whether the descriptor can be used to produce its address(es) without needing a cache or private keys. */
    virtual bool CanSelfExpand() const = 0;

    /** Expand a descriptor at a specified position.
     *
     * @param[in] pos The position at which to expand the descriptor. If IsRange() is false, this is ignored.
     * @param[in] provider The provider to query for private keys in case of hardened derivation.
     * @param[out] output_scripts The expanded scriptPubKeys.
     * @param[out] out Scripts and public keys necessary for solving the expanded scriptPubKeys (may be equal to `provider`).
     * @param[out] write_cache Cache data necessary to evaluate the descriptor at this point without access to private keys.
     */
    virtual bool Expand(int pos, const SigningProvider& provider, std::vector<CScript>& output_scripts, FlatSigningProvider& out, DescriptorCache* write_cache = nullptr) const = 0;

    /** Expand a descriptor at a specified position using cached expansion data.
     *
     * @param[in] pos The position at which to expand the descriptor. If IsRange() is false, this is ignored.
     * @param[in] read_cache Cached expansion data.
     * @param[out] output_scripts The expanded scriptPubKeys.
     * @param[out] out Scripts and public keys necessary for solving the expanded scriptPubKeys (may be equal to `provider`).
     */
    virtual bool ExpandFromCache(int pos, const DescriptorCache& read_cache, std::vector<CScript>& output_scripts, FlatSigningProvider& out) const = 0;

    /** Expand the private key for a descriptor at a specified position, if possible.
     *
     * @param[in] pos The position at which to expand the descriptor. If IsRange() is false, this is ignored.
     * @param[in] provider The provider to query for the private keys.
     * @param[out] out Any private keys available for the specified `pos`.
     */
    virtual void ExpandPrivate(int pos, const SigningProvider& provider, FlatSigningProvider& out) const = 0;

    /** @return The OutputType of the scriptPubKey(s) produced by this descriptor. Or nullopt if indeterminate (multiple or none) */
    virtual std::optional<OutputType> GetOutputType() const = 0;

    /** Get the size of the scriptPubKey for this descriptor. */
    virtual std::optional<int64_t> ScriptSize() const = 0;

    /** Get the maximum size of a satisfaction for this descriptor, in weight units.
     *
     * @param use_max_sig Whether to assume ECDSA signatures will have a high-r.
     */
    virtual std::optional<int64_t> MaxSatisfactionWeight(bool use_max_sig) const = 0;

    /** Get the maximum size number of stack elements for satisfying this descriptor. */
    virtual std::optional<int64_t> MaxSatisfactionElems() const = 0;

    /** Return all (extended) public keys for this descriptor, including any from subdescriptors.
     *
     * @param[out] pubkeys Any public keys
     * @param[out] ext_pubs Any extended public keys
     */
    virtual void GetPubKeys(std::set<CPubKey>& pubkeys, std::set<CExtPubKey>& ext_pubs) const = 0;

    /** Whether this descriptor produces any scripts with the Expand functions */
    virtual bool HasScripts() const = 0;

    /** Semantic/safety warnings (includes subdescriptors). */
    virtual std::vector<std::string> Warnings() const = 0;

    /** Get the maximum key expression index. Used only for tests */
    virtual uint32_t GetMaxKeyExpr() const = 0;

    /** Get the number of key expressions in this descriptor. Used only for tests */
    virtual size_t GetKeyCount() const = 0;
};

/** Parse a `descriptor` string. Included private keys are put in `out`.
 *
 * If the descriptor has a checksum, it must be valid. If `require_checksum`
 * is set, the checksum is mandatory - otherwise it is optional.
 *
 * If a parse error occurs, or the checksum is missing/invalid, or anything
 * else is wrong, an empty vector is returned.
 */
std::vector<std::unique_ptr<Descriptor>> Parse(std::string_view descriptor, FlatSigningProvider& out, std::string& error, bool require_checksum = false);

/** Get the checksum for a `descriptor`.
 *
 * - If it already has one, and it is correct, return the checksum in the input.
 * - If it already has one that is wrong, return "".
 * - If it does not already have one, return the checksum that would need to be added.
 */
std::string GetDescriptorChecksum(const std::string& descriptor);

/** Find a descriptor for the specified `script`, using information from `provider` where possible.
 *
 * A non-ranged descriptor which only generates the specified script will be returned in all
 * circumstances.
 *
 * For public keys with key origin information, this information will be preserved in the returned
 * descriptor.
 *
 * - If all information for solving `script` is present in `provider`, a descriptor will be returned
 *   which is IsSolvable() and encapsulates said information.
 * - Failing that, if `script` corresponds to a known address type, an "addr()" descriptor will be
 *   returned (which is not IsSolvable()).
 * - Failing that, a "raw()" descriptor is returned.
 */
std::unique_ptr<Descriptor> InferDescriptor(const CScript& script, const SigningProvider& provider);

/** Unique identifier that may not change over time, unless explicitly marked as not backwards compatible.
*   This is not part of BIP 380, not guaranteed to be interoperable and should not be exposed to the user.
*/
uint256 DescriptorID(const Descriptor& desc);

#endif // BITCOIN_SCRIPT_DESCRIPTOR_H
