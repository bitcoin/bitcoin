// Copyright (c) 2020-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <key.h>
#include <test/util/setup_common.h>
#include <script/solver.h>
#include <wallet/scriptpubkeyman.h>
#include <wallet/wallet.h>
#include <wallet/test/util.h>

#include <boost/test/unit_test.hpp>

namespace wallet {
BOOST_FIXTURE_TEST_SUITE(scriptpubkeyman_tests, BasicTestingSetup)

// Test LegacyScriptPubKeyMan::CanProvide behavior, making sure it returns true
// for recognized scripts even when keys may not be available for signing.
BOOST_AUTO_TEST_CASE(CanProvide)
{
    // Set up wallet and keyman variables.
    CWallet wallet(m_node.chain.get(), "", CreateMockableWalletDatabase());
    LegacyScriptPubKeyMan& keyman = *wallet.GetOrCreateLegacyScriptPubKeyMan();

    // Make a 1 of 2 multisig script
    std::vector<CKey> keys(2);
    std::vector<CPubKey> pubkeys;
    for (CKey& key : keys) {
        key.MakeNewKey(true);
        pubkeys.emplace_back(key.GetPubKey());
    }
    CScript multisig_script = GetScriptForMultisig(1, pubkeys);
    CScript p2sh_script = GetScriptForDestination(ScriptHash(multisig_script));
    SignatureData data;

    // Verify the p2sh(multisig) script is not recognized until the multisig
    // script is added to the keystore to make it solvable
    BOOST_CHECK(!keyman.CanProvide(p2sh_script, data));
    keyman.AddCScript(multisig_script);
    BOOST_CHECK(keyman.CanProvide(p2sh_script, data));
}

static void legacy_IsKeyActive(const node::NodeContext& node, bool implicit_segwit)
{
    const bool save_g_implicit_segwit{g_implicit_segwit};
    g_implicit_segwit = implicit_segwit;
    CWallet wallet(node.chain.get(), "", CreateMockableWalletDatabase());
    {
        LOCK(wallet.cs_wallet);
        wallet.SetMinVersion(FEATURE_LATEST);
        wallet.m_keypool_size = 10;
    }
    LegacyScriptPubKeyMan& spkm = *wallet.GetOrCreateLegacyScriptPubKeyMan();

    // Start off empty
    BOOST_CHECK(spkm.GetScriptPubKeys().empty());

    // Generate 20 keypool keys (10 internal, 10 external)
    {
        LOCK(wallet.cs_wallet);
        spkm.SetupGeneration();
    }

    // 4 scripts per keypool key (P2PK, P2PKH, P2WPKH, P2SH-P2WPKH)
    // Plus 4 scripts for the seed key
    // (If !implicit_segwit, P2WPKH and P2SH-P2WPKH are not generated.)
    auto scripts1 = spkm.GetScriptPubKeys();
    BOOST_CHECK_EQUAL(scripts1.size(), implicit_segwit ? 84 : 42);

    // All keys are active
    for (const CScript& script : scripts1) {
        BOOST_CHECK(spkm.IsKeyActive(script));
    }

    // Requesting single from spkm should not deactivate key
    CTxDestination dest1;
    {
        LOCK(wallet.cs_wallet);
        auto result = spkm.GetNewDestination(OutputType::BECH32);
        dest1 = result.value();
    }
    CScript script = GetScriptForDestination(dest1);
    BOOST_CHECK(spkm.IsKeyActive(script));

    // Key pool size did not change
    // (If !implicit_segwit, the two segwit addresses are added back.)
    auto scripts2 = spkm.GetScriptPubKeys();
    BOOST_CHECK_EQUAL(scripts2.size(), implicit_segwit ? 84 : 44);

    // Use key that is not the next key
    // (i.e. address gap in wallet recovery)
    {
        LOCK(wallet.cs_wallet);
        LOCK(spkm.cs_KeyStore);
        auto keys = spkm.MarkReserveKeysAsUsed(5);
        BOOST_CHECK_EQUAL(keys.size(), 4); // Because we already used one with GetNewDestination
    }

    // Key pool size did not change
    auto scripts3 = spkm.GetScriptPubKeys();
    BOOST_CHECK_EQUAL(scripts3.size(), implicit_segwit ? 84 : 44);

    // All keys are still active
    for (const CScript& script : scripts3) {
        BOOST_CHECK(spkm.IsKeyActive(script));
    }

    // When user encrypts wallet for the first time,
    // all existing keys are removed from active keypool
    {
        LOCK(wallet.cs_wallet);
        // called by EncryptWallet()
        spkm.SetupGeneration(true);
    }

    // 20 new keys were added
    auto scripts4 = spkm.GetScriptPubKeys();
    BOOST_CHECK_EQUAL(scripts4.size(), (implicit_segwit ? 84 : 43) * 2);

    // All 10 original keys are now inactive
    for (const CScript& script : scripts3) {
        BOOST_CHECK(!spkm.IsKeyActive(script));
    }
    g_implicit_segwit = save_g_implicit_segwit;
}

BOOST_AUTO_TEST_CASE(Legacy_IsKeyActive)
{
    legacy_IsKeyActive(m_node, /*implicit_segwit=*/true);
}

BOOST_AUTO_TEST_CASE(Legacy_IsKeyActive_no_implicit_segwit)
{
    legacy_IsKeyActive(m_node, /*implicit_segwit=*/false);
}

BOOST_AUTO_TEST_CASE(Descriptor_IsKeyActive)
{
    CWallet wallet(m_node.chain.get(), "", CreateMockableWalletDatabase());
    {
        LOCK(wallet.cs_wallet);
        wallet.LoadMinVersion(FEATURE_LATEST);
        wallet.SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
        wallet.m_keypool_size = 10;
        wallet.SetupDescriptorScriptPubKeyMans();
    }
    DescriptorScriptPubKeyMan* spkm = dynamic_cast<DescriptorScriptPubKeyMan*>(wallet.GetScriptPubKeyMan(OutputType::BECH32, /*internal=*/false));

    // Start off with 10 pre-generated keys, 1 script each
    auto scripts1 = spkm->GetScriptPubKeys();
    BOOST_CHECK_EQUAL(scripts1.size(), 10);

    // All keys are active
    for (const CScript& script : scripts1) {
        BOOST_CHECK(spkm->IsKeyActive(script));
    }

    // Requesting single key from spkm should not deactivate key
    auto dest1 = spkm->GetNewDestination(OutputType::BECH32);
    CScript script = GetScriptForDestination(dest1.value());
    BOOST_CHECK(spkm->IsKeyActive(script));

    // Key pool size did not change
    auto scripts2 = spkm->GetScriptPubKeys();
    BOOST_CHECK_EQUAL(scripts2.size(), 10);

    // Use key that is not the next key
    // (i.e. address gap in wallet recovery)
    {
        LOCK(spkm->cs_desc_man);
        WalletDescriptor descriptor = spkm->GetWalletDescriptor();
        FlatSigningProvider provider;
        std::vector<CScript> scripts3;
        descriptor.descriptor->ExpandFromCache(/*pos=*/5, descriptor.cache, scripts3, provider);

        BOOST_CHECK_EQUAL(scripts3.size(), 1);
        spkm->MarkUnusedAddresses(scripts3.front());
    }

    // Key pool size increased to replace used keys
    auto scripts4 = spkm->GetScriptPubKeys();
    BOOST_CHECK_EQUAL(scripts4.size(), 16);

    // All keys are still active
    for (const CScript& script : scripts4) {
        BOOST_CHECK(spkm->IsKeyActive(script));
    }

    // When user encrypts wallet for the first time,
    // all existing keys are removed from active keypool
    {
        LOCK(wallet.cs_wallet);
        // called by EncryptWallet()
        wallet.SetupDescriptorScriptPubKeyMans();
    }

    // This SPKM is not affected
    for (const CScript& script : scripts4) {
        BOOST_CHECK(spkm->IsKeyActive(script));
    }

    // ...but at the wallet level all the keys from that SPKM are deactivated
    int num_script_keys_not_found = 0;
    for (const CScript& script : scripts4) {
        if (!wallet.IsDestinationActive(WitnessV0ScriptHash(script))) {
            ++num_script_keys_not_found;
        }
    }
    BOOST_CHECK_EQUAL(num_script_keys_not_found, 16);
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace wallet
