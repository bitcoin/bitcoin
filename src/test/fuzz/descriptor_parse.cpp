// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <key_io.h>
#include <pubkey.h>
#include <script/descriptor.h>
#include <script/descriptor_analyzer.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util/descriptor.h>
#include <util/chaintype.h>
#include <util/check.h>
#include <util/strencodings.h>

#include <memory>
#include <span>

//! The converter of mocked descriptors, needs to be initialized when the target is.
MockedDescriptorConverter MOCKED_DESC_CONVERTER;

/** Check the multisig analysis of the descriptors parsed from one string. */
static void TestAnalysis(std::span<const std::unique_ptr<Descriptor>> descs)
{
    if (descs.empty()) return;
    const auto analysis{AnalyzeDescriptor(descs)};
    const size_t num_keys{analysis.policy.keys.occurrences.size()};
    if (analysis.IsRecognized()) {
        assert(analysis.issues.empty());
        assert(analysis.threshold >= 1 && analysis.threshold <= analysis.cosigners.size());
    } else {
        // An unrecognized descriptor claims no setup.
        assert(!analysis.issues.empty());
        assert(!analysis.sorted && analysis.key_path == DescriptorAnalysis::KeyPath::NONE);
        assert(analysis.threshold == 0 && analysis.cosigners.empty());
    }
    for (const size_t id : analysis.cosigners) assert(id < num_keys);
    for (const auto& issue : analysis.issues) {
        for (const size_t id : issue.keys) assert(id < num_keys);
    }
}

/** Check the public policy projection, including inferred key indices. */
static void TestPolicy(const Descriptor& desc)
{
    const auto policy{desc.GetPolicy()};
    assert(!policy.nodes.empty());
    assert(policy.keys.occurrences.size() == desc.GetKeyCount());
    std::vector<size_t> parents(policy.nodes.size(), 0), key_uses(policy.keys.occurrences.size(), 0);
    using Kind = DescriptorPolicy::Kind;
    using Role = DescriptorKeyInventory::Role;
    for (size_t i = 0; i < policy.nodes.size(); ++i) {
        const auto& node{policy.nodes[i]};
        const auto* kind{std::get_if<Kind>(&node.kind)};
        for (const auto& child : node.children) {
            assert(child.node > i && child.node < parents.size());
            ++parents[child.node];
            // Taproot leaves only under tr(), scripts only under sh() or wsh(),
            // and Miniscript operand roles only under Miniscript fragments.
            if (std::holds_alternative<DescriptorPolicy::TapLeaf>(child.role)) {
                assert(kind && *kind == Kind::TR);
            } else if (std::get<DescriptorPolicy::ChildRole>(child.role) == DescriptorPolicy::ChildRole::SCRIPT) {
                assert(kind && (*kind == Kind::SH || *kind == Kind::WSH));
            } else {
                assert(!kind);
            }
        }
        assert(parents[i] == (i == 0 ? 0U : 1U));
        for (const auto key : node.keys) {
            assert(key < key_uses.size());
            ++key_uses[key];
            const Role role{policy.keys.occurrences[key].role};
            assert((role == Role::TAPROOT_INTERNAL) == (kind && *kind == Kind::TR));
            assert((role == Role::TAPROOT_OUTPUT) == (kind && *kind == Kind::RAWTR));
        }
    }
    const auto& occurrences{policy.keys.occurrences};
    std::vector<size_t> key_parents(occurrences.size(), 0);
    for (size_t i = 0; i < occurrences.size(); ++i) {
        if (const auto* musig = std::get_if<DescriptorKeyInfo::MuSigKey>(&occurrences[i].expression.key)) {
            for (const auto participant : musig->participants) {
                assert(participant > i && participant < occurrences.size());
                ++key_parents[participant];
            }
        }
    }
    for (size_t i = 0; i < occurrences.size(); ++i) {
        const bool participant{occurrences[i].role == DescriptorKeyInventory::Role::MUSIG_PARTICIPANT};
        assert(key_parents[i] == (participant ? 1U : 0U));
        assert(key_uses[i] == (participant ? 0U : 1U));
    }
}

/** Test a successfully parsed descriptor. */
static void TestDescriptor(const Descriptor& desc, FlatSigningProvider& sig_provider, std::string& dummy, std::optional<bool>& is_ranged, std::optional<bool>& is_solvable)
{
    // Trivial helpers.
    (void)desc.IsRange();
    (void)desc.IsSingleType();
    (void)desc.GetOutputType();
    TestPolicy(desc);

    if (is_ranged.has_value()) {
        assert(desc.IsRange() == *is_ranged);
    } else {
        is_ranged = desc.IsRange();
    }
    if (is_solvable.has_value()) {
        assert(desc.IsSolvable() == *is_solvable);
    } else {
        is_solvable = desc.IsSolvable();
    }

    // Serialization to string representation.
    (void)desc.ToString();
    (void)desc.ToPrivateString(sig_provider, dummy);
    (void)desc.ToNormalizedString(sig_provider, dummy);

    // Serialization to Script.
    DescriptorCache cache;
    std::vector<CScript> out_scripts;
    (void)desc.Expand(0, sig_provider, out_scripts, sig_provider, &cache);
    (void)desc.ExpandPrivate(0, sig_provider, sig_provider);
    (void)desc.ExpandFromCache(0, cache, out_scripts, sig_provider);

    // If we could serialize to script we must be able to infer using the same provider.
    if (!out_scripts.empty()) {
        const auto inferred{InferDescriptor(out_scripts.back(), sig_provider)};
        assert(inferred);
        TestPolicy(*inferred);

        // The ScriptSize() must match the size of the serialized Script. (ScriptSize() is set for all descs but 'combo()'.)
        const bool is_combo{!desc.IsSingleType()};
        assert(is_combo || desc.ScriptSize() == out_scripts.back().size());
    }

    const auto max_sat_maxsig{desc.MaxSatisfactionWeight(true)};
    const auto max_sat_nonmaxsig{desc.MaxSatisfactionWeight(false)};
    // Whether an estimate is available must not depend on the signature-size
    // assumption, and assuming non-max-size signatures must never increase it.
    assert(max_sat_maxsig.has_value() == max_sat_nonmaxsig.has_value());
    assert(max_sat_nonmaxsig <= max_sat_maxsig);
    const auto max_elems{desc.MaxSatisfactionElems()};
    // We must be able to estimate the max satisfaction size for any solvable descriptor (but combo).
    const bool is_nontop_or_nonsolvable{!*is_solvable || !desc.GetOutputType()};
    const bool is_input_size_info_set{max_sat_maxsig && max_sat_nonmaxsig && max_elems};
    assert(is_input_size_info_set || is_nontop_or_nonsolvable);

    auto max_key_expr = desc.GetMaxKeyExpr();
    auto key_count = desc.GetKeyCount();
    assert((max_key_expr == 0 && key_count == 0) || max_key_expr + 1 == key_count);
}

void initialize_descriptor_parse()
{
    static ECC_Context ecc_context{};
    SelectParams(ChainType::MAIN);
}

void initialize_mocked_descriptor_parse()
{
    initialize_descriptor_parse();
    MOCKED_DESC_CONVERTER.Init();
}

FUZZ_TARGET(mocked_descriptor_parse, .init = initialize_mocked_descriptor_parse)
{
    const std::string mocked_descriptor{buffer.begin(), buffer.end()};
    if (const auto descriptor = MOCKED_DESC_CONVERTER.GetDescriptor(mocked_descriptor)) {
        if (IsTooExpensive(MakeUCharSpan(*descriptor))) return;
        FlatSigningProvider signing_provider;
        std::string error;
        const auto desc = Parse(*descriptor, signing_provider, error);
        std::optional<bool> is_ranged;
        std::optional<bool> is_solvable;
        for (const auto& d : desc) {
            assert(d);
            TestDescriptor(*d, signing_provider, error, is_ranged, is_solvable);
        }
        TestAnalysis(desc);
    }
}

FUZZ_TARGET(descriptor_parse, .init = initialize_descriptor_parse)
{
    if (IsTooExpensive(buffer)) return;

    const std::string descriptor(buffer.begin(), buffer.end());
    FlatSigningProvider signing_provider;
    std::string error;
    for (const bool require_checksum : {true, false}) {
        const auto desc = Parse(descriptor, signing_provider, error, require_checksum);
        std::optional<bool> is_ranged;
        std::optional<bool> is_solvable;
        for (const auto& d : desc) {
            assert(d);
            TestDescriptor(*d, signing_provider, error, is_ranged, is_solvable);
        }
        TestAnalysis(desc);
    }
}
