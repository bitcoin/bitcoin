// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit.

#include <script/descriptor_analyzer.h>

#include <hash.h>
#include <pubkey.h>
#include <script/descriptor.h>
#include <util/bip32.h>
#include <util/check.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <numeric>
#include <optional>
#include <ranges>
#include <utility>
#include <variant>
#include <vector>

namespace {

using Kind = DescriptorPolicy::Kind;
using IssueKind = DescriptorAnalysis::Issue::Kind;

/** Root public key of a key occurrence, compared without parity: a holder of
 *  the private key for P can also sign for -P. */
std::optional<XOnlyPubKey> RootKey(const DescriptorKeyInfo::Node& node)
{
    if (const auto* key{std::get_if<DescriptorKeyInfo::BIP32Key>(&node.key)}) return XOnlyPubKey{key->root.pubkey};
    if (const auto* key{std::get_if<DescriptorKeyInfo::ConstKey>(&node.key)}) return XOnlyPubKey{key->pubkey};
    return std::nullopt;
}

bool HasHardenedStep(const DescriptorKeyInfo::Derivation& derivation)
{
    return std::ranges::any_of(derivation.path, [](uint32_t step) { return step >> 31; });
}

//! Whether deriving from the key needs its private key.
bool NeedsPrivateDerivation(const DescriptorKeyInfo::BIP32Key& key)
{
    return key.derivation.type == DescriptorKeyInfo::DeriveType::HARDENED_RANGED || HasHardenedStep(key.derivation);
}

/** Whether `child` is the unhardened child of `parent` that its metadata
 *  claims. Metadata can hide a child, but deriving it can't fake one. A
 *  hardened child can't be derived from `parent`, so it is never found. */
bool IsDirectChild(const CExtPubKey& parent, const CExtPubKey& child)
{
    if (child.fingerprint != parent.id_key_fingerprint()) return false;
    // Deriving a hardened child needs the private key.
    if (child.nChild & BIP32_HARDENED_FLAG) return false;
    CExtPubKey derived;
    return parent.Derive(derived, child.nChild) && derived.pubkey == child.pubkey && derived.chaincode == child.chaincode;
}

//! Root extended public key of a key occurrence, if it has one.
const CExtPubKey* RootExtKey(const DescriptorKeyInfo::Node& node)
{
    const auto* key{std::get_if<DescriptorKeyInfo::BIP32Key>(&node.key)};
    return key ? &key->root : nullptr;
}

const Kind* NodeKind(const DescriptorPolicy::Node& node)
{
    return std::get_if<Kind>(&node.kind);
}

/** Check that every cosigner slot can derive addresses on its own. */
void CheckCosignerKeys(DescriptorAnalysis& result)
{
    for (const size_t id : result.cosigners) {
        const auto& node{result.policy.keys.occurrences.at(id).expression};
        const auto* key{std::get_if<DescriptorKeyInfo::BIP32Key>(&node.key)};
        if (!key) {
            result.issues.push_back({IssueKind::NOT_EXTENDED_KEY, {id}});
        } else if (NeedsPrivateDerivation(*key)) {
            result.issues.push_back({IssueKind::HARDENED_DERIVATION, {id}});
        } else if (key->derivation.type != DescriptorKeyInfo::DeriveType::UNHARDENED_RANGED) {
            result.issues.push_back({IssueKind::NOT_RANGED, {id}});
        }
    }
}

/** Public key a cosigner slot derives at the first address, if it derives
 *  without private keys. */
std::optional<XOnlyPubKey> FirstDerivedKey(const DescriptorKeyInfo::Node& node)
{
    const auto* key{std::get_if<DescriptorKeyInfo::BIP32Key>(&node.key)};
    if (!key || key->derivation.type != DescriptorKeyInfo::DeriveType::UNHARDENED_RANGED || HasHardenedStep(key->derivation)) return std::nullopt;
    CExtPubKey derived{key->root};
    for (const uint32_t step : key->derivation.path) {
        if (!derived.Derive(derived, step)) return std::nullopt;
    }
    if (!derived.Derive(derived, 0)) return std::nullopt;
    return XOnlyPubKey{derived.pubkey};
}

/** Report every group of cosigner slots held by the same key: slots that share
 *  a root key (e.g. the same xpub with two paths), that derive the same key, or
 *  where one xpub is the direct child of another. Slots linked through a chain
 *  of these are one group. */
void CheckRepeatedKeys(DescriptorAnalysis& result)
{
    const auto& occurrences{result.policy.keys.occurrences};
    const size_t count{result.cosigners.size()};
    std::vector<std::optional<XOnlyPubKey>> roots, derived;
    std::vector<const CExtPubKey*> ext_roots;
    roots.reserve(count);
    derived.reserve(count);
    ext_roots.reserve(count);
    for (const size_t id : result.cosigners) {
        const auto& node{occurrences.at(id).expression};
        roots.push_back(RootKey(node));
        derived.push_back(FirstDerivedKey(node));
        ext_roots.push_back(RootExtKey(node));
    }
    const auto same_holder = [&](size_t i, size_t j) {
        if ((roots[i] && roots[i] == roots[j]) || (derived[i] && derived[i] == derived[j])) return true;
        return ext_roots[i] && ext_roots[j] && (IsDirectChild(*ext_roots[i], *ext_roots[j]) || IsDirectChild(*ext_roots[j], *ext_roots[i]));
    };

    // Each slot points to an earlier slot of its group, the first one pointing to itself.
    std::vector<size_t> parent(count);
    std::iota(parent.begin(), parent.end(), size_t{0});
    const auto first_of = [&](size_t i) {
        while (parent[i] != i) i = parent[i];
        return i;
    };
    for (size_t i{0}; i < count; ++i) {
        for (size_t j{i + 1}; j < count; ++j) {
            if (!same_holder(i, j)) continue;
            const size_t a{first_of(i)}, b{first_of(j)};
            parent[std::max(a, b)] = std::min(a, b);
        }
    }

    std::vector<std::vector<size_t>> groups(count);
    for (size_t i{0}; i < count; ++i) groups[first_of(i)].push_back(result.cosigners[i]);
    for (auto& group : groups) {
        if (group.size() > 1) result.issues.push_back({IssueKind::REPEATED_KEY, std::move(group)});
    }
}

/** Check that every cosigner can derive each participant of a musig()
 *  internal key, and that no participant is repeated. */
void CheckMuSigParticipants(DescriptorAnalysis& result, size_t internal_id)
{
    const auto& occurrences{result.policy.keys.occurrences};
    const auto* musig{std::get_if<DescriptorKeyInfo::MuSigKey>(&occurrences.at(internal_id).expression.key)};
    if (!musig) return;
    std::vector<std::pair<XOnlyPubKey, std::vector<size_t>>> groups;
    for (const size_t id : musig->participants) {
        const auto& participant{occurrences.at(id).expression};
        // Other cosigners couldn't derive the key path.
        if (const auto* key{std::get_if<DescriptorKeyInfo::BIP32Key>(&participant.key)}; key && NeedsPrivateDerivation(*key)) {
            result.issues.push_back({IssueKind::HARDENED_DERIVATION, {id}});
        }
        const auto root{RootKey(participant)};
        if (!root) continue;
        const auto group{std::ranges::find(groups, *root, &decltype(groups)::value_type::first)};
        if (group == groups.end()) {
            groups.push_back({*root, {id}});
        } else {
            group->second.push_back(id);
        }
    }
    for (auto& [root, ids] : groups) {
        if (ids.size() > 1) result.issues.push_back({IssueKind::REPEATED_KEY, std::move(ids)});
    }
}

/** Classify the Taproot internal key against the cosigners of the leaf, or
 *  nullopt if it may let fewer than `threshold` cosigners spend. */
std::optional<DescriptorAnalysis::KeyPath> ClassifyKeyPath(const DescriptorAnalysis& result, size_t internal_id)
{
    const auto& occurrences{result.policy.keys.occurrences};
    const auto& internal{occurrences.at(internal_id).expression};

    if (const auto* key{std::get_if<DescriptorKeyInfo::ConstKey>(&internal.key)}) {
        if (XOnlyPubKey{key->pubkey} == XOnlyPubKey::NUMS_H) return DescriptorAnalysis::KeyPath::NUMS;
    }
    if (const auto* key{std::get_if<DescriptorKeyInfo::BIP32Key>(&internal.key)}) {
        // Unhardened children of H are as unspendable as H.
        if (XOnlyPubKey{key->root.pubkey} == XOnlyPubKey::NUMS_H && !HasHardenedStep(key->derivation) &&
            key->derivation.type != DescriptorKeyInfo::DeriveType::HARDENED_RANGED) {
            return DescriptorAnalysis::KeyPath::NUMS;
        }
    }
    if (const auto* key{std::get_if<DescriptorKeyInfo::MuSigKey>(&internal.key)}) {
        // Any `threshold` cosigners can already spend through the leaf, so an
        // aggregate of at least that many distinct cosigners adds no new way to spend.
        std::vector<XOnlyPubKey> cosigner_roots;
        for (const size_t id : result.cosigners) {
            if (const auto root{RootKey(occurrences.at(id).expression)}) cosigner_roots.push_back(*root);
        }
        std::vector<XOnlyPubKey> participants;
        for (const size_t id : key->participants) {
            const auto root{RootKey(occurrences.at(id).expression)};
            if (!root || std::ranges::find(cosigner_roots, *root) == cosigner_roots.end()) return std::nullopt;
            // A repeated participant adds no cosigner.
            if (std::ranges::find(participants, *root) == participants.end()) participants.push_back(*root);
        }
        if (participants.size() >= result.threshold) return DescriptorAnalysis::KeyPath::MUSIG;
    }
    return std::nullopt;
}

/** Mark the result unrecognized and drop the setup it would have claimed. */
void Unrecognize(DescriptorAnalysis& result)
{
    result.shape = DescriptorAnalysis::Shape::UNRECOGNIZED;
    result.sorted = false;
    result.key_path = DescriptorAnalysis::KeyPath::NONE;
    result.threshold = 0;
    result.cosigners.clear();
}

/** Find the multisig node of a recognized shape, or nullptr. */
const DescriptorPolicy::Node* FindMultisig(DescriptorAnalysis& result)
{
    const auto& nodes{result.policy.nodes};
    if (nodes.empty()) return nullptr;
    const auto* root_kind{NodeKind(nodes[0])};
    if (!root_kind || nodes[0].children.size() != 1) return nullptr;
    const auto& child{nodes[0].children[0]};
    const auto& multisig{nodes.at(child.node)};
    const auto* kind{NodeKind(multisig)};
    if (!kind || !multisig.children.empty()) return nullptr;

    if (*root_kind == Kind::WSH && (*kind == Kind::MULTI || *kind == Kind::SORTEDMULTI)) {
        result.shape = DescriptorAnalysis::Shape::WSH_MULTI;
        result.sorted = *kind == Kind::SORTEDMULTI;
        return &multisig;
    }
    if (*root_kind == Kind::TR && (*kind == Kind::MULTI_A || *kind == Kind::SORTEDMULTI_A)) {
        const auto* leaf{std::get_if<DescriptorPolicy::TapLeaf>(&child.role)};
        if (!leaf || leaf->depth != 0 || nodes[0].keys.size() != 1) return nullptr;
        result.shape = DescriptorAnalysis::Shape::TR_MULTI_A;
        result.sorted = *kind == Kind::SORTEDMULTI_A;
        return &multisig;
    }
    return nullptr;
}

} // namespace

DescriptorAnalysis AnalyzeDescriptor(const Descriptor& descriptor)
{
    DescriptorAnalysis result;
    result.policy = descriptor.GetPolicy();

    const auto* multisig{FindMultisig(result)};
    if (!multisig) {
        result.issues.push_back({IssueKind::UNSUPPORTED_SHAPE, {}});
        return result;
    }
    result.threshold = multisig->value;
    result.cosigners = multisig->keys;

    CheckCosignerKeys(result);
    CheckRepeatedKeys(result);
    if (result.shape == DescriptorAnalysis::Shape::TR_MULTI_A) {
        const size_t internal_id{result.policy.nodes[0].keys[0]};
        CheckMuSigParticipants(result, internal_id);
        if (const auto key_path{ClassifyKeyPath(result, internal_id)}) {
            result.key_path = *key_path;
        } else {
            result.issues.push_back({IssueKind::KEY_PATH_SPENDABLE, {internal_id}});
        }
    }

    // Checked last: the checks above need the cosigners.
    if (!result.issues.empty()) Unrecognize(result);
    return result;
}

DescriptorAnalysis AnalyzeDescriptor(std::span<const std::unique_ptr<Descriptor>> descriptors)
{
    Assume(!descriptors.empty());
    if (descriptors.empty()) return {};
    DescriptorAnalysis first{AnalyzeDescriptor(*descriptors.front())};

    const auto cosigner_roots = [](const DescriptorAnalysis& analysis) {
        std::vector<std::optional<XOnlyPubKey>> roots;
        roots.reserve(analysis.cosigners.size());
        for (const size_t id : analysis.cosigners) roots.push_back(RootKey(analysis.policy.keys.occurrences.at(id).expression));
        return roots;
    };
    for (const auto& descriptor : descriptors.subspan(1)) {
        const DescriptorAnalysis other{AnalyzeDescriptor(*descriptor)};
        if (other.shape != first.shape || other.sorted != first.sorted || other.key_path != first.key_path ||
            other.threshold != first.threshold || cosigner_roots(other) != cosigner_roots(first)) {
            Unrecognize(first);
            first.issues.push_back({DescriptorAnalysis::Issue::Kind::BRANCHES_DIFFER, {}});
            break;
        }
    }
    return first;
}
