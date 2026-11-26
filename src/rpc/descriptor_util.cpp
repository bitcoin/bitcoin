// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit.

#include <rpc/descriptor_util.h>

#include <crypto/hex_base.h>
#include <key_io.h>
#include <pubkey.h>
#include <rpc/util.h>
#include <script/descriptor_analysis.h>
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
