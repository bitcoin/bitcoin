// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/bip352.h>
#include <chainparams.h>
#include <coins.h>
#include <span.h>
#include <addresstype.h>
#include <policy/policy.h>
#include <script/interpreter.h>
#include <script/script.h>
#include <script/script_error.h>
#include <script/solver.h>
#include <test/data/bip352_send_and_receive_vectors.json.h>

#include <test/util/common.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>
#include <test/util/json.h>
#include <vector>
#include <util/chaintype.h>
#include <util/strencodings.h>
#include <streams.h>

namespace bip352 {
BOOST_FIXTURE_TEST_SUITE(bip352_tests, BasicTestingSetup)

CKey ParseHexToCKey(std::string_view hex) {
    CKey output;
    std::vector<unsigned char> hex_data = ParseHex(hex);
    output.Set(hex_data.begin(), hex_data.end(), true);
    return output;
};

BOOST_AUTO_TEST_CASE(bip352_send_and_receive_test_vectors)
{
    UniValue tests;
    BOOST_REQUIRE(tests.read(json_tests::bip352_send_and_receive_vectors));

    for (const auto& vec : tests.getValues()) {
        // run sending tests
        BOOST_TEST_MESSAGE(vec["comment"].get_str());

        for (const auto& sender : vec["sending"].getValues()) {
            const UniValue& given = sender["given"];
            const UniValue& expected = sender["expected"];

            std::vector<COutPoint> outpoints;
            std::vector<CKey> keys;
            std::vector<KeyPair> taproot_keys;
            for (const auto& input : given["vin"].getValues()) {
                COutPoint outpoint{Txid::FromHex(input["txid"].get_str()).value(), input["vout"].getInt<uint32_t>()};
                outpoints.push_back(outpoint);
                const auto& spk_bytes = ParseHex(input["prevout"]["scriptPubKey"]["hex"].get_str());
                CScript spk = CScript(spk_bytes.begin(), spk_bytes.end());
                const auto& script_sig_bytes = ParseHex(input["scriptSig"].get_str());
                CScript script_sig = CScript(script_sig_bytes.begin(), script_sig_bytes.end());
                CTxIn txin{outpoint, script_sig};
                // Deserialize the txinwitness field into txin.scriptWitness.
                const auto witness_str = ParseHex(input["txinwitness"].get_str());
                if (!witness_str.empty()) {
                    SpanReader(witness_str) >> txin.scriptWitness;
                }

                // check if this is a silent payments input by trying to extract the public key
                const auto& pubkey = GetPubKeyFromInput(txin, spk);
                if (pubkey.has_value()) {
                    std::vector<std::vector<unsigned char>> solutions;
                    TxoutType type = Solver(spk, solutions);
                    if (type == TxoutType::WITNESS_V1_TAPROOT) {
                        taproot_keys.emplace_back(ParseHexToCKey(input["private_key"].get_str()).ComputeKeyPair(nullptr));
                    } else {
                        keys.emplace_back(ParseHexToCKey(input["private_key"].get_str()));
                    }
                }
            }
            if (taproot_keys.empty() && keys.empty()) {
                BOOST_CHECK(expected["outputs"].getValues()[0].empty());
                continue;
            }
            // silent payments logic
            auto smallest_outpoint = std::min_element(outpoints.begin(), outpoints.end(), BIP352Comparator());
            std::map<size_t, SilentPaymentsDestination> sp_dests;
            const std::vector<UniValue>& silent_payments_addresses = given["recipients"].getValues();
            size_t sp_index = 0;
            for (size_t i = 0; i < silent_payments_addresses.size(); ++i) {
                auto sp = DecodeSilentPaymentsAddress(silent_payments_addresses[i]["address"].get_str(), Params());
                BOOST_REQUIRE(sp.has_value());
                if (!silent_payments_addresses[i]["scan_pub_key"].isNull()) {
                    BOOST_CHECK_EQUAL(HexStr(sp->GetScanPubKey()), silent_payments_addresses[i]["scan_pub_key"].get_str());
                    BOOST_CHECK_EQUAL(HexStr(sp->GetSpendPubKey()), silent_payments_addresses[i]["spend_pub_key"].get_str());
                }
                size_t count = silent_payments_addresses[i]["count"].isNull() ? 1 : (size_t)silent_payments_addresses[i]["count"].getInt<int>();
                for (size_t j = 0; j < count; ++j) {
                    sp_dests.emplace(sp_index++, *sp);
                }
            }
            auto sp_tr_dests = GenerateSilentPaymentsTaprootDestinations(sp_dests, keys, taproot_keys, *smallest_outpoint);
            // This means the inputs summed to zero, which realistically would only happen maliciously. In this case, just move on
            if (!sp_tr_dests.has_value()) {
                // Check that we actually expect zero outputs to be generated for this test
                BOOST_CHECK(expected["outputs"].getValues()[0].empty());
                continue;
            }
            bool match = false;
            for (const auto& candidate_set : expected["outputs"].getValues()) {
                BOOST_CHECK(sp_tr_dests->size() == candidate_set.size());
                std::vector<WitnessV1Taproot> expected_spks;
                for (const auto& output : candidate_set.getValues()) {
                    const WitnessV1Taproot tap{XOnlyPubKey(ParseHex(output.get_str()))};
                    expected_spks.push_back(tap);
                }
                match = std::all_of(sp_tr_dests->begin(), sp_tr_dests->end(), [&](const auto& entry) {
                    return std::find(expected_spks.begin(), expected_spks.end(), entry.second) != expected_spks.end();
                });
                if (match) break;
            }
            BOOST_CHECK(match);
        }

        // Test receiving
        for (const auto& recipient : vec["receiving"].getValues()) {

            const UniValue& given = recipient["given"];
            const UniValue& expected = recipient["expected"];

            std::vector<CTxIn> vin;
            std::map<COutPoint, Coin> coins;
            for (const auto& input : given["vin"].getValues()) {
                COutPoint outpoint{Txid::FromHex(input["txid"].get_str()).value(), input["vout"].getInt<uint32_t>()};
                const auto& spk_bytes = ParseHex(input["prevout"]["scriptPubKey"]["hex"].get_str());
                CScript spk = CScript(spk_bytes.begin(), spk_bytes.end());
                const auto& script_sig_bytes = ParseHex(input["scriptSig"].get_str());
                CScript script_sig = CScript(script_sig_bytes.begin(), script_sig_bytes.end());
                CTxIn txin{outpoint, script_sig};
                // Deserialize the txinwitness field into txin.scriptWitness.
                const auto witness_str = ParseHex(input["txinwitness"].get_str());
                if (!witness_str.empty()) {
                    SpanReader(witness_str) >> txin.scriptWitness;
                }
                vin.push_back(txin);
                coins[outpoint] = Coin{CTxOut{{}, spk}, 0, false};
            }
            auto pub_tweak_data = GetSilentPaymentsPrevoutsSummary(vin, coins);
            // If we don't get any tweak data from the transaction inputs, it is not a silent payment
            // transaction, so we skip it.
            if (!pub_tweak_data.has_value()) {
                // Make sure this is expected and not just a failure of the GetSilentPaymentsPrevoutsSummary func
                BOOST_CHECK(expected["outputs"].empty());
                continue;
            }
            std::vector<XOnlyPubKey> output_pub_keys;
            for (const auto& pubkey : given["outputs"].getValues()) {
                output_pub_keys.emplace_back(ParseHex(pubkey.get_str()));
            }

            CKey scan_priv_key = ParseHexToCKey(given["key_material"]["scan_priv_key"].get_str());
            CKey spend_priv_key = ParseHexToCKey(given["key_material"]["spend_priv_key"].get_str());
            SilentPaymentsDestination sp_address{SilentPaymentsDestination::From(scan_priv_key.GetPubKey(), spend_priv_key.GetPubKey()).value()};
            auto expected_address = DecodeSilentPaymentsAddress(expected["addresses"][0].get_str(), Params());
            BOOST_REQUIRE(expected_address.has_value());
            BOOST_CHECK(sp_address == *expected_address);

            // The change label is registered automatically; only non-change labels need registering.
            SilentPaymentsReceiver receiver{scan_priv_key, sp_address.GetSpendPubKey()};
            auto given_labels{given["labels"].getValues()};
            for (size_t i = 0; i < given_labels.size(); i++) {
                const uint32_t m = given_labels[i].getInt<uint32_t>();
                const SilentPaymentsDestination labeled_addr = (m == 0) ? receiver.GetChangeDestination() : receiver.GenerateLabeledAddress(m);
                // expected["addresses"] contains the base silent payments address (at index 0)
                // followed by the labeled addresses
                auto sp = DecodeSilentPaymentsAddress(expected["addresses"][i+1].get_str(), Params());
                BOOST_REQUIRE(sp.has_value());
                BOOST_CHECK(labeled_addr == *sp);
            }

            // Scanning
            const auto& found_outputs = receiver.Scan(*pub_tweak_data, output_pub_keys);
            BOOST_REQUIRE(found_outputs.has_value());
            // The transaction may be a silent payments transaction, but it does not contain any outputs for us,
            // so we continue to the next transaction.
            if (found_outputs->empty()) {
                BOOST_CHECK(expected["outputs"].empty());
                continue;
            }
            if (!expected["n_outputs"].isNull()) {
                BOOST_CHECK_EQUAL(found_outputs->size(), (size_t)expected["n_outputs"].getInt<int>());
            } else {
                std::map<XOnlyPubKey, uint256> expected_outputs;
                for (const auto& output : expected["outputs"].getValues()) {
                    expected_outputs.emplace(
                        XOnlyPubKey{ParseHex(output["pub_key"].get_str())},
                        uint256{ParseHex(output["priv_key_tweak"].get_str())});
                }
                BOOST_TEST_MESSAGE(found_outputs->size());
                BOOST_REQUIRE_EQUAL(found_outputs->size(), expected_outputs.size());
                for (const auto& output : *found_outputs) {
                    const auto expected_output = expected_outputs.find(output.output);
                    BOOST_REQUIRE(expected_output != expected_outputs.end());
                    BOOST_CHECK(output.tweak == expected_output->second);
                }
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(bip352_preserves_requested_output_indexes)
{
    CKey sender_key = ParseHexToCKey("0000000000000000000000000000000000000000000000000000000000000001");
    CKey scan_key = ParseHexToCKey("0000000000000000000000000000000000000000000000000000000000000002");
    CKey spend_key = ParseHexToCKey("0000000000000000000000000000000000000000000000000000000000000003");
    SilentPaymentsDestination sp_dest{SilentPaymentsDestination::From(scan_key.GetPubKey(), spend_key.GetPubKey()).value()};
    std::map<size_t, SilentPaymentsDestination> sp_dests{{2, sp_dest}, {5, sp_dest}};
    COutPoint smallest_outpoint{Txid::FromHex("0000000000000000000000000000000000000000000000000000000000000001").value(), 0};

    auto generated = GenerateSilentPaymentsTaprootDestinations(sp_dests, {sender_key}, {}, smallest_outpoint);

    BOOST_REQUIRE(generated.has_value());
    BOOST_CHECK_EQUAL(generated->size(), sp_dests.size());
    BOOST_CHECK_EQUAL(generated->count(0), 0);
    BOOST_CHECK_EQUAL(generated->count(1), 0);
    BOOST_CHECK_EQUAL(generated->count(2), 1);
    BOOST_CHECK_EQUAL(generated->count(5), 1);
}

BOOST_AUTO_TEST_CASE(bip352_skips_transactions_spending_unknown_segwit_versions)
{
    CKey key = ParseHexToCKey("0000000000000000000000000000000000000000000000000000000000000001");
    CPubKey pubkey = key.GetPubKey();
    COutPoint eligible_outpoint{Txid::FromHex("0000000000000000000000000000000000000000000000000000000000000001").value(), 0};
    COutPoint unknown_segwit_outpoint{Txid::FromHex("0000000000000000000000000000000000000000000000000000000000000002").value(), 0};

    CTxIn eligible_input{eligible_outpoint};
    eligible_input.scriptWitness = CScriptWitness{{std::vector<unsigned char>(64, 0), ToByteVector(pubkey)}};

    std::map<COutPoint, Coin> coins;
    coins[eligible_outpoint] = Coin{CTxOut{{}, GetScriptForDestination(WitnessV0KeyHash{pubkey})}, 0, false};

    BOOST_REQUIRE(GetSilentPaymentsPrevoutsSummary({eligible_input}, coins).has_value());
    for (unsigned int version = 2; version <= 16; ++version) {
        const CScript unknown_spk{GetScriptForDestination(WitnessUnknown{version, std::vector<unsigned char>(32, 1)})};
        coins[unknown_segwit_outpoint] = Coin{CTxOut{{}, unknown_spk}, 0, false};
        const auto result{GetSilentPaymentsPrevoutsSummary({eligible_input, CTxIn{unknown_segwit_outpoint}}, coins)};
        BOOST_REQUIRE(!result.has_value());
        BOOST_CHECK(result.error() == PrevoutsSummaryError::NOT_ELIGIBLE);
    }
}

BOOST_AUTO_TEST_CASE(bip352_scan_skips_invalid_taproot_outputs)
{
    CKey sender_key = ParseHexToCKey("0000000000000000000000000000000000000000000000000000000000000001");
    CKey scan_key = ParseHexToCKey("0000000000000000000000000000000000000000000000000000000000000002");
    CKey spend_key = ParseHexToCKey("0000000000000000000000000000000000000000000000000000000000000003");
    const COutPoint outpoint{Txid::FromHex("0000000000000000000000000000000000000000000000000000000000000001").value(), 0};

    std::map<size_t, SilentPaymentsDestination> sp_dests;
    sp_dests.emplace(0, SilentPaymentsDestination::From(scan_key.GetPubKey(), spend_key.GetPubKey()).value());
    const auto sp_tr_dests = GenerateSilentPaymentsTaprootDestinations(sp_dests, {sender_key}, {}, outpoint);
    BOOST_REQUIRE(sp_tr_dests.has_value());
    const XOnlyPubKey expected_output{sp_tr_dests->begin()->second};

    CTxIn txin{outpoint};
    const CPubKey sender_pubkey{sender_key.GetPubKey()};
    txin.scriptWitness = CScriptWitness{{{}, ToByteVector(sender_pubkey)}};

    std::map<COutPoint, Coin> coins;
    coins[outpoint] = Coin{CTxOut{{}, GetScriptForDestination(WitnessV0KeyHash{sender_pubkey})}, 0, false};
    const auto prevouts_summary = GetSilentPaymentsPrevoutsSummary({txin}, coins);
    BOOST_REQUIRE(prevouts_summary.has_value());

    std::vector<XOnlyPubKey> output_pub_keys;
    output_pub_keys.emplace_back(ParseHex("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"));
    output_pub_keys.push_back(expected_output);

    SilentPaymentsReceiver receiver{scan_key, spend_key.GetPubKey()};
    const auto found_outputs = receiver.Scan(*prevouts_summary, output_pub_keys);
    BOOST_REQUIRE(found_outputs.has_value());
    BOOST_REQUIRE_EQUAL(found_outputs->size(), 1);
    BOOST_CHECK_EQUAL(found_outputs->front().output, expected_output);

    // Finding nothing is not an error, even when no output is a valid x-only key.
    const auto none{receiver.Scan(*prevouts_summary, {output_pub_keys[0]})};
    BOOST_REQUIRE(none.has_value());
    BOOST_CHECK(none->empty());
}

BOOST_AUTO_TEST_CASE(bip352_p2pkh_pubkey_extraction_with_checksig_in_scriptsig)
{
    // P2PKH scriptSigs are third-party malleable. Construct one that contains an OP_CHECKSIG with a
    // bogus (but strictly DER-encoded) signature, followed by a conditional push of a wrong pubkey:
    //
    //   <sig> <pubkey> <bogus_sig> OP_OVER OP_CHECKSIG OP_IF <wrong_pubkey> OP_ENDIF
    //
    // In real validation the OP_CHECKSIG fails, the OP_IF branch is skipped and the final stack is
    // <sig> <pubkey>, i.e. the spend is still valid and the pubkey must be extracted from it.
    const CKey key = ParseHexToCKey("0000000000000000000000000000000000000000000000000000000000000001");
    const CPubKey pubkey = key.GetPubKey();
    const CPubKey wrong_pubkey = ParseHexToCKey("0000000000000000000000000000000000000000000000000000000000000002").GetPubKey();
    const CScript spk{GetScriptForDestination(PKHash{pubkey})};

    CMutableTransaction tx;
    tx.vin.emplace_back(COutPoint{Txid::FromHex("0000000000000000000000000000000000000000000000000000000000000001").value(), 0});
    std::vector<unsigned char> sig;
    BOOST_REQUIRE(key.Sign(SignatureHash(spk, tx, 0, SIGHASH_ALL, /*amount=*/0, SigVersion::BASE), sig));
    sig.push_back(SIGHASH_ALL);
    const std::vector<unsigned char> bogus_sig{ParseHex("300602010102010101")}; // r=1, s=1, SIGHASH_ALL
    tx.vin[0].scriptSig = CScript() << sig << ToByteVector(pubkey) << bogus_sig << OP_OVER << OP_CHECKSIG
                                    << OP_IF << ToByteVector(wrong_pubkey) << OP_ENDIF;
    const CTxIn& txin{tx.vin[0]};

    // Verify that the malleated spend is consensus-valid
    ScriptError serror = SCRIPT_ERR_OK;
    BOOST_CHECK_MESSAGE(VerifyScript(txin.scriptSig, spk, nullptr, MANDATORY_SCRIPT_VERIFY_FLAGS,
                                     MutableTransactionSignatureChecker(&tx, 0, /*amountIn=*/0, MissingDataBehavior::ASSERT_FAIL), &serror),
                        ScriptErrorString(serror));

    const auto extracted_pubkey = GetPubKeyFromInput(txin, spk);
    BOOST_REQUIRE(extracted_pubkey.has_value());
    BOOST_CHECK_EQUAL(std::get<CPubKey>(*extracted_pubkey), pubkey);
}

BOOST_AUTO_TEST_CASE(bip352_label_serialize_roundtrip)
{
    CKey scan_key = ParseHexToCKey("0000000000000000000000000000000000000000000000000000000000000002");
    CPubKey spend_pubkey = ParseHexToCKey("0000000000000000000000000000000000000000000000000000000000000003").GetPubKey();
    SilentPaymentsReceiver receiver{scan_key, spend_pubkey};

    BOOST_REQUIRE_EQUAL(receiver.GetLabels().size(), 1);
    const SilentPaymentsLabel& label = receiver.GetLabels().begin()->first;

    DataStream stream;
    label.Serialize(stream);
    auto roundtripped = SilentPaymentsLabel::Unserialize(stream);
    BOOST_REQUIRE(roundtripped.has_value());
    BOOST_CHECK(*roundtripped == label);
    BOOST_CHECK(stream.empty());
}

BOOST_AUTO_TEST_CASE(bip352_decode_address)
{
    struct ValidVector {
        ChainType chain;
        std::string address;
        std::string scan_pubkey;
        std::string spend_pubkey;
        std::string extension_data;
        uint8_t version;
    };
    const ValidVector valid_vectors[]{
        {ChainType::MAIN,
         "sp1qq22l5s6l9460ww6t4tkzsy2a7zejurcmzz35pt0ffrzk5erlaykdcqugecjjnjqf7ggq39vl6wexjlm00n66z94v675n7wcux6d2krr68gdjvfn2",
         "0295fa435f2d74f73b4baaec28115df0b32e0f1b10a340ade948c56a647fe92cdc",
         "0388ce2529c809f21008959fd3b2697f6f7cf5a116acd7a93f3b1c369aab0c7a3a",
         "", 0},
        {ChainType::MAIN,
         "sp1pq22l5s6l9460ww6t4tkzsy2a7zejurcmzz35pt0ffrzk5erlaykdcqugecjjnjqf7ggq39vl6wexjlm00n66z94v675n7wcux6d2krr68t02m0h0s8cwhy",
         "0295fa435f2d74f73b4baaec28115df0b32e0f1b10a340ade948c56a647fe92cdc",
         "0388ce2529c809f21008959fd3b2697f6f7cf5a116acd7a93f3b1c369aab0c7a3a",
         "deadbeef", 1},
        {ChainType::TESTNET4,
         "tsp1qqthpye3hdcnydp9temp7yduy6uw5h2nw8u9fz677ccrna280qwj3uq60zeqs3zfpj3age62h4ljq2lyawwdecmk8a545yysk4x3tu3skjqm2thu6",
         "02ee1266376e264684abcec3e23784d71d4baa6e3f0a916bdec6073ea8ef03a51e",
         "034f1641088921947a8ce957afe4057c9d739b9c6ec7ed2b421216a9a2be461690",
         "", 0},
        {ChainType::SIGNET,
         "tsp1qqvrl2accqtatgtkllv5fdsfapq6vqnrr0uf2whhmjyas4teg7ljfqqlpec0ze90m97t3stv92p5ekzkcwzmpx0x8p7ljj9xqcjjpvnfurc00tjn9",
         "0307f5771802fab42edffb2896c13d0834c04c637f12a75efb913b0aaf28f7e490",
         "03e1ce1e2c95fb2f97182d8550699b0ad870b6133cc70fbf2914c0c4a4164d3c1e",
         "", 0},
        {ChainType::REGTEST,
         "sprt1qq04xgllnqqfdlxjkr355rmsahfn9t8l6sd0k20t4uka4c73nvvpnwqu4kvg0nm62d5jtmqp54lkc7tul0dzt25ejtn4f80505z3u0h5jag59rdg4",
         "03ea647ff30012df9a561c6941ee1dba66559ffa835f653d75e5bb5c7a33630337",
         "0395b310f9ef4a6d24bd8034afed8f2f9f7b44b553325cea93be8fa0a3c7de92ea",
         "", 0},
    };

    for (const auto& vec : valid_vectors) {
        SelectParams(vec.chain);

        auto sp = DecodeSilentPaymentsAddress(vec.address, Params());
        BOOST_REQUIRE_MESSAGE(sp.has_value(), vec.address);
        BOOST_CHECK_EQUAL(sp->GetVersion(), vec.version);
        BOOST_CHECK_EQUAL(HexStr(sp->GetScanPubKey()), vec.scan_pubkey);
        BOOST_CHECK_EQUAL(HexStr(sp->GetSpendPubKey()), vec.spend_pubkey);
        BOOST_CHECK_EQUAL(HexStr(sp->GetExtensionData()), vec.extension_data);

        // Bech32(m) is case-insensitive as a whole; an all-uppercase address must decode identically.
        std::string flipped = ToUpper(vec.address);
        auto sp_flipped = DecodeSilentPaymentsAddress(flipped, Params());
        BOOST_REQUIRE_MESSAGE(sp_flipped.has_value(), flipped);
        BOOST_CHECK(*sp_flipped == *sp);
    }

    // `chain` is the one chain (if any) whose HRP matches the address, where `expected_error`
    // applies; on every other chain the HRP check itself is expected to fail first.
    // `chain_independent` addresses fail bech32m checksum verification before the HRP is even
    // compared, so `expected_error` applies on every chain instead.
    struct InvalidVector {
        std::string address;
        std::string expected_error;
        std::optional<ChainType> chain = std::nullopt;
        bool chain_independent = false;
    };
    const InvalidVector invalid_vectors[]{
        {"spx1qq22l5s6l9460ww6t4tkzsy2a7zejurcmzz35pt0ffrzk5erlaykdcqugecjjnjqf7ggq39vl6wexjlm00n66z94v675n7wcux6d2krr68g37pn04",
         "Invalid or unsupported prefix for Silent Payments address (expected sp, got spx)."}, // wrong HRP; never matches any chain
        {"sp1qq22l5s6l9460ww6t4tkzsy2a7zejurcmzz35pt0ffrzk5erlaykdcqugecjjnjqf7ggq39vl6wexjlm00n66z94v675n7wcux6d2krr68gcwu9kg",
         "Silent Payments address must use Bech32m checksum", std::nullopt, /*chain_independent=*/true}, // bad checksum
        {"sp1qqgqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq2qugecjjnjqf7ggq39vl6wexjlm00n66z94v675n7wcux6d2krr68g25havg",
         "Invalid Silent payments address", ChainType::MAIN}, // invalid scan pubkey
        {"sp1qq22l5s6l9460ww6t4tkzsy2a7zejurcmzz35pt0ffrzk5erlaykdcqsqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq57qc9nf",
         "Invalid Silent payments address", ChainType::MAIN}, // invalid spend pubkey
        {"sp1lq22l5s6l9460ww6t4tkzsy2a7zejurcmzz35pt0ffrzk5erlaykdcqugecjjnjqf7ggq39vl6wexjlm00n66z94v675n7wcux6d2krr68gaafydt",
         "This implementation only supports Silent payments addresses v0 through v30 (got 31).", ChainType::MAIN}, // reserved version 31
        {"sp1qq22l5s6l9460ww6t4tkzsy2a7zejurcmzz35pt0ffrzk5erlaykdcznsxsn",
         "Silent payments data payload is too small (expected at least 66, got 33).", ChainType::MAIN}, // payload too small
        {"tsp1qqthpye3hdcnydp9temp7yduy6uw5h2nw8u9fz677ccrna280qwj3uq60zeqs3zfpj3age62h4ljq2lyawwdecmk8a545yysk4x3tu3skjqm2thum",
         "Silent Payments address must use Bech32m checksum", std::nullopt, /*chain_independent=*/true}, // bad checksum
        {"sp1qqgqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqsqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqf26rn7",
         "Silent Payments address must use Bech32m checksum", std::nullopt, /*chain_independent=*/true}, // bad checksum
        {"sprt1qqgqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq2qu4kvg0nm62d5jtmqp54lkc7tul0dzt25ejtn4f80505z3u0h5jagf9zkp5",
         "Invalid Silent payments address", ChainType::REGTEST}, // invalid scan pubkey
        {"sp1qqtrqglu5g8kh6mfsg4qxa9wq0nv9cauwfwxw70984wkqnw2uwz0w2qhexz9qryjccvgyjdz0shuf653fk5cus3vrd7vmppsp7yfmecpklyqqanq9zg",
         "Silent payments version is v0 but data is not the correct size (expected 66, got 67).", ChainType::MAIN}, // v0 with one extra byte
    };

    for (const auto& vec : invalid_vectors) {
        for (const auto chain : {ChainType::MAIN, ChainType::TESTNET, ChainType::TESTNET4, ChainType::SIGNET, ChainType::REGTEST}) {
            SelectParams(chain);
            auto sp = DecodeSilentPaymentsAddress(vec.address, Params());
            BOOST_REQUIRE_MESSAGE(!sp.has_value(), vec.address);
            if (vec.chain_independent || chain == vec.chain) {
                BOOST_CHECK_EQUAL(sp.error(), vec.expected_error);
            } else {
                BOOST_CHECK_MESSAGE(sp.error().starts_with("Invalid or unsupported prefix for Silent Payments address"), sp.error());
            }
        }
    }

    SelectParams(ChainType::MAIN);
}

static COutPoint TestOutpoint(uint8_t n)
{
    return COutPoint{Txid::FromUint256(uint256{n}), 0};
}

//! A P2WPKH input spending `key`, with a dummy signature, and the matching coin.
static CTxIn P2WPKHInput(const COutPoint& outpoint, const CKey& key, std::map<COutPoint, Coin>& coins)
{
    CTxIn txin{outpoint};
    const CPubKey pubkey{key.GetPubKey()};
    txin.scriptWitness = CScriptWitness{{std::vector<unsigned char>(64, 0), ToByteVector(pubkey)}};
    coins[outpoint] = Coin{CTxOut{{}, GetScriptForDestination(WitnessV0KeyHash{pubkey})}, 0, false};
    return txin;
}

BOOST_AUTO_TEST_CASE(bip352_scan_reports_labels)
{
    const CKey sender_key = ParseHexToCKey("0000000000000000000000000000000000000000000000000000000000000001");
    const CKey scan_key = ParseHexToCKey("0000000000000000000000000000000000000000000000000000000000000002");
    const CPubKey spend_pubkey = ParseHexToCKey("0000000000000000000000000000000000000000000000000000000000000003").GetPubKey();

    SilentPaymentsReceiver receiver{scan_key, spend_pubkey};
    BOOST_REQUIRE_EQUAL(receiver.GetLabels().size(), 1);
    const SilentPaymentsLabel change_label{receiver.GetLabels().begin()->first};
    const SilentPaymentsDestination labeled_dest{receiver.GenerateLabeledAddress(1)};
    BOOST_REQUIRE_EQUAL(receiver.GetLabels().size(), 2);
    std::optional<SilentPaymentsLabel> label_1;
    for (const auto& [label, _] : receiver.GetLabels()) {
        if (label != change_label) label_1 = label;
    }
    BOOST_REQUIRE(label_1.has_value());

    // Assigning a label copies both its key bytes and its secp256k1 object.
    SilentPaymentsLabel assigned{change_label};
    assigned = *label_1;
    BOOST_CHECK(assigned == *label_1);
    BOOST_CHECK(SilentPaymentsLabel{*assigned.Get()} == *label_1);

    std::map<COutPoint, Coin> coins;
    const COutPoint outpoint{TestOutpoint(1)};
    const CTxIn txin{P2WPKHInput(outpoint, sender_key, coins)};
    const auto prevouts_summary{GetSilentPaymentsPrevoutsSummary({txin}, coins)};
    BOOST_REQUIRE(prevouts_summary.has_value());

    // Pay the base address, the labeled address and the change address in the same transaction.
    const SilentPaymentsDestination base_dest{SilentPaymentsDestination::From(scan_key.GetPubKey(), spend_pubkey).value()};
    const std::map<size_t, SilentPaymentsDestination> sp_dests{
        {0, base_dest},
        {1, labeled_dest},
        {2, receiver.GetChangeDestination()},
    };
    const auto tr_dests{GenerateSilentPaymentsTaprootDestinations(sp_dests, {sender_key}, {}, outpoint)};
    BOOST_REQUIRE(tr_dests.has_value());
    BOOST_REQUIRE_EQUAL(tr_dests->size(), 3);
    std::vector<XOnlyPubKey> tx_outputs;
    for (const auto& [_, dest] : *tr_dests) {
        tx_outputs.emplace_back(dest);
    }

    const auto found{receiver.Scan(*prevouts_summary, tx_outputs)};
    BOOST_REQUIRE(found.has_value());
    BOOST_REQUIRE_EQUAL(found->size(), 3);
    int matched{0};
    for (const auto& output : *found) {
        if (output.output == XOnlyPubKey{tr_dests->at(0)}) {
            BOOST_CHECK(!output.label.has_value());
            ++matched;
        } else if (output.output == XOnlyPubKey{tr_dests->at(1)}) {
            BOOST_REQUIRE(output.label.has_value());
            BOOST_CHECK(*output.label == *label_1);
            ++matched;
        } else if (output.output == XOnlyPubKey{tr_dests->at(2)}) {
            BOOST_REQUIRE(output.label.has_value());
            BOOST_CHECK(*output.label == change_label);
            ++matched;
        }
    }
    BOOST_CHECK_EQUAL(matched, 3);
}

BOOST_AUTO_TEST_CASE(bip352_p2pkh_pubkey_extraction_skips_unrelated_pushes)
{
    // A third party can malleate a P2PKH scriptSig by prepending a push of another (valid) pubkey
    // followed by OP_DROP. The spend stays valid and the pubkey matching the output must be used.
    const CKey key = ParseHexToCKey("0000000000000000000000000000000000000000000000000000000000000001");
    const CPubKey pubkey = key.GetPubKey();
    const CPubKey wrong_pubkey = ParseHexToCKey("0000000000000000000000000000000000000000000000000000000000000002").GetPubKey();
    const CScript spk{GetScriptForDestination(PKHash{pubkey})};

    CMutableTransaction tx;
    tx.vin.emplace_back(TestOutpoint(1));
    std::vector<unsigned char> sig;
    BOOST_REQUIRE(key.Sign(SignatureHash(spk, tx, /*nIn=*/0, SIGHASH_ALL, /*amount=*/0, SigVersion::BASE), sig));
    sig.push_back(SIGHASH_ALL);
    tx.vin[0].scriptSig = CScript() << ToByteVector(wrong_pubkey) << OP_DROP << sig << ToByteVector(pubkey);
    const CTxIn& txin{tx.vin[0]};

    ScriptError serror = SCRIPT_ERR_OK;
    const MutableTransactionSignatureChecker checker{&tx, /*nInIn=*/0, /*amountIn=*/0, MissingDataBehavior::ASSERT_FAIL};
    BOOST_CHECK_MESSAGE(VerifyScript(txin.scriptSig, spk, nullptr, MANDATORY_SCRIPT_VERIFY_FLAGS, checker, &serror),
                        ScriptErrorString(serror));

    const auto extracted_pubkey = GetPubKeyFromInput(txin, spk);
    BOOST_REQUIRE(extracted_pubkey.has_value());
    BOOST_CHECK_EQUAL(std::get<CPubKey>(*extracted_pubkey), pubkey);
}

BOOST_AUTO_TEST_CASE(bip352_invalid_pubkeys_are_not_extracted)
{
    // 0x02 || x with x = 0 is not on the curve, so spending any of these outputs is consensus-invalid.
    // Such a key must never be returned: callers expect every extracted key to parse, and the prevouts
    // summary asserts on it.
    const std::vector<unsigned char> invalid_bytes(ParseHex("020000000000000000000000000000000000000000000000000000000000000000"));
    const CPubKey invalid_pubkey{invalid_bytes};
    BOOST_REQUIRE(!invalid_pubkey.IsFullyValid());
    const std::vector<unsigned char> dummy_sig(64, 0);

    // P2PKH whose hash commits to the invalid key.
    CTxIn p2pkh_in{TestOutpoint(1)};
    p2pkh_in.scriptSig = CScript() << dummy_sig << invalid_bytes;
    BOOST_CHECK(!GetPubKeyFromInput(p2pkh_in, GetScriptForDestination(PKHash{invalid_pubkey})).has_value());

    // P2WPKH with the invalid key on the witness.
    CTxIn p2wpkh_in{TestOutpoint(2)};
    p2wpkh_in.scriptWitness = CScriptWitness{{dummy_sig, invalid_bytes}};
    BOOST_CHECK(!GetPubKeyFromInput(p2wpkh_in, GetScriptForDestination(WitnessV0KeyHash{invalid_pubkey})).has_value());

    // P2SH-P2WPKH with the invalid key on the witness.
    const CScript invalid_redeem{GetScriptForDestination(WitnessV0KeyHash{invalid_pubkey})};
    CTxIn p2sh_in{TestOutpoint(3)};
    p2sh_in.scriptSig = CScript() << ToByteVector(invalid_redeem);
    p2sh_in.scriptWitness = CScriptWitness{{dummy_sig, invalid_bytes}};
    BOOST_CHECK(!GetPubKeyFromInput(p2sh_in, GetScriptForDestination(ScriptHash{invalid_redeem})).has_value());

    // P2TR key-path spend of an output key that is not on the curve.
    CTxIn p2tr_in{TestOutpoint(4)};
    p2tr_in.scriptWitness = CScriptWitness{{dummy_sig}};
    const CScript invalid_p2tr{CScript() << OP_1 << std::vector<unsigned char>(32, 0)};
    BOOST_CHECK(!GetPubKeyFromInput(p2tr_in, invalid_p2tr).has_value());
}

BOOST_AUTO_TEST_CASE(bip352_p2sh_must_wrap_p2wpkh)
{
    const CKey key = ParseHexToCKey("0000000000000000000000000000000000000000000000000000000000000001");
    const CPubKey pubkey = key.GetPubKey();
    const std::vector<unsigned char> dummy_sig(64, 0);

    // P2SH-P2WPKH is eligible.
    const CScript p2wpkh_redeem{GetScriptForDestination(WitnessV0KeyHash{pubkey})};
    CTxIn nested_in{TestOutpoint(1)};
    nested_in.scriptSig = CScript() << ToByteVector(p2wpkh_redeem);
    nested_in.scriptWitness = CScriptWitness{{dummy_sig, ToByteVector(pubkey)}};
    const auto nested_pubkey{GetPubKeyFromInput(nested_in, GetScriptForDestination(ScriptHash{p2wpkh_redeem}))};
    BOOST_REQUIRE(nested_pubkey.has_value());
    BOOST_CHECK_EQUAL(std::get<CPubKey>(*nested_pubkey), pubkey);

    // Any other P2SH redeem script is not, even if the last witness item is a valid pubkey.
    const CScript p2wsh_redeem{GetScriptForDestination(WitnessV0ScriptHash{CScript{} << OP_TRUE})};
    CTxIn p2wsh_in{TestOutpoint(2)};
    p2wsh_in.scriptSig = CScript() << ToByteVector(p2wsh_redeem);
    p2wsh_in.scriptWitness = CScriptWitness{{dummy_sig, ToByteVector(pubkey)}};
    BOOST_CHECK(!GetPubKeyFromInput(p2wsh_in, GetScriptForDestination(ScriptHash{p2wsh_redeem})).has_value());

    // A scriptSig that fails to evaluate is not eligible either, even if it pushes a P2WPKH redeem script.
    CTxIn failing_in{TestOutpoint(3)};
    failing_in.scriptSig = CScript() << ToByteVector(p2wpkh_redeem) << OP_RETURN;
    failing_in.scriptWitness = CScriptWitness{{dummy_sig, ToByteVector(pubkey)}};
    BOOST_CHECK(!GetPubKeyFromInput(failing_in, GetScriptForDestination(ScriptHash{p2wpkh_redeem})).has_value());
}

BOOST_AUTO_TEST_CASE(bip352_taproot_script_path_spends)
{
    const CKey key = ParseHexToCKey("0000000000000000000000000000000000000000000000000000000000000001");
    const XOnlyPubKey output_key{key.GetPubKey()};
    const CScript spk{GetScriptForDestination(WitnessV1Taproot{output_key})};
    const std::vector<unsigned char> leaf_script{OP_TRUE};
    const XOnlyPubKey other_internal_key{ParseHexToCKey("0000000000000000000000000000000000000000000000000000000000000002").GetPubKey()};

    const auto control_block = [](const XOnlyPubKey& internal_key, size_t path_bytes) {
        std::vector<unsigned char> control{TAPROOT_LEAF_TAPSCRIPT};
        control.insert(control.end(), internal_key.begin(), internal_key.end());
        control.resize(TAPROOT_CONTROL_BASE_SIZE + path_bytes, 0x01);
        return control;
    };
    const auto extract = [&](const std::vector<std::vector<unsigned char>>& stack) {
        CTxIn txin{TestOutpoint(1)};
        txin.scriptWitness = CScriptWitness{stack};
        return GetPubKeyFromInput(txin, spk);
    };

    // A script-path spend with a two-item witness (script, control block) is eligible unless its
    // internal key is NUMS_H.
    const auto eligible{extract({leaf_script, control_block(other_internal_key, 0)})};
    BOOST_REQUIRE(eligible.has_value());
    BOOST_CHECK(std::get<XOnlyPubKey>(*eligible) == output_key);
    BOOST_CHECK(!extract({leaf_script, control_block(XOnlyPubKey::NUMS_H, 0)}).has_value());

    // An annex (a last witness item starting with ANNEX_TAG, in a witness of at least two items) is
    // ignored, for key-path and script-path spends alike.
    const std::vector<unsigned char> annex{ANNEX_TAG};
    BOOST_CHECK(extract({std::vector<unsigned char>(64, 0), annex}).has_value());
    BOOST_CHECK(extract({leaf_script, control_block(other_internal_key, 0), annex}).has_value());
    BOOST_CHECK(!extract({leaf_script, control_block(XOnlyPubKey::NUMS_H, 0), annex}).has_value());

    // A control block with the longest valid path is eligible.
    const size_t max_path{TAPROOT_CONTROL_MAX_NODE_COUNT * TAPROOT_CONTROL_NODE_SIZE};
    BOOST_CHECK(extract({leaf_script, control_block(other_internal_key, max_path)}).has_value());

    // Control blocks that are too short, not 33 + 32 * m bytes, or too long make the spend
    // consensus-invalid (BIP341). Such inputs are not eligible, and no key is read from them.
    BOOST_CHECK(!extract({leaf_script, {TAPROOT_LEAF_TAPSCRIPT}}).has_value());
    BOOST_CHECK(!extract({leaf_script, control_block(other_internal_key, 1)}).has_value());
    const size_t too_long{max_path + TAPROOT_CONTROL_NODE_SIZE};
    BOOST_CHECK(!extract({leaf_script, control_block(other_internal_key, too_long)}).has_value());
}

BOOST_AUTO_TEST_CASE(bip352_prevouts_summary_errors)
{
    const CKey key = ParseHexToCKey("0000000000000000000000000000000000000000000000000000000000000001");
    std::map<COutPoint, Coin> coins;
    const CTxIn eligible_input{P2WPKHInput(TestOutpoint(1), key, coins)};
    BOOST_REQUIRE(GetSilentPaymentsPrevoutsSummary({eligible_input}, coins).has_value());

    // An input whose coin is missing makes the whole lookup fail.
    const auto missing{GetSilentPaymentsPrevoutsSummary({eligible_input, CTxIn{TestOutpoint(2)}}, coins)};
    BOOST_REQUIRE(!missing.has_value());
    BOOST_CHECK(missing.error() == PrevoutsSummaryError::MISSING_COIN);

    // Input public keys that sum to the point at infinity make the transaction ineligible.
    // n - 1, whose public key is -G, the negation of the public key of 1.
    const CKey negated_key = ParseHexToCKey("fffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364140");
    const CTxIn negated_input{P2WPKHInput(TestOutpoint(3), negated_key, coins)};
    const auto infinity{GetSilentPaymentsPrevoutsSummary({eligible_input, negated_input}, coins)};
    BOOST_REQUIRE(!infinity.has_value());
    BOOST_CHECK(infinity.error() == PrevoutsSummaryError::NOT_ELIGIBLE);
}

BOOST_AUTO_TEST_CASE(bip352_destination_from_rejects_invalid)
{
    const CPubKey scan_pubkey = ParseHexToCKey("0000000000000000000000000000000000000000000000000000000000000002").GetPubKey();
    const CPubKey spend_pubkey = ParseHexToCKey("0000000000000000000000000000000000000000000000000000000000000003").GetPubKey();
    const std::vector<unsigned char> extension{0xde, 0xad};

    BOOST_CHECK(SilentPaymentsDestination::From(scan_pubkey, spend_pubkey, /*version=*/30, extension).has_value());
    BOOST_CHECK(!SilentPaymentsDestination::From(scan_pubkey, spend_pubkey, /*version=*/31).has_value());
    BOOST_CHECK(!SilentPaymentsDestination::From(scan_pubkey, spend_pubkey, /*version=*/0, extension).has_value());

    const std::vector<unsigned char> scan_secret{ParseHex("0000000000000000000000000000000000000000000000000000000000000002")};
    CKey uncompressed_key;
    uncompressed_key.Set(scan_secret.begin(), scan_secret.end(), /*fCompressedIn=*/false);
    BOOST_REQUIRE(!uncompressed_key.GetPubKey().IsCompressed());
    BOOST_CHECK(!SilentPaymentsDestination::From(uncompressed_key.GetPubKey(), spend_pubkey).has_value());
    BOOST_CHECK(!SilentPaymentsDestination::From(scan_pubkey, uncompressed_key.GetPubKey()).has_value());
    const CPubKey invalid_pubkey{ParseHex("020000000000000000000000000000000000000000000000000000000000000000")};
    BOOST_CHECK(!SilentPaymentsDestination::From(scan_pubkey, invalid_pubkey).has_value());
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace bip352
