// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <key.h>
#include <key_io.h>
#include <sync.h>
#include <test/util/common.h>
#include <test/util/setup_common.h>
#include <script/solver.h>
#include <wallet/export.h>
#include <wallet/scriptpubkeyman.h>
#include <wallet/wallet.h>
#include <wallet/test/util.h>

#include <array>
#include <memory>
#include <optional>
#include <set>
#include <string>
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
    const auto unsupported = ExportAddressKeys(wallet, WitnessV0ScriptHash{GetScriptForMultisig(1, {key.GetPubKey(), extra.GetPubKey(), key.GetPubKey()})}, AddressKeyType::WALLET);
    BOOST_REQUIRE(!unsupported);
    BOOST_CHECK(unsupported.error() == AddressKeyError::UNSUPPORTED);
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
