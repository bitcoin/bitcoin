// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit.

#include <rpc/descriptor_util.h>

#include <crypto/hex_base.h>
#include <key_io.h>
#include <pubkey.h>
#include <rpc/util.h>
#include <script/descriptor_analysis.h>
#include <script/descriptor_analyzer.h>
#include <script/keyorigin.h>
#include <univalue.h>
#include <util/bip32.h>
#include <util/check.h>
#include <util/overloaded.h>

#include <optional>
#include <span>
#include <string>
#include <utility>
#include <variant>
#include <vector>

const std::string HELP_DESCRIPTOR_ANALYSIS{"Whether the descriptor is a recognized k-of-n multisig setup: "
    "one where any threshold of the cosigners can spend, and nothing else can. Recognized shapes are wsh(multi) and wsh(sortedmulti), "
    "and tr(K,multi_a) and tr(K,sortedmulti_a) with a single leaf, where K is the BIP 341 NUMS point H or a musig() of at least threshold distinct cosigners. "
    "Every cosigner must be an extended key with an unhardened ranged derivation, and no two cosigners may be the same key. "
    "This looks at the descriptor only: it does not say which keys belong to whom."};

std::vector<RPCResult> DescriptorKeyResultFields()
{
    return {
        {RPCResult::Type::STR, "role", "How the key is used: \"ecdsa\", \"taproot_internal\", \"taproot_output\", \"tapscript\", \"musig_participant\" or \"unused\""},
        {RPCResult::Type::STR, "type", "The kind of key expression: \"pubkey\", \"bip32\" or \"musig\""},
        {RPCResult::Type::STR_HEX, "pubkey", /*optional=*/true, "For a \"pubkey\": the public key, as 32 bytes if x-only"},
        {RPCResult::Type::STR, "xpub", /*optional=*/true, "For a \"bip32\": the extended public key"},
        {RPCResult::Type::ARR, "participants", /*optional=*/true, "For a \"musig\": the key IDs of its participants, in expression order",
        {
            {RPCResult::Type::NUM, "", "Key ID"},
        }},
        {RPCResult::Type::STR, "derivation", /*optional=*/true, "For a \"bip32\" or \"musig\": the derivation applied to it, if any, e.g. \"/1/*\""},
        {RPCResult::Type::OBJ, "origin", /*optional=*/true, "The key origin, if given",
        {
            {RPCResult::Type::STR_HEX, "master_fingerprint", "The fingerprint of the master key"},
            {RPCResult::Type::STR, "path", "The derivation path from the master key"},
        }},
    };
}

std::vector<RPCResult> DescriptorAnalysisResultFields()
{
    return {
        {RPCResult::Type::BOOL, "recognized", "Whether the descriptor is a recognized setup. If false, only the issues are returned"},
        {RPCResult::Type::STR, "shape", /*optional=*/true, "The setup: \"wsh_multi\" or \"tr_multi_a\""},
        {RPCResult::Type::BOOL, "sorted", /*optional=*/true, "Whether the keys are sorted (sortedmulti or sortedmulti_a)"},
        {RPCResult::Type::STR, "key_path", /*optional=*/true, "For \"tr_multi_a\": the Taproot internal key, \"nums\" (unspendable) or \"musig\" (an aggregate of cosigners)"},
        {RPCResult::Type::NUM, "threshold", /*optional=*/true, "The number of cosigners needed to spend"},
        {RPCResult::Type::ARR, "cosigners", /*optional=*/true, "The key IDs of the cosigner slots, in expression order",
        {
            {RPCResult::Type::NUM, "", "Key ID"},
        }},
        {RPCResult::Type::ARR, "issues", "Why the descriptor is not recognized. Empty if it is",
        {
            {RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "type", "\"unsupported_shape\" (not a recognized shape), "
                    "\"repeated_key\" (cosigner slots, or musig() participants, held by the same key), "
                    "\"key_path_spendable\" (the Taproot internal key may let fewer than threshold cosigners spend), "
                    "\"not_extended_key\" (a cosigner is not an extended key), "
                    "\"not_ranged\" (a cosigner does not end in an unhardened wildcard), "
                    "\"hardened_derivation\" (a cosigner or musig() participant needs a private key to derive addresses), or "
                    "\"branches_differ\" (the paths of a multipath descriptor are not the same setup)"},
                {RPCResult::Type::ARR, "keys", "The key IDs involved, if any",
                {
                    {RPCResult::Type::NUM, "", "Key ID"},
                }},
            }},
        }},
    };
}

static std::string KeyRoleName(DescriptorKeyInventory::Role role)
{
    using Role = DescriptorKeyInventory::Role;
    switch (role) {
    case Role::ECDSA: return "ecdsa";
    case Role::TAPROOT_INTERNAL: return "taproot_internal";
    case Role::TAPROOT_OUTPUT: return "taproot_output";
    case Role::TAPSCRIPT: return "tapscript";
    case Role::MUSIG_PARTICIPANT: return "musig_participant";
    case Role::UNUSED: return "unused";
    } // no default case, so the compiler can warn about missing cases
    NONFATAL_UNREACHABLE();
}

//! Format a derivation in descriptor syntax, e.g. "/1/*". Empty if there is none.
static std::string FormatDerivation(const DescriptorKeyInfo::Derivation& derivation)
{
    std::string result{FormatHDKeypath(derivation.path)};
    switch (derivation.type) {
    case DescriptorKeyInfo::DeriveType::NON_RANGED: return result;
    case DescriptorKeyInfo::DeriveType::UNHARDENED_RANGED: return result + "/*";
    case DescriptorKeyInfo::DeriveType::HARDENED_RANGED: return result + "/*h";
    } // no default case, so the compiler can warn about missing cases
    NONFATAL_UNREACHABLE();
}

UniValue IdsToUniv(const std::vector<size_t>& ids)
{
    UniValue result(UniValue::VARR);
    for (const size_t id : ids) {
        result.push_back(id);
    }
    return result;
}

UniValue DescriptorKeysToUniv(const DescriptorKeyInventory& inventory)
{
    UniValue keys(UniValue::VARR);
    for (const auto& occurrence : inventory.occurrences) {
        UniValue key(UniValue::VOBJ);
        key.pushKV("role", KeyRoleName(occurrence.role));
        std::string derivation;
        std::visit(util::Overloaded{
            [&](const DescriptorKeyInfo::ConstKey& constant) {
                key.pushKV("type", "pubkey");
                key.pushKV("pubkey", constant.xonly ? HexStr(XOnlyPubKey{constant.pubkey}) : HexStr(constant.pubkey));
            },
            [&](const DescriptorKeyInfo::BIP32Key& bip32) {
                key.pushKV("type", "bip32");
                key.pushKV("xpub", EncodeExtPubKey(bip32.root));
                derivation = FormatDerivation(bip32.derivation);
            },
            [&](const DescriptorKeyInfo::MuSigKey& musig) {
                key.pushKV("type", "musig");
                key.pushKV("participants", IdsToUniv(musig.participants));
                derivation = FormatDerivation(musig.derivation);
            },
        }, occurrence.expression.key);
        if (!derivation.empty()) {
            key.pushKV("derivation", derivation);
        }
        if (const auto& origin{occurrence.expression.origin}) {
            UniValue origin_obj(UniValue::VOBJ);
            origin_obj.pushKV("master_fingerprint", HexStr(origin->fingerprint));
            origin_obj.pushKV("path", WriteHDKeypath(origin->path));
            key.pushKV("origin", std::move(origin_obj));
        }
        keys.push_back(std::move(key));
    }
    return keys;
}

static std::string ShapeName(DescriptorAnalysis::Shape shape)
{
    using Shape = DescriptorAnalysis::Shape;
    switch (shape) {
    case Shape::UNRECOGNIZED: return "unrecognized";
    case Shape::WSH_MULTI: return "wsh_multi";
    case Shape::TR_MULTI_A: return "tr_multi_a";
    } // no default case, so the compiler can warn about missing cases
    NONFATAL_UNREACHABLE();
}

static std::string KeyPathName(DescriptorAnalysis::KeyPath key_path)
{
    using KeyPath = DescriptorAnalysis::KeyPath;
    switch (key_path) {
    case KeyPath::NONE: return "none";
    case KeyPath::NUMS: return "nums";
    case KeyPath::MUSIG: return "musig";
    } // no default case, so the compiler can warn about missing cases
    NONFATAL_UNREACHABLE();
}

static std::string IssueName(DescriptorAnalysis::Issue::Kind kind)
{
    using Kind = DescriptorAnalysis::Issue::Kind;
    switch (kind) {
    case Kind::UNSUPPORTED_SHAPE: return "unsupported_shape";
    case Kind::REPEATED_KEY: return "repeated_key";
    case Kind::KEY_PATH_SPENDABLE: return "key_path_spendable";
    case Kind::NOT_EXTENDED_KEY: return "not_extended_key";
    case Kind::NOT_RANGED: return "not_ranged";
    case Kind::HARDENED_DERIVATION: return "hardened_derivation";
    case Kind::BRANCHES_DIFFER: return "branches_differ";
    } // no default case, so the compiler can warn about missing cases
    NONFATAL_UNREACHABLE();
}

UniValue DescriptorAnalysisToUniv(const DescriptorAnalysis& analysis)
{
    UniValue result(UniValue::VOBJ);
    result.pushKV("recognized", analysis.IsRecognized());
    // An unrecognized descriptor claims no setup, only the issues.
    if (analysis.IsRecognized()) {
        result.pushKV("shape", ShapeName(analysis.shape));
        result.pushKV("sorted", analysis.sorted);
        if (analysis.key_path != DescriptorAnalysis::KeyPath::NONE) result.pushKV("key_path", KeyPathName(analysis.key_path));
        result.pushKV("threshold", analysis.threshold);
        result.pushKV("cosigners", IdsToUniv(analysis.cosigners));
    }
    UniValue issues(UniValue::VARR);
    for (const auto& issue : analysis.issues) {
        UniValue issue_obj(UniValue::VOBJ);
        issue_obj.pushKV("type", IssueName(issue.kind));
        issue_obj.pushKV("keys", IdsToUniv(issue.keys));
        issues.push_back(std::move(issue_obj));
    }
    result.pushKV("issues", std::move(issues));
    return result;
}
