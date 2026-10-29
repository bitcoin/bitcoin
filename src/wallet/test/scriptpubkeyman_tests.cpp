// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <key.h>
#include <key_io.h>
#include <musig.h>
#include <script/descriptor.h>
#include <script/interpreter.h>
#include <sync.h>
#include <test/util/common.h>
#include <test/util/setup_common.h>
#include <script/solver.h>
#include <threadsafety.h>
#include <tinyformat.h>
#include <util/bip32.h>
#include <wallet/export.h>
#include <wallet/scriptpubkeyman.h>
#include <wallet/wallet.h>
#include <wallet/test/util.h>

#include <algorithm>
#include <array>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <boost/test/unit_test.hpp>

namespace wallet {
BOOST_FIXTURE_TEST_SUITE(scriptpubkeyman_tests, BasicTestingSetup)

class ExportTestScriptPubKeyMan : public DescriptorScriptPubKeyMan
{
public:
    ExportTestScriptPubKeyMan(WalletStorage& storage, WalletDescriptor& descriptor, const KeyMap& keys)
        : DescriptorScriptPubKeyMan(storage, CompatDescriptorHash(*descriptor.descriptor), descriptor, /*keypool_size=*/3, keys, {}) {}

    void DropCache()
    {
        LOCK(cs_desc_man);
        m_wallet_descriptor.cache = DescriptorCache{};
    }

    void CorruptCache(uint32_t index, const CExtPubKey& pubkey, uint32_t expression = 0)
    {
        LOCK(cs_desc_man);
        m_wallet_descriptor.cache.CacheDerivedExtPubKey(expression, index, pubkey);
    }
};

//! Load public expansion metadata independently of the stored secret map, including corrupt fixtures.
static std::unique_ptr<ExportTestScriptPubKeyMan> LoadExportDescriptor(CWallet& wallet, const std::string& text, const std::optional<KeyMap>& stored_keys = std::nullopt) EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet)
{
    AssertLockHeld(wallet.cs_wallet);
    FlatSigningProvider provider;
    std::string error;
    auto descriptors = Parse(text, provider, error, /*require_checksum=*/false);
    BOOST_REQUIRE_EQUAL(descriptors.size(), 1U);
    const int end = descriptors.front()->IsRange() ? 3 : 1;
    WalletDescriptor descriptor(std::move(descriptors.front()), /*creation_time=*/1, /*range_start=*/0, /*range_end=*/end, /*next_index=*/0);
    for (int index = 0; index < end; ++index) {
        FlatSigningProvider expanded;
        std::vector<CScript> scripts;
        BOOST_REQUIRE(descriptor.descriptor->Expand(index, provider, scripts, expanded, &descriptor.cache));
    }
    return std::make_unique<ExportTestScriptPubKeyMan>(wallet, descriptor, stored_keys.value_or(provider.keys));
}

BOOST_AUTO_TEST_CASE(export_key_script_index_binding)
{
    CWallet wallet(m_node.chain.get(), "", CreateMockableWalletDatabase());
    LOCK(wallet.cs_wallet);
    wallet.SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
    const auto fixed = GenerateRandomKey();
    CExtKey root;
    root.SetSeed(std::array<std::byte, 32>{});
    auto manager = LoadExportDescriptor(wallet, "wsh(multi(1," + EncodeSecret(fixed) + "," + EncodeExtKey(root) + "/*))");
    CExtKey first, last;
    BOOST_REQUIRE(root.Derive(first, 0));
    BOOST_REQUIRE(root.Derive(last, 2));
    const CScript script = GetScriptForDestination(WitnessV0ScriptHash{GetScriptForMultisig(1, {fixed.GetPubKey(), last.key.GetPubKey()})});
    const auto before = WITH_LOCK(manager->cs_desc_man, return manager->GetWalletDescriptor());
    for (bool warm : {false, true}) {
        if (warm) {
            const auto provider = manager->GetSolvingProvider(script);
            const auto flat = dynamic_cast<const FlatSigningProvider*>(provider.get());
            BOOST_REQUIRE(flat);
            BOOST_CHECK(flat->keys.empty());
            BOOST_CHECK(flat->musig2_secnonces == nullptr);
        }
        LOCK(manager->cs_desc_man);
        for (const auto* key : std::array<const CKey*, 2>{&fixed, &last.key}) {
            const auto exported = manager->ExportKey(script, key->GetPubKey().GetID());
            BOOST_REQUIRE(exported && *exported);
            BOOST_CHECK((**exported).GetPubKey() == key->GetPubKey());
        }
        const auto wrong_index = manager->ExportKey(script, first.key.GetPubKey().GetID());
        BOOST_REQUIRE(wrong_index);
        BOOST_CHECK(!*wrong_index);
        const auto wrong_script = manager->ExportKey(GetScriptForDestination(PKHash{last.key.GetPubKey()}), last.key.GetPubKey().GetID());
        BOOST_REQUIRE(wrong_script);
        BOOST_CHECK(!*wrong_script);
    }
    const auto after = WITH_LOCK(manager->cs_desc_man, return manager->GetWalletDescriptor());
    BOOST_CHECK_EQUAL(before.GetNext(), after.GetNext());
    BOOST_CHECK_EQUAL(before.GetEnd(), after.GetEnd());
    BOOST_CHECK(before.cache.GetCachedParentExtPubKeys() == after.cache.GetCachedParentExtPubKeys());
    BOOST_CHECK(before.cache.GetCachedDerivedExtPubKeys() == after.cache.GetCachedDerivedExtPubKeys());
    // Even after private extraction, the ordinary public provider cache remains secret-free.
    const auto provider = manager->GetSolvingProvider(script);
    const auto flat = dynamic_cast<const FlatSigningProvider*>(provider.get());
    BOOST_REQUIRE(flat);
    BOOST_CHECK(flat->keys.empty());
    BOOST_CHECK(flat->musig2_secnonces == nullptr);
}

BOOST_AUTO_TEST_CASE(export_key_candidate_and_corruption)
{
    CWallet wallet(m_node.chain.get(), "", CreateMockableWalletDatabase());
    LOCK(wallet.cs_wallet);
    wallet.SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
    const auto key = GenerateRandomKey();
    const auto unrelated = GenerateRandomKey();
    const auto script = GetScriptForDestination(PKHash{key.GetPubKey()});
    const auto key_id = key.GetPubKey().GetID();
    const std::string descriptor = "pkh(" + EncodeSecret(key) + ")";

    // An invalid, unrelated stored key must not be fetched at all.
    KeyMap stored{{key_id, key}, {unrelated.GetPubKey().GetID(), CKey{}}};
    auto manager = LoadExportDescriptor(wallet, descriptor, stored);
    {
        LOCK(manager->cs_desc_man);
        const auto exported = manager->ExportKey(script, key_id);
        BOOST_REQUIRE(exported && *exported);
        BOOST_CHECK((**exported).GetPubKey() == key.GetPubKey());
        const auto absent = manager->ExportKey(script, unrelated.GetPubKey().GetID());
        BOOST_REQUIRE(absent);
        BOOST_CHECK(!*absent);
    }
    // Watch-only metadata is ordinary unavailability, not inconsistent secret data.
    auto public_only = LoadExportDescriptor(wallet, descriptor, KeyMap{});
    const auto absent = WITH_LOCK(public_only->cs_desc_man, return public_only->ExportKey(script, key_id));
    BOOST_REQUIRE(absent);
    BOOST_CHECK(!*absent);
    std::vector<std::unique_ptr<DescriptorScriptPubKeyMan>> corrupt_managers;
    for (const auto& invalid : {CKey{}, unrelated}) {
        const auto& corrupt = corrupt_managers.emplace_back(LoadExportDescriptor(wallet, descriptor, KeyMap{{key_id, invalid}}));
        const auto exported = WITH_LOCK(corrupt->cs_desc_man, return corrupt->ExportKey(script, key_id));
        BOOST_REQUIRE(!exported);
        BOOST_CHECK(exported.error() == AddressKeyError::INCONSISTENT);
        // A valid manager must not conceal corruption in another exact matching manager.
        const auto result = ExportAddressKeys(wallet, PKHash{key.GetPubKey()}, AddressKeyType::WALLET);
        BOOST_REQUIRE(!result);
        BOOST_CHECK(result.error() == AddressKeyError::INCONSISTENT);
    }
}

BOOST_AUTO_TEST_CASE(export_key_duplicate_managers_and_candidates)
{
    CWallet wallet(m_node.chain.get(), "", CreateMockableWalletDatabase());
    LOCK(wallet.cs_wallet);
    wallet.SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
    const auto key = GenerateRandomKey();
    const auto extra = GenerateRandomKey();
    auto pkh = CreateDescriptor(wallet, "pkh(" + EncodeSecret(key) + ")", /*success=*/true);
    auto combo = CreateDescriptor(wallet, "combo(" + EncodeSecret(key) + ")", /*success=*/true);
    BOOST_REQUIRE(pkh && combo);
    const auto result = ExportAddressKeys(wallet, PKHash{key.GetPubKey()}, AddressKeyType::WALLET);
    BOOST_REQUIRE(result);
    BOOST_REQUIRE_EQUAL(result->keys.size(), 1U);
    BOOST_CHECK(result->legacy_aliases);
    BOOST_CHECK(result->keys.front().key.GetPubKey() == key.GetPubKey());
    BOOST_CHECK(result->keys.front().roles == std::set{AddressKeyRole::SCRIPT});

    // Expansion contains other secrets and repeated public keys. Export only the requested key.
    auto multi = LoadExportDescriptor(wallet, "wsh(multi(1," + EncodeSecret(key) + "," + EncodeSecret(extra) + "," + EncodeSecret(key) + "))");
    const auto script = GetScriptForDestination(WitnessV0ScriptHash{GetScriptForMultisig(1, {key.GetPubKey(), extra.GetPubKey(), key.GetPubKey()})});
    const auto exported = WITH_LOCK(multi->cs_desc_man, return multi->ExportKey(script, key.GetPubKey().GetID()));
    BOOST_REQUIRE(exported && *exported);
    BOOST_CHECK((**exported).GetPubKey() == key.GetPubKey());
    const auto multiple = ExportAddressKeys(wallet, WitnessV0ScriptHash{GetScriptForMultisig(1, {key.GetPubKey(), extra.GetPubKey(), key.GetPubKey()})}, AddressKeyType::WALLET);
    BOOST_REQUIRE(multiple);
    BOOST_CHECK_EQUAL(multiple->keys.size(), 2U);
    BOOST_CHECK(!multiple->legacy_aliases);
}

BOOST_AUTO_TEST_CASE(export_script_keys_partial_and_wrappers)
{
    const std::array<CKey, 3> keys{GenerateRandomKey(), GenerateRandomKey(), GenerateRandomKey()};
    for (const std::string wrapper : {"sh", "wsh", "sh-wsh"}) {
        for (bool sorted : {false, true}) {
            for (int threshold : {1, 2, 3}) {
                for (int local : {1, 2, 3}) {
                    CWallet wallet(m_node.chain.get(), "", CreateMockableWalletDatabase());
                    LOCK(wallet.cs_wallet);
                    wallet.SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
                    std::string desc = (sorted ? "sortedmulti(" : "multi(") + strprintf("%d", threshold);
                    std::set<CPubKey> expected;
                    std::vector<CPubKey> pubkeys;
                    for (int i = 0; i < 3; ++i) {
                        desc += "," + (i < local ? EncodeSecret(keys[i]) : HexStr(keys[i].GetPubKey()));
                        pubkeys.push_back(keys[i].GetPubKey());
                        if (i < local) expected.insert(keys[i].GetPubKey());
                    }
                    desc += ')';
                    if (sorted) std::sort(pubkeys.begin(), pubkeys.end());
                    const CScript inner = GetScriptForMultisig(threshold, pubkeys);
                    CTxDestination dest{wrapper == "sh" ? CTxDestination{ScriptHash{inner}} : CTxDestination{WitnessV0ScriptHash{inner}}};
                    desc = (wrapper == "sh" ? "sh(" : "wsh(") + desc + ")";
                    if (wrapper == "sh-wsh") {
                        dest = ScriptHash{GetScriptForDestination(dest)};
                        desc = "sh(" + desc + ")";
                    }
                    auto manager = LoadExportDescriptor(wallet, desc);
                    const auto result = ExportAddressKeys(wallet, dest, AddressKeyType::WALLET);
                    BOOST_REQUIRE(result);
                    BOOST_CHECK(!result->legacy_aliases);
                    std::vector<CPubKey> actual;
                    for (const auto& record : result->keys) {
                        actual.push_back(record.key.GetPubKey());
                        BOOST_CHECK(record.roles == std::set{AddressKeyRole::SCRIPT});
                    }
                    BOOST_CHECK(actual == std::vector<CPubKey>(expected.begin(), expected.end()));
                }
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(export_script_keys_recognition)
{
    const auto key = GenerateRandomKey();
    const auto pubkey = key.GetPubKey();
    for (const std::string& text : {"sh(wpkh(" + EncodeSecret(key) + "))", "sh(pk(" + EncodeSecret(key) + "))", "sh(pkh(" + EncodeSecret(key) + "))", "wsh(pk(" + EncodeSecret(key) + "))"}) {
        CWallet wallet(m_node.chain.get(), "", CreateMockableWalletDatabase());
        LOCK(wallet.cs_wallet);
        wallet.SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
        auto manager = LoadExportDescriptor(wallet, text);
        const auto scripts = manager->GetScriptPubKeys();
        BOOST_REQUIRE_EQUAL(scripts.size(), 1U);
        CTxDestination dest;
        BOOST_REQUIRE(ExtractDestination(*scripts.begin(), dest));
        const auto result = ExportAddressKeys(wallet, dest, AddressKeyType::WALLET);
        BOOST_REQUIRE(result);
        BOOST_REQUIRE_EQUAL(result->keys.size(), 1U);
        BOOST_CHECK(result->keys.front().key.GetPubKey() == pubkey);
    }
    // Key-shaped pushed bytes are not evidence of a signing-key association.
    CWallet wallet(m_node.chain.get(), "", CreateMockableWalletDatabase());
    LOCK(wallet.cs_wallet);
    wallet.SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
    const CScript unused = CScript{} << ToByteVector(pubkey) << OP_DROP << OP_TRUE;
    const CTxDestination dest{WitnessV0ScriptHash{unused}};
    FlatSigningProvider known;
    known.pubkeys.emplace(pubkey.GetID(), pubkey);
    known.scripts.emplace(CScriptID{unused}, unused);
    const CScript signing = GetScriptForRawPubKey(pubkey);
    known.scripts.emplace(CScriptID{signing}, signing);
    // Both fixtures supply their full witness script and known public key. The
    // positive control checks that recognition is not failing from missing data.
    for (const auto& [inner, supported] : {std::pair{signing, true}, std::pair{unused, false}}) {
        const CScript outer = GetScriptForDestination(WitnessV0ScriptHash{inner});
        CScript retrieved;
        BOOST_REQUIRE(known.GetCScript(CScriptID{inner}, retrieved));
        BOOST_CHECK(retrieved == inner);
        const auto concrete = InferDescriptor(outer, known);
        BOOST_CHECK_EQUAL(concrete->IsSolvable(), supported);
        FlatSigningProvider expanded;
        std::vector<CScript> scripts;
        BOOST_REQUIRE(concrete->Expand(0, known, scripts, expanded));
        BOOST_CHECK(std::find(scripts.begin(), scripts.end(), outer) != scripts.end());
        std::set<CPubKey> candidates;
        std::set<CExtPubKey> ext_keys;
        concrete->GetPubKeys(candidates, ext_keys);
        BOOST_CHECK(candidates == (supported ? std::set{pubkey} : std::set<CPubKey>{}));
        BOOST_CHECK(ext_keys.empty());
    }
    auto manager = LoadExportDescriptor(wallet, "raw(" + HexStr(GetScriptForDestination(dest)) + ")", KeyMap{{pubkey.GetID(), key}});
    const auto result = ExportAddressKeys(wallet, dest, AddressKeyType::WALLET);
    BOOST_REQUIRE(!result);
    BOOST_CHECK(result.error() == AddressKeyError::MISSING_METADATA);
}

BOOST_AUTO_TEST_CASE(export_cross_manager_keys_and_order)
{
    const auto first = GenerateRandomKey();
    const auto second = GenerateRandomKey();
    const auto script = GetScriptForMultisig(2, {first.GetPubKey(), second.GetPubKey()});
    for (bool reverse : {false, true}) {
        CWallet wallet(m_node.chain.get(), "", CreateMockableWalletDatabase());
        LOCK(wallet.cs_wallet);
        wallet.SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
        const std::string a = "wsh(multi(2,[01020304]" + EncodeSecret(first) + "," + HexStr(second.GetPubKey()) + "))";
        const std::string b = "wsh(multi(2," + HexStr(first.GetPubKey()) + ",[05060708]" + EncodeSecret(second) + "))";
        BOOST_REQUIRE(CreateDescriptor(wallet, reverse ? b : a, /*success=*/true));
        BOOST_REQUIRE(CreateDescriptor(wallet, reverse ? a : b, /*success=*/true));
        const auto result = ExportAddressKeys(wallet, WitnessV0ScriptHash{script}, AddressKeyType::WALLET);
        BOOST_REQUIRE(result);
        std::set<CPubKey> expected{first.GetPubKey(), second.GetPubKey()};
        std::vector<CPubKey> actual;
        for (const auto& record : result->keys) actual.push_back(record.key.GetPubKey());
        BOOST_CHECK(actual == std::vector<CPubKey>(expected.begin(), expected.end()));
    }
}

BOOST_AUTO_TEST_CASE(export_cross_manager_cached_and_stored_keys)
{
    CWallet wallet(m_node.chain.get(), "", CreateMockableWalletDatabase());
    LOCK(wallet.cs_wallet);
    wallet.SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
    CExtKey root, child;
    root.SetSeed(std::array<std::byte, 32>{});
    BOOST_REQUIRE(root.Derive(child, 2));
    // The target descriptor has no secrets. Its concrete key is cached in another descriptor.
    BOOST_REQUIRE(CreateDescriptor(wallet, "wsh(pk(" + HexStr(child.key.GetPubKey()) + "))", /*success=*/true));
    FlatSigningProvider keys;
    std::string error;
    auto descs = Parse("wpkh(" + EncodeExtKey(root) + "/*)", keys, error, /*require_checksum=*/false);
    BOOST_REQUIRE_EQUAL(descs.size(), 1U);
    WalletDescriptor descriptor(std::move(descs.front()), /*creation_time=*/1, /*range_start=*/0, /*range_end=*/3, /*next_index=*/0);
    BOOST_REQUIRE(wallet.AddWalletDescriptor(descriptor, keys, "", /*internal=*/false));
    for (const CTxDestination& dest : {CTxDestination{WitnessV0ScriptHash{GetScriptForRawPubKey(child.key.GetPubKey())}},
            CTxDestination{PKHash{child.key.GetPubKey()}}, CTxDestination{WitnessV0KeyHash{child.key.GetPubKey()}},
            CTxDestination{ScriptHash{GetScriptForDestination(WitnessV0KeyHash{child.key.GetPubKey()})}}}) {
        const auto result = ExportAddressKeys(wallet, dest, AddressKeyType::WALLET);
        BOOST_REQUIRE(result);
        BOOST_REQUIRE_EQUAL(result->keys.size(), 1U);
        BOOST_CHECK(result->keys.front().key.GetPubKey() == child.key.GetPubKey());
        BOOST_CHECK_EQUAL(result->legacy_aliases, std::holds_alternative<PKHash>(dest) || std::holds_alternative<WitnessV0KeyHash>(dest));
    }
    // A stored root is also usable in direct address forms, without deriving a new index.
    const auto direct = ExportAddressKeys(wallet, PKHash{root.key.GetPubKey()}, AddressKeyType::WALLET);
    BOOST_REQUIRE(direct);
    BOOST_CHECK(direct->keys.front().key.GetPubKey() == root.key.GetPubKey());
}

BOOST_AUTO_TEST_CASE(export_public_metadata_and_wallet_isolation)
{
    const auto key = GenerateRandomKey();
    const auto script = GetScriptForRawPubKey(key.GetPubKey());
    const CTxDestination dest{WitnessV0ScriptHash{script}};
    CWallet wallet(m_node.chain.get(), "", CreateMockableWalletDatabase());
    CWallet other(m_node.chain.get(), "", CreateMockableWalletDatabase());
    LOCK2(wallet.cs_wallet, other.cs_wallet);
    wallet.SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
    other.SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
    BOOST_REQUIRE(CreateDescriptor(other, "unused(" + EncodeSecret(key) + ")", /*success=*/true));
    for (const auto& text : {"addr(" + EncodeDestination(dest) + ")", "raw(" + HexStr(GetScriptForDestination(dest)) + ")"}) {
        BOOST_REQUIRE(CreateDescriptor(wallet, text, /*success=*/true));
        const auto missing = ExportAddressKeys(wallet, dest, AddressKeyType::WALLET);
        BOOST_REQUIRE(!missing);
        BOOST_CHECK(missing.error() == AddressKeyError::MISSING_METADATA);
    }
    BOOST_REQUIRE(CreateDescriptor(wallet, "wsh(pk(" + HexStr(key.GetPubKey()) + "))", /*success=*/true));
    const auto absent = ExportAddressKeys(wallet, dest, AddressKeyType::WALLET);
    BOOST_REQUIRE(!absent);
    BOOST_CHECK(absent.error() == AddressKeyError::NO_KEY);
    BOOST_REQUIRE(CreateDescriptor(wallet, "unused(" + EncodeSecret(key) + ")", /*success=*/true));
    const auto result = ExportAddressKeys(wallet, dest, AddressKeyType::WALLET);
    BOOST_REQUIRE(result);
    BOOST_REQUIRE_EQUAL(result->keys.size(), 1U);
    BOOST_CHECK(result->keys.front().key.GetPubKey() == key.GetPubKey());
}

BOOST_AUTO_TEST_CASE(export_checked_partial_derivation_failure)
{
    // A depth-255 xpub can have public cached metadata, but cannot derive a child.
    // This must fail even though another cosigner is normally unavailable.
    CExtKey root;
    root.SetSeed(std::array<std::byte, 32>{});
    root.nDepth = 255;
    const auto cosigner = GenerateRandomKey();
    FlatSigningProvider roots;
    std::string error;
    auto descs = Parse("wsh(multi(2," + EncodeExtKey(root) + "/*h," + HexStr(cosigner.GetPubKey()) + "))", roots, error, /*require_checksum=*/false);
    BOOST_REQUIRE_EQUAL(descs.size(), 1U);
    BOOST_CHECK(!descs.front()->HavePrivateKeys(roots));
    FlatSigningProvider expanded;
    BOOST_CHECK(!descs.front()->ExpandPrivateChecked(2, roots, expanded));
    BOOST_CHECK(expanded.keys.empty());
    const auto stored = roots.keys;
    roots.keys.clear();
    BOOST_CHECK(descs.front()->ExpandPrivateChecked(2, roots, expanded));
    CWallet wallet(m_node.chain.get(), "", CreateMockableWalletDatabase());
    LOCK(wallet.cs_wallet);
    wallet.SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
    WalletDescriptor descriptor(std::move(descs.front()), /*creation_time=*/1, /*range_start=*/0, /*range_end=*/1, /*next_index=*/0);
    descriptor.cache.CacheDerivedExtPubKey(0, 0, root.Neuter());
    auto corrupt = std::make_unique<ExportTestScriptPubKeyMan>(wallet, descriptor, stored);
    BOOST_REQUIRE(CreateDescriptor(wallet, "wsh(multi(2," + EncodeSecret(root.key) + "," + HexStr(cosigner.GetPubKey()) + "))", /*success=*/true));
    const CTxDestination dest{WitnessV0ScriptHash{GetScriptForMultisig(2, {root.key.GetPubKey(), cosigner.GetPubKey()})}};
    const auto result = ExportAddressKeys(wallet, dest, AddressKeyType::WALLET);
    BOOST_REQUIRE(!result);
    BOOST_CHECK(result.error() == AddressKeyError::INCONSISTENT);
}

BOOST_AUTO_TEST_CASE(export_script_keys_miniscript_branches)
{
    const auto first = GenerateRandomKey();
    const auto second = GenerateRandomKey();
    for (bool partial : {false, true}) {
        for (const std::string guard : {"older(1000)", "sha256(2a8ce30189b2ec3200b47aeb4feaac8fcad7c0ba170389729f4898b0b7933bcb)"}) {
            CWallet wallet(m_node.chain.get(), "", CreateMockableWalletDatabase());
            LOCK(wallet.cs_wallet);
            wallet.SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
            auto manager = LoadExportDescriptor(wallet, "wsh(or_i(pk(" + EncodeSecret(first) + "),and_v(v:" + guard + ",pk(" +
                (partial ? HexStr(second.GetPubKey()) : EncodeSecret(second)) + "))))");
            const auto scripts = manager->GetScriptPubKeys();
            BOOST_REQUIRE_EQUAL(scripts.size(), 1U);
            CTxDestination dest;
            BOOST_REQUIRE(ExtractDestination(*scripts.begin(), dest));
            const auto result = ExportAddressKeys(wallet, dest, AddressKeyType::WALLET);
            BOOST_REQUIRE(result);
            std::set<CPubKey> actual;
            for (const auto& record : result->keys) actual.insert(record.key.GetPubKey());
            std::set<CPubKey> expected{first.GetPubKey()};
            if (!partial) expected.insert(second.GetPubKey());
            BOOST_CHECK(actual == expected);
        }
    }
}

BOOST_AUTO_TEST_CASE(export_key_inconsistent_public_expansion)
{
    CWallet wallet(m_node.chain.get(), "", CreateMockableWalletDatabase());
    LOCK(wallet.cs_wallet);
    wallet.SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
    CExtKey root, child;
    root.SetSeed(std::array<std::byte, 32>{});
    BOOST_REQUIRE(root.Derive(child, 0x80000002));
    auto uncached = LoadExportDescriptor(wallet, "wpkh(" + EncodeExtKey(root) + "/*h)");
    const auto script = GetScriptForDestination(WitnessV0KeyHash{child.key.GetPubKey()});
    uncached->CorruptCache(/*index=*/2, root.Neuter());
    const auto mismatched = WITH_LOCK(uncached->cs_desc_man, return uncached->ExportKey(script, child.key.GetPubKey().GetID()));
    BOOST_REQUIRE(!mismatched);
    BOOST_CHECK(mismatched.error() == AddressKeyError::INCONSISTENT);
    uncached->DropCache();
    const auto missing_cache = WITH_LOCK(uncached->cs_desc_man, return uncached->ExportKey(script, child.key.GetPubKey().GetID()));
    BOOST_REQUIRE(!missing_cache);
    BOOST_CHECK(missing_cache.error() == AddressKeyError::INCONSISTENT);
}

BOOST_AUTO_TEST_CASE(export_taproot_roles_and_partial_ownership)
{
    const auto internal = GenerateRandomKey();
    const auto first = GenerateRandomKey();
    const auto second = GenerateRandomKey();
    const auto unrelated = GenerateRandomKey();
    for (bool own_internal : {false, true}) {
        for (int local_leaves : {0, 1, 2}) {
            for (const std::string kind : {"multi_a", "sortedmulti_a", "miniscript", "repeated"}) {
                CWallet wallet(m_node.chain.get(), "", CreateMockableWalletDatabase());
                LOCK(wallet.cs_wallet);
                wallet.SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
                const auto i = own_internal ? EncodeSecret(internal) : HexStr(internal.GetPubKey());
                const auto a = local_leaves >= 1 ? EncodeSecret(first) : HexStr(first.GetPubKey());
                const auto b = local_leaves >= 2 ? EncodeSecret(second) : HexStr(second.GetPubKey());
                const std::string leaf = kind == "miniscript" ? "or_i(pk(" + a + "),and_v(v:older(1000),pk(" + b + ")))" :
                    kind == "repeated" ? "{pk(" + a + "),{pk(" + b + "),pk(" + a + ")}}" : kind + "(2," + a + "," + b + ")";
                auto manager = LoadExportDescriptor(wallet, "tr(" + i + "," + leaf + ")");
                BOOST_REQUIRE(CreateDescriptor(wallet, "unused(" + EncodeSecret(unrelated) + ")", /*success=*/true));
                const auto scripts = manager->GetScriptPubKeys();
                BOOST_REQUIRE_EQUAL(scripts.size(), 1U);
                CTxDestination destination;
                BOOST_REQUIRE(ExtractDestination(*scripts.begin(), destination));
                const auto public_data = WITH_LOCK(manager->cs_desc_man, return manager->GetExportProvider(*scripts.begin()));
                BOOST_REQUIRE(public_data);
                const auto candidate_data = GetTaprootKeyCandidates(std::get<WitnessV1Taproot>(destination), public_data->tr_trees.begin()->second.GetSpendData(), *public_data);
                BOOST_REQUIRE_MESSAGE(candidate_data, "collector error " << (candidate_data ? -1 : int(candidate_data.error())) << " policy=" << kind);
                const auto result = ExportAddressKeys(wallet, destination, AddressKeyType::WALLET);
                if (!own_internal && !local_leaves) {
                    BOOST_REQUIRE(!result);
                    BOOST_CHECK_MESSAGE(result.error() == AddressKeyError::NO_KEY, "unexpected error " << int(result.error()));
                    continue;
                }
                BOOST_REQUIRE_MESSAGE(result, "unexpected error " << (result ? -1 : int(result.error())) << " internal=" << own_internal << " leaves=" << local_leaves << " policy=" << kind);
                BOOST_CHECK(!result->legacy_aliases);
                std::map<CPubKey, std::set<AddressKeyRole>> expected;
                if (own_internal) expected[internal.GetPubKey()].insert(AddressKeyRole::TAPROOT_INTERNAL);
                if (local_leaves >= 1) expected[first.GetPubKey()].insert(AddressKeyRole::TAPROOT_SCRIPT);
                if (local_leaves >= 2) expected[second.GetPubKey()].insert(AddressKeyRole::TAPROOT_SCRIPT);
                std::map<CPubKey, std::set<AddressKeyRole>> actual;
                for (const auto& record : result->keys) actual[record.key.GetPubKey()] = record.roles;
                BOOST_CHECK(actual == expected);
            }
        }
    }
    // A key repeated as internal and in multiple leaves receives both roles once.
    CWallet wallet(m_node.chain.get(), "", CreateMockableWalletDatabase());
    LOCK(wallet.cs_wallet);
    wallet.SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
    auto manager = LoadExportDescriptor(wallet, "tr(" + EncodeSecret(first) + ",{pk(" + EncodeSecret(first) + "),pk(" + EncodeSecret(first) + ")})");
    CTxDestination destination;
    BOOST_REQUIRE(ExtractDestination(*manager->GetScriptPubKeys().begin(), destination));
    const auto result = ExportAddressKeys(wallet, destination, AddressKeyType::WALLET);
    BOOST_REQUIRE(result);
    BOOST_REQUIRE_EQUAL(result->keys.size(), 1U);
    BOOST_CHECK(result->keys.front().roles == (std::set{AddressKeyRole::TAPROOT_INTERNAL, AddressKeyRole::TAPROOT_SCRIPT}));
}

BOOST_AUTO_TEST_CASE(export_taproot_parity_and_direct_keys)
{
    // Scalars 1 and n-1 have identical x-only keys but distinct held full SEC identities.
    CKey even, odd;
    const auto one = ParseHex("0000000000000000000000000000000000000000000000000000000000000001");
    const auto minus_one = ParseHex("fffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364140");
    even.Set(one.begin(), one.end(), true);
    odd.Set(minus_one.begin(), minus_one.end(), true);
    BOOST_REQUIRE(even.GetPubKey()[0] == 2 && odd.GetPubKey()[0] == 3);
    BOOST_REQUIRE(XOnlyPubKey{even.GetPubKey()} == XOnlyPubKey{odd.GetPubKey()});
    for (bool reverse : {false, true}) {
        CWallet wallet(m_node.chain.get(), "", CreateMockableWalletDatabase());
        LOCK(wallet.cs_wallet);
        wallet.SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
        // Secrets live in other descriptor managers; the target need not have been generated.
        BOOST_REQUIRE(CreateDescriptor(wallet, "wpkh(" + EncodeSecret(reverse ? odd : even) + ")", /*success=*/true));
        BOOST_REQUIRE(CreateDescriptor(wallet, "unused(" + EncodeSecret(reverse ? even : odd) + ")", /*success=*/true));
        const XOnlyPubKey xonly{even.GetPubKey()};
        const CTxDestination direct{WitnessV1Taproot{xonly}};
        const auto check = [&](const CTxDestination& destination, const std::set<AddressKeyRole>& roles) EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet) {
            const auto result = ExportAddressKeys(wallet, destination, AddressKeyType::WALLET);
            BOOST_REQUIRE_MESSAGE(result, "unexpected error " << (result ? -1 : int(result.error())));
            BOOST_REQUIRE_EQUAL(result->keys.size(), 2U);
            BOOST_CHECK(!result->legacy_aliases);
            BOOST_CHECK(result->keys[0].key.GetPubKey() == even.GetPubKey());
            BOOST_CHECK(result->keys[1].key.GetPubKey() == odd.GetPubKey());
            BOOST_CHECK(HexStr(result->keys[0].key) == HexStr(even));
            BOOST_CHECK(HexStr(result->keys[1].key) == HexStr(odd));
            for (const auto& record : result->keys) BOOST_CHECK(record.roles == roles);
        };
        check(direct, {AddressKeyRole::TAPROOT_OUTPUT});
        auto raw = LoadExportDescriptor(wallet, "rawtr(" + HexStr(odd.GetPubKey()) + ")", KeyMap{});
        check(direct, {AddressKeyRole::TAPROOT_OUTPUT});
        auto internal = LoadExportDescriptor(wallet, "tr(" + HexStr(odd.GetPubKey()) + ")", KeyMap{});
        CTxDestination tweaked;
        BOOST_REQUIRE(ExtractDestination(*internal->GetScriptPubKeys().begin(), tweaked));
        BOOST_CHECK(tweaked != direct);
        check(tweaked, {AddressKeyRole::TAPROOT_INTERNAL});
        auto leaf = LoadExportDescriptor(wallet, "tr(" + HexStr(XOnlyPubKey::NUMS_H) + ",pk(" + HexStr(odd.GetPubKey()) + "))", KeyMap{});
        CTxDestination script_only;
        BOOST_REQUIRE(ExtractDestination(*leaf->GetScriptPubKeys().begin(), script_only));
        check(script_only, {AddressKeyRole::TAPROOT_SCRIPT});
        const auto external = GenerateRandomKey();
        for (const auto* key : {&even, &odd}) {
            const std::string origin = key == &even ? "[01020304]" : "[05060708]";
            BOOST_REQUIRE(CreateDescriptor(wallet, "tr(" + HexStr(XOnlyPubKey::NUMS_H) + ",or_i(pk(" + origin + EncodeSecret(*key) +
                "),and_v(v:older(1000),pk(" + HexStr(external.GetPubKey()) + "))))", /*success=*/true));
        }
        TaprootBuilder miniscript;
        const CScript branch = CScript{} << OP_IF << ToByteVector(xonly) << OP_CHECKSIG << OP_ELSE << 1000 <<
            OP_CHECKSEQUENCEVERIFY << OP_VERIFY << ToByteVector(XOnlyPubKey{external.GetPubKey()}) << OP_CHECKSIG << OP_ENDIF;
        miniscript.Add(0, branch, TAPROOT_LEAF_TAPSCRIPT).Finalize(XOnlyPubKey::NUMS_H);
        check(miniscript.GetOutput(), {AddressKeyRole::TAPROOT_SCRIPT});
        CKey uncompressed;
        uncompressed.Set(one.begin(), one.end(), false);
        BOOST_REQUIRE(CreateDescriptor(wallet, "pkh(" + EncodeSecret(uncompressed) + ")", /*success=*/true));
        for (const auto& [destination, roles] : std::map<CTxDestination, std::set<AddressKeyRole>>{
                {direct, {AddressKeyRole::TAPROOT_OUTPUT}}, {tweaked, {AddressKeyRole::TAPROOT_INTERNAL}},
                {script_only, {AddressKeyRole::TAPROOT_SCRIPT}}}) {
            const auto exported = ExportAddressKeys(wallet, destination, AddressKeyType::WALLET);
            BOOST_REQUIRE(exported);
            BOOST_REQUIRE_EQUAL(exported->keys.size(), 3U);
            BOOST_CHECK(exported->keys.back().key.GetPubKey() == uncompressed.GetPubKey());
            BOOST_CHECK(!exported->keys.back().key.IsCompressed());
            BOOST_CHECK(HexStr(exported->keys.back().key) == HexStr(even));
            for (const auto& record : exported->keys) BOOST_CHECK(record.roles == roles);
        }
    }
    // Cached derived public keys in a different descriptor resolve both internal and leaf roles.
    CWallet wallet(m_node.chain.get(), "", CreateMockableWalletDatabase());
    LOCK(wallet.cs_wallet);
    wallet.SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
    CExtKey root, child;
    root.SetSeed(std::array<std::byte, 32>{});
    BOOST_REQUIRE(root.Derive(child, 2));
    FlatSigningProvider roots;
    std::string error;
    auto descriptors = Parse("wpkh(" + EncodeExtKey(root) + "/*)", roots, error, /*require_checksum=*/false);
    BOOST_REQUIRE_EQUAL(descriptors.size(), 1U);
    WalletDescriptor cached(std::move(descriptors.front()), /*creation_time=*/1, /*range_start=*/0, /*range_end=*/3, /*next_index=*/0);
    BOOST_REQUIRE(wallet.AddWalletDescriptor(cached, roots, "", /*internal=*/false));
    auto target = LoadExportDescriptor(wallet, "tr(" + HexStr(child.key.GetPubKey()) + ",pk(" + HexStr(child.key.GetPubKey()) + "))", KeyMap{});
    CTxDestination destination;
    BOOST_REQUIRE(ExtractDestination(*target->GetScriptPubKeys().begin(), destination));
    const auto result = ExportAddressKeys(wallet, destination, AddressKeyType::WALLET);
    BOOST_REQUIRE(result);
    BOOST_REQUIRE_EQUAL(result->keys.size(), 1U);
    BOOST_CHECK(result->keys.front().key.GetPubKey() == child.key.GetPubKey());
    BOOST_CHECK(result->keys.front().roles == (std::set{AddressKeyRole::TAPROOT_INTERNAL, AddressKeyRole::TAPROOT_SCRIPT}));
}

BOOST_AUTO_TEST_CASE(export_taproot_commitment_validation)
{
    const auto key = GenerateRandomKey();
    const XOnlyPubKey internal{key.GetPubKey()};
    const CScript leaf{CScript{} << ToByteVector(internal) << OP_CHECKSIG};
    for (bool empty : {false, true}) {
        TaprootBuilder builder;
        if (!empty) builder.Add(0, leaf, TAPROOT_LEAF_TAPSCRIPT);
        builder.Finalize(internal);
        const auto output = builder.GetOutput();
        const auto data = builder.GetSpendData();
        const auto valid = GetTaprootKeyCandidates(output, data, DUMMY_SIGNING_PROVIDER);
        BOOST_REQUIRE(valid);
        BOOST_REQUIRE_EQUAL(valid->size(), 1U);
        BOOST_CHECK(valid->at(internal) == (empty ? std::set{AddressKeyRole::TAPROOT_INTERNAL} :
            std::set{AddressKeyRole::TAPROOT_INTERNAL, AddressKeyRole::TAPROOT_SCRIPT}));
        auto missing = data;
        missing.internal_key = XOnlyPubKey{};
        const auto absent = GetTaprootKeyCandidates(output, missing, DUMMY_SIGNING_PROVIDER);
        BOOST_REQUIRE(!absent);
        BOOST_CHECK(absent.error() == AddressKeyError::MISSING_METADATA);
        const auto reject = [&](const TaprootSpendData& corrupt) {
            const auto result = GetTaprootKeyCandidates(output, corrupt, DUMMY_SIGNING_PROVIDER);
            BOOST_REQUIRE(!result);
            BOOST_CHECK(result.error() == AddressKeyError::INCONSISTENT);
        };
        auto corrupt = data;
        corrupt.merkle_root = uint256::ONE;
        reject(corrupt);
        corrupt = data;
        corrupt.internal_key = XOnlyPubKey::NUMS_H;
        reject(corrupt);
        if (empty) continue;
        const auto& entry = *data.scripts.begin();
        const auto control = *entry.second.begin();
        for (int mutation = 0; mutation < 6; ++mutation) {
            auto changed = control;
            if (mutation == 0) changed[0] ^= 1; // Wrong output parity.
            if (mutation == 1) changed[1] ^= 1; // Wrong internal key.
            if (mutation == 2) changed[0] ^= 2; // Mismatched leaf version.
            if (mutation == 3) changed.pop_back();
            if (mutation == 4) changed.resize(TAPROOT_CONTROL_MAX_SIZE + TAPROOT_CONTROL_NODE_SIZE);
            if (mutation == 5) changed.push_back(0);
            corrupt = data;
            // Retain a valid control too: corruption may not hide behind another valid path.
            corrupt.scripts.begin()->second.insert(changed);
            reject(corrupt);
        }
        corrupt = data;
        corrupt.scripts.begin()->second.clear();
        reject(corrupt);
        // A known exact root still proves internal membership when some leaves are unavailable.
        auto partial = data;
        partial.scripts.clear();
        const auto known_root = GetTaprootKeyCandidates(output, partial, DUMMY_SIGNING_PROVIDER);
        BOOST_REQUIRE(known_root);
        BOOST_CHECK(known_root->at(internal) == std::set{AddressKeyRole::TAPROOT_INTERNAL});
    }
    // Deepest supported tree, including repeated scripts and control paths.
    TaprootBuilder deep;
    for (int depth = 1; depth < 128; ++depth) deep.Add(depth, leaf, TAPROOT_LEAF_TAPSCRIPT);
    deep.Add(128, leaf, TAPROOT_LEAF_TAPSCRIPT).Add(128, leaf, TAPROOT_LEAF_TAPSCRIPT).Finalize(internal);
    const auto deepest = GetTaprootKeyCandidates(deep.GetOutput(), deep.GetSpendData(), DUMMY_SIGNING_PROVIDER);
    BOOST_REQUIRE(deepest);
    BOOST_CHECK(deepest->at(internal).contains(AddressKeyRole::TAPROOT_SCRIPT));
    // Unsupported semantics and future leaf versions must never turn pushed bytes into candidates.
    for (int version : {int{TAPROOT_LEAF_TAPSCRIPT}, 0xc2}) {
        TaprootBuilder unsupported;
        const auto other = GenerateRandomKey();
        const CScript bytes = CScript{} << ToByteVector(XOnlyPubKey{other.GetPubKey()}) << OP_DROP << OP_TRUE;
        unsupported.Add(0, bytes, version).Finalize(internal);
        const auto result = GetTaprootKeyCandidates(unsupported.GetOutput(), unsupported.GetSpendData(), DUMMY_SIGNING_PROVIDER);
        BOOST_REQUIRE(result);
        BOOST_REQUIRE_EQUAL(result->size(), 1U);
        BOOST_CHECK(result->at(internal) == std::set{AddressKeyRole::TAPROOT_INTERNAL});
    }
}

BOOST_AUTO_TEST_CASE(export_taproot_output_modes)
{
    std::set<std::pair<bool, bool>> parities;
    for (int value = 1; value <= 12; ++value) {
        const auto bytes = ParseHex(strprintf("%064x", value));
        CKey internal;
        internal.Set(bytes.begin(), bytes.end(), true);
        const auto leaf_bytes = ParseHex(strprintf("%064x", 99));
        CKey leaf;
        leaf.Set(leaf_bytes.begin(), leaf_bytes.end(), true);
        for (bool tree : {false, true}) {
            CWallet wallet(m_node.chain.get(), "", CreateMockableWalletDatabase());
            LOCK(wallet.cs_wallet);
            wallet.SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
            const std::string text = "tr(" + EncodeSecret(internal) + (tree ? ",pk(" + EncodeSecret(leaf) + ")" : "") + ')';
            auto manager = LoadExportDescriptor(wallet, text);
            const auto scripts = manager->GetScriptPubKeys();
            BOOST_REQUIRE_EQUAL(scripts.size(), 1U);
            CTxDestination destination;
            BOOST_REQUIRE(ExtractDestination(*scripts.begin(), destination));
            const auto ordinary = ExportAddressKeys(wallet, destination, AddressKeyType::WALLET);
            BOOST_REQUIRE(ordinary);
            BOOST_CHECK_EQUAL(ordinary->keys.size(), tree ? 2U : 1U);
            const auto result = ExportAddressKeys(wallet, destination, AddressKeyType::TAPROOT_OUTPUT);
            BOOST_REQUIRE(result);
            BOOST_REQUIRE_EQUAL(result->keys.size(), 1U);
            BOOST_CHECK(!result->legacy_aliases);
            const auto& output = result->keys.front();
            BOOST_CHECK(output.roles == std::set{AddressKeyRole::TAPROOT_OUTPUT});
            BOOST_CHECK(XOnlyPubKey{output.key.GetPubKey()} == std::get<WitnessV1Taproot>(destination));
            parities.emplace(internal.GetPubKey()[0] & 1, output.key.GetPubKey()[0] & 1);
            const auto provider = WITH_LOCK(manager->cs_desc_man, return manager->GetExportProvider(*scripts.begin()));
            BOOST_REQUIRE(provider);
            const auto data = provider->tr_trees.begin()->second.GetSpendData();
            const auto expected = internal.ComputeKeyPair(&data.merkle_root).GetKey();
            BOOST_REQUIRE(expected);
            BOOST_CHECK(output.key == *expected);
            BOOST_CHECK(HexStr(output.key) != HexStr(internal));
            std::array<unsigned char, 64> signature;
            BOOST_REQUIRE(output.key.SignSchnorr(uint256::ONE, signature, nullptr, uint256{}));
            BOOST_CHECK(std::get<WitnessV1Taproot>(destination).VerifySchnorr(uint256::ONE, signature));
        }
    }
    BOOST_CHECK_EQUAL(parities.size(), 4U);
}

BOOST_AUTO_TEST_CASE(export_taproot_output_availability_and_direct_keys)
{
    const auto first = GenerateRandomKey();
    const auto second = GenerateRandomKey();
    for (const std::string& text : {"tr(" + HexStr(XOnlyPubKey::NUMS_H) + ",pk(" + EncodeSecret(first) + "))",
            "rawtr(musig(" + EncodeSecret(first) + ',' + EncodeSecret(second) + "))",
            "tr(musig(" + EncodeSecret(first) + ',' + EncodeSecret(second) + "))"}) {
        CWallet wallet(m_node.chain.get(), "", CreateMockableWalletDatabase());
        LOCK(wallet.cs_wallet);
        wallet.SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
        auto manager = LoadExportDescriptor(wallet, text);
        CTxDestination destination;
        BOOST_REQUIRE(ExtractDestination(*manager->GetScriptPubKeys().begin(), destination));
        BOOST_REQUIRE(ExportAddressKeys(wallet, destination, AddressKeyType::WALLET));
        const auto unavailable = ExportAddressKeys(wallet, destination, AddressKeyType::TAPROOT_OUTPUT);
        BOOST_REQUIRE(!unavailable);
        BOOST_CHECK(unavailable.error() == AddressKeyError::NO_TAPROOT_OUTPUT_KEY);
    }
    CWallet wallet(m_node.chain.get(), "", CreateMockableWalletDatabase());
    LOCK(wallet.cs_wallet);
    wallet.SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
    TaprootBuilder builder;
    const CScript script = CScript{} << ToByteVector(XOnlyPubKey{second.GetPubKey()}) << OP_CHECKSIG;
    builder.Add(0, script, TAPROOT_LEAF_TAPSCRIPT).Finalize(XOnlyPubKey{first.GetPubKey()});
    const auto destination = builder.GetOutput();
    BOOST_REQUIRE(CreateDescriptor(wallet, "addr(" + EncodeDestination(destination) + ')', /*success=*/true));
    BOOST_REQUIRE(CreateDescriptor(wallet, "pkh(" + EncodeSecret(first) + ')', /*success=*/true));
    const auto missing = ExportAddressKeys(wallet, destination, AddressKeyType::TAPROOT_OUTPUT);
    BOOST_REQUIRE(!missing);
    BOOST_CHECK(missing.error() == AddressKeyError::MISSING_METADATA);
    // A separately held complete output scalar succeeds despite absent tree metadata.
    const auto root = builder.GetSpendData().merkle_root;
    const auto complete = first.ComputeKeyPair(&root).GetKey();
    BOOST_REQUIRE(complete);
    CKey uncompressed;
    uncompressed.Set(complete->begin(), complete->end(), false);
    BOOST_REQUIRE(CreateDescriptor(wallet, "pkh(" + EncodeSecret(uncompressed) + ')', /*success=*/true));
    const auto direct = ExportAddressKeys(wallet, destination, AddressKeyType::TAPROOT_OUTPUT);
    BOOST_REQUIRE(direct);
    BOOST_REQUIRE_EQUAL(direct->keys.size(), 1U);
    BOOST_CHECK(direct->keys.front().key == uncompressed);

    const auto bytes = ParseHex("0000000000000000000000000000000000000000000000000000000000000001");
    const auto opposite_bytes = ParseHex("fffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364140");
    CKey positive, opposite;
    positive.Set(bytes.begin(), bytes.end(), true);
    opposite.Set(opposite_bytes.begin(), opposite_bytes.end(), false);
    BOOST_REQUIRE(CreateDescriptor(wallet, "wpkh(" + EncodeSecret(positive) + ')', /*success=*/true));
    BOOST_REQUIRE(CreateDescriptor(wallet, "pkh(" + EncodeSecret(opposite) + ')', /*success=*/true));
    auto raw = LoadExportDescriptor(wallet, "rawtr(" + HexStr(positive.GetPubKey()) + ')', KeyMap{});
    CTxDestination raw_destination;
    BOOST_REQUIRE(ExtractDestination(*raw->GetScriptPubKeys().begin(), raw_destination));
    const auto unchanged = ExportAddressKeys(wallet, raw_destination, AddressKeyType::TAPROOT_OUTPUT);
    BOOST_REQUIRE(unchanged);
    BOOST_REQUIRE_EQUAL(unchanged->keys.size(), 2U);
    BOOST_CHECK(unchanged->keys[0].key == positive);
    BOOST_CHECK(unchanged->keys[1].key == opposite);
    auto tr = LoadExportDescriptor(wallet, "tr(" + HexStr(positive.GetPubKey()) + ')', KeyMap{});
    CTxDestination tweaked;
    BOOST_REQUIRE(ExtractDestination(*tr->GetScriptPubKeys().begin(), tweaked));
    const auto normalized = ExportAddressKeys(wallet, tweaked, AddressKeyType::TAPROOT_OUTPUT);
    BOOST_REQUIRE(normalized);
    BOOST_REQUIRE_EQUAL(normalized->keys.size(), 1U);
}

BOOST_AUTO_TEST_CASE(export_musig_participant_roles)
{
    const std::array<CKey, 3> keys{GenerateRandomKey(), GenerateRandomKey(), GenerateRandomKey()};
    for (int local = 0; local <= 3; ++local) {
        for (bool repeated : {false, true}) {
            for (const std::string context : {"tr", "rawtr", "leaf", "miniscript", "multi_a", "sortedmulti_a", "pkh"}) {
                CWallet wallet(m_node.chain.get(), "", CreateMockableWalletDatabase());
                LOCK(wallet.cs_wallet);
                wallet.SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
                std::string expression = "musig(";
                std::vector<CPubKey> participants;
                for (int i = 0; i < 3; ++i) {
                    if (i) expression += ',';
                    expression += i < local ? EncodeSecret(keys[i]) : HexStr(keys[i].GetPubKey());
                    participants.push_back(keys[i].GetPubKey());
                }
                if (repeated) {
                    expression += ',' + HexStr(keys[0].GetPubKey());
                    participants.push_back(keys[0].GetPubKey());
                }
                expression += ')';
                std::string text;
                if (context == "tr" || context == "rawtr") text = context + '(' + expression + ')';
                if (context == "leaf") text = "tr(" + HexStr(XOnlyPubKey::NUMS_H) + ",pk(" + expression + "))";
                if (context == "miniscript") text = "tr(" + HexStr(XOnlyPubKey::NUMS_H) + ",or_i(pk(" + expression + "),pk(" + HexStr(keys[0].GetPubKey()) + ")))";
                if (context == "multi_a" || context == "sortedmulti_a") text = "tr(" + HexStr(XOnlyPubKey::NUMS_H) + ',' + context + "(1," + expression + ',' + HexStr(keys[0].GetPubKey()) + "))";
                if (context == "pkh") text = "tr(" + HexStr(XOnlyPubKey::NUMS_H) + ",and_v(v:pkh(" + expression + "),after(1)))";
                auto manager = LoadExportDescriptor(wallet, text);
                const auto scripts = manager->GetScriptPubKeys();
                BOOST_REQUIRE_EQUAL(scripts.size(), 1U);
                CTxDestination destination;
                BOOST_REQUIRE(ExtractDestination(*scripts.begin(), destination));
                std::vector<DescriptorMuSig2KeyInfo> info;
                const auto provider = WITH_LOCK(manager->cs_desc_man, return manager->GetExportProvider(*scripts.begin(), &info));
                BOOST_REQUIRE(provider);
                BOOST_REQUIRE_EQUAL(info.size(), 1U);
                std::sort(participants.begin(), participants.end());
                BOOST_CHECK(info.front().participants == participants);
                BOOST_CHECK(info.front().aggregate == *MuSig2AggregatePubkeys(participants));
                BOOST_CHECK(info.front().path.empty());
                BOOST_CHECK(info.front().derived == info.front().aggregate);
                BOOST_CHECK(provider->keys.empty());
                BOOST_CHECK(provider->musig2_secnonces == nullptr);
                const auto result = ExportAddressKeys(wallet, destination, AddressKeyType::WALLET);
                if (!local) {
                    BOOST_REQUIRE(!result);
                    BOOST_CHECK(result.error() == AddressKeyError::NO_KEY);
                    continue;
                }
                BOOST_REQUIRE(result);
                BOOST_CHECK(!result->legacy_aliases);
                BOOST_REQUIRE_EQUAL(result->keys.size(), local);
                std::map<CPubKey, std::set<AddressKeyRole>> expected;
                for (int i = 0; i < local; ++i) expected[keys[i].GetPubKey()].insert(AddressKeyRole::MUSIG2_PARTICIPANT);
                if (context == "miniscript" || context == "multi_a" || context == "sortedmulti_a") expected[keys[0].GetPubKey()].insert(AddressKeyRole::TAPROOT_SCRIPT);
                std::map<CPubKey, std::set<AddressKeyRole>> actual;
                for (const auto& record : result->keys) actual[record.key.GetPubKey()] = record.roles;
                BOOST_CHECK(actual == expected);
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(export_musig_ranged_and_aggregate_derivation)
{
    std::array<CExtKey, 2> roots;
    roots[0].SetSeed(std::array<std::byte, 32>{});
    roots[1].SetSeed(std::array<std::byte, 32>{std::byte{1}});
    for (bool derive_aggregate : {false, true}) {
        for (int local : {1, 2}) {
            for (bool cross_manager : {false, true}) {
                CWallet wallet(m_node.chain.get(), "", CreateMockableWalletDatabase());
                LOCK(wallet.cs_wallet);
                wallet.SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
                std::string expression = "musig(";
                std::vector<CPubKey> expected;
                for (int i = 0; i < 2; ++i) {
                    CExtKey participant;
                    BOOST_REQUIRE(roots[i].Derive(participant, 5));
                    if (!derive_aggregate) BOOST_REQUIRE(participant.Derive(participant, 2));
                    if (i < local) expected.push_back(participant.key.GetPubKey());
                    if (cross_manager && i < local) {
                        FlatSigningProvider secret;
                        std::string error;
                        auto source = Parse("wpkh(" + EncodeExtKey(roots[i]) + "/5/*)", secret, error, /*require_checksum=*/false);
                        BOOST_REQUIRE_EQUAL(source.size(), 1U);
                        WalletDescriptor stored(std::move(source.front()), /*creation_time=*/1, /*range_start=*/0, /*range_end=*/3, /*next_index=*/0);
                        BOOST_REQUIRE(wallet.AddWalletDescriptor(stored, secret, "", /*internal=*/false));
                        // The fixed participant child /5 is a stored derivation, too.
                        BOOST_REQUIRE(CreateDescriptor(wallet, "wpkh(" + EncodeExtKey(roots[i]) + "/5)", /*success=*/true));
                    }
                    if (i) expression += ',';
                    expression += (i < local && !cross_manager ? EncodeExtKey(roots[i]) : EncodeExtPubKey(roots[i].Neuter())) + "/5" + (derive_aggregate ? "" : "/*");
                }
                expression += derive_aggregate ? ")/7/*" : ")";
                auto manager = LoadExportDescriptor(wallet, "rawtr(" + expression + ")");
                const auto descriptor = WITH_LOCK(manager->cs_desc_man, return manager->GetWalletDescriptor());
                FlatSigningProvider expanded;
                std::vector<CScript> scripts;
                BOOST_REQUIRE(descriptor.descriptor->ExpandFromCache(2, descriptor.cache, scripts, expanded));
                BOOST_REQUIRE_EQUAL(scripts.size(), 1U);
                CTxDestination destination;
                BOOST_REQUIRE(ExtractDestination(scripts.front(), destination));
                std::vector<DescriptorMuSig2KeyInfo> info;
                BOOST_REQUIRE(WITH_LOCK(manager->cs_desc_man, return manager->GetExportProvider(scripts.front(), &info)));
                BOOST_REQUIRE_EQUAL(info.size(), 1U);
                BOOST_CHECK(info.front().path == (derive_aggregate ? std::vector<uint32_t>{7, 2} : std::vector<uint32_t>{}));
                auto derived = CreateMuSig2SyntheticXpub(info.front().aggregate);
                for (const auto child : info.front().path) BOOST_REQUIRE(derived.Derive(derived, child));
                BOOST_CHECK(info.front().derived == (derive_aggregate ? derived.pubkey : info.front().aggregate));
                const auto result = ExportAddressKeys(wallet, destination, AddressKeyType::WALLET);
                BOOST_REQUIRE(result);
                std::sort(expected.begin(), expected.end());
                std::vector<CPubKey> actual;
                for (const auto& record : result->keys) {
                    actual.push_back(record.key.GetPubKey());
                    BOOST_CHECK(record.roles == std::set{AddressKeyRole::MUSIG2_PARTICIPANT});
                }
                BOOST_CHECK(actual == expected);
                if (local == 1 && !cross_manager) {
                    CExtKey external;
                    BOOST_REQUIRE(roots[1].Derive(external, 5));
                    if (!derive_aggregate) BOOST_REQUIRE(external.Derive(external, 2));
                    std::map<CPubKey, CKey> signers{{result->keys.front().key.GetPubKey(), result->keys.front().key}, {external.key.GetPubKey(), external.key}};
                    std::map<CPubKey, MuSig2SecNonce> secret_nonces;
                    std::map<CPubKey, std::vector<uint8_t>> public_nonces;
                    const uint256 message = uint256::ONE;
                    for (const auto& [pubkey, key] : signers) {
                        public_nonces[pubkey] = CreateMuSig2Nonce(secret_nonces[pubkey], message, key, info.front().aggregate, info.front().participants);
                        BOOST_REQUIRE_EQUAL(public_nonces[pubkey].size(), MUSIG2_PUBNONCE_SIZE);
                    }
                    std::vector<std::pair<uint256, bool>> tweaks;
                    auto tweaked = CreateMuSig2SyntheticXpub(info.front().aggregate);
                    for (const auto child : info.front().path) {
                        uint256 tweak;
                        BOOST_REQUIRE(tweaked.Derive(tweaked, child, &tweak));
                        tweaks.emplace_back(tweak, false);
                    }
                    std::map<CPubKey, uint256> partials;
                    for (const auto& [pubkey, key] : signers) {
                        const auto partial = CreateMuSig2PartialSig(message, key, info.front().aggregate, info.front().participants, public_nonces, secret_nonces.at(pubkey), tweaks);
                        BOOST_REQUIRE(partial);
                        partials.emplace(pubkey, *partial);
                    }
                    const auto signature = CreateMuSig2AggregateSig(info.front().participants, info.front().aggregate, tweaks, message, public_nonces, partials);
                    BOOST_REQUIRE(signature);
                    BOOST_CHECK(XOnlyPubKey{info.front().derived}.VerifySchnorr(message, *signature));
                }
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(export_musig_multipath_provenance)
{
    std::array<CExtKey, 2> roots;
    roots[0].SetSeed(std::array<std::byte, 32>{});
    roots[1].SetSeed(std::array<std::byte, 32>{std::byte{1}});
    for (bool derive_aggregate : {false, true}) {
        CWallet wallet(m_node.chain.get(), "", CreateMockableWalletDatabase());
        LOCK(wallet.cs_wallet);
        wallet.SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
        const auto suffix = derive_aggregate ? "/5" : "/<5;6>/*";
        const auto path = derive_aggregate ? "/<7;8>/*" : "";
        FlatSigningProvider secret;
        std::string error;
        auto descriptors = Parse("rawtr(musig(" + EncodeExtKey(roots[0]) + suffix + ',' + EncodeExtPubKey(roots[1].Neuter()) + suffix + ')' + path + ')', secret, error, /*require_checksum=*/false);
        BOOST_REQUIRE_MESSAGE(descriptors.size() == 2, error);
        for (uint32_t branch = 0; branch < descriptors.size(); ++branch) {
            WalletDescriptor descriptor(std::move(descriptors[branch]), /*creation_time=*/1, /*range_start=*/0, /*range_end=*/3, /*next_index=*/0);
            const auto added = wallet.AddWalletDescriptor(descriptor, secret, "", /*internal=*/false);
            BOOST_REQUIRE(added);
            auto& manager = added->get();
            const auto cached = WITH_LOCK(manager.cs_desc_man, return manager.GetWalletDescriptor());
            FlatSigningProvider expanded;
            std::vector<CScript> scripts;
            BOOST_REQUIRE(cached.descriptor->ExpandFromCache(2, cached.cache, scripts, expanded));
            BOOST_REQUIRE_EQUAL(scripts.size(), 1U);
            std::vector<DescriptorMuSig2KeyInfo> info;
            BOOST_REQUIRE(WITH_LOCK(manager.cs_desc_man, return manager.GetExportProvider(scripts.front(), &info)));
            BOOST_REQUIRE_EQUAL(info.size(), 1U);
            BOOST_CHECK(info.front().path == (derive_aggregate ? std::vector<uint32_t>{7 + branch, 2} : std::vector<uint32_t>{}));
            CExtKey expected;
            BOOST_REQUIRE(roots[0].Derive(expected, derive_aggregate ? 5 : 5 + branch));
            if (!derive_aggregate) BOOST_REQUIRE(expected.Derive(expected, 2));
            CTxDestination destination;
            BOOST_REQUIRE(ExtractDestination(scripts.front(), destination));
            const auto result = ExportAddressKeys(wallet, destination, AddressKeyType::WALLET);
            BOOST_REQUIRE(result);
            BOOST_REQUIRE_EQUAL(result->keys.size(), 1U);
            BOOST_CHECK(result->keys.front().key.GetPubKey() == expected.key.GetPubKey());
            BOOST_CHECK(result->keys.front().roles == std::set{AddressKeyRole::MUSIG2_PARTICIPANT});
        }
    }
    CWallet wallet(m_node.chain.get(), "", CreateMockableWalletDatabase());
    LOCK(wallet.cs_wallet);
    wallet.SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
    auto hardened = LoadExportDescriptor(wallet, "rawtr(musig(" + EncodeExtKey(roots[0]) + "/5/*h," + HexStr(roots[1].key.GetPubKey()) + "))");
    const auto cached = WITH_LOCK(hardened->cs_desc_man, return hardened->GetWalletDescriptor());
    FlatSigningProvider expanded;
    std::vector<CScript> scripts;
    BOOST_REQUIRE(cached.descriptor->ExpandFromCache(2, cached.cache, scripts, expanded));
    BOOST_REQUIRE_EQUAL(scripts.size(), 1U);
    CExtKey expected;
    BOOST_REQUIRE(roots[0].Derive(expected, 5));
    BOOST_REQUIRE(expected.Derive(expected, 0x80000002));
    CTxDestination destination;
    BOOST_REQUIRE(ExtractDestination(scripts.front(), destination));
    const auto exported = ExportAddressKeys(wallet, destination, AddressKeyType::WALLET);
    BOOST_REQUIRE(exported);
    BOOST_REQUIRE_EQUAL(exported->keys.size(), 1U);
    BOOST_CHECK(exported->keys.front().key.GetPubKey() == expected.key.GetPubKey());
    BOOST_CHECK(exported->keys.front().roles == std::set{AddressKeyRole::MUSIG2_PARTICIPANT});
}

BOOST_AUTO_TEST_CASE(export_musig_checked_derivation_and_aggregate_mismatch)
{
    CExtKey root;
    root.SetSeed(std::array<std::byte, 32>{});
    root.nDepth = 255;
    const auto external = GenerateRandomKey();
    FlatSigningProvider stored;
    std::string error;
    auto descriptors = Parse("rawtr(musig(" + EncodeExtKey(root) + "/*h," + HexStr(external.GetPubKey()) + "))", stored, error, /*require_checksum=*/false);
    BOOST_REQUIRE_EQUAL(descriptors.size(), 1U);
    DescriptorCache cache;
    cache.CacheDerivedExtPubKey(0, 0, root.Neuter());
    WalletDescriptor descriptor(std::move(descriptors.front()), /*creation_time=*/1, /*range_start=*/0, /*range_end=*/1, /*next_index=*/0);
    descriptor.cache = cache;
    FlatSigningProvider derived;
    BOOST_CHECK(!descriptor.descriptor->ExpandPrivateChecked(0, stored, derived));
    stored.keys.clear();
    BOOST_CHECK(descriptor.descriptor->ExpandPrivateChecked(0, stored, derived));
    CWallet wallet(m_node.chain.get(), "", CreateMockableWalletDatabase());
    LOCK(wallet.cs_wallet);
    wallet.SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
    auto corrupt = std::make_unique<ExportTestScriptPubKeyMan>(wallet, descriptor, KeyMap{{root.key.GetPubKey().GetID(), root.key}});
    const auto scripts = corrupt->GetScriptPubKeys();
    BOOST_REQUIRE_EQUAL(scripts.size(), 1U);
    BOOST_REQUIRE(CreateDescriptor(wallet, "rawtr(musig(" + EncodeSecret(root.key) + ',' + HexStr(external.GetPubKey()) + "))", /*success=*/true));
    CTxDestination destination;
    BOOST_REQUIRE(ExtractDestination(*scripts.begin(), destination));
    const auto result = ExportAddressKeys(wallet, destination, AddressKeyType::WALLET);
    BOOST_REQUIRE(!result);
    BOOST_CHECK(result.error() == AddressKeyError::INCONSISTENT);

    root.nDepth = 0;
    const auto output_failure = ExportAddressKeys(wallet, destination, AddressKeyType::TAPROOT_OUTPUT);
    BOOST_REQUIRE(!output_failure);
    BOOST_CHECK(output_failure.error() == AddressKeyError::INCONSISTENT);
    auto aggregate = LoadExportDescriptor(wallet, "rawtr(musig(" + EncodeExtKey(root) + "," + EncodeExtPubKey(root.Neuter()) + ")/*)");
    const auto aggregate_descriptor = WITH_LOCK(aggregate->cs_desc_man, return aggregate->GetWalletDescriptor());
    FlatSigningProvider aggregate_public;
    std::vector<CScript> original_scripts;
    BOOST_REQUIRE(aggregate_descriptor.descriptor->ExpandFromCache(0, aggregate_descriptor.cache, original_scripts, aggregate_public));
    BOOST_REQUIRE_EQUAL(original_scripts.size(), 1U);
    aggregate->CorruptCache(0, root.Neuter(), /*expression=*/2);
    const auto mismatch = WITH_LOCK(aggregate->cs_desc_man, return aggregate->GetExportProvider(original_scripts.front()));
    // A cached aggregate with unrelated public derivation cannot establish output membership.
    if (!mismatch) BOOST_CHECK(mismatch.error() == AddressKeyError::INCONSISTENT);
    else {
        std::vector<DescriptorMuSig2KeyInfo> info;
        const auto checked = WITH_LOCK(aggregate->cs_desc_man, return aggregate->GetExportProvider(original_scripts.front(), &info));
        BOOST_REQUIRE(!checked);
        BOOST_CHECK(checked.error() == AddressKeyError::INCONSISTENT);
    }
}

BOOST_AUTO_TEST_CASE(export_musig_inconsistent_public_participant_cache)
{
    CWallet wallet(m_node.chain.get(), "", CreateMockableWalletDatabase());
    LOCK(wallet.cs_wallet);
    wallet.SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
    const auto local = GenerateRandomKey();
    CExtKey remote, unrelated;
    remote.SetSeed(std::array<std::byte, 32>{});
    unrelated.SetSeed(std::array<std::byte, 32>{std::byte{1}});
    FlatSigningProvider keys;
    std::string error;
    auto descriptors = Parse("rawtr(musig(" + EncodeSecret(local) + ',' + EncodeExtPubKey(remote.Neuter()) + "/0/*))", keys, error, /*require_checksum=*/false);
    BOOST_REQUIRE_EQUAL(descriptors.size(), 1U);
    WalletDescriptor descriptor(std::move(descriptors.front()), /*creation_time=*/1, /*range_start=*/0, /*range_end=*/1, /*next_index=*/0);
    // The inconsistent cache predates Load: its resulting script map therefore agrees
    // with the cache but not the original participant's publicly derivable expression.
    descriptor.cache.CacheParentExtPubKey(/*key_exp_pos=*/1, unrelated.Neuter());
    auto manager = std::make_unique<ExportTestScriptPubKeyMan>(wallet, descriptor, keys.keys);
    const auto scripts = manager->GetScriptPubKeys();
    BOOST_REQUIRE_EQUAL(scripts.size(), 1U);
    CTxDestination destination;
    BOOST_REQUIRE(ExtractDestination(*scripts.begin(), destination));
    const auto result = ExportAddressKeys(wallet, destination, AddressKeyType::WALLET);
    BOOST_REQUIRE(!result);
    BOOST_CHECK(result.error() == AddressKeyError::INCONSISTENT);
}

BOOST_AUTO_TEST_CASE(export_taproot_inconsistent_public_expression_cache)
{
    const auto local = GenerateRandomKey();
    CExtKey remote, unrelated, parent, child, hardened;
    remote.SetSeed(std::array<std::byte, 32>{});
    unrelated.SetSeed(std::array<std::byte, 32>{std::byte{1}});
    BOOST_REQUIRE(remote.Derive(parent, 0));
    BOOST_REQUIRE(parent.Derive(child, 0));
    BOOST_REQUIRE(remote.Derive(hardened, BIP32_HARDENED_FLAG | 1));
    for (const std::string context : {"internal", "unknown_internal", "local_internal"}) {
        for (bool other_hardened : {false, true}) {
            for (bool derived_cache : {false, true}) {
                for (bool corrupt : {false, true}) {
                    CWallet wallet(m_node.chain.get(), "", CreateMockableWalletDatabase());
                    LOCK(wallet.cs_wallet);
                    wallet.SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
                    const auto remote_expression = EncodeExtPubKey(remote.Neuter()) + "/0/*";
                    const auto local_expression = "pk(" + EncodeSecret(local) + ')';
                    const auto hardened_expression = "pk(" + EncodeExtPubKey(remote.Neuter()) + "/1h/*)";
                    const auto leaves = other_hardened ? '{' + hardened_expression + ",{pk(" + remote_expression + ")," + local_expression + "}}" :
                        "{pk(" + remote_expression + ")," + local_expression + '}';
                    const auto expression = context == "internal" ? "tr(" + remote_expression + ',' +
                        (other_hardened ? '{' + hardened_expression + ',' + local_expression + '}' : local_expression) + ')' :
                        "tr(" + (context == "local_internal" ? EncodeSecret(local) : HexStr(XOnlyPubKey::NUMS_H)) + ',' + leaves + ')';
                    FlatSigningProvider keys;
                    std::string error;
                    auto descriptors = Parse(expression, keys, error, /*require_checksum=*/false);
                    BOOST_REQUIRE_EQUAL(descriptors.size(), 1U);
                    BOOST_REQUIRE_EQUAL(keys.keys.size(), 1U);
                    WalletDescriptor descriptor(std::move(descriptors.front()), /*creation_time=*/1, /*range_start=*/0, /*range_end=*/1, /*next_index=*/0);
                    const uint32_t remote_index = context == "internal" ? 0 : (other_hardened ? 2 : 1);
                    descriptor.cache.CacheParentExtPubKey(remote_index, parent.Neuter());
                    if (other_hardened) descriptor.cache.CacheParentExtPubKey(1, hardened.Neuter());
                    FlatSigningProvider original_public;
                    std::vector<CScript> original_scripts;
                    BOOST_REQUIRE(descriptor.descriptor->ExpandFromCache(0, descriptor.cache, original_scripts, original_public));
                    if (derived_cache) descriptor.cache.CacheDerivedExtPubKey(remote_index, 0, corrupt ? unrelated.Neuter() : child.Neuter());
                    else if (corrupt) descriptor.cache.CacheParentExtPubKey(remote_index, unrelated.Neuter());
                    FlatSigningProvider cached_public;
                    std::vector<CScript> cached_scripts;
                    BOOST_REQUIRE(descriptor.descriptor->ExpandFromCache(0, descriptor.cache, cached_scripts, cached_public));
                    BOOST_CHECK((cached_scripts != original_scripts) == corrupt);
                    // Corruption predates Load. Both the script map and requested destination
                    // match the cached output, so rejection must check the original expression.
                    auto manager = std::make_unique<ExportTestScriptPubKeyMan>(wallet, descriptor, keys.keys);
                    const auto scripts = manager->GetScriptPubKeys();
                    BOOST_REQUIRE_EQUAL(scripts.size(), 1U);
                    const std::set<CScript> expected{cached_scripts.begin(), cached_scripts.end()};
                    const std::set<CScript> loaded{scripts.begin(), scripts.end()};
                    BOOST_CHECK(loaded == expected);
                    CTxDestination destination;
                    BOOST_REQUIRE(ExtractDestination(*scripts.begin(), destination));
                    for (const auto mode : {AddressKeyType::WALLET, AddressKeyType::TAPROOT_OUTPUT}) {
                        const auto result = ExportAddressKeys(wallet, destination, mode);
                        if (corrupt) {
                            BOOST_CHECK(!result);
                            if (!result) BOOST_CHECK(result.error() == AddressKeyError::INCONSISTENT);
                        } else if (mode == AddressKeyType::TAPROOT_OUTPUT && context != "local_internal") {
                            BOOST_REQUIRE(!result);
                            BOOST_CHECK(result.error() == AddressKeyError::NO_TAPROOT_OUTPUT_KEY);
                        } else {
                            BOOST_REQUIRE(result);
                            BOOST_REQUIRE_EQUAL(result->keys.size(), 1U);
                            const auto& record = result->keys.front();
                            if (mode == AddressKeyType::WALLET) {
                                BOOST_CHECK(record.key == local);
                                std::set<AddressKeyRole> roles{AddressKeyRole::TAPROOT_SCRIPT};
                                if (context == "local_internal") roles.insert(AddressKeyRole::TAPROOT_INTERNAL);
                                BOOST_CHECK(record.roles == roles);
                            } else {
                                BOOST_CHECK(record.roles == std::set{AddressKeyRole::TAPROOT_OUTPUT});
                                BOOST_CHECK(XOnlyPubKey{record.key.GetPubKey()} == std::get<WitnessV1Taproot>(destination));
                            }
                        }
                    }
                }
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(export_musig_held_sec_identities)
{
    const auto one = ParseHex("0000000000000000000000000000000000000000000000000000000000000001");
    const auto minus_one = ParseHex("fffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364140");
    CKey participant, uncompressed, opposite, opposite_uncompressed;
    participant.Set(one.begin(), one.end(), true);
    uncompressed.Set(one.begin(), one.end(), false);
    opposite.Set(minus_one.begin(), minus_one.end(), true);
    opposite_uncompressed.Set(minus_one.begin(), minus_one.end(), false);
    const auto external = GenerateRandomKey();
    for (const std::string storage : {"pkh", "unused"}) {
        CWallet wallet(m_node.chain.get(), "", CreateMockableWalletDatabase());
        LOCK(wallet.cs_wallet);
        wallet.SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
        auto manager = LoadExportDescriptor(wallet, "rawtr(musig(" + HexStr(participant.GetPubKey()) + ',' + HexStr(external.GetPubKey()) + "))", KeyMap{});
        std::vector<CPubKey> participants{participant.GetPubKey(), external.GetPubKey()};
        std::sort(participants.begin(), participants.end());
        const auto aggregate = MuSig2AggregatePubkeys(participants);
        BOOST_REQUIRE(aggregate);
        const CTxDestination destination{WitnessV1Taproot{XOnlyPubKey{*aggregate}}};
        BOOST_REQUIRE(CreateDescriptor(wallet, "pkh(" + EncodeSecret(opposite) + ')', /*success=*/true));
        BOOST_REQUIRE(CreateDescriptor(wallet, "pkh(" + EncodeSecret(opposite_uncompressed) + ')', /*success=*/true));
        const auto absent = ExportAddressKeys(wallet, destination, AddressKeyType::WALLET);
        BOOST_REQUIRE(!absent);
        BOOST_CHECK(absent.error() == AddressKeyError::NO_KEY);
        BOOST_REQUIRE(CreateDescriptor(wallet, storage + '(' + EncodeSecret(uncompressed) + ')', /*success=*/true));
        const auto exported = ExportAddressKeys(wallet, destination, AddressKeyType::WALLET);
        BOOST_REQUIRE(exported);
        BOOST_REQUIRE_EQUAL(exported->keys.size(), 1U);
        const auto& held = exported->keys.front();
        BOOST_CHECK(held.key.GetPubKey() == uncompressed.GetPubKey());
        BOOST_CHECK(!held.key.IsCompressed());
        BOOST_CHECK(HexStr(held.key) == HexStr(participant));
        BOOST_CHECK(held.roles == std::set{AddressKeyRole::MUSIG2_PARTICIPANT});

        // The coordinator serializes the same exported scalar for MuSig's compressed
        // participant protocol identity. Export itself must retain the held identity.
        CKey protocol_key;
        protocol_key.Set(held.key.begin(), held.key.end(), true);
        BOOST_REQUIRE(protocol_key.GetPubKey() == participant.GetPubKey());
        const uint256 message = uint256::ONE;
        MuSig2SecNonce ours_nonce, theirs_nonce;
        std::map<CPubKey, std::vector<uint8_t>> nonces;
        nonces[participant.GetPubKey()] = CreateMuSig2Nonce(ours_nonce, message, protocol_key, *aggregate, participants);
        nonces[external.GetPubKey()] = CreateMuSig2Nonce(theirs_nonce, message, external, *aggregate, participants);
        const auto ours = CreateMuSig2PartialSig(message, protocol_key, *aggregate, participants, nonces, ours_nonce, {});
        const auto theirs = CreateMuSig2PartialSig(message, external, *aggregate, participants, nonces, theirs_nonce, {});
        BOOST_REQUIRE(ours && theirs);
        const auto signature = CreateMuSig2AggregateSig(participants, *aggregate, {}, message, nonces,
            {{participant.GetPubKey(), *ours}, {external.GetPubKey(), *theirs}});
        BOOST_REQUIRE(signature);
        BOOST_CHECK(XOnlyPubKey{*aggregate}.VerifySchnorr(message, *signature));

        BOOST_REQUIRE(CreateDescriptor(wallet, "wpkh(" + EncodeSecret(participant) + ')', /*success=*/true));
        const auto both = ExportAddressKeys(wallet, destination, AddressKeyType::WALLET);
        BOOST_REQUIRE(both);
        BOOST_REQUIRE_EQUAL(both->keys.size(), 2U);
        BOOST_CHECK(both->keys[0].key.GetPubKey() == participant.GetPubKey());
        BOOST_CHECK(both->keys[1].key.GetPubKey() == uncompressed.GetPubKey());
        for (const auto& record : both->keys) {
            BOOST_CHECK(HexStr(record.key) == HexStr(participant));
            BOOST_CHECK(record.roles == std::set{AddressKeyRole::MUSIG2_PARTICIPANT});
        }
        auto negative = LoadExportDescriptor(wallet, "rawtr(musig(" + HexStr(opposite.GetPubKey()) + ',' + HexStr(external.GetPubKey()) + "))", KeyMap{});
        CTxDestination opposite_destination;
        BOOST_REQUIRE(ExtractDestination(*negative->GetScriptPubKeys().begin(), opposite_destination));
        const auto explicit_opposite = ExportAddressKeys(wallet, opposite_destination, AddressKeyType::WALLET);
        BOOST_REQUIRE(explicit_opposite);
        BOOST_REQUIRE_EQUAL(explicit_opposite->keys.size(), 2U);
        BOOST_CHECK(explicit_opposite->keys[0].key.GetPubKey() == opposite.GetPubKey());
        BOOST_CHECK(explicit_opposite->keys[1].key.GetPubKey() == opposite_uncompressed.GetPubKey());
    }
}

BOOST_AUTO_TEST_CASE(export_musig_nonce_snapshot_and_cooperative_signatures)
{
    const auto local = GenerateRandomKey();
    const auto external = GenerateRandomKey();
    std::vector<CPubKey> participants{local.GetPubKey(), external.GetPubKey()};
    std::sort(participants.begin(), participants.end());
    const auto aggregate = MuSig2AggregatePubkeys(participants);
    BOOST_REQUIRE(aggregate);
    CWallet wallet(m_node.chain.get(), "", CreateMockableWalletDatabase());
    LOCK(wallet.cs_wallet);
    wallet.SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
    auto manager = CreateDescriptor(wallet, "rawtr(musig(" + EncodeSecret(local) + "," + HexStr(external.GetPubKey()) + "))", /*success=*/true);
    BOOST_REQUIRE(manager);
    const auto provider = manager->GetSigningProvider(local.GetPubKey());
    BOOST_REQUIRE(provider && provider->musig2_secnonces);
    const uint256 message = uint256::ONE;
    MuSig2SecNonce local_nonce, external_nonce;
    std::map<CPubKey, std::vector<uint8_t>> nonces;
    nonces[local.GetPubKey()] = CreateMuSig2Nonce(local_nonce, message, local, *aggregate, participants);
    nonces[external.GetPubKey()] = CreateMuSig2Nonce(external_nonce, message, external, *aggregate, participants);
    BOOST_REQUIRE_EQUAL(nonces.at(local.GetPubKey()).size(), MUSIG2_PUBNONCE_SIZE);
    BOOST_REQUIRE_EQUAL(nonces.at(external.GetPubKey()).size(), MUSIG2_PUBNONCE_SIZE);
    const auto id = MuSig2SessionID(*aggregate, local.GetPubKey(), message, nonces.at(local.GetPubKey()));
    provider->SetMuSig2SecNonce(id, std::move(local_nonce));
    const auto nonce_before = provider->GetMuSig2SecNonce(id);
    BOOST_REQUIRE(nonce_before && nonce_before->get().IsValid());
    const auto identity = nonce_before->get().Get();
    const auto before = WITH_LOCK(manager->cs_desc_man, return manager->GetWalletDescriptor());
    for (int i = 0; i < 2; ++i) {
        const auto exported = ExportAddressKeys(wallet, WitnessV1Taproot{XOnlyPubKey{*aggregate}}, AddressKeyType::WALLET);
        BOOST_REQUIRE(exported);
        BOOST_REQUIRE_EQUAL(exported->keys.size(), 1U);
        BOOST_CHECK(exported->keys.front().key.GetPubKey() == local.GetPubKey());
        BOOST_CHECK(exported->keys.front().roles == std::set{AddressKeyRole::MUSIG2_PARTICIPANT});
        BOOST_REQUIRE_EQUAL(provider->musig2_secnonces->size(), 1U);
        const auto nonce_after = provider->GetMuSig2SecNonce(id);
        BOOST_REQUIRE(nonce_after && nonce_after->get().IsValid());
        BOOST_CHECK(nonce_after->get().Get() == identity);
    }
    const auto after = WITH_LOCK(manager->cs_desc_man, return manager->GetWalletDescriptor());
    BOOST_CHECK_EQUAL(before.GetNext(), after.GetNext());
    BOOST_CHECK(before.cache.GetCachedDerivedExtPubKeys() == after.cache.GetCachedDerivedExtPubKeys());
    const auto exported = ExportAddressKeys(wallet, WitnessV1Taproot{XOnlyPubKey{*aggregate}}, AddressKeyType::WALLET);
    BOOST_REQUIRE(exported);
    // Consume the pre-export nonce only after export; partial-signature verification inside
    // Core and final aggregate verification prove its signing state was preserved.
    const auto ours = CreateMuSig2PartialSig(message, exported->keys.front().key, *aggregate, participants, nonces, nonce_before->get(), {});
    const auto theirs = CreateMuSig2PartialSig(message, external, *aggregate, participants, nonces, external_nonce, {});
    BOOST_REQUIRE(ours && theirs);
    std::map<CPubKey, uint256> partials{{local.GetPubKey(), *ours}, {external.GetPubKey(), *theirs}};
    BOOST_CHECK(!CreateMuSig2AggregateSig(participants, *aggregate, {}, message, nonces, {{local.GetPubKey(), *ours}}));
    const auto signature = CreateMuSig2AggregateSig(participants, *aggregate, {}, message, nonces, partials);
    BOOST_REQUIRE(signature);
    BOOST_CHECK(XOnlyPubKey{*aggregate}.VerifySchnorr(message, *signature));
    provider->DeleteMuSig2Session(id);
    BOOST_CHECK(provider->musig2_secnonces->empty());
}

BOOST_AUTO_TEST_CASE(DescriptorScriptPubKeyManTests)
{
    std::unique_ptr<interfaces::Chain>& chain = m_node.chain;

    CWallet keystore(chain.get(), "", CreateMockableWalletDatabase());
    auto key_scriptpath = GenerateRandomKey();

    // Verify that a SigningProvider for a pubkey is only returned if its corresponding private key is available
    auto key_internal = GenerateRandomKey();
    std::string desc_str = "tr(" + EncodeSecret(key_internal) + ",pk(" + HexStr(key_scriptpath.GetPubKey()) + "))";
    auto spk_man1 = CreateDescriptor(keystore, desc_str, true);
    BOOST_CHECK(spk_man1 != nullptr);
    auto signprov_keypath_spendable = spk_man1->GetSigningProvider(key_internal.GetPubKey());
    BOOST_CHECK(signprov_keypath_spendable != nullptr);

    desc_str = "tr(" + HexStr(XOnlyPubKey::NUMS_H) + ",pk(" + HexStr(key_scriptpath.GetPubKey()) + "))";
    auto spk_man2 = CreateDescriptor(keystore, desc_str, true);
    BOOST_CHECK(spk_man2 != nullptr);
    auto signprov_keypath_nums_h = spk_man2->GetSigningProvider(XOnlyPubKey::NUMS_H.GetEvenCorrespondingCPubKey());
    BOOST_CHECK(signprov_keypath_nums_h == nullptr);
}

BOOST_AUTO_TEST_CASE(desc_spkm_topup_fail)
{
    // Attempting to construct a DescriptorSPKM that cannot be topped up (hardened derivation without private keys)
    // should throw even though it is valid and can be parsed
    CExtKey extkey;
    extkey.SetSeed(std::array<std::byte, 32>{});
    CWallet keystore(m_node.chain.get(), "", CreateMockableWalletDatabase());
    BOOST_CHECK_EXCEPTION(
        CreateDescriptor(keystore, "wpkh(" + EncodeExtPubKey(extkey.Neuter()) + "/*h)", /*success=*/true),
        std::runtime_error, HasReason("Could not top up scriptPubKeys"));
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace wallet
