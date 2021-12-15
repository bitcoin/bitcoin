// Copyright (c) 2018-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SCRIPT_DESCRIPTOR_H
#define BITCOIN_SCRIPT_DESCRIPTOR_H

#include <outputtype.h>
#include <script/script.h>
#include <script/sign.h>
#include <script/signingprovider.h>

#include <optional>
#include <vector>

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
     * @param[in] xpub The CExtPubKey to get from cache
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
     * @param[in] xpub The CExtPubKey to get from cache
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
     * @param[in] xpub The CExtPubKey to get from cache
     */
    bool GetCachedLastHardenedExtPubKey(uint32_t key_exp_pos, CExtPubKey& xpub) const;

    /** Retrieve all cached parent xpubs */
    const ExtPubKeyMap GetCachedParentExtPubKeys() const;
    /** Retrieve all cached derived xpubs */
    const std::unordered_map<uint32_t, ExtPubKeyMap> GetCachedDerivedExtPubKeys() const;
    /** Retrieve all cached last hardened xpubs */
    const ExtPubKeyMap GetCachedLastHardenedExtPubKeys() const;

    /** Combine another DescriptorCache into this one.
     * Returns a cache containing the items from the other cache unknown to current cache
     */
    DescriptorCache MergeAndDiff(const DescriptorCache& other);
};

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
    virtual std::string ToString() const = 0;

    /** Whether this descriptor will return one scriptPubKey or multiple (aka is or is not combo) */
    virtual bool IsSingleType() const = 0;

    /** Convert the descriptor to a private string. This fails if the provided provider does not have the relevant private keys. */
    virtual bool ToPrivateString(const SigningProvider& provider, std::string& out) const = 0;

    /** Convert the descriptor to a normalized string. Normalized descriptors have the xpub at the last hardened step. This fails if the provided provider does not have the private keys to derive that xpub. */
    virtual bool ToNormalizedString(const SigningProvider& provider, std::string& out, const DescriptorCache* cache = nullptr) const = 0;

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
};

/** Parse a `descriptor` string. Included private keys are put in `out`.
 *
 * If the descriptor has a checksum, it must be valid. If `require_checksum`
 * is set, the checksum is mandatory - otherwise it is optional.
 *
 * If a parse error occurs, or the checksum is missing/invalid, or anything
 * else is wrong, `nullptr` is returned.
 */
std::unique_ptr<Descriptor> Parse(const std::string& descriptor, FlatSigningProvider& out, std::string& error, bool require_checksum = false);

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

#endif // BITCOIN_SCRIPT_DESCRIPTOR_H
