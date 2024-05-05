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

CKey GetKeyFromBIP32Path(std::vector<std::byte> seed, std::vector<uint32_t> path)
{
    CExtKey key;
    key.SetSeed(seed);
    for (auto index : path) {
        BOOST_CHECK(key.Derive(key, index));
    }
    return key.key;
}

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

                // check if this is a silent payment input by trying to extract the public key
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
            std::map<size_t, V0SilentPaymentDestination> sp_dests;
            const std::vector<UniValue>& silent_payment_addresses = given["recipients"].getValues();
            for (size_t i = 0; i < silent_payment_addresses.size(); ++i) {
                const CTxDestination& tx_dest = DecodeDestination(silent_payment_addresses[i].get_str());
                if (const auto* sp = std::get_if<V0SilentPaymentDestination>(&tx_dest)) {
                    sp_dests[i] = *sp;
                }
            }
            auto sp_tr_dests = bip352::GenerateSilentPaymentTaprootDestinations(sp_dests, keys, taproot_keys, *smallest_outpoint);
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
            V0SilentPaymentDestination sp_address{scan_priv_key.GetPubKey(), spend_priv_key.GetPubKey()};

            std::map<CPubKey, uint256> labels;
            // Always calculate the change key, whether we use labels or not
            const auto& [change_pubkey, change_tweak] = bip352::CreateLabelTweak(scan_priv_key, 0);
            labels[change_pubkey] = change_tweak;
            // If labels are used, add them to the dictionary
            for (const auto& label : given["labels"].getValues()) {
                const auto& [label_pubkey, label_tweak] = bip352::CreateLabelTweak(scan_priv_key, label.getInt<int>());
                labels[label_pubkey] = label_tweak;
            }


            // Scanning
            const auto& found_outputs = bip352::ScanForSilentPaymentOutputs(scan_priv_key, *pub_tweak_data, sp_address.m_spend_pubkey, output_pub_keys, labels);
            // The transaction may be a silent payment transaction, but it does not contain any outputs for us,
            // so we continue to the next transaction.
            if (!found_outputs.has_value()) {
                BOOST_CHECK(expected["outputs"].empty());
                continue;
            }
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
BOOST_AUTO_TEST_SUITE_END()
} // namespace wallet
