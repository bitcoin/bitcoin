// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SCRIPT_DESCRIPTOR_H
#define BITCOIN_SCRIPT_DESCRIPTOR_H

#include <script/script.h>
#include <script/sign.h>

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

    /** Convert the descriptor back to a string, undoing parsing. */
    virtual std::string ToString() const = 0;

    /** Convert the descriptor to a private string. This fails if the provided provider does not have the relevant private keys. */
    virtual bool ToPrivateString(const SigningProvider& provider, std::string& out) const = 0;

    /** Expand a descriptor at a specified position.
     *
     * pos: the position at which to expand the descriptor. If IsRange() is false, this is ignored.
     * provider: the provider to query for private keys in case of hardened derivation.
     * output_script: the expanded scriptPubKeys will be put here.
     * out: scripts and public keys necessary for solving the expanded scriptPubKeys will be put here (may be equal to provider).
     */
    virtual bool Expand(int pos, const SigningProvider& provider, std::vector<CScript>& output_scripts, FlatSigningProvider& out) const = 0;
};

/** Parse a descriptor string. Included private keys are put in out. Returns nullptr if parsing fails. */
std::unique_ptr<Descriptor> Parse(const std::string& descriptor, FlatSigningProvider& out);

#endif // BITCOIN_SCRIPT_DESCRIPTOR_H

