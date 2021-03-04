// Copyright (c) 2017-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <key_io.h>
#include <util/bip32.h>
#include <util/strencodings.h>
#include <wallet/wallet.h>

#include <boost/test/unit_test.hpp>
#include <test/util/setup_common.h>
#include <wallet/test/wallet_test_fixture.h>

BOOST_FIXTURE_TEST_SUITE(psbt_wallet_tests, WalletTestingSetup)

BOOST_AUTO_TEST_CASE(psbt_updater_test)
{
    auto spk_man = m_wallet.GetOrCreateLegacyScriptPubKeyMan();
    LOCK2(m_wallet.cs_wallet, spk_man->cs_KeyStore);

    // Create prevtxs and add to wallet
    CDataStream s_prev_tx1(ParseHex("0200000000010158e87a21b56daf0c23be8e7070456c336f7cbaa5c8757924f545887bb2abdd7501000000171600145f275f436b09a8cc9a2eb2a2f528485c68a56323feffffff02d8231f1b0100000017a914aed962d6654f9a2b36608eb9d64d2b260db4f1118700c2eb0b0000000017a914b7f5faf40e3d40a5a459b1db3535f2b72fa921e88702483045022100a22edcc6e5bc511af4cc4ae0de0fcd75c7e04d8c1c3a8aa9d820ed4b967384ec02200642963597b9b1bc22c75e9f3e117284a962188bf5e8a74c895089046a20ad770121035509a48eb623e10aace8bfd0212fdb8a8e5af3c94b0b133b95e114cab89e4f7965000000"), SER_NETWORK, PROTOCOL_VERSION);
    CTransactionRef prev_tx1;
    s_prev_tx1 >> prev_tx1;
    m_wallet.mapWallet.emplace(std::piecewise_construct, std::forward_as_tuple(prev_tx1->GetHash()), std::forward_as_tuple(&m_wallet, prev_tx1));

    CDataStream s_prev_tx2(ParseHex("0200000001aad73931018bd25f84ae400b68848be09db706eac2ac18298babee71ab656f8b0000000048473044022058f6fc7c6a33e1b31548d481c826c015bd30135aad42cd67790dab66d2ad243b02204a1ced2604c6735b6393e5b41691dd78b00f0c5942fb9f751856faa938157dba01feffffff0280f0fa020000000017a9140fb9463421696b82c833af241c78c17ddbde493487d0f20a270100000017a91429ca74f8a08f81999428185c97b5d852e4063f618765000000"), SER_NETWORK, PROTOCOL_VERSION);
    CTransactionRef prev_tx2;
    s_prev_tx2 >> prev_tx2;
    m_wallet.mapWallet.emplace(std::piecewise_construct, std::forward_as_tuple(prev_tx2->GetHash()), std::forward_as_tuple(&m_wallet, prev_tx2));

    // Add scripts
    CScript rs1;
    CDataStream s_rs1(ParseHex("475221029583bf39ae0a609747ad199addd634fa6108559d6c5cd39b4c2183f1ab96e07f2102dab61ff49a14db6a7d02b0cd1fbb78fc4b18312b5b4e54dae4dba2fbfef536d752ae"), SER_NETWORK, PROTOCOL_VERSION);
    s_rs1 >> rs1;
    spk_man->AddCScript(rs1);

    CScript rs2;
    CDataStream s_rs2(ParseHex("2200208c2353173743b595dfb4a07b72ba8e42e3797da74e87fe7d9d7497e3b2028903"), SER_NETWORK, PROTOCOL_VERSION);
    s_rs2 >> rs2;
    spk_man->AddCScript(rs2);

    CScript ws1;
    CDataStream s_ws1(ParseHex("47522103089dc10c7ac6db54f91329af617333db388cead0c231f723379d1b99030b02dc21023add904f3d6dcf59ddb906b0dee23529b7ffb9ed50e5e86151926860221f0e7352ae"), SER_NETWORK, PROTOCOL_VERSION);
    s_ws1 >> ws1;
    spk_man->AddCScript(ws1);

    // Add hd seed
    CKey key = DecodeSecret("5KSSJQ7UNfFGwVgpCZDSHm5rVNhMFcFtvWM3zQ8mW4qNDEN7LFd"); // Mainnet and uncompressed form of cUkG8i1RFfWGWy5ziR11zJ5V4U4W3viSFCfyJmZnvQaUsd1xuF3T
    CPubKey master_pub_key = spk_man->DeriveNewSeed(key);
    spk_man->SetHDSeed(master_pub_key);
    spk_man->NewKeyPool();

    // Call FillPSBT
    PartiallySignedTransaction psbtx;
    CDataStream ssData(ParseHex("70736274ff01009a020000000258e87a21b56daf0c23be8e7070456c336f7cbaa5c8757924f545887bb2abdd750000000000ffffffff838d0427d0ec650a68aa46bb0b098aea4422c071b2ca78352a077959d07cea1d0100000000ffffffff0270aaf00800000000160014d85c2b71d0060b09c9886aeb815e50991dda124d00e1f5050000000016001400aea9a2e5f0f876a588df5546e8742d1d87008f000000000000000000"), SER_NETWORK, PROTOCOL_VERSION);
    ssData >> psbtx;

    // Fill transaction with our data
    bool complete = true;
    BOOST_REQUIRE_EQUAL(TransactionError::OK, m_wallet.FillPSBT(psbtx, complete, SIGHASH_ALL, false, true));

    // Get the final tx
    CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
    ssTx << psbtx;
    std::string final_hex = HexStr(ssTx);
    BOOST_CHECK_EQUAL(final_hex, "70736274ff01009a020000000258e87a21b56daf0c23be8e7070456c336f7cbaa5c8757924f545887bb2abdd750000000000ffffffff838d0427d0ec650a68aa46bb0b098aea4422c071b2ca78352a077959d07cea1d0100000000ffffffff0270aaf00800000000160014d85c2b71d0060b09c9886aeb815e50991dda124d00e1f5050000000016001400aea9a2e5f0f876a588df5546e8742d1d87008f00000000000100bb0200000001aad73931018bd25f84ae400b68848be09db706eac2ac18298babee71ab656f8b0000000048473044022058f6fc7c6a33e1b31548d481c826c015bd30135aad42cd67790dab66d2ad243b02204a1ced2604c6735b6393e5b41691dd78b00f0c5942fb9f751856faa938157dba01feffffff0280f0fa020000000017a9140fb9463421696b82c833af241c78c17ddbde493487d0f20a270100000017a91429ca74f8a08f81999428185c97b5d852e4063f6187650000000104475221029583bf39ae0a609747ad199addd634fa6108559d6c5cd39b4c2183f1ab96e07f2102dab61ff49a14db6a7d02b0cd1fbb78fc4b18312b5b4e54dae4dba2fbfef536d752ae2206029583bf39ae0a609747ad199addd634fa6108559d6c5cd39b4c2183f1ab96e07f10d90c6a4f000000800000008000000080220602dab61ff49a14db6a7d02b0cd1fbb78fc4b18312b5b4e54dae4dba2fbfef536d710d90c6a4f0000008000000080010000800001008a020000000158e87a21b56daf0c23be8e7070456c336f7cbaa5c8757924f545887bb2abdd7501000000171600145f275f436b09a8cc9a2eb2a2f528485c68a56323feffffff02d8231f1b0100000017a914aed962d6654f9a2b36608eb9d64d2b260db4f1118700c2eb0b0000000017a914b7f5faf40e3d40a5a459b1db3535f2b72fa921e8876500000001012000c2eb0b0000000017a914b7f5faf40e3d40a5a459b1db3535f2b72fa921e88701042200208c2353173743b595dfb4a07b72ba8e42e3797da74e87fe7d9d7497e3b2028903010547522103089dc10c7ac6db54f91329af617333db388cead0c231f723379d1b99030b02dc21023add904f3d6dcf59ddb906b0dee23529b7ffb9ed50e5e86151926860221f0e7352ae2206023add904f3d6dcf59ddb906b0dee23529b7ffb9ed50e5e86151926860221f0e7310d90c6a4f000000800000008003000080220603089dc10c7ac6db54f91329af617333db388cead0c231f723379d1b99030b02dc10d90c6a4f00000080000000800200008000220203a9a4c37f5996d3aa25dbac6b570af0650394492942460b354753ed9eeca5877110d90c6a4f000000800000008004000080002202027f6399757d2eff55a136ad02c684b1838b6556e5f1b6b34282a94b6b5005109610d90c6a4f00000080000000800500008000");

    // Mutate the transaction so that one of the inputs is invalid
    psbtx.tx->vin[0].prevout.n = 2;

    // Try to sign the mutated input
    SignatureData sigdata;
    BOOST_CHECK(spk_man->FillPSBT(psbtx, PrecomputePSBTData(psbtx), SIGHASH_ALL, true, true) != TransactionError::OK);
}

BOOST_AUTO_TEST_CASE(parse_hd_keypath)
{
    std::vector<uint32_t> keypath;

    BOOST_CHECK(ParseHDKeypath("1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1", keypath));
    BOOST_CHECK(!ParseHDKeypath("///////////////////////////", keypath));

    BOOST_CHECK(ParseHDKeypath("1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1'/1", keypath));
    BOOST_CHECK(!ParseHDKeypath("//////////////////////////'/", keypath));

    BOOST_CHECK(ParseHDKeypath("1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/", keypath));
    BOOST_CHECK(!ParseHDKeypath("1///////////////////////////", keypath));

    BOOST_CHECK(ParseHDKeypath("1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1'/", keypath));
    BOOST_CHECK(!ParseHDKeypath("1/'//////////////////////////", keypath));

    BOOST_CHECK(ParseHDKeypath("", keypath));
    BOOST_CHECK(!ParseHDKeypath(" ", keypath));

    BOOST_CHECK(ParseHDKeypath("0", keypath));
    BOOST_CHECK(!ParseHDKeypath("O", keypath));

    BOOST_CHECK(ParseHDKeypath("0000'/0000'/0000'", keypath));
    BOOST_CHECK(!ParseHDKeypath("0000,/0000,/0000,", keypath));

    BOOST_CHECK(ParseHDKeypath("01234", keypath));
    BOOST_CHECK(!ParseHDKeypath("0x1234", keypath));

    BOOST_CHECK(ParseHDKeypath("1", keypath));
    BOOST_CHECK(!ParseHDKeypath(" 1", keypath));

    BOOST_CHECK(ParseHDKeypath("42", keypath));
    BOOST_CHECK(!ParseHDKeypath("m42", keypath));

    BOOST_CHECK(ParseHDKeypath("4294967295", keypath)); // 4294967295 == 0xFFFFFFFF (uint32_t max)
    BOOST_CHECK(!ParseHDKeypath("4294967296", keypath)); // 4294967296 == 0xFFFFFFFF (uint32_t max) + 1

    BOOST_CHECK(ParseHDKeypath("m", keypath));
    BOOST_CHECK(!ParseHDKeypath("n", keypath));

    BOOST_CHECK(ParseHDKeypath("m/", keypath));
    BOOST_CHECK(!ParseHDKeypath("n/", keypath));

    BOOST_CHECK(ParseHDKeypath("m/0", keypath));
    BOOST_CHECK(!ParseHDKeypath("n/0", keypath));

    BOOST_CHECK(ParseHDKeypath("m/0'", keypath));
    BOOST_CHECK(!ParseHDKeypath("m/0''", keypath));

    BOOST_CHECK(ParseHDKeypath("m/0'/0'", keypath));
    BOOST_CHECK(!ParseHDKeypath("m/'0/0'", keypath));

    BOOST_CHECK(ParseHDKeypath("m/0/0", keypath));
    BOOST_CHECK(!ParseHDKeypath("n/0/0", keypath));

    BOOST_CHECK(ParseHDKeypath("m/0/0/00", keypath));
    BOOST_CHECK(!ParseHDKeypath("m/0/0/f00", keypath));

    BOOST_CHECK(ParseHDKeypath("m/0/0/000000000000000000000000000000000000000000000000000000000000000000000000000000000000", keypath));
    BOOST_CHECK(!ParseHDKeypath("m/1/1/111111111111111111111111111111111111111111111111111111111111111111111111111111111111", keypath));

    BOOST_CHECK(ParseHDKeypath("m/0/00/0", keypath));
    BOOST_CHECK(!ParseHDKeypath("m/0'/00/'0", keypath));

    BOOST_CHECK(ParseHDKeypath("m/1/", keypath));
    BOOST_CHECK(!ParseHDKeypath("m/1//", keypath));

    BOOST_CHECK(ParseHDKeypath("m/0/4294967295", keypath)); // 4294967295 == 0xFFFFFFFF (uint32_t max)
    BOOST_CHECK(!ParseHDKeypath("m/0/4294967296", keypath)); // 4294967296 == 0xFFFFFFFF (uint32_t max) + 1

    BOOST_CHECK(ParseHDKeypath("m/4294967295", keypath)); // 4294967295 == 0xFFFFFFFF (uint32_t max)
    BOOST_CHECK(!ParseHDKeypath("m/4294967296", keypath)); // 4294967296 == 0xFFFFFFFF (uint32_t max) + 1
}

BOOST_AUTO_TEST_SUITE_END()
