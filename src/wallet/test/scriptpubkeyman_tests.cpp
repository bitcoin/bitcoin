// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <key.h>
#include <key_io.h>
#include <script/descriptor.h>
#include <script/signingprovider.h>
#include <test/util/common.h>
#include <test/util/setup_common.h>
#include <script/solver.h>
#include <wallet/scriptpubkeyman.h>
#include <wallet/wallet.h>
#include <wallet/test/util.h>

#include <boost/test/unit_test.hpp>

#include <initializer_list>
#include <vector>

namespace wallet {
BOOST_FIXTURE_TEST_SUITE(scriptpubkeyman_tests, BasicTestingSetup)

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

BOOST_AUTO_TEST_CASE(desc_spkm_availability_notifications)
{
    CWallet wallet(m_node.chain.get(), "", CreateMockableWalletDatabase());
    LOCK(wallet.cs_wallet);
    wallet.SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
    wallet.SetWalletFlag(WALLET_FLAG_DISABLE_PRIVATE_KEYS);
    wallet.m_keypool_size = 1;

    CExtKey master_key;
    master_key.SetSeed(std::array<std::byte, 32>{});
    FlatSigningProvider provider;
    std::string error;
    auto descriptors = Parse("wpkh(" + EncodeExtPubKey(master_key.Neuter()) + "/*h)", provider, error, /*require_checksum=*/false);
    BOOST_REQUIRE_EQUAL(descriptors.size(), 1);
    WalletDescriptor descriptor{std::move(descriptors[0]), /*creation_time=*/0, /*range_start=*/0, /*range_end=*/1, /*next_index=*/0};

    // Restore a watch-only descriptor with a hardened wildcard. Only cached
    // children can be used, and one extra cached child can extend its keypool.
    FlatSigningProvider private_provider;
    private_provider.keys.emplace(master_key.key.GetPubKey().GetID(), master_key.key);
    std::vector<CScript> scripts;
    for (int i = 0; i < 2; ++i) {
        FlatSigningProvider expanded;
        std::vector<CScript> output;
        BOOST_REQUIRE(descriptor.descriptor->Expand(i, private_provider, output, expanded, &descriptor.cache));
        BOOST_REQUIRE_EQUAL(output.size(), 1);
        scripts.push_back(output[0]);
    }
    const auto id = CompatDescriptorHash(*descriptor.descriptor);
    wallet.LoadDescriptorScriptPubKeyMan(id, descriptor, /*keys=*/{}, /*ckeys=*/{});
    wallet.AddActiveScriptPubKeyMan(id, OutputType::BECH32, /*internal=*/false);
    wallet.ConnectScriptPubKeyManNotifiers();
    auto* spkm = wallet.GetDescriptorScriptPubKeyMan(descriptor);
    BOOST_REQUIRE(spkm);
    BOOST_CHECK(!spkm->HavePrivateKeys());
    BOOST_CHECK(wallet.CanGetAddresses());

    std::vector<bool> availability;
    const btcsignals::scoped_connection connection{wallet.NotifyCanGetAddressesChanged.connect([&] {
        // Match the synchronous GUI callback: query the wallet, which locks the
        // same SPKM again, and inspect the completed descriptor update.
        availability.push_back(wallet.CanGetAddresses());
        const auto current_descriptor = spkm->GetWalletDescriptor();
        BOOST_CHECK_EQUAL(spkm->GetKeyPoolSize(), current_descriptor.GetEnd() - current_descriptor.GetNext());
    })};
    const auto check_notifications = [&](std::initializer_list<bool> expected) {
        BOOST_CHECK_EQUAL_COLLECTIONS(availability.begin(), availability.end(), expected.begin(), expected.end());
        availability.clear();
    };

    // An exception before any state change must not emit a notification.
    BOOST_CHECK_EXCEPTION((void)spkm->GetNewDestination(OutputType::LEGACY),
        std::runtime_error, HasReason("Types are inconsistent. Stored type does not match type of newly generated address"));
    check_notifications({});

    // Taking and returning the last cached address exercises IncIndex and DecIndex.
    auto destination = wallet.GetNewDestination(OutputType::BECH32, /*label=*/"");
    BOOST_REQUIRE(destination);
    check_notifications({false});
    spkm->ReturnDestination(/*index=*/0, /*internal=*/false, *destination);
    check_notifications({true});

    // Reservation must also defer notifications until the outer lock is released.
    int64_t index{-1};
    BOOST_REQUIRE(spkm->GetReservedDestination(OutputType::BECH32, /*internal=*/false, index));
    BOOST_CHECK_EQUAL(index, 0);
    check_notifications({false});

    // Extending the range into the remaining cached child exercises SetRangeEnd.
    BOOST_CHECK(spkm->TopUp(/*size=*/1));
    check_notifications({true});

    // Marking the last address used notifies even when the subsequent top-up fails.
    BOOST_CHECK_EQUAL(spkm->MarkUnusedAddresses(scripts[1]).size(), 1);
    check_notifications({false});
    BOOST_CHECK(!spkm->TopUp(/*size=*/1));
    BOOST_CHECK(!spkm->GetNewDestination(OutputType::BECH32));
    check_notifications({});

    // Updating a descriptor also calls the locked top-up helper. Supply two more
    // cached children, with the first becoming available during the update.
    auto updated_descriptor = spkm->GetWalletDescriptor();
    for (int i = 2; i < 4; ++i) {
        FlatSigningProvider expanded;
        std::vector<CScript> output;
        BOOST_REQUIRE(updated_descriptor.descriptor->Expand(i, private_provider, output, expanded, &updated_descriptor.cache));
        BOOST_REQUIRE_EQUAL(output.size(), 1);
        scripts.push_back(output[0]);
    }
    BOOST_REQUIRE(spkm->UpdateWalletDescriptor(updated_descriptor, /*provider=*/{}));
    check_notifications({true});

    // Consuming the last available child and topping up again leaves availability
    // unchanged, so intermediate transitions should not emit notifications.
    BOOST_CHECK_EQUAL(spkm->MarkUnusedAddresses(scripts[2]).size(), 1);
    check_notifications({});
    BOOST_REQUIRE(spkm->GetNewDestination(OutputType::BECH32));
    check_notifications({false});

    index = -1;
    BOOST_CHECK(!spkm->GetReservedDestination(OutputType::BECH32, /*internal=*/false, index));
    BOOST_CHECK_EQUAL(index, -1);
    check_notifications({});

    // Extending the declared range changes availability before topping up throws
    // on the missing cached child. Notify after unlocking, then propagate the error.
    auto failed_descriptor = spkm->GetWalletDescriptor();
    failed_descriptor.SetEnd(5);
    BOOST_CHECK_EXCEPTION((void)spkm->UpdateWalletDescriptor(failed_descriptor, /*provider=*/{}),
        std::runtime_error, HasReason("Could not top up scriptPubKeys"));
    BOOST_CHECK(wallet.CanGetAddresses());
    check_notifications({true});
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace wallet
