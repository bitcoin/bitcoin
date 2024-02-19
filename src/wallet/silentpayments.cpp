#include <wallet/silentpayments.h>
#include <addresstype.h>
#include <arith_uint256.h>
#include <coins.h>
#include <crypto/common.h>
#include <crypto/hmac_sha512.h>
#include <key_io.h>
#include <undo.h>
#include <logging.h>
#include <pubkey.h>
#include <policy/policy.h>
#include <script/sign.h>
#include <script/solver.h>
#include <util/check.h>

namespace wallet {

const HashWriter HASHER_INPUTS{TaggedHash("BIP0352/Inputs")};
const HashWriter HASHER_LABEL{TaggedHash("BIP0352/Label")};
const HashWriter HASHER_SHARED_SECRET{TaggedHash("BIP0352/SharedSecret")};

CPubKey ComputeECDH(const CKey& seckey, const CPubKey& pubkey, const uint256& inputs_hash)
{
    CKey tweaked_seckey{seckey};
    tweaked_seckey.TweakMultiply(inputs_hash.begin());
    CPubKey ecdh = tweaked_seckey.UnhashedECDH(pubkey);
    assert(ecdh.IsValid());
    return ecdh;
}

uint256 ComputeInputsHash(const std::vector<COutPoint>& tx_outpoints, const CPubKey& sum_input_pubkeys)
{
    HashWriter h{HASHER_INPUTS};
    auto min_outpoint = std::min_element(tx_outpoints.begin(), tx_outpoints.end());
    h << *min_outpoint;
    h.write(MakeByteSpan(sum_input_pubkeys));
    return h.GetSHA256();
}

uint256 ComputeSharedSecretTweak(const CPubKey& ecdh_pubkey, const uint32_t k)
{
    HashWriter h{HASHER_SHARED_SECRET};
    h.write(MakeByteSpan(ecdh_pubkey));
    unsigned char serialized[4];
    WriteBE32(serialized, k);
    h << serialized;
    return h.GetSHA256();
}

uint256 ComputeSilentPaymentLabelTweak(const CKey& scan_key, const int m)
{
    HashWriter h{HASHER_LABEL};
    unsigned char serialized[4];
    WriteBE32(serialized, m);
    h.write(scan_key);
    h << serialized;
    return h.GetSHA256();
}

CPubKey GenerateOutput(const CPubKey& ecdh_pubkey, const CPubKey& spend_pubkey, const uint32_t k)
{
    uint256 shared_secret = ComputeSharedSecretTweak(ecdh_pubkey, k);
    CPubKey output_k{spend_pubkey};
    output_k.TweakAdd(shared_secret.begin());
    return output_k;
}

V0SilentPaymentDestination GenerateSilentPaymentLabeledAddress(const V0SilentPaymentDestination& receiver, const uint256& label)
{
    CPubKey labeled_spend_pubkey{receiver.m_spend_pubkey};
    labeled_spend_pubkey.TweakAdd(label.data());
    return V0SilentPaymentDestination{receiver.m_scan_pubkey, labeled_spend_pubkey};
}

std::map<size_t, WitnessV1Taproot> GenerateSilentPaymentTaprootDestinations(const CKey& seckey, const uint256& inputs_hash, const std::map<size_t, V0SilentPaymentDestination>& sp_dests)
{
    std::map<CPubKey, std::vector<std::pair<CPubKey, size_t>>> sp_groups;
    std::map<size_t, WitnessV1Taproot> tr_dests;

    for (const auto& [out_idx, sp_dest] : sp_dests) {
        sp_groups[sp_dest.m_scan_pubkey].emplace_back(sp_dest.m_spend_pubkey, out_idx);
    }

    for (const auto& [scan_pubkey, spend_pubkeys] : sp_groups) {
        CPubKey ecdh_pubkey{ComputeECDH(seckey, scan_pubkey, inputs_hash)};
        for (size_t k = 0; k < spend_pubkeys.size(); ++k) {
            const auto& [spend_pubkey, out_idx] = spend_pubkeys.at(k);
            tr_dests.emplace(out_idx, XOnlyPubKey{GenerateOutput(ecdh_pubkey, spend_pubkey, k)});
        }
    }
    return tr_dests;
}

std::optional<CPubKey> GetPubKeyFromInput(const CTxIn& txin, const CScript& spk)
{
    std::vector<std::vector<unsigned char>> solutions;
    TxoutType type = Solver(spk, solutions);
    if (type == TxoutType::WITNESS_V1_TAPROOT) {
        // Check for H point in script path spend
        if (txin.scriptWitness.stack.size() > 1) {
            // Check for annex
            bool has_annex = txin.scriptWitness.stack.back()[0] == ANNEX_TAG;
            size_t post_annex_size = txin.scriptWitness.stack.size() - (has_annex ? 1 : 0);
            if (post_annex_size > 1) {
                // Actually a script path spend
                const std::vector<unsigned char>& control = txin.scriptWitness.stack.at(post_annex_size - 1);
                Assert(control.size() >= 33);
                if (std::equal(NUMS_H.begin(), NUMS_H.end(), control.begin() + 1)) {
                    // Skip script path with H internal key
                    return std::nullopt;
                }
            }
        }

        std::vector<unsigned char> pubkey;
        pubkey.resize(33);
        pubkey[0] = 0x02;
        std::copy(solutions[0].begin(), solutions[0].end(), pubkey.begin() + 1);
        return CPubKey{pubkey};
    } else if (type == TxoutType::WITNESS_V0_KEYHASH) {
        return CPubKey{txin.scriptWitness.stack.back()};
    } else if (type == TxoutType::PUBKEYHASH || type == TxoutType::SCRIPTHASH) {
        // Use the script interpreter to get the stack after executing the scriptSig
        std::vector<std::vector<unsigned char>> stack;
        ScriptError serror;
        Assert(EvalScript(stack, txin.scriptSig, MANDATORY_SCRIPT_VERIFY_FLAGS, DUMMY_CHECKER, SigVersion::BASE, &serror));
        if (type == TxoutType::PUBKEYHASH) {
            return CPubKey{stack.back()};
        } else if (type == TxoutType::SCRIPTHASH) {
            // Check if the redeemScript is P2WPKH
            CScript redeem_script{stack.back().begin(), stack.back().end()};
            TxoutType rs_type = Solver(redeem_script, solutions);
            if (rs_type == TxoutType::WITNESS_V0_KEYHASH) {
                return CPubKey{txin.scriptWitness.stack.back()};
            }
        }
    }
    return std::nullopt;
}

std::optional<std::pair<uint256, CPubKey>> GetSilentPaymentTweakDataFromTxInputs(const std::vector<CTxIn>& vin, const std::map<COutPoint, Coin>& coins)
{
    // Extract the keys from the inputs
    // or skip if no valid inputs
    std::vector<CPubKey> input_pubkeys;
    std::vector<COutPoint> input_outpoints;
    for (const CTxIn& txin : vin) {
        const Coin& coin = coins.at(txin.prevout);
        Assert(!coin.IsSpent());
        input_outpoints.emplace_back(txin.prevout);
        auto pubkey = GetPubKeyFromInput(txin, coin.out.scriptPubKey);
        if (pubkey.has_value()) {
            input_pubkeys.push_back(*pubkey);
        }
    }
    if (input_pubkeys.size() == 0) return std::nullopt;
    CPubKey input_pubkeys_sum = SumInputPubKeys(input_pubkeys);
    uint256 inputs_hash = ComputeInputsHash(input_outpoints, input_pubkeys_sum);
    return std::make_pair(inputs_hash, input_pubkeys_sum);
}

std::optional<std::vector<uint256>> GetTxOutputTweaks(const CPubKey& spend_pubkey, const CPubKey& ecdh_pubkey, std::vector<XOnlyPubKey> output_pub_keys, const std::map<CPubKey, uint256>& labels)
{
    // Because a sender can create multiple outputs for us, we first check the outputs vector for an output with
    // output index 0. If we find it, we remove it from the vector and then iterate over the vector again looking for
    // an output with index 1, and so on until one of the following happens:
    //
    //     1. We have determined all outputs belong to us (the vector is empty)
    //     2. We have passed over the vector and found no outputs belonging to us
    //

    // Define a convenience lambda for correctly combining uint256 tweak data
    auto combine_tweaks = [](const uint256& tweak, const uint256& label_tweak) -> uint256 {
        uint256 combined_tweaks;
        CKey tmp;
        tmp.Set(tweak.begin(), tweak.end(), /*fCompressedIn=*/true);
        tmp.TweakAdd(label_tweak.begin());
        std::copy(UCharCast(tmp.begin()), UCharCast(tmp.end()), combined_tweaks.begin());
        return combined_tweaks;
    };

    bool keep_going;
    uint32_t k{0};
    std::vector<uint256> tweaks;
    do {
        // We haven't found anything yet on this pass and if we make to the end without finding any
        // silent payment outputs everything left in the vector is not for us, so we stop scanning.
        keep_going = false;

        // t_k = hash(ecdh_shared_secret || k)
        uint256 t_k = ComputeSharedSecretTweak(ecdh_pubkey, k);

        // Compute P_k = B_spend + t_k * G, convert P_k to a P2TR output
        CPubKey P_k{spend_pubkey};
        P_k.TweakAdd(t_k.data());
        const XOnlyPubKey& P_k_xonly = XOnlyPubKey{P_k};

        // Scan the transaction outputs, only continue scanning if there is a match
        output_pub_keys.erase(std::remove_if(output_pub_keys.begin(), output_pub_keys.end(), [&](XOnlyPubKey output_pubkey) {
            bool found = P_k_xonly == output_pubkey;
            if (!found) {
                // We use P_k_negated for subtraction: output + ( - P_k )
                bool parity = (P_k.begin()[0] == 0x02) ? true : false;
                CPubKey P_k_negated = P_k_xonly.ConvertToCompressedPubKey(/*even=*/!parity);
                // First do the subtraction with the even Y coordinate for the output_key.
                // If not found, negate it (use the odd Y coordinate) and check again
                auto it = labels.find(output_pubkey.ConvertToCompressedPubKey(/*even=*/true) + P_k_negated);
                if (it == labels.end()) {
                    it = labels.find(output_pubkey.ConvertToCompressedPubKey(/*even=*/false) + P_k_negated);
                }
                if (it != labels.end()) {
                    t_k = combine_tweaks(t_k, it->second);
                    found = true;
                }
            }
            if (found) {
                // Since we found an output, we need to increment k and check the vector again
                tweaks.emplace_back(t_k);
                keep_going = true;
                k++;
                // Return true so that this output pubkey is removed the from vector and not checked again
                return true;
            }
            return false;
        }), output_pub_keys.end());
    } while (!output_pub_keys.empty() && keep_going);
    if (tweaks.empty()) return std::nullopt;
    return tweaks;
}

CKey SumInputPrivKeys(const std::vector<std::pair<CKey, bool>>& sender_secret_keys)
{
    // Grab the first key, copy it to the accumulator, and negate if necessary
    const auto& [seckey, is_taproot] = sender_secret_keys.at(0);
    CKey sum_seckey{seckey};
    if (is_taproot && sum_seckey.GetPubKey()[0] == 3) sum_seckey.Negate();
    if (sender_secret_keys.size() == 1) return sum_seckey;

    // Add the rest of the keys, negating if necessary
    for (size_t i = 1; i < sender_secret_keys.size(); i++) {
        const auto& [sender_seckey, sender_is_taproot] = sender_secret_keys.at(i);
        CKey temp_key{sender_seckey};
        if (sender_is_taproot && sender_seckey.GetPubKey()[0] == 3) {
            temp_key.Negate();
        }
        sum_seckey.TweakAdd(UCharCast(temp_key.begin()));
    }
    return sum_seckey;
}

CPubKey SumInputPubKeys(const std::vector<CPubKey>& pubkeys)
{
    CPubKey sum_pubkey{pubkeys.at(0)};
    if (pubkeys.size() == 1) return sum_pubkey;
    for (size_t i = 1; i < pubkeys.size(); i++) {
        sum_pubkey = sum_pubkey + pubkeys.at(i);
    }
    return sum_pubkey;
}
}
