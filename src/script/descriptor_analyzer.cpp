// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit.

#include <script/descriptor_analyzer.h>

#include <pubkey.h>
#include <script/descriptor.h>
#include <util/bip32.h>
#include <util/check.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <limits>
#include <map>
#include <numeric>
#include <optional>
#include <set>
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

/** Whether `child` has the key of the unhardened child of `parent` that its
 *  metadata claims. Metadata can hide a child, but deriving it can't fake one.
 *  As for root keys, the chain code and parity don't matter: whoever holds the
 *  parent's private key holds the child's. A hardened child can't be derived
 *  from `parent`, so it is never found. */
bool IsDirectChild(const CExtPubKey& parent, const CExtPubKey& child)
{
    if (child.fingerprint != parent.id_key_fingerprint()) return false;
    // Deriving a hardened child needs the private key.
    if (child.nChild & BIP32_HARDENED_FLAG) return false;
    CExtPubKey derived;
    return parent.Derive(derived, child.nChild) && XOnlyPubKey{derived.pubkey} == XOnlyPubKey{child.pubkey};
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

/** Public key an extended key derives at index 0, if it can be derived
 *  without private keys. Deriving fails past the maximum BIP32 depth. */
std::optional<XOnlyPubKey> DeriveFirstKey(const DescriptorKeyInfo::BIP32Key& key)
{
    if (NeedsPrivateDerivation(key)) return std::nullopt;
    CExtPubKey derived{key.root};
    for (const uint32_t step : key.derivation.path) {
        if (!derived.Derive(derived, step)) return std::nullopt;
    }
    if (key.derivation.type == DescriptorKeyInfo::DeriveType::UNHARDENED_RANGED && !derived.Derive(derived, 0)) return std::nullopt;
    return XOnlyPubKey{derived.pubkey};
}

/** Check that every cosigner slot can derive addresses on its own, and that
 *  someone can sign for it. */
void CheckCosignerKeys(DescriptorAnalysis& result)
{
    for (const size_t id : result.cosigners) {
        const auto& node{result.policy.keys.occurrences.at(id).expression};
        const auto* key{std::get_if<DescriptorKeyInfo::BIP32Key>(&node.key)};
        if (RootKey(node) == XOnlyPubKey::NUMS_H) {
            result.issues.push_back({IssueKind::UNSPENDABLE_KEY, {id}});
        } else if (!key) {
            result.issues.push_back({IssueKind::NOT_EXTENDED_KEY, {id}});
        } else if (NeedsPrivateDerivation(*key)) {
            result.issues.push_back({IssueKind::HARDENED_DERIVATION, {id}});
        } else if (key->derivation.type != DescriptorKeyInfo::DeriveType::UNHARDENED_RANGED) {
            result.issues.push_back({IssueKind::NOT_RANGED, {id}});
        } else if (!DeriveFirstKey(*key)) {
            result.issues.push_back({IssueKind::NOT_DERIVABLE, {id}});
        }
    }
}

/** Public key a cosigner slot derives at the first address, if it derives
 *  without private keys. */
std::optional<XOnlyPubKey> FirstDerivedKey(const DescriptorKeyInfo::Node& node)
{
    const auto* key{std::get_if<DescriptorKeyInfo::BIP32Key>(&node.key)};
    if (!key || key->derivation.type != DescriptorKeyInfo::DeriveType::UNHARDENED_RANGED) return std::nullopt;
    return DeriveFirstKey(*key);
}

/** Indices below a count, grouped by links: two indices are in one group if
 *  a chain of links connects them. */
class LinkedGroups
{
    //! Each index points to an earlier index of its group, the first one pointing to itself.
    std::vector<size_t> m_parent;

public:
    explicit LinkedGroups(size_t count) : m_parent(count)
    {
        std::iota(m_parent.begin(), m_parent.end(), size_t{0});
    }

    //! The first index of `i`'s group.
    size_t First(size_t i)
    {
        // Point each index visited to its grandparent, keeping later searches short.
        while (m_parent[i] != i) i = m_parent[i] = m_parent[m_parent[i]];
        return i;
    }

    void Link(size_t i, size_t j)
    {
        const size_t a{First(i)}, b{First(j)};
        m_parent[std::max(a, b)] = std::min(a, b);
    }

    //! Link the indices with equal values. A missing value links nothing.
    template <typename T>
    void LinkEqual(const std::vector<std::optional<T>>& values)
    {
        std::map<T, size_t> first;
        for (size_t i{0}; i < values.size(); ++i) {
            if (!values[i]) continue;
            const auto [it, inserted]{first.try_emplace(*values[i], i)};
            if (!inserted) Link(it->second, i);
        }
    }

    //! Groups of at least two indices, each in index order, listed by its first index.
    std::vector<std::vector<size_t>> Repeated()
    {
        std::vector<std::vector<size_t>> groups(m_parent.size());
        for (size_t i{0}; i < m_parent.size(); ++i) groups[First(i)].push_back(i);
        std::erase_if(groups, [](const auto& group) { return group.size() < 2; });
        return groups;
    }
};

/** Report each group of `groups` as one REPEATED_KEY issue, with the occurrence
 *  IDs `ids` gives for its indices. */
void ReportRepeated(DescriptorAnalysis& result, LinkedGroups& groups, const std::vector<size_t>& ids)
{
    for (const auto& group : groups.Repeated()) {
        std::vector<size_t> keys;
        keys.reserve(group.size());
        for (const size_t i : group) keys.push_back(ids[i]);
        result.issues.push_back({IssueKind::REPEATED_KEY, std::move(keys)});
    }
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
    LinkedGroups groups{count};
    groups.LinkEqual(roots);
    groups.LinkEqual(derived);

    // Only an xpub whose fingerprint a child names can be its parent, so a
    // child is derived again only from those, and not when the two slots are
    // already in one group. Xpubs sharing a root key are one group already, and
    // can share a fingerprint with any number of chain codes, so only the first
    // of them is a candidate parent: a child of a later one is left out of the
    // group, which is reported either way.
    std::multimap<KeyFingerprint, size_t> by_fingerprint;
    std::set<XOnlyPubKey> parent_roots;
    for (size_t i{0}; i < count; ++i) {
        if (ext_roots[i] && parent_roots.insert(*roots[i]).second) by_fingerprint.emplace(ext_roots[i]->id_key_fingerprint(), i);
    }
    for (size_t child{0}; child < count; ++child) {
        if (!ext_roots[child]) continue;
        const auto [begin, end]{by_fingerprint.equal_range(ext_roots[child]->fingerprint)};
        for (auto it{begin}; it != end; ++it) {
            const size_t parent{it->second};
            if (groups.First(parent) != groups.First(child) && IsDirectChild(*ext_roots[parent], *ext_roots[child])) {
                groups.Link(parent, child);
            }
        }
    }
    ReportRepeated(result, groups, result.cosigners);
}

/** Check that every cosigner can derive each participant of a musig()
 *  internal key, and that no participant is repeated: participants with the
 *  same root key, or that derive the same key, are one. */
void CheckMuSigParticipants(DescriptorAnalysis& result, size_t internal_id)
{
    const auto& occurrences{result.policy.keys.occurrences};
    const auto* musig{std::get_if<DescriptorKeyInfo::MuSigKey>(&occurrences.at(internal_id).expression.key)};
    if (!musig) return;
    const auto& participants{musig->participants};
    std::vector<std::optional<XOnlyPubKey>> roots, derived;
    roots.reserve(participants.size());
    derived.reserve(participants.size());
    for (const size_t id : participants) {
        const auto& participant{occurrences.at(id).expression};
        roots.push_back(RootKey(participant));
        derived.push_back(roots.back());
        const auto* key{std::get_if<DescriptorKeyInfo::BIP32Key>(&participant.key)};
        if (!key) continue;
        derived.back() = DeriveFirstKey(*key);
        if (NeedsPrivateDerivation(*key)) {
            // Other cosigners couldn't derive the key path.
            result.issues.push_back({IssueKind::HARDENED_DERIVATION, {id}});
        } else if (!derived.back()) {
            result.issues.push_back({IssueKind::NOT_DERIVABLE, {id}});
        }
    }
    LinkedGroups groups{participants.size()};
    groups.LinkEqual(roots);
    groups.LinkEqual(derived);
    ReportRepeated(result, groups, participants);
}

/** Whether a NUMS or musig() internal key can derive its first key. Deriving
 *  fails past the maximum BIP32 depth, and a musig() aggregate starts as an
 *  xpub at depth 0. */
bool CanDeriveInternalKey(const DescriptorKeyInfo::Node& node)
{
    if (const auto* key{std::get_if<DescriptorKeyInfo::BIP32Key>(&node.key)}) return DeriveFirstKey(*key).has_value();
    if (const auto* key{std::get_if<DescriptorKeyInfo::MuSigKey>(&node.key)}) {
        const bool ranged{key->derivation.type == DescriptorKeyInfo::DeriveType::UNHARDENED_RANGED};
        return key->derivation.path.size() + (ranged ? 1 : 0) <= std::numeric_limits<unsigned char>::max();
    }
    return true;
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
        std::set<XOnlyPubKey> cosigner_roots;
        for (const size_t id : result.cosigners) {
            if (const auto root{RootKey(occurrences.at(id).expression)}) cosigner_roots.insert(*root);
        }
        // A repeated participant adds no cosigner.
        std::set<XOnlyPubKey> participants;
        for (const size_t id : key->participants) {
            const auto root{RootKey(occurrences.at(id).expression)};
            if (!root || !cosigner_roots.contains(*root)) return std::nullopt;
            participants.insert(*root);
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
            if (!CanDeriveInternalKey(result.policy.keys.occurrences.at(internal_id).expression)) {
                result.issues.push_back({IssueKind::NOT_DERIVABLE, {internal_id}});
            }
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
    // The branches share their key IDs, so each issue is reported once,
    // whichever branch it comes from.
    std::set<DescriptorAnalysis::Issue> seen(first.issues.begin(), first.issues.end());
    bool differ{false};
    for (const auto& descriptor : descriptors.subspan(1)) {
        const DescriptorAnalysis other{AnalyzeDescriptor(*descriptor)};
        differ |= other.shape != first.shape || other.sorted != first.sorted || other.key_path != first.key_path ||
                  other.threshold != first.threshold || cosigner_roots(other) != cosigner_roots(first);
        for (const auto& issue : other.issues) {
            if (seen.insert(issue).second) first.issues.push_back(issue);
        }
    }
    if (differ) first.issues.push_back({IssueKind::BRANCHES_DIFFER, {}});
    if (!first.issues.empty()) Unrecognize(first);
    return first;
}
