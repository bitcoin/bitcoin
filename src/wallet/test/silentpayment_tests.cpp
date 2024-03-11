#include <addresstype.h>
#include <wallet/silentpayments.h>
#include <policy/policy.h>
#include <script/solver.h>
#include <test/data/bip352_send_and_receive_vectors.json.h>

#include <test/util/setup_common.h>
#include <hash.h>

#include <boost/test/unit_test.hpp>
#include <test/util/json.h>
#include <vector>
#include <util/bip32.h>
#include <wallet/wallet.h>
#include <streams.h>

namespace wallet {
BOOST_FIXTURE_TEST_SUITE(silentpayment_tests, BasicTestingSetup)

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
            std::vector<std::pair<CKey, bool>> sender_secret_keys;
            for (const auto& input : given["vin"].getValues()) {
                COutPoint outpoint{TxidFromString(input["txid"].get_str()), input["vout"].getInt<uint32_t>()};
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
                auto pubkey = GetPubKeyFromInput(txin, spk);
                if (pubkey.has_value()) {
                    std::vector<std::vector<unsigned char>> solutions;
                    TxoutType type = Solver(spk, solutions);
                    sender_secret_keys.emplace_back(ParseHexToCKey(input["private_key"].get_str()), (type == TxoutType::WITNESS_V1_TAPROOT));
                }
            }

            std::vector<CRecipient> silent_payment_addresses;
            for (const auto& recipient : given["recipients"].getValues()) {
                std::string silent_payment_address = recipient[0].get_str();
                CAmount amount = recipient[1].get_real() * COIN;
                const CTxDestination& sp = DecodeDestination(silent_payment_address);
                silent_payment_addresses.push_back(CRecipient{sp, amount, false});
            }

            // silent payments logic
            CKey seckeys_sum{SumInputPrivKeys(sender_secret_keys)};
            uint256 inputs_hash = ComputeInputsHash(outpoints, seckeys_sum.GetPubKey());
            std::map<size_t, V0SilentPaymentDestination> sp_dests;
            for (size_t i = 0; i < silent_payment_addresses.size(); ++i) {
                if (const auto* sp = std::get_if<V0SilentPaymentDestination>(&silent_payment_addresses.at(i).dest)) {
                    sp_dests[i] = *sp;
                }
            }
            std::map<size_t, WitnessV1Taproot> sp_tr_dests = GenerateSilentPaymentTaprootDestinations(seckeys_sum, inputs_hash, sp_dests);

            for (const auto& [out_idx, tr_dest] : sp_tr_dests) {
                assert(out_idx < silent_payment_addresses.size());
                silent_payment_addresses[out_idx].dest = tr_dest;
            }

            std::vector<CRecipient> expected_spks;
            for (const auto& recipient : expected["outputs"].getValues()) {
                std::string pubkey_hex = recipient[0].get_str();
                CAmount amount = recipient[1].get_real() * COIN;
                const WitnessV1Taproot tap{XOnlyPubKey(ParseHex(pubkey_hex))};
                expected_spks.push_back(CRecipient{tap, amount, false});
            }

            BOOST_CHECK(silent_payment_addresses.size() == expected_spks.size());
            for (const auto& spk : silent_payment_addresses) {
                BOOST_CHECK(std::find(expected_spks.begin(), expected_spks.end(), spk) != expected_spks.end());
            }
        }

        // Test receiving
        for (const auto& recipient : vec["receiving"].getValues()) {

            const UniValue& given = recipient["given"];
            const UniValue& expected = recipient["expected"];

            std::vector<CTxIn> vin;
            std::map<COutPoint, Coin> coins;
            for (const auto& input : given["vin"].getValues()) {
                COutPoint outpoint{TxidFromString(input["txid"].get_str()), input["vout"].getInt<uint32_t>()};
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

            std::vector<XOnlyPubKey> output_pub_keys;
            for (const auto& pubkey : given["outputs"].getValues()) {
                output_pub_keys.emplace_back(ParseHex(pubkey.get_str()));
            }

            CKey scan_priv_key = ParseHexToCKey(given["key_material"]["scan_priv_key"].get_str());
            CKey spend_priv_key = ParseHexToCKey(given["key_material"]["spend_priv_key"].get_str());
            V0SilentPaymentDestination sp_address{scan_priv_key.GetPubKey(), spend_priv_key.GetPubKey()};

            std::map<CPubKey, uint256> labels;
            // Always calculate the change key, whether we use labels or not
            uint256 change_tweak = ComputeSilentPaymentLabelTweak(scan_priv_key, 0);
            CKey change_key;
            change_key.Set(change_tweak.begin(), change_tweak.end(), /*fCompressedIn=*/true);
            labels[change_key.GetPubKey()] = change_tweak;
            // If labels are used, add them to the dictionary
            for (const auto& label : given["labels"].getValues()) {
                uint256 tweak{ComputeSilentPaymentLabelTweak(scan_priv_key, label.getInt<int>())};
                CKey key;
                key.Set(tweak.begin(), tweak.end(), /*fCompressedIn=*/true);
                labels[key.GetPubKey()] = tweak;
            }


            // Scanning
            const auto& tweak_data = GetSilentPaymentTweakDataFromTxInputs(vin, coins);
            const auto& [inputs_hash, sum_input_pubkeys] = *tweak_data;
            const auto expected_addresses = expected["addresses"].getValues();
            CPubKey ecdh_pubkey = ComputeECDH(scan_priv_key, sum_input_pubkeys, inputs_hash);
            auto found_tweaks = GetTxOutputTweaks(sp_address.m_spend_pubkey, ecdh_pubkey, output_pub_keys, labels);
            // if no tweaks are found in the outputs, this is not a silent payments transaction to us
            if (!found_tweaks.has_value()) continue;
            std::vector<XOnlyPubKey> expected_outputs;
            for (const auto& output : expected["outputs"].getValues()) {
                std::string pubkey_hex = output["pub_key"].get_str();
                expected_outputs.emplace_back(ParseHex(pubkey_hex));
            }
            std::vector<XOnlyPubKey> outputs;
            for (const uint256& tweak : *found_tweaks) {
                CPubKey pubkey{sp_address.m_spend_pubkey};
                pubkey.TweakAdd(tweak.data());
                CKey privkey{spend_priv_key};
                privkey.TweakAdd(tweak.data());
                BOOST_CHECK(privkey.VerifyPubKey(pubkey));
                outputs.emplace_back(pubkey);
            }
            BOOST_CHECK(outputs == expected_outputs);
        }
    }
}
BOOST_AUTO_TEST_SUITE_END()
} // namespace wallet
