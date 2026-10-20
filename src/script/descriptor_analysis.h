// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit.

#ifndef BITCOIN_SCRIPT_DESCRIPTOR_ANALYSIS_H
#define BITCOIN_SCRIPT_DESCRIPTOR_ANALYSIS_H

#include <pubkey.h>
#include <script/keyorigin.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <variant>
#include <vector>

/** Public structure of one parsed key expression, without deriving any keys.
 *
 * Nodes are stored in preorder, with the root at index 0. Participant indices
 * refer only to this result, not to descriptor-cache indices or policy slots.
 * Origins are explicit annotations, separate from the derivation program.
 */
struct DescriptorKeyInfo {
    enum class DeriveType {
        NON_RANGED,
        UNHARDENED_RANGED,
        HARDENED_RANGED,
    };

    struct Derivation {
        std::vector<uint32_t> path;              //!< Numeric BIP32 child indices, including hardened bits.
        DeriveType type{DeriveType::NON_RANGED}; //!< Optional final wildcard, separate from path.
    };

    /** A constant key. When xonly is set, only the x coordinate is significant and
     *  pubkey holds its even form; match it as an XOnlyPubKey, which tries both
     *  parities, not by CKeyID. */
    struct ConstKey {
        CPubKey pubkey;
        bool xonly{false}; //!< The provider's key encoding; does not identify the signing context.
    };

    struct BIP32Key {
        CExtPubKey root;
        Derivation derivation;
    };

    struct MuSigKey {
        std::vector<size_t> participants; //!< In expression order, preserving multiplicity.
        Derivation derivation;            //!< Derivation applied after aggregation, not to participants.
    };

    struct Node {
        std::variant<ConstKey, BIP32Key, MuSigKey> key;
        std::optional<KeyOriginInfo> origin;
    };

    std::vector<Node> nodes;
};

#endif // BITCOIN_SCRIPT_DESCRIPTOR_ANALYSIS_H
