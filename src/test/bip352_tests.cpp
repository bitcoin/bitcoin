#include <common/bip352.h>
#include <span.h>
#include <addresstype.h>
#include <policy/policy.h>
#include <script/solver.h>
#include <test/data/bip352_send_and_receive_vectors.json.h>

#include <test/util/setup_common.h>
#include <hash.h>

#include <boost/test/unit_test.hpp>
#include <test/util/json.h>
#include <vector>
#include <util/bip32.h>
#include <util/strencodings.h>
#include <key_io.h>
#include <streams.h>

namespace wallet {
BOOST_FIXTURE_TEST_SUITE(bip352_tests, BasicTestingSetup)

CKey ParseHexToCKey(std::string hex) {
    CKey output;
    std::vector<unsigned char> hex_data = ParseHex(hex);
    output.Set(hex_data.begin(), hex_data.end(), true);
    return output;
};

BOOST_AUTO_TEST_CASE(bip352_send_and_receive_test_vectors)
{
    UniValue tests;
    tests.read(json_tests::bip352_send_and_receive_vectors);

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
                CScriptWitness witness;
                // read the field txWitness as a stream and write txWitness >> witness.stack;
                auto witness_str = ParseHex(input["txinwitness"].get_str());
                if (!witness_str.empty()) {
                    SpanReader(witness_str) >> witness.stack;
                    txin.scriptWitness = witness;
                }

                // check if this is a silent payments input by trying to extract the public key
                const auto& pubkey = bip352::GetPubKeyFromInput(txin, spk);
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
            auto smallest_outpoint = std::min_element(outpoints.begin(), outpoints.end(), bip352::BIP352Comparator());
            std::map<size_t, V0SilentPaymentsDestination> sp_dests;
            const std::vector<UniValue>& silent_payments_addresses = given["recipients"].getValues();
            size_t sp_index = 0;
            for (size_t i = 0; i < silent_payments_addresses.size(); ++i) {
                const CTxDestination& tx_dest = DecodeDestination(silent_payments_addresses[i]["address"].get_str());
                if (const auto* sp = std::get_if<V0SilentPaymentsDestination>(&tx_dest)) {
                    size_t count = silent_payments_addresses[i]["count"].isNull() ? 1 : (size_t)silent_payments_addresses[i]["count"].getInt<int>();
                    for (size_t j = 0; j < count; ++j) {
                        sp_dests.emplace(sp_index++, *sp);
                    }
                }
            }
            auto sp_tr_dests = bip352::GenerateSilentPaymentsTaprootDestinations(sp_dests, keys, taproot_keys, *smallest_outpoint);
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
                match = true;
                for (const auto& [_, spk]: *sp_tr_dests) {
                    if (std::find(expected_spks.begin(), expected_spks.end(), spk) == expected_spks.end()) {
                        match = false;
                        break;
                    }
                }
                if (!match) {
                    continue;
                } else {
                    break;
                }
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
                CScriptWitness witness;
                // read the field txWitness as a stream and write txWitness >> witness.stack;
                auto witness_str = ParseHex(input["txinwitness"].get_str());
                if (!witness_str.empty()) {
                    SpanReader(witness_str) >> witness.stack;
                    txin.scriptWitness = witness;
                }
                vin.push_back(txin);
                coins[outpoint] = Coin{CTxOut{{}, spk}, 0, false};
            }
            auto pub_tweak_data = bip352::GetSilentPaymentsPrevoutsSummary(vin, coins);
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
            V0SilentPaymentsDestination sp_address{scan_priv_key.GetPubKey(), spend_priv_key.GetPubKey()};

            std::map<CPubKey, uint256> labels;
            // Always calculate the change key, whether we use labels or not
            const auto& [change_pubkey, change_tweak] = bip352::CreateLabel(scan_priv_key, 0);
            labels[change_pubkey] = change_tweak;
            // If labels are used, add them to the dictionary
            auto given_labels{given["labels"].getValues()};
            for (size_t i = 0; i < given_labels.size(); i++) {
                const auto& label{given_labels[i]};
                const auto& [label_pubkey, label_tweak] = bip352::CreateLabel(scan_priv_key, label.getInt<uint32_t>());
                labels[label_pubkey] = label_tweak;

                // Test labeled address generation
                const auto labeled_addr{bip352::GenerateSilentPaymentsLabeledAddress(sp_address, label_pubkey)};
                // expected["addresses"] contains the base silent payments address (at index 0)
                // followed by the labeled addresses
                const auto decoded{DecodeDestination(expected["addresses"][i+1].get_str())};
                const auto* sp = std::get_if<V0SilentPaymentsDestination>(&decoded);
                BOOST_CHECK(labeled_addr == *sp);
            }

            // Scanning
            const auto& found_outputs = bip352::ScanForSilentPaymentsOutputs(scan_priv_key, *pub_tweak_data, sp_address.GetSpendPubKey(), output_pub_keys, labels);
            // The transaction may be a silent payments transaction, but it does not contain any outputs for us,
            // so we continue to the next transaction.
            if (!found_outputs.has_value()) {
                BOOST_CHECK(expected["outputs"].empty());
                continue;
            }
            if (!expected["n_outputs"].isNull()) {
                BOOST_CHECK(found_outputs->size() == (size_t)expected["n_outputs"].getInt<int>());
            } else {
                std::vector<XOnlyPubKey> expected_outputs;
                for (const auto& output : expected["outputs"].getValues()) {
                    std::string pubkey_hex = output["pub_key"].get_str();
                    expected_outputs.emplace_back(ParseHex(pubkey_hex));
                }
                BOOST_TEST_MESSAGE(found_outputs->size());
                BOOST_CHECK(found_outputs->size() == expected_outputs.size());
                for (const auto& output : *found_outputs) {
                    BOOST_CHECK(std::find(expected_outputs.begin(), expected_outputs.end(), output.output) != expected_outputs.end());
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
    V0SilentPaymentsDestination sp_dest{scan_key.GetPubKey(), spend_key.GetPubKey()};
    std::map<size_t, V0SilentPaymentsDestination> sp_dests{{2, sp_dest}, {5, sp_dest}};
    COutPoint smallest_outpoint{Txid::FromHex("0000000000000000000000000000000000000000000000000000000000000001").value(), 0};

    auto generated = bip352::GenerateSilentPaymentsTaprootDestinations(sp_dests, {sender_key}, {}, smallest_outpoint);

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

    BOOST_REQUIRE(bip352::GetSilentPaymentsPrevoutsSummary({eligible_input}, coins).has_value());
    BOOST_CHECK(!bip352::GetSilentPaymentsPrevoutsSummary({eligible_input, CTxIn{unknown_segwit_outpoint}}, coins).has_value());
}

BOOST_AUTO_TEST_CASE(bip352_scan_skips_invalid_taproot_outputs)
{
    CKey sender_key = ParseHexToCKey("0000000000000000000000000000000000000000000000000000000000000001");
    CKey scan_key = ParseHexToCKey("0000000000000000000000000000000000000000000000000000000000000002");
    CKey spend_key = ParseHexToCKey("0000000000000000000000000000000000000000000000000000000000000003");
    const COutPoint outpoint{Txid::FromHex("0000000000000000000000000000000000000000000000000000000000000001").value(), 0};

    std::map<size_t, V0SilentPaymentsDestination> sp_dests;
    sp_dests.emplace(0, V0SilentPaymentsDestination{scan_key.GetPubKey(), spend_key.GetPubKey()});
    const auto sp_tr_dests = bip352::GenerateSilentPaymentsTaprootDestinations(sp_dests, {sender_key}, {}, outpoint);
    BOOST_REQUIRE(sp_tr_dests.has_value());
    const XOnlyPubKey expected_output{sp_tr_dests->begin()->second};

    CTxIn txin{outpoint};
    const CPubKey sender_pubkey{sender_key.GetPubKey()};
    txin.scriptWitness.stack.emplace_back();
    txin.scriptWitness.stack.emplace_back(sender_pubkey.begin(), sender_pubkey.end());

    std::map<COutPoint, Coin> coins;
    coins[outpoint] = Coin{CTxOut{{}, GetScriptForDestination(WitnessV0KeyHash{sender_pubkey})}, 0, false};
    const auto prevouts_summary = bip352::GetSilentPaymentsPrevoutsSummary({txin}, coins);
    BOOST_REQUIRE(prevouts_summary.has_value());

    std::vector<XOnlyPubKey> output_pub_keys;
    output_pub_keys.emplace_back(ParseHex("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"));
    output_pub_keys.push_back(expected_output);

    std::map<CPubKey, uint256> labels;
    const auto found_outputs = bip352::ScanForSilentPaymentsOutputs(scan_key, *prevouts_summary, spend_key.GetPubKey(), output_pub_keys, labels);
    BOOST_REQUIRE(found_outputs.has_value());
    BOOST_REQUIRE_EQUAL(found_outputs->size(), 1);
    BOOST_CHECK(found_outputs->front().output == expected_output);
}
BOOST_AUTO_TEST_SUITE_END()
} // namespace wallet
