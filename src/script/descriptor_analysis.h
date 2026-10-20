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

/** Public key occurrences in an already parsed descriptor.
 *
 * Each vector index is an ID local to this inventory, independent of descriptor
 * cache indices. Equal keys remain separate occurrences. MuSig participant
 * links refer to this vector. IDs follow descriptor traversal and stored key
 * argument order, and need not survive inference or serialization round trips.
 * No policy nodes or wallet ownership information are included.
 */
struct DescriptorKeyInventory {
    enum class Role {
        ECDSA,
        TAPROOT_INTERNAL,
        TAPROOT_OUTPUT,
        TAPSCRIPT,
        MUSIG_PARTICIPANT, //!< Full public keys are used by aggregation, even in Taproot.
        UNUSED,            //!< Key material recorded without a signing requirement.
    };

    struct Occurrence {
        DescriptorKeyInfo::Node expression;
        Role role;
    };

    std::vector<Occurrence> occurrences;
};

#endif // BITCOIN_SCRIPT_DESCRIPTOR_ANALYSIS_H
