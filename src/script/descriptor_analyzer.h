// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit.

#ifndef BITCOIN_SCRIPT_DESCRIPTOR_ANALYZER_H
#define BITCOIN_SCRIPT_DESCRIPTOR_ANALYZER_H

#include <script/descriptor_analysis.h>

#include <compare>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

struct Descriptor;

/** Whether a parsed descriptor is a recognized k-of-n multisig setup.
 *
 * This is the part of a multisig setup check that needs no wallet: it looks at
 * the descriptor's structure only. It does not say which key belongs to whom.
 * A recognized shape is one where any `threshold` of the `cosigners` can spend,
 * and nothing else can. Anything else is reported as unrecognized, together
 * with the reasons, and no setup is claimed for it: `sorted`, `key_path`,
 * `threshold` and `cosigners` keep their defaults, and each issue lists the
 * keys involved instead.
 *
 * Recognized shapes:
 * - wsh(multi(k,...)) and wsh(sortedmulti(k,...))
 * - tr(K,multi_a(k,...)) and tr(K,sortedmulti_a(k,...)) with a single leaf,
 *   where K is the NUMS point H from BIP 341, or a musig() of at least k
 *   distinct cosigners.
 * Every cosigner must be an extended key other than H, with an unhardened
 * ranged derivation that can derive addresses, and no two cosigners may be
 * found to be the same key: the same root key, the same key at the first
 * address, or an xpub and its unhardened child xpub. Other related keys (e.g.
 * a grandchild xpub) can't be found from the descriptor alone, and neither can
 * other keys no one can sign for. Private keys in the descriptor are not
 * looked at: whoever holds one can also sign for its key.
 */
struct DescriptorAnalysis {
    enum class Shape {
        UNRECOGNIZED,
        WSH_MULTI,
        TR_MULTI_A,
    };

    //! How a Taproot output can be spent without the script path.
    enum class KeyPath {
        NONE,  //!< Not a recognized Taproot shape.
        NUMS,  //!< Provably unspendable internal key.
        MUSIG, //!< Aggregate of at least `threshold` distinct cosigners.
    };

    struct Issue {
        enum class Kind {
            UNSUPPORTED_SHAPE,     //!< Not one of the recognized shapes.
            REPEATED_KEY,          //!< Cosigner slots, or musig() participants, held by the same key.
            KEY_PATH_SPENDABLE,    //!< Taproot internal key is not NUMS or a musig() of cosigners.
            NOT_EXTENDED_KEY,      //!< A cosigner is not an extended key.
            NOT_RANGED,            //!< A cosigner does not end in an unhardened wildcard.
            HARDENED_DERIVATION,   //!< A cosigner or musig() participant needs a private key to derive.
            NOT_DERIVABLE,         //!< A cosigner, musig() participant or Taproot internal key can't derive its keys, e.g. past the maximum BIP32 depth.
            UNSPENDABLE_KEY,       //!< A cosigner is the NUMS point H, which no one can sign for.
            BRANCHES_DIFFER,       //!< Multipath branches are not the same setup.
        };
        Kind kind;
        //! Occurrence IDs in `policy.keys` involved, if any.
        std::vector<size_t> keys;

        friend auto operator<=>(const Issue&, const Issue&) = default;
    };

    Shape shape{Shape::UNRECOGNIZED};
    bool sorted{false};
    KeyPath key_path{KeyPath::NONE};
    uint32_t threshold{0};
    //! Occurrence IDs in `policy.keys` of the cosigner slots, in expression order.
    std::vector<size_t> cosigners;
    std::vector<Issue> issues;
    //! The structure the analysis refers to.
    DescriptorPolicy policy;

    bool IsRecognized() const { return shape != Shape::UNRECOGNIZED; }
};

/** Analyze one parsed descriptor. */
DescriptorAnalysis AnalyzeDescriptor(const Descriptor& descriptor);

/** Analyze the descriptors returned by Parse() for one descriptor string.
 *
 * A multipath descriptor is parsed into one descriptor per path. Each is
 * analyzed, and the result is recognized only if every branch is the same
 * recognized setup: same shape, threshold, key path and cosigner root keys.
 * The returned analysis is the first branch's, with the issues of every
 * branch, each once.
 */
DescriptorAnalysis AnalyzeDescriptor(std::span<const std::unique_ptr<Descriptor>> descriptors);

#endif // BITCOIN_SCRIPT_DESCRIPTOR_ANALYZER_H
