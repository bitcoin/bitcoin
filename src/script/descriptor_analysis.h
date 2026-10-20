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

namespace miniscript {
enum class Fragment;
} // namespace miniscript

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

/** Owned policy structure and its public key occurrences.
 *
 * Node IDs are vector indices local to this result, in preorder with root 0.
 * Key references use keys.occurrences, with the same order as GetKeyInventory().
 * A MuSig policy slot references its aggregate; participants remain linked
 * through the key inventory. Neither kind of ID is a persistent identity.
 *
 * Miniscript operators use the parsed fragments from script/miniscript.h,
 * including desugared aliases. The same construct can therefore appear in two
 * forms: a key check or multisig that is a whole descriptor argument, such as
 * wsh(multi(...)) or the tr() leaf pk(A), is a Kind node, while inside a
 * Miniscript expression it is a Fragment (MULTI, or c: over PK_K). Consumers
 * must handle both. No node contains a serialized subtree. Raw
 * scripts and addresses are opaque: their bytes are retained without inferring
 * policy or keys. This is structural inspection, not a satisfiability result.
 */
struct DescriptorPolicy {
    enum class Kind {
        PK,
        PKH,
        WPKH,
        COMBO,
        MULTI,
        SORTEDMULTI,
        MULTI_A,
        SORTEDMULTI_A,
        SH,
        WSH,
        TR,
        RAWTR,
        UNUSED,
        RAW,
        ADDR,
    };

    enum class ChildRole {
        SCRIPT,          //!< Script contained in sh() or wsh().
        WRAPPED,         //!< Operand of a Miniscript wrapper.
        LEFT,            //!< First operand of a binary Miniscript combinator.
        RIGHT,           //!< Second operand of a binary Miniscript combinator.
        CONDITION,       //!< First operand of andor().
        IF_TRUE,         //!< True branch of or_i() or andor().
        IF_FALSE,        //!< False branch of or_i() or andor().
        THRESHOLD_INPUT, //!< An ordered input to thresh().
    };

    //! Tapscript leaf; descriptors support no other leaf version.
    struct TapLeaf {
        uint32_t depth;
    };

    struct Child {
        size_t node;
        // Taproot leaves are ordered as in the parsed tree. Their depths encode
        // its binary structure without copying a root-to-leaf path per node.
        std::variant<ChildRole, TapLeaf> role;
    };

    struct Node {
        std::variant<Kind, miniscript::Fragment> kind;
        uint32_t value{0};               //!< Multisig threshold or Miniscript K (timelock/threshold); zero otherwise.
        std::vector<unsigned char> data; //!< Hash bytes, raw script, or addr() scriptPubKey; empty otherwise.
        std::vector<size_t> keys;        //!< Ordered occurrence IDs; sorted multisig retains expression order.
        std::vector<Child> children;
    };

    DescriptorKeyInventory keys;
    std::vector<Node> nodes;
};

#endif // BITCOIN_SCRIPT_DESCRIPTOR_ANALYSIS_H
