// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <key.h>
#include <key_io.h>
#include <script/descriptor.h>
#include <sync.h>
#include <test/util/common.h>
#include <test/util/setup_common.h>
#include <script/solver.h>
#include <threadsafety.h>
#include <tinyformat.h>
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

    void CorruptCache(uint32_t index, const CExtPubKey& pubkey)
    {
        LOCK(cs_desc_man);
        m_wallet_descriptor.cache.CacheDerivedExtPubKey(/*key_exp_pos=*/0, index, pubkey);
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
