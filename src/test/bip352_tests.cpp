// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/bip352.h>
#include <chainparams.h>
#include <coins.h>
#include <span.h>
#include <addresstype.h>
#include <script/solver.h>
#include <test/data/bip352_send_and_receive_vectors.json.h>

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
                // read the field txWitness as a stream and write txWitness >> witness.stack;
                const auto witness_str = ParseHex(input["txinwitness"].get_str());
                if (!witness_str.empty()) {
                    SpanReader(witness_str) >> txin.scriptWitness.stack;
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
                // read the field txWitness as a stream and write txWitness >> witness.stack;
                const auto witness_str = ParseHex(input["txinwitness"].get_str());
                if (!witness_str.empty()) {
                    SpanReader(witness_str) >> txin.scriptWitness.stack;
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
    eligible_input.scriptWitness.stack.emplace_back(64, 0);
    eligible_input.scriptWitness.stack.emplace_back(pubkey.begin(), pubkey.end());

    std::map<COutPoint, Coin> coins;
    coins[eligible_outpoint] = Coin{CTxOut{{}, GetScriptForDestination(WitnessV0KeyHash{pubkey})}, 0, false};
    coins[unknown_segwit_outpoint] = Coin{CTxOut{{}, GetScriptForDestination(WitnessUnknown{2, std::vector<unsigned char>(32, 1)})}, 0, false};

    BOOST_REQUIRE(GetSilentPaymentsPrevoutsSummary({eligible_input}, coins).has_value());
    BOOST_CHECK(!GetSilentPaymentsPrevoutsSummary({eligible_input, CTxIn{unknown_segwit_outpoint}}, coins).has_value());
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
    txin.scriptWitness.stack.emplace_back();
    txin.scriptWitness.stack.emplace_back(sender_pubkey.begin(), sender_pubkey.end());

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
    BOOST_CHECK(found_outputs->front().output == expected_output);
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

BOOST_AUTO_TEST_SUITE_END()
} // namespace bip352
