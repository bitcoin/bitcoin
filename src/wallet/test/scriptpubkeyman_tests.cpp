// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <key.h>
#include <key_io.h>
#include <script/standard.h>
#include <test/util/setup_common.h>
#include <wallet/scriptpubkeyman.h>
#include <wallet/wallet.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(scriptpubkeyman_tests, BasicTestingSetup)

// Test LegacyScriptPubKeyMan::CanProvide behavior, making sure it returns true
// for recognized scripts even when keys may not be available for signing.
BOOST_AUTO_TEST_CASE(CanProvide)
{
    // Set up wallet and keyman variables.
    NodeContext node;
    std::unique_ptr<interfaces::Chain> chain = interfaces::MakeChain(node);
    CWallet wallet(chain.get(), "", CreateDummyWalletDatabase());
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

BOOST_AUTO_TEST_CASE(StealthAddresses)
{
    // Set up wallet and keyman variables.
    NodeContext node;
    std::unique_ptr<interfaces::Chain> chain = interfaces::MakeChain(node);
    CWallet wallet(chain.get(), "", CreateMockWalletDatabase());
    wallet.SetMinVersion(WalletFeature::FEATURE_HD_SPLIT);
    LegacyScriptPubKeyMan& keyman = *wallet.GetOrCreateLegacyScriptPubKeyMan();

    // Set HD seed
    CKey key = DecodeSecret("6usgJoGKXW12i7Ruxy8Z1C5hrRMVGfLmi9NU9uDQJMPXDJ6tQAH");
    CPubKey seed = keyman.DeriveNewSeed(key);
    keyman.SetHDSeed(seed);
    keyman.TopUp();

    // Check generated MWEB keychain
    mw::Keychain::Ptr mweb_keychain = keyman.GetMWEBKeychain();
    BOOST_CHECK(mweb_keychain != nullptr);
    BOOST_CHECK(mweb_keychain->GetSpendSecret().ToHex() == "2396e5c33b07dfa2d9e70da1dcbdad0ad2399e5672ff2d4afbe3b20bccf3ba1b");
    BOOST_CHECK(mweb_keychain->GetScanSecret().ToHex() == "918271168655385e387907612ee09d755be50c4685528f9f53eabae380ecba97");

    // Check "change" (idx=0) address is USED
    StealthAddress change_address = mweb_keychain->GetStealthAddress(0);
    BOOST_CHECK(EncodeDestination(change_address) == "ltcmweb1qq20e2arnhvxw97katjkmsd35agw3capxjkrkh7dk8d30rczm8ypxuq329nwh2twmchhqn3jqh7ua4ps539f6aazh79jy76urqht4qa59ts3at6gf");
    BOOST_CHECK(keyman.IsMine(change_address) == ISMINE_SPENDABLE);
    BOOST_CHECK(keyman.GetAllReserveKeys().find(change_address.B().GetID()) == keyman.GetAllReserveKeys().end());
    BOOST_CHECK(*keyman.GetMetadata(change_address)->mweb_index == 0);

    // Check "peg-in" (idx=1) address is USED
    StealthAddress pegin_address = mweb_keychain->GetStealthAddress(1);
    BOOST_CHECK(EncodeDestination(pegin_address) == "ltcmweb1qqg5hddkl4uhspjwg9tkmatxa4s6gswdaq9swl8vsg5xxznmye7phcqatzc62mzkg788tsrfcuegxe9q3agf5cplw7ztqdusqf7x3n2tl55x4gvyt");
    BOOST_CHECK(keyman.IsMine(pegin_address) == ISMINE_SPENDABLE);
    BOOST_CHECK(keyman.GetAllReserveKeys().find(pegin_address.B().GetID()) == keyman.GetAllReserveKeys().end());
    BOOST_CHECK(*keyman.GetMetadata(pegin_address)->mweb_index == 1);

    // Check first receive (idx=2) address is UNUSED
    StealthAddress receive_address = mweb_keychain->GetStealthAddress(2);
    BOOST_CHECK(EncodeDestination(receive_address) == "ltcmweb1qq0yq03ewm830ugmkkvrvjmyyeslcpwk8ayd7k27qx63sryy6kx3ksqm3k6jd24ld3r5dp5lzx7rm7uyxfujf8sn7v4nlxeqwrcq6k6xxwqdc6tl3");
    BOOST_CHECK(keyman.IsMine(receive_address) == ISMINE_SPENDABLE);
    BOOST_CHECK(keyman.GetAllReserveKeys().find(receive_address.B().GetID()) != keyman.GetAllReserveKeys().end());
    BOOST_CHECK(*keyman.GetMetadata(receive_address)->mweb_index == 2);

    BOOST_CHECK(keyman.GetHDChain().nMWEBIndexCounter == 1002);
}

BOOST_AUTO_TEST_SUITE_END()
