// Copyright (c) 2018-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SCRIPT_DESCRIPTOR_H
#define BITCOIN_SCRIPT_DESCRIPTOR_H

#include <script/script.h>
#include <script/sign.h>
#include <script/signingprovider.h>

#include <vector>

// Descriptors are strings that describe a set of scriptPubKeys, together with
// all information necessary to solve them. By combining all information into
// one, they avoid the need to separately import keys and scripts.
//
// Descriptors may be ranged, which occurs when the public keys inside are
// specified in the form of HD chains (xpubs).
//
// Descriptors always represent public information - public keys and scripts -
// but in cases where private keys need to be conveyed along with a descriptor,
// they can be included inside by changing public keys to private keys (WIF
// format), and changing xpubs by xprvs.
//
// Reference documentation about the descriptor language can be found in
// doc/descriptors.md.

/** Interface for parsed descriptor objects. */
struct Descriptor {
    virtual ~Descriptor() = default;

    /** Whether the expansion of this descriptor depends on the position. */
    virtual bool IsRange() const = 0;

    /** Whether this descriptor has all information about signing ignoring lack of private keys.
     *  This is true for all descriptors except ones that use `raw` or `addr` constructions. */
    virtual bool IsSolvable() const = 0;

    /** Convert the descriptor back to a string, undoing parsing. */
    virtual std::string ToString() const = 0;

    /** Convert the descriptor to a private string. This fails if the provided provider does not have the relevant private keys. */
    virtual bool ToPrivateString(const SigningProvider& provider, std::string& out) const = 0;

    /** Expand a descriptor at a specified position.
     *
     * pos: the position at which to expand the descriptor. If IsRange() is false, this is ignored.
     * provider: the provider to query for private keys in case of hardened derivation.
     * output_scripts: the expanded scriptPubKeys will be put here.
     * out: scripts and public keys necessary for solving the expanded scriptPubKeys will be put here (may be equal to provider).
     * cache: vector which will be overwritten with cache data necessary to evaluate the descriptor at this point without access to private keys.
     */
    virtual bool Expand(int pos, const SigningProvider& provider, std::vector<CScript>& output_scripts, FlatSigningProvider& out, std::vector<unsigned char>* cache = nullptr) const = 0;

    /** Expand a descriptor at a specified position using cached expansion data.
     *
     * pos: the position at which to expand the descriptor. If IsRange() is false, this is ignored.
     * cache: vector from which cached expansion data will be read.
     * output_scripts: the expanded scriptPubKeys will be put here.
     * out: scripts and public keys necessary for solving the expanded scriptPubKeys will be put here (may be equal to provider).
     */
    virtual bool ExpandFromCache(int pos, const std::vector<unsigned char>& cache, std::vector<CScript>& output_scripts, FlatSigningProvider& out) const = 0;

    /** Expand the private key for a descriptor at a specified position, if possible.
     *
     * pos: the position at which to expand the descriptor. If IsRange() is false, this is ignored.
     * provider: the provider to query for the private keys.
     * out: any private keys available for the specified pos will be placed here.
     */
    virtual void ExpandPrivate(int pos, const SigningProvider& provider, FlatSigningProvider& out) const = 0;
};

/** Parse a descriptor string. Included private keys are put in out.
 *
 * If the descriptor has a checksum, it must be valid. If require_checksum
 * is set, the checksum is mandatory - otherwise it is optional.
 *
 * If a parse error occurs, or the checksum is missing/invalid, or anything
 * else is wrong, nullptr is returned.
 */
std::unique_ptr<Descriptor> Parse(const std::string& descriptor, FlatSigningProvider& out, std::string& error, bool require_checksum = false);

/** Get the checksum for a descriptor.
 *
 * If it already has one, and it is correct, return the checksum in the input.
 * If it already has one that is wrong, return "".
 * If it does not already have one, return the checksum that would need to be added.
 */
std::string GetDescriptorChecksum(const std::string& descriptor);

/** Find a descriptor for the specified script, using information from provider where possible.
 *
 * A non-ranged descriptor which only generates the specified script will be returned in all
 * circumstances.
 *
 * For public keys with key origin information, this information will be preserved in the returned
 * descriptor.
 *
 * - If all information for solving `script` is present in `provider`, a descriptor will be returned
 *   which is `IsSolvable()` and encapsulates said information.
 * - Failing that, if `script` corresponds to a known address type, an "addr()" descriptor will be
 *   returned (which is not `IsSolvable()`).
 * - Failing that, a "raw()" descriptor is returned.
 */
std::unique_ptr<Descriptor> InferDescriptor(const CScript& script, const SigningProvider& provider);

#endif // BITCOIN_SCRIPT_DESCRIPTOR_H
