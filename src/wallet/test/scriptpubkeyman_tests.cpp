// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <key.h>
#include <key_io.h>
#include <musig.h>
#include <test/util/common.h>
#include <test/util/setup_common.h>
#include <script/solver.h>
#include <wallet/scriptpubkeyman.h>
#include <wallet/wallet.h>
#include <wallet/test/util.h>

#include <boost/test/unit_test.hpp>

namespace wallet {
BOOST_FIXTURE_TEST_SUITE(scriptpubkeyman_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(DescriptorScriptPubKeyManTests)
{
    std::unique_ptr<interfaces::Chain>& chain = m_node.chain;

    CWallet keystore(chain.get(), "", CreateMockableWalletDatabase());
    LOCK(keystore.cs_wallet);
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

BOOST_AUTO_TEST_CASE(desc_spkm_musig2_secnonces_are_bounded)
{
    CWallet keystore(m_node.chain.get(), "", CreateMockableWalletDatabase());
    LOCK(keystore.cs_wallet);
    auto key = GenerateRandomKey();
    auto spk_man = CreateDescriptor(keystore, "tr(" + EncodeSecret(key) + ")", /*success=*/true);
    BOOST_REQUIRE(spk_man != nullptr);
    auto provider = spk_man->GetSigningProvider(key.GetPubKey());
    BOOST_REQUIRE(provider != nullptr);

    // Sessions are looked up through another provider of the same descriptor.
    auto other_provider = spk_man->GetSigningProvider(key.GetPubKey());
    BOOST_REQUIRE(other_provider != nullptr);
    std::vector<uint256> session_ids;
    auto start_session{[&] {
        session_ids.emplace_back(m_rng.rand256());
        provider->SetMuSig2SecNonce(session_ids.back(), MuSig2SecNonce{});
    }};
    auto kept{[&](size_t first) {
        size_t n{0};
        for (size_t i = first; i < session_ids.size(); ++i) {
            n += other_provider->GetMuSig2SecNonce(session_ids[i]).has_value();
        }
        return n;
    }};

    // Up to the limit, every session that is not finished is kept.
    for (size_t i = 0; i < MuSig2SecNonces::MAX_SESSIONS; ++i) start_session();
    BOOST_CHECK_EQUAL(kept(0), MuSig2SecNonces::MAX_SESSIONS);

    // One more session drops the oldest one only.
    start_session();
    BOOST_CHECK(!other_provider->GetMuSig2SecNonce(session_ids.front()).has_value());
    BOOST_CHECK_EQUAL(kept(1), MuSig2SecNonces::MAX_SESSIONS);

    // A session that is kept can be deleted.
    other_provider->DeleteMuSig2Session(session_ids.back());
    BOOST_CHECK(!provider->GetMuSig2SecNonce(session_ids.back()).has_value());
}

BOOST_AUTO_TEST_CASE(desc_spkm_musig2_secnonces_are_bounded_per_wallet)
{
    CWallet keystore(m_node.chain.get(), "", CreateMockableWalletDatabase());
    LOCK(keystore.cs_wallet);
    auto key1 = GenerateRandomKey();
    auto key2 = GenerateRandomKey();
    auto spk_man1 = CreateDescriptor(keystore, "tr(" + EncodeSecret(key1) + ")", /*success=*/true);
    auto spk_man2 = CreateDescriptor(keystore, "tr(" + EncodeSecret(key2) + ")", /*success=*/true);
    BOOST_REQUIRE(spk_man1 != nullptr && spk_man2 != nullptr);
    auto provider1 = spk_man1->GetSigningProvider(key1.GetPubKey());
    auto provider2 = spk_man2->GetSigningProvider(key2.GetPubKey());
    BOOST_REQUIRE(provider1 != nullptr && provider2 != nullptr);

    // Fill the limit through the first descriptor.
    std::vector<uint256> session_ids;
    for (size_t i = 0; i < MuSig2SecNonces::MAX_SESSIONS; ++i) {
        session_ids.emplace_back(m_rng.rand256());
        provider1->SetMuSig2SecNonce(session_ids.back(), MuSig2SecNonce{});
    }

    // A session of the second descriptor drops the oldest session of the first.
    session_ids.emplace_back(m_rng.rand256());
    provider2->SetMuSig2SecNonce(session_ids.back(), MuSig2SecNonce{});
    BOOST_CHECK(!provider1->GetMuSig2SecNonce(session_ids.front()).has_value());
    size_t kept{0};
    for (size_t i = 1; i < session_ids.size(); ++i) {
        kept += provider2->GetMuSig2SecNonce(session_ids[i]).has_value();
    }
    BOOST_CHECK_EQUAL(kept, MuSig2SecNonces::MAX_SESSIONS);
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace wallet
