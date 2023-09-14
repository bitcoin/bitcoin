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

uint256 GenerateSilentPaymentChangeTweak(const CKey& scan_key)
{
    HashWriter hasher;
    hasher.write(scan_key);
    return hasher.GetSHA256();
}

V0SilentPaymentDestination GenerateSilentPaymentLabeledAddress(const V0SilentPaymentDestination& receiver, const uint256& label)
{
    CPubKey labeled_spend_pubkey{receiver.m_spend_pubkey};
    labeled_spend_pubkey.TweakAdd(label.data());
    return V0SilentPaymentDestination{receiver.m_scan_pubkey, labeled_spend_pubkey};
}

CPubKey ComputeECDHSharedSecret(const CKey& scan_key, const CPubKey& sender_public_key, const uint256& outpoints_hash)
{
    CKey tweaked_scan_seckey{scan_key};
    tweaked_scan_seckey.TweakMultiply(outpoints_hash.begin());
    CPubKey result = tweaked_scan_seckey.UnhashedECDH(sender_public_key);
    assert(result.IsValid());
    return result;
}

uint256 ComputeTweak(const CPubKey& ecdh_pubkey, const uint32_t output_index)
{
    HashWriter h;
    h.write(Span{ecdh_pubkey});
    unsigned char num[4];
    WriteBE32(num, output_index);
    h << num;
    return h.GetSHA256();
}

CPubKey CreateOutput(const CKey& ecdh_scalar, const CPubKey& scan_pubkey, const CPubKey& spend_pubkey, const uint32_t output_index)
{
    CPubKey ecdh_pubkey = ecdh_scalar.UnhashedECDH(scan_pubkey);
    HashWriter h;
    h.write(Span{ecdh_pubkey});
    unsigned char num[4];
    WriteBE32(num, output_index);
    h << num;
    uint256 shared_secret = h.GetSHA256();
    CPubKey silent_payment_output{spend_pubkey};
    silent_payment_output.TweakAdd(shared_secret.begin());
    return silent_payment_output;
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

CKey SumInputPrivKeys(const std::vector<std::pair<CKey, bool>>& sender_secret_keys)
{
    // Grab the first key, copy it to the accumulator, and negate if necessary
    const auto& [seckey, is_taproot] = sender_secret_keys.at(0);
    CKey sum_seckey{seckey};
    if (is_taproot && sum_seckey.GetPubKey()[0] == 3) sum_seckey.Negate();
    if (sender_secret_keys.size() == 1) return sum_seckey;

    // If there are more keys, check if the key needs to be negated and add it to the accumulator
    for (size_t i = 1; i < sender_secret_keys.size(); i++) {
        const auto& [sender_seckey, sender_is_taproot] = sender_secret_keys.at(i);
        CKey temp_key{sender_seckey};
        if (sender_is_taproot && sender_seckey.GetPubKey()[0] == 3) {
            temp_key.Negate();
        }
        sum_seckey.TweakAdd(temp_key.begin());
    }
    return sum_seckey;
}

CKey PrepareScalarECDHInput(const std::vector<std::pair<CKey, bool>>& sender_secret_keys, const std::vector<COutPoint>& tx_outpoints)
{
    CKey sum_input_secret_keys = SumInputPrivKeys(sender_secret_keys);
    uint256 outpoints_hash = HashOutpoints(tx_outpoints);
    sum_input_secret_keys.TweakMultiply(outpoints_hash.begin());
    return sum_input_secret_keys;
}

std::map<size_t, WitnessV1Taproot> GenerateSilentPaymentTaprootDestinations(const CKey& ecdh_scalar, const std::map<size_t, V0SilentPaymentDestination>& sp_dests)
{
    std::map<CPubKey, std::vector<std::pair<CPubKey, size_t>>> sp_groups;
    std::map<size_t, WitnessV1Taproot> tr_dests;

    for (const auto& [out_idx, sp_dest] : sp_dests) {
        sp_groups[sp_dest.m_scan_pubkey].emplace_back(sp_dest.m_spend_pubkey, out_idx);
    }

    for (const auto& [scan_pubkey, spend_pubkeys] : sp_groups) {
        for (size_t i = 0; i < spend_pubkeys.size(); ++i) {
            const auto& [spend_pubkey, out_idx] = spend_pubkeys.at(i);
            tr_dests.emplace(out_idx, XOnlyPubKey{CreateOutput(ecdh_scalar, scan_pubkey, spend_pubkey, i)});
        }
    }
    return tr_dests;
}

uint256 HashOutpoints(const std::vector<COutPoint>& tx_outpoints)
{

    // Make a local copy of the outpoints so we can sort them before hashing.
    // This is to ensure the sender and receiver deterministically arrive at the same outpoint hash,
    // regardless of how the outpoints are ordered in the transaction.

    std::vector<COutPoint> outpoints{tx_outpoints};
    std::sort(outpoints.begin(), outpoints.end());

    HashWriter h;
    for (const auto& outpoint: outpoints) {
        h << outpoint;
    }
    return h.GetSHA256();
}

std::vector<uint256> GetTxOutputTweaks(const CPubKey& spend_pubkey, const CPubKey& ecdh_pubkey, std::vector<XOnlyPubKey> output_pub_keys, const std::map<CPubKey, uint256>& labels)
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
        std::copy(tmp.begin(), tmp.end(), combined_tweaks.begin());
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
        uint256 t_k = ComputeTweak(ecdh_pubkey, k);

        // Compute P_k = B_spend + t_k * G, convert P_k to a P2TR output
        CPubKey P_k{spend_pubkey};
        P_k.TweakAdd(t_k.data());
        const XOnlyPubKey& P_k_xonly = XOnlyPubKey{P_k};

        // Scan the transaction outputs, only continue scanning if there is a match
        output_pub_keys.erase(std::remove_if(output_pub_keys.begin(), output_pub_keys.end(), [&](XOnlyPubKey output_pubkey) {
            bool found = P_k_xonly == output_pubkey;
            if (!found) {
                if (labels.size() < 3) {
                    // If we have less than 3 labels, it's cheaper to check by adding each label to each output.
                    // This ends up being at most N * 2 ECC additions, where N is the number of taproot outputs in the transaction.
                    // This is primarily for wallets that are only using a single label for generating silent payments change outputs.
                    for (const auto& [m_G, m] : labels) {
                        // Check P_k + m*G
                        CPubKey labeled_spend_key{P_k + m_G};
                        if (output_pubkey.ConvertToCompressedPubKey(/*even=*/true) == labeled_spend_key) {
                            t_k = combine_tweaks(t_k, m);
                            found = true;
                            break;
                        }
                        // If we don't find a label with the even output, also check the odd (negated) output
                        // Note: changing the parity byte from even to odd is not as expensive as negating an EC point
                        if (output_pubkey.ConvertToCompressedPubKey(/*even=*/false) == labeled_spend_key) {
                            t_k = combine_tweaks(t_k, m);
                            found = true;
                            break;
                        }
                    }
                } else {
                    // If we have 3 or more labels, checking via subtraction is cheaper.
                    // This ends up being at most N * 2 ECC operations (two subtractions) and a lookup in the labels map,
                    // where N is the number of taproot outputs in the transaction
                    std::map<CPubKey, uint256>::const_iterator it = labels.end();
                    // For an XOnlyPubKey, converting it to an odd compressed key is equivalent to negation.
                    // We use P_k_negated for subtraction: output + ( - P_k ),
                    // so first we need to check the parity byte of the compute P_K public key
                    bool parity = (P_k.begin()[0] == 0x02) ? true : false;
                    CPubKey P_k_negated = P_k_xonly.ConvertToCompressedPubKey(/*even=*/!parity);
                    it = labels.find(output_pubkey.ConvertToCompressedPubKey(/*even=*/true) + P_k_negated);
                    if (it == labels.end()) {
                        // If not found on the first try, negate the output and check again.
                        it = labels.find(output_pubkey.ConvertToCompressedPubKey(/*even=*/false) + P_k_negated);
                    }
                    if (it != labels.end()) {
                        t_k = combine_tweaks(t_k, it->second);
                        found = true;
                    }
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
    return tweaks;
}

std::optional<std::pair<uint256, CPubKey>> GetSilentPaymentsTweakDataFromTxInputs(const std::vector<CTxIn>& vin, const std::map<COutPoint, Coin>& coins)
{
    // Extract the keys from the inputs
    // or skip if no valid inputs
    std::vector<CPubKey> input_pubkeys;
    std::vector<COutPoint> input_outpoints;
    for (const CTxIn& txin : vin) {
        const Coin& coin = coins.at(txin.prevout);
        Assert(!coin.IsSpent());
        input_outpoints.emplace_back(txin.prevout);

        std::vector<std::vector<unsigned char>> solutions;
        TxoutType type = Solver(coin.out.scriptPubKey, solutions);
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
                        continue;
                    }
                }
            }

            std::vector<unsigned char> pubkey;
            pubkey.resize(33);
            pubkey[0] = 0x02;
            std::copy(solutions[0].begin(), solutions[0].end(), pubkey.begin() + 1);
            input_pubkeys.emplace_back(pubkey);
        } else if (type == TxoutType::WITNESS_V0_KEYHASH) {
            input_pubkeys.emplace_back(txin.scriptWitness.stack.back());
        } else if (type == TxoutType::PUBKEYHASH || type == TxoutType::SCRIPTHASH) {
            // Use the script interpreter to get the stack after executing the scriptSig
            std::vector<std::vector<unsigned char>> stack;
            ScriptError serror;
            Assert(EvalScript(stack, txin.scriptSig, MANDATORY_SCRIPT_VERIFY_FLAGS, DUMMY_CHECKER, SigVersion::BASE, &serror));
            if (type == TxoutType::PUBKEYHASH) {
                input_pubkeys.emplace_back(stack.back());
            } else if (type == TxoutType::SCRIPTHASH) {
                // Check if the redeemScript is P2WPKH
                CScript redeem_script{stack.back().begin(), stack.back().end()};
                TxoutType rs_type = Solver(redeem_script, solutions);
                if (rs_type == TxoutType::WITNESS_V0_KEYHASH) {
                    input_pubkeys.emplace_back(txin.scriptWitness.stack.back());
                }
            }
        } else if (type == TxoutType::PUBKEY) {
            input_pubkeys.emplace_back(solutions[0]);
        }
    }
    if (input_pubkeys.size() == 0) return std::nullopt;
    const uint256& outpoints_hash = HashOutpoints(input_outpoints);
    CPubKey input_pubkeys_sum = SumInputPubKeys(input_pubkeys);
    return std::make_pair(outpoints_hash, input_pubkeys_sum);
}
}
